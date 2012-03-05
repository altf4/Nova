//============================================================================
// Name        : Novad.cpp
// Copyright   : DataSoft Corporation 2011-2012
//	Nova is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   Nova is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with Nova.  If not, see <http://www.gnu.org/licenses/>.
// Description : Nova Daemon to perform network anti-reconnaissance
//============================================================================

#include "Config.h"
#include "SuspectTable.h"
#include "NovaUtil.h"
#include "Logger.h"
#include "Point.h"
#include "Novad.h"
#include "ClassificationEngine.h"

#include <vector>
#include <math.h>
#include <string>
#include <errno.h>
#include <fstream>
#include <sstream>
#include <sys/un.h>
#include <signal.h>
#include <sys/inotify.h>
#include <netinet/if_ether.h>
#include <iostream>

#include <boost/format.hpp>

using namespace std;
using boost::format;

string userHomePath, novaConfigPath;

// Maintains a list of suspects and information on network activity
SuspectTable suspects;
SuspectTable suspectsSinceLastSave;
static TCPSessionHashTable SessionTable;

static struct sockaddr_in hostAddr;

//** Main (ReceiveSuspectData) **
struct sockaddr_un remote;
struct sockaddr* remoteSockAddr = (struct sockaddr *)&remote;

int IPCsock;

//** Silent Alarm **
struct sockaddr_un alarmRemote;
struct sockaddr* alarmRemotePtr =(struct sockaddr *)&alarmRemote;
struct sockaddr_in serv_addr;
struct sockaddr* serv_addrPtr = (struct sockaddr *)&serv_addr;

int oldClassification;
int sockfd = 1;

//** ReceiveGUICommand **
int GUISocket;

//** SendToUI **
struct sockaddr_un GUISendRemote;
struct sockaddr* GUISendPtr = (struct sockaddr *)&GUISendRemote;
int GUISendSocket;
int GUILen;

//Universal Socket variables (constants that can be re-used)
int socketSize = sizeof(remote);
int inSocketSize = sizeof(serv_addr);
socklen_t * sockSizePtr = (socklen_t*)&socketSize;

// Misc
int len;

time_t lastLoadTime;
time_t lastSaveTime;

bool enableGUI = true;

//HS Vars
string dhcpListFile = "/var/log/honeyd/ipList";
vector <string> haystackAddresses;
vector <string> haystackDhcpAddresses;
pcap_t *handle;
bpf_u_int32 maskp;				/* subnet mask */
bpf_u_int32 netp; 				/* ip          */

int notifyFd;
int watch;
bool usePcapFile;

pthread_t TCP_timeout_thread;
pthread_rwlock_t sessionLock;


ClassificationEngine *engine;

int main()
{
	suspects.Resize(INIT_SIZE_SMALL);
	suspectsSinceLastSave.Resize(INIT_SIZE_SMALL);

	SessionTable.set_empty_key("");
	SessionTable.resize(INIT_SIZE_HUGE);

	//TODO: Perhaps move this into its own init function?
	userHomePath = GetHomePath();
	novaConfigPath = userHomePath + "/Config/NOVAConfig.txt";
	if(chdir(userHomePath.c_str()) == -1)
		LOG(INFO, "Failed to change directory to " + userHomePath, "Failed to change directory to " + userHomePath);

	pthread_rwlock_init(&sessionLock, NULL);

	signal(SIGINT, SaveAndExit);

	lastLoadTime = time(NULL);
	if (lastLoadTime == ((time_t)-1))
		LOG(ERROR, (format("File %1% at line %2%: Unable to get system time with time()")%__LINE__%__FILE__).str());

	lastSaveTime = time(NULL);
	if (lastSaveTime == ((time_t)-1))
		LOG(ERROR, (format("File %1% at line %2%: Unable to get system time with time()")%__LINE__%__FILE__).str());

	pthread_t classificationLoopThread;
	pthread_t trainingLoopThread;
	pthread_t silentAlarmListenThread;
	pthread_t GUIListenThread;
	pthread_t ipUpdateThread;

	engine = new ClassificationEngine(&suspects);

	Reload();



	pthread_create(&GUIListenThread, NULL, GUIListenLoop, NULL);
	//Are we Training or Classifying?
	if(Config::Inst()->getIsTraining())
	{
		pthread_create(&trainingLoopThread,NULL,TrainingLoop,NULL);
	}
	else
	{
		pthread_create(&classificationLoopThread,NULL,ClassificationLoop, NULL);
		pthread_create(&silentAlarmListenThread,NULL,SilentAlarmLoop, NULL);
	}

	notifyFd = inotify_init ();

	if (notifyFd > 0)
	{
		watch = inotify_add_watch (notifyFd, dhcpListFile.c_str(), IN_CLOSE_WRITE | IN_MOVED_TO | IN_MODIFY | IN_DELETE);
		pthread_create(&ipUpdateThread, NULL, UpdateIPFilter,NULL);
	}
	else
	{
		LOG(ERROR, (format("File %1% at line %2%: Unable to set up file watcher for the honeyd IP list file. DHCP addresse in honeyd will not be read")
				%__LINE__%__FILE__).str());
	}

	Start_Packet_Handler();

	//Shouldn't get here!
	LOG(CRITICAL, (format("File %1% at line %2%: Main thread ended. This should never happen, something went very wrong.")%__LINE__%__FILE__).str());
	close(IPCsock);

	return EXIT_FAILURE;
}

//Called when process receives a SIGINT, like if you press ctrl+c
void Nova::SaveAndExit(int param)
{
	AppendToStateFile();
	exit(EXIT_SUCCESS);
}

void Nova::AppendToStateFile()
{
	lastSaveTime = time(NULL);
	if (lastSaveTime == ((time_t)-1))
		LOG(ERROR, (format("File %1% at line %2%: Unable to get timestamp, call to time() failed")%__LINE__%__FILE__).str());

	// Don't bother saving if no new suspect data, just confuses deserialization
	if (suspectsSinceLastSave.Size() <= 0)
		return;

	u_char tableBuffer[MAX_MSG_SIZE];
	uint32_t dataSize = 0;

	suspectsSinceLastSave.Rdlock();
	// Compute the total dataSize
	for(SuspectTableIterator it = suspectsSinceLastSave.Begin(); it.GetIndex() !=  suspectsSinceLastSave.Size(); ++it)
	{
		if (!it.Current().GetFeatureSet().m_packetCount)
			continue;

		dataSize += it.Current().SerializeSuspectWithData(tableBuffer);
	}
	suspectsSinceLastSave.Unlock();
	// No suspects with packets to update
	if (dataSize == 0)
		return;

	ofstream out(Config::Inst()->getPathCESaveFile().data(), ofstream::binary | ofstream::app);
	if(!out.is_open())
	{
		LOG(ERROR, (format("File %1% at line %2%: Unable to open the CE state file %3%")%__LINE__%__FILE__%Config::Inst()->getPathCESaveFile()).str());
		return;
	}

	out.write((char*)&lastSaveTime, sizeof lastSaveTime);
	out.write((char*)&dataSize, sizeof dataSize);

	LOG(DEBUG, (format("File %1% at line %2%: Appending %3% bytes to the CE state file")%__LINE__%__FILE__%dataSize).str());

	suspectsSinceLastSave.Rdlock();
	// Serialize our suspect table
	for(SuspectTableIterator it = suspectsSinceLastSave.Begin(); it.GetIndex() != suspectsSinceLastSave.Size(); ++it)
	{
		if (!it.Current().GetFeatureSet().m_packetCount)
			continue;

		dataSize = it.Current().SerializeSuspectWithData(tableBuffer);
		out.write((char*) tableBuffer, dataSize);
	}
	suspectsSinceLastSave.Rdlock();

	out.close();
	suspectsSinceLastSave.Clear();
}

void Nova::LoadStateFile()
{
	time_t timeStamp;
	uint32_t dataSize;

	lastLoadTime = time(NULL);
	if (lastLoadTime == ((time_t)-1))
		LOG(ERROR, (format("File %1% at line %2%: Unable to get timestamp, call to time() failed")%__LINE__%__FILE__).str());

	// Open input file
	ifstream in(Config::Inst()->getPathCESaveFile().data(), ios::binary | ios::in);
	if(!in.is_open())
	{
		LOG(ERROR, (format("File %1% at line %2%: Unable to open CE state file. This is normal for the first run.")%__LINE__%__FILE__).str());
		return;
	}

	// get length of input for error checking of partially written files
	in.seekg (0, ios::end);
	uint lengthLeft = in.tellg();
	in.seekg (0, ios::beg);

	while (in.is_open() && !in.eof() && lengthLeft)
	{
		// Bytes left, but not enough to make a header (timestamp + size)?
		if (lengthLeft < (sizeof timeStamp + sizeof dataSize))
		{
			LOG(ERROR, "The CE state file may be corrupt",
					(format("File %1% at line %2%: CE state file should have another entry, but only contains %3% more bytes")%__LINE__%__FILE__%lengthLeft).str());
			break;
		}

		in.read((char*) &timeStamp, sizeof timeStamp);
		lengthLeft -= sizeof timeStamp;

		in.read((char*) &dataSize, sizeof dataSize);
		lengthLeft -= sizeof dataSize;

		if (Config::Inst()->getDataTTL() && (timeStamp < lastLoadTime - Config::Inst()->getDataTTL()))
		{
			LOG(DEBUG, (format("File %1% at line %2%: Throwing out old CE state with timestamp of %3%")%__LINE__%__FILE__%(int)timeStamp).str());
			in.seekg(dataSize, ifstream::cur);
			lengthLeft -= dataSize;
			continue;
		}

		// Not as many bytes left as the size of the entry?
		if (lengthLeft < dataSize)
		{
			LOG(ERROR, "The CE state file may be corruput, unable to read all data from it",
					(format("File %1% at line %2%: CE state file should have another entry of size %3% but only has %4% bytes left")
							%__LINE__%__FILE__%dataSize%lengthLeft).str());
			break;
		}

		u_char tableBuffer[dataSize];
		in.read((char*) tableBuffer, dataSize);
		lengthLeft -= dataSize;

		suspects.Wrlock();
		// Read each suspect
		uint32_t bytesSoFar = 0;
		while (bytesSoFar < dataSize)
		{
			Suspect* newSuspect = new Suspect();
			uint32_t suspectBytes = 0;
			suspectBytes += newSuspect->DeserializeSuspect(tableBuffer + bytesSoFar + suspectBytes);
			suspectBytes += newSuspect->GetFeatureSet().DeserializeFeatureData(tableBuffer + bytesSoFar + suspectBytes);
			bytesSoFar += suspectBytes;

			if(suspects.Peek(newSuspect->GetIpAddress()).GetIpAddress() != newSuspect->GetIpAddress())
			{
				suspects.AddNewSuspect(newSuspect);
				suspects[newSuspect->GetIpAddress()].SetNeedsFeatureUpdate(true);
				suspects[newSuspect->GetIpAddress()].SetNeedsClassificationUpdate(true);
			}
			else
			{
				FeatureSet fs = newSuspect->GetFeatureSet();
				suspects[newSuspect->GetIpAddress()].AddFeatureSet(&fs);
				suspects[newSuspect->GetIpAddress()].SetNeedsFeatureUpdate(true);
				suspects[newSuspect->GetIpAddress()].SetNeedsClassificationUpdate(true);
				delete newSuspect;
			}
		}
		suspects.Unlock();
	}

	in.close();
}

void Nova::RefreshStateFile()
{
	time_t timeStamp;
	uint32_t dataSize;
	vector<in_addr_t> deletedKeys;

	lastLoadTime = time(NULL);
	if (lastLoadTime == ((time_t)-1))
		LOG(ERROR, (format("File %1% at line %2%: Unable to get timestamp, call to time() failed")%__LINE__%__FILE__).str());

	// Open input file
	ifstream in(Config::Inst()->getPathCESaveFile().data(), ios::binary | ios::in);
	if(!in.is_open())
	{
		LOG(ERROR, (format("File %1% at line %2%: Unable to open the CE state file at %3%")%__LINE__%__FILE__%Config::Inst()->getPathCESaveFile()).str());
		return;
	}

	// Open the tmp file
	string tmpFile = Config::Inst()->getPathCESaveFile() + ".tmp";
	ofstream out(tmpFile.data(), ios::binary);
	if(!out.is_open())
	{
		LOG(ERROR, (format("File %1% at line %2%: Unable to open the temporary CE state file at %3%")%__LINE__%__FILE__%tmpFile).str());
		in.close();
		return;
	}

	// get length of input for error checking of partially written files
	in.seekg (0, ios::end);
	uint lengthLeft = in.tellg();
	in.seekg (0, ios::beg);

	while (in.is_open() && !in.eof() && lengthLeft)
	{
		// Bytes left, but not enough to make a header (timestamp + size)?
		if (lengthLeft < (sizeof timeStamp + sizeof dataSize))
		{
			LOG(ERROR, "The CE state file may be corrupt", (format("File %1% at line %2%: CE state file should have another entry, but only contains %3% more bytes")
					%__LINE__%__FILE__%lengthLeft).str());
			break;
		}

		in.read((char*) &timeStamp, sizeof timeStamp);
		lengthLeft -= sizeof timeStamp;

		in.read((char*) &dataSize, sizeof dataSize);
		lengthLeft -= sizeof dataSize;

		if (Config::Inst()->getDataTTL() && (timeStamp < lastLoadTime - Config::Inst()->getDataTTL()))
		{
			LOG(DEBUG, (format("File %1% at line %2%: Throwing out old CE state with timestamp of %3%")%__LINE__%__FILE__%(int)timeStamp).str());
			in.seekg(dataSize, ifstream::cur);
			lengthLeft -= dataSize;
			continue;
		}

		// Not as many bytes left as the size of the entry?
		if (lengthLeft < dataSize)
		{
			LOG(ERROR, "The CE state file may be corrupt",
					(format("File %1% at line %2%: Data file should have another entry of size %3%, but contains only %4% bytes left")%__LINE__%__FILE__%dataSize%lengthLeft).str());
			break;
		}

		u_char tableBuffer[dataSize];
		in.read((char*) tableBuffer, dataSize);
		lengthLeft -= dataSize;

		suspects.Wrlock();
		// Read each suspect
		uint32_t bytesSoFar = 0;
		while (bytesSoFar < dataSize)
		{
			Suspect* newSuspect = new Suspect();
			uint32_t suspectBytes = 0;
			suspectBytes += newSuspect->DeserializeSuspect(tableBuffer + bytesSoFar + suspectBytes);
			suspectBytes += newSuspect->GetFeatureSet().DeserializeFeatureData(tableBuffer + bytesSoFar + suspectBytes);

			// Did a suspect get cleared? Not in suspects anymore, but still is suspectsSinceLastSave
			// XXX 'suspectsSinceLastSave' SuspectTableIterator todo
			SuspectTableIterator saveIter = suspectsSinceLastSave.Find(newSuspect->GetIpAddress());
			// XXX 'suspectsSinceLastSave' SuspectTableIterator todo
			SuspectTableIterator normalIter = suspects.Find(newSuspect->GetIpAddress());
			if ((normalIter.GetIndex() == suspects.Size()) && (saveIter.GetIndex() != suspectsSinceLastSave.Size()))
			{
				in_addr_t key = newSuspect->GetIpAddress();
				deletedKeys.push_back(key);
				suspectsSinceLastSave.Erase(key);
			}

			vector<in_addr_t>::iterator vIter = find (deletedKeys.begin(), deletedKeys.end(), newSuspect->GetIpAddress());
			if (vIter != deletedKeys.end())
			{
				// Shift the rest of the data over on top of our bad suspect
				memmove(tableBuffer+bytesSoFar,
						tableBuffer+bytesSoFar+suspectBytes,
						(dataSize-bytesSoFar-suspectBytes)*sizeof(tableBuffer[0]));
				dataSize -= suspectBytes;
			}
			else
			{
				bytesSoFar += suspectBytes;
			}

			delete newSuspect;
		}
		suspects.Unlock();

		// If the entry is valid still, write it to the tmp file
		if (dataSize > 0)
		{
			out.write((char*) &timeStamp, sizeof timeStamp);
			out.write((char*) &dataSize, sizeof dataSize);
			out.write((char*) tableBuffer, dataSize);
		}
	}

	out.close();
	in.close();

	string copyCommand = "cp -f " + tmpFile + " " + Config::Inst()->getPathCESaveFile();
	if (system(copyCommand.c_str()) == -1)
		LOG(ERROR, "Failed to write to the CE state file. This may be a permission problem, or the folder may not exist.",
				(format("File %1% at line %2%: Unable to copy CE state tmp file to CE state file. System call to '%3' failed")
						%__LINE__%__FILE__%copyCommand).str());
}

void Nova::Reload()
{
	// Aquire a lock to stop the other threads from classifying till we're done
	suspects.Wrlock();
	suspectsSinceLastSave.Wrlock();

	LoadConfiguration();

	// Reload the configuration file
	Config::Inst()->LoadConfig();

	// Did our data file move?
	Config::Inst()->getPathTrainingFile() = userHomePath + "/" +Config::Inst()->getPathTrainingFile();

	engine->LoadDataPointsFromFile(Config::Inst()->getPathTrainingFile());

	// Set everyone to be reclassified
	for(SuspectTableIterator it = suspects.Begin(); it.GetIndex() != suspects.Size(); ++it)
	{
		suspects[it.Current().GetIpAddress()].SetNeedsFeatureUpdate(true);
		suspects[it.Current().GetIpAddress()].SetNeedsClassificationUpdate(true);
	}

	// Aquire a lock to stop the other threads from classifying till we're done
	suspectsSinceLastSave.Unlock();
	suspects.Unlock();
}

//Infinite loop that recieves messages from the GUI
void *Nova::GUIListenLoop(void *ptr)
{
	struct sockaddr_un GUIAddress;
	int len;

	if((GUISocket = socket(AF_UNIX,SOCK_STREAM,0)) == -1)
	{
		LOG(CRITICAL, "Unable to make socket to connect to GUI. Is another instance of Nova already running?",
				(format("File %1% at line %2%: Unable to create socket for GUI. Errno: ")%__LINE__%__FILE__%strerror(errno)).str());
		close(GUISocket);
		exit(EXIT_FAILURE);
	}

	GUIAddress.sun_family = AF_UNIX;

	//Builds the key path
	string key = NOVAD_OUT_FILENAME;
	key = userHomePath + key;

	strcpy(GUIAddress.sun_path, key.c_str());
	unlink(GUIAddress.sun_path);
	len = strlen(GUIAddress.sun_path) + sizeof(GUIAddress.sun_family);

	if(bind(GUISocket,(struct sockaddr *)&GUIAddress,len) == -1)
	{
		LOG(CRITICAL, "Unable to make socket to connect to GUI. Is another instance of Nova already running?",
				(format("File %1% at line %2%: Unable to bind to socket for GUI. Errno: ")%__LINE__%__FILE__%strerror(errno)).str());
		close(GUISocket);
		exit(EXIT_FAILURE);
	}

	if(listen(GUISocket, SOCKET_QUEUE_SIZE) == -1)
	{
		LOG(CRITICAL, "Unable to make socket to connect to GUI. Is another instance of Nova already running?",
				(format("File %1% at line %2%: Unable to listen to socket for GUI. Errno: ")%__LINE__%__FILE__%strerror(errno)).str());
		close(GUISocket);
		exit(EXIT_FAILURE);
	}
	while(true)
	{
		ReceiveGUICommand();
	}
}

void *Nova::ClassificationLoop(void *ptr)
{
	//Builds the GUI address
	string GUIKey = userHomePath + NOVAD_IN_FILENAME;
	GUISendRemote.sun_family = AF_UNIX;
	strcpy(GUISendRemote.sun_path, GUIKey.c_str());
	GUILen = strlen(GUISendRemote.sun_path) + sizeof(GUISendRemote.sun_family);

	//Builds the Silent Alarm Network address
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(Config::Inst()->getSaPort());

	LoadStateFile();

	//Classification Loop
	do
	{
		suspects.Wrlock();
		sleep(Config::Inst()->getClassificationTimeout());
		//Calculate the "true" Feature Set for each Suspect
		// XXX 'suspects' SuspectTableIterator todo
		for(SuspectTableIterator it = suspects.Begin(); it.GetIndex() != suspects.Size(); ++it)
		{
			if(it.Current().GetNeedsFeatureUpdate())
			{
				if(suspects[it.GetKey()].UpdateEvidence())
				{
					suspects.Unlock();
					suspects[it.GetKey()].SetOwner(pthread_self());
					suspects.Wrlock();
					suspects[it.GetKey()].UpdateEvidence();
					suspects[it.GetKey()].CalculateFeatures(featureMask);
					suspects[it.GetKey()].ResetOwner();
				}
				else
				{
					suspects[it.GetKey()].CalculateFeatures(featureMask);
				}
			}
		}
		//Calculate the normalized feature sets, actually used by ANN
		//	Writes into Suspect ANNPoints
		engine->NormalizeDataPoints();

		//Perform classification on each suspect
		for(SuspectTableIterator it = suspects.Begin(); it.GetIndex() != suspects.Size(); ++it)
		{
			if(it.Current().GetNeedsClassificationUpdate())
			{
				oldClassification = it.Current().GetIsHostile();
				engine->Classify(&suspects[it.GetKey()]);

				//If suspect is hostile and this Nova instance has unique information
				// 			(not just from silent alarms)
				if(it.Current().GetIsHostile() || oldClassification)
				{
					if(it.Current().GetIsLive())
						SilentAlarm(&suspects[it.GetKey()]);
				}
				SendToUI(&suspects[it.GetKey()]);
			}
		}
		suspects.Unlock();

		if(globalConfig->getSaveFreq() > 0)
		{
			if ((time(NULL) - lastSaveTime) > globalConfig->getSaveFreq())
		if (Config::Inst()->getSaveFreq() > 0)
			if ((time(NULL) - lastSaveTime) > Config::Inst()->getSaveFreq())
			{
				AppendToStateFile();
			}
		}

		if (Config::Inst()->getDataTTL() > 0)
		{
			if((time(NULL) - lastLoadTime) > globalConfig->getDataTTL())
			{
				AppendToStateFile();
				RefreshStateFile();
				suspects.Clear();
				suspectsSinceLastSave.Clear();
				LoadStateFile();
			}

	} while(Config::Inst()->getClassificationTimeout());

	//Shouldn't get here!!
	if(Config::Inst()->getClassificationTimeout())
		LOG(CRITICAL, "The code should never get here, something went very wrong.", (format("File %1% at line %2%: Should never get here")%__LINE__%__FILE__).str());

	return NULL;
}


void *Nova::TrainingLoop(void *ptr)
{
	//Builds the GUI address
	string GUIKey = userHomePath + NOVAD_IN_FILENAME;
	GUISendRemote.sun_family = AF_UNIX;
	strcpy(GUISendRemote.sun_path, GUIKey.c_str());
	GUILen = strlen(GUISendRemote.sun_path) + sizeof(GUISendRemote.sun_family);

	// We suffix the training capture files with the date/time
	time_t rawtime;
	time ( &rawtime );
	struct tm * timeinfo = localtime(&rawtime);
	char buffer [40];
	strftime (buffer,40,"%m-%d-%y_%H-%M-%S",timeinfo);


	string trainingCapFile = userHomePath + "/" + Config::Inst()->getPathTrainingCapFolder() + "/training" + buffer + ".dump";

	//Training Loop
	do
	{
		sleep(Config::Inst()->getClassificationTimeout());
		ofstream myfile (trainingCapFile.data(), ios::app);

		if (myfile.is_open())
		{
			suspects.Wrlock();
			//Calculate the "true" Feature Set for each Suspect
			for(SuspectTableIterator it = suspects.Begin(); it.GetIndex() != suspects.Size(); ++it)
			{

				if(it.Current().GetNeedsFeatureUpdate())
				{
					ANNpoint aNN = it.Current().GetAnnPoint();
					suspects[it.GetKey()].CalculateFeatures(featureMask);
					if(aNN == NULL)
						aNN = annAllocPt(DIM);

					for(int j=0; j < DIM; j++)
					{
						aNN[j] = it.Current().GetFeatureSet().m_features[j];

						myfile << string(inet_ntoa(it.Current().GetInAddr())) << " ";
						for(int j=0; j < DIM; j++)
						{
							myfile << aNN[j] << " ";
						}
						myfile << "\n";
					}
					suspects[it.GetKey()].SetAnnPoint(aNN);
					suspects[it.GetKey()].SetNeedsFeatureUpdate(false);
					//cout << it->second->ToString(featureEnabled);
					SendToUI(&suspects[it.GetKey()]);
				}
			}
			suspects.Unlock();
		}
		else
		{
			LOG(CRITICAL, (format("File %1% at line %2%: Unable to open the training capture file %3% for writing. Can not save training data.")
					%__LINE__%__FILE__%trainingCapFile).str());
		}
		myfile.close();
	} while(Config::Inst()->getClassificationTimeout());

	//Shouldn't get here!
	if (Config::Inst()->getClassificationTimeout())
		LOG(CRITICAL, "The code should never get here, something went very wrong.", (format("File %1% at line %2%: Should never get here")
				%__LINE__%__FILE__).str());

	return NULL;
}

void *Nova::SilentAlarmLoop(void *ptr)
{
	int sockfd;
	u_char buf[MAX_MSG_SIZE];
	struct sockaddr_in sendaddr;

	if((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1)
	{
		LOG(CRITICAL, (format("File %1% at line %2%: Unable to create the silent alarm socket. Errno: %3%")%__LINE__%__FILE__%strerror(errno)).str());
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	sendaddr.sin_family = AF_INET;
	sendaddr.sin_port = htons(Config::Inst()->getSaPort());
	sendaddr.sin_addr.s_addr = INADDR_ANY;

	memset(sendaddr.sin_zero,'\0', sizeof sendaddr.sin_zero);
	struct sockaddr* sockaddrPtr = (struct sockaddr*) &sendaddr;
	socklen_t sendaddrSize = sizeof sendaddr;

	if(bind(sockfd,sockaddrPtr,sendaddrSize) == -1)
	{
		LOG(CRITICAL, (format("File %1% at line %2%: Unable to bind to the silent alarm socket. Errno: %3%")%__LINE__%__FILE__%strerror(errno)).str());
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	stringstream ss;
	ss << "sudo iptables -A INPUT -p udp --dport " << Config::Inst()->getSaPort() << " -j REJECT --reject-with icmp-port-unreachable";
	if(system(ss.str().c_str()) == -1)
	{
	    LOG(ERROR, "Failed to update iptables.", "Failed to update iptables.");
	}
	ss.str("");
	ss << "sudo iptables -A INPUT -p tcp --dport " << Config::Inst()->getSaPort() << " -j REJECT --reject-with tcp-reset";
	if(system(ss.str().c_str()) == -1)
	{
	    LOG(ERROR, "Failed to update iptables.", "Failed to update iptables.");
	}

    if(listen(sockfd, SOCKET_QUEUE_SIZE) == -1)
    {
    	LOG(CRITICAL, (format("File %1% at line %2%: Unable to listen on the silent alarm socket. Errno: %3%")%__LINE__%__FILE__%strerror(errno)).str());
		close(sockfd);
        exit(EXIT_FAILURE);
    }

	int connectionSocket, bytesRead;

	//Accept incoming Silent Alarm TCP Connections
	while(1)
	{

		bzero(buf, MAX_MSG_SIZE);

		//Blocking call
		if((connectionSocket = accept(sockfd, sockaddrPtr, &sendaddrSize)) == -1)
		{
			LOG(ERROR, (format("File %1% at line %2%: Problem when accepting incoming silent alarm connection. Errno: %3%")
					%__LINE__%__FILE__%strerror(errno)).str());
			close(connectionSocket);
			continue;
		}

		if((bytesRead = recv(connectionSocket, buf, MAX_MSG_SIZE, MSG_WAITALL)) == -1)
		{
			LOG(CRITICAL, (format("File %1% at line %2%: Problem when receiving incoming silent alarm connection. Errno: %3%")
					%__LINE__%__FILE__%strerror(errno)).str());
			close(connectionSocket);
			continue;
		}

		//If this is from ourselves, then drop it.
		if(hostAddr.sin_addr.s_addr == sendaddr.sin_addr.s_addr)
		{
			close(connectionSocket);
			continue;
		}
		CryptBuffer(buf, bytesRead, DECRYPT);

		suspects.Wrlock();

		uint addr = GetSerializedAddr(buf);
		SuspectTableIterator it = suspects.Find(addr);

		//If this is a new suspect put it in the table
		if(it.GetIndex() == suspects.Size())
		{
			Suspect * s = new Suspect();
			s->DeserializeSuspectWithData(buf, BROADCAST_DATA);
			s->SetIsHostile(false);
			//We set isHostile to false so that when we classify the first time
			// the suspect will go from benign to hostile and be sent to the doppelganger module
			suspects.AddNewSuspect(s);
		}
		//If this suspect exists, update the information
		else
		{
			//This function will overwrite everything except the information used to calculate the classification
			// a combined classification will be given next classification loop
			suspects[addr].DeserializeSuspectWithData(buf, BROADCAST_DATA);
		}
		suspects[addr].SetFlaggedByAlarm(true);
		//We need to move host traffic data from broadcast into the bin for this host, and remove the old bin
		LOG(CRITICAL, (format("File %1% at line %2%: Got a silent alarm!. Suspect: %3%")%__LINE__%__FILE__%suspects[addr]->ToString()).str());

		suspects.Unlock();

		if(!Config::Inst()->getClassificationTimeout())
			ClassificationLoop(NULL);

		close(connectionSocket);
	}
	close(sockfd);
	LOG(CRITICAL, "The code should never get here, something went very wrong.", (format("File %1% at line %2%: Should never get here")%__LINE__%__FILE__).str());
	return NULL;
}



/*void Nova::FormKdTree()
{
	delete kdTree;
	//Normalize the data points
	//Foreach data point
	for(int j = 0;j < enabledFeatures;j++)
	{
		//Foreach feature within the data point
		for(int i=0;i < nPts;i++)
		{
			if(maxFeatureValues[j] != 0)
			{
				normalizedDataPts[i][j] = Normalize(normalization[j], dataPts[i][j], minFeatureValues[j], maxFeatureValues[j]);
			}
			else
			{
				logger->Log(ERROR, (format("File %1% at line %2%: The max value of a feature was 0. Is the training data file corrupt or missing?")%__LINE__%__FILE__).str());
				break;
			}
		}
	}
	kdTree = new ANNkd_tree(					// build search structure
			normalizedDataPts,					// the data points
					nPts,						// number of points
					enabledFeatures);						// dimension of space
	updateKDTree = false;
}


void Nova::Classify(Suspect *suspect)
{
	int k = globalConfig->getK();
	double d;
	ANNidxArray nnIdx = new ANNidx[k];			// allocate near neigh indices
	ANNdistArray dists = new ANNdist[k];		// allocate near neighbor dists
	ANNpoint aNN = suspect->GetAnnPoint();
	featureIndex fi;

	kdTree->annkSearch(							// search
			aNN,								// query point
			k,									// number of near neighbors
			nnIdx,								// nearest neighbors (returned)
			dists,								// distance (returned)
			globalConfig->getEps());								// error bound

	for(int i = 0; i < DIM; i++)
	{
		fi = (featureIndex)i;
		suspect->SetFeatureAccuracy(fi, 0);
	}

	suspect->SetHostileNeighbors(0);

	//Determine classification according to weight by distance
	//	.5 + E[(1-Dist) * Class] / 2k (Where Class is -1 or 1)
	//	This will make the classification range from 0 to 1
	double classifyCount = 0;

	for(int i = 0; i < k; i++)
	{
		dists[i] = sqrt(dists[i]);				// unsquare distance
		for(int j = 0; j < DIM; j++)
		{
			if (featureEnabled[j])
			{
				double distance = (aNN[j] - kdTree->thePoints()[nnIdx[i]][j]);

				if (distance < 0)
				{
					distance *= -1;
				}

				fi = (featureIndex)j;
				d  = suspect->GetFeatureAccuracy(fi) + distance;
				suspect->SetFeatureAccuracy(fi, d);
			}
		}

		if(nnIdx[i] == -1)
		{
			logger->Log(ERROR, (format("File %1% at line %2%: Unable to find a nearest neighbor for Data point %3% Try decreasing the Error bound")
					%__LINE__%__FILE__%i).str());
		}
		else
		{
			//If Hostile
			if(dataPtsWithClass[nnIdx[i]]->m_classification == 1)
			{
				classifyCount += (sqrtDIM - dists[i]);
				suspect->SetHostileNeighbors(suspect->GetHostileNeighbors()+1);
			}
			//If benign
			else if(dataPtsWithClass[nnIdx[i]]->m_classification == 0)
			{
				classifyCount -= (sqrtDIM - dists[i]);
			}
			else
			{
				//error case; Data points must be 0 or 1
				logger->Log(ERROR, (format("File %1% at line %2%: Data point has invalid classification. Should by 0 or 1, but is %3%")
						%__LINE__%__FILE__%dataPtsWithClass[nnIdx[i]]->m_classification).str());

				suspect->SetClassification(-1);
				delete [] nnIdx;							// clean things up
				delete [] dists;
				annClose();
				return;
			}
		}
	}
	for(int j = 0; j < DIM; j++)
	{
		fi = (featureIndex)j;
		d = suspect->GetFeatureAccuracy(fi) / k;
		suspect->SetFeatureAccuracy(fi, d);
	}


	suspect->SetClassification(.5 + (classifyCount / ((2.0 * (double)k) * sqrtDIM )));

	// Fix for rounding errors caused by double's not being precise enough if DIM is something like 2
	if (suspect->GetClassification() < 0)
		suspect->SetClassification(0);
	else if (suspect->GetClassification() > 1)
		suspect->SetClassification(1);

	if( suspect->GetClassification() > globalConfig->getClassificationThreshold())
	{
		suspect->SetIsHostile(true);
	}
	else
	{
		suspect->SetIsHostile(false);
	}
	delete [] nnIdx;							// clean things up
    delete [] dists;

    annClose();
	suspect->SetNeedsClassificationUpdate(false);
}


void Nova::NormalizeDataPoints()
{
	for(SuspectTableIterator it = suspects.Begin(); it.GetIndex() != suspects.Size(); ++it)
	{
		// Used for matching the 0->DIM index with the 0->enabledFeatures index
		int ai = 0;
		FeatureSet fs = it.Current().GetFeatureSet();
		for(int i = 0;i < DIM;i++)
		{
			if (featureEnabled[i])
			{
				if(fs.m_features[i] > maxFeatureValues[ai])
				{
					fs.m_features[i] = maxFeatureValues[ai];
					//For proper normalization the upper bound for a feature is the max value of the data.
				}
				else if (fs.m_features[i] < minFeatureValues[ai])
				{
					fs.m_features[i] = minFeatureValues[ai];
				}
				ai++;
			}
		}
		suspects[it.GetKey()].SetFeatureSet(&fs);
	}

	//Normalize the suspect points
	for(SuspectTableIterator it = suspects.Begin(); it.GetIndex() != suspects.Size(); ++it)
	{
		if(it.Current().GetNeedsFeatureUpdate())
		{
			int ai = 0;
			ANNpoint aNN = it.Current().GetAnnPoint();
			if(aNN == NULL)
			{
				suspects[it.GetKey()].SetAnnPoint(annAllocPt(enabledFeatures));
				aNN = it.Current().GetAnnPoint();
			}

			for(int i = 0; i < DIM; i++)
			{
				if (featureEnabled[i])
				{
					if(maxFeatureValues[ai] != 0)
					{
						aNN[ai] = Normalize(normalization[i], it.Current().GetFeatureSet().m_features[i], minFeatureValues[ai], maxFeatureValues[ai]);
					}
					else
					{
						logger->Log(ERROR, (format("File %1% at line %2%: Max value for a feature is 0. Normalization failed. Is the training data corrupt or missing?")
								%__LINE__%__FILE__).str());
					}
					ai++;
				}
			}
			suspects[it.GetKey()].SetAnnPoint(aNN);
			suspects[it.GetKey()].SetNeedsFeatureUpdate(false);
		}
	}
}


void Nova::PrintPt(ostream &out, ANNpoint p)
{
	out << "(" << p[0];
	for(int i = 1;i < enabledFeatures;i++)
	{
		out << ", " << p[i];
	}
	out << ")\n";
}


void Nova::LoadDataPointsFromFile(string inFilePath)
{
	ifstream myfile (inFilePath.data());
	string line;

	//string array to check whether a line in data.txt file has the right number of fields
	string fieldsCheck[DIM];
	bool valid = true;

	int i = 0;
	int k = 0;
	int badLines = 0;

	//Count the number of data points for allocation
	if (myfile.is_open())
	{
		while (!myfile.eof())
		{
			if(myfile.peek() == EOF)
			{
				break;
			}
			getline(myfile,line);
			i++;
		}
	}

	else
	{
		logger->Log(ERROR, (format("File %1% at line %2%: Unable to open the training data file at %3%")%__LINE__%__FILE__%globalConfig->getPathTrainingFile()).str());
	}

	myfile.close();
	int maxPts = i;

	//Open the file again, allocate the number of points and assign
	myfile.open(inFilePath.data(), ifstream::in);
	dataPts = annAllocPts(maxPts, enabledFeatures); // allocate data points
	normalizedDataPts = annAllocPts(maxPts, enabledFeatures);


	if (myfile.is_open())
	{
		i = 0;

		while (!myfile.eof() && (i < maxPts))
		{
			k = 0;

			if(myfile.peek() == EOF)
			{
				break;
			}

			//initializes fieldsCheck to have all array indices contain the string "NotPresent"
			for(int j = 0; j < DIM; j++)
			{
				fieldsCheck[j] = "NotPresent";
			}

			//this will grab a line of values up to a newline or until DIM values have been taken in.
			while(myfile.peek() != '\n' && k < DIM)
			{
				getline(myfile, fieldsCheck[k], ' ');
				k++;
			}

			//starting from the end of fieldsCheck, if NotPresent is still inside the array, then
			//the line of the data.txt file is incorrect, set valid to false. Note that this
			//only works in regards to the 9 data points preceding the classification,
			//not the classification itself.
			for(int m = DIM - 1; m >= 0 && valid; m--)
			{
				if(!fieldsCheck[m].compare("NotPresent"))
				{
					valid = false;
				}
			}

			//if the next character is a newline after extracting as many data points as possible,
			//then the classification is not present. For now, I will merely discard the line;
			//there may be a more elegant way to do it. (i.e. pass the data to Classify or something)
			if(myfile.peek() == '\n' || myfile.peek() == ' ')
			{
				valid = false;
			}

			//if the line is valid, continue as normal
			if(valid)
			{
				dataPtsWithClass.push_back(new Point(enabledFeatures));

				// Used for matching the 0->DIM index with the 0->enabledFeatures index
				int actualDimension = 0;
				for(int defaultDimension = 0;defaultDimension < DIM;defaultDimension++)
				{
					double temp = strtod(fieldsCheck[defaultDimension].data(), NULL);

					if (featureEnabled[defaultDimension])
					{
						dataPtsWithClass[i]->m_annPoint[actualDimension] = temp;
						dataPts[i][actualDimension] = temp;

						//Set the max values of each feature. (Used later in normalization)
						if(temp > maxFeatureValues[actualDimension])
							maxFeatureValues[actualDimension] = temp;

						meanFeatureValues[actualDimension] += temp;

						actualDimension++;
					}
				}
				getline(myfile,line);
				dataPtsWithClass[i]->m_classification = atoi(line.data());
				i++;
			}
			//but if it isn't, just get to the next line without incrementing i.
			//this way every correct line will be inserted in sequence
			//without any gaps due to perhaps multiple line failures, etc.
			else
			{
				getline(myfile,line);
				badLines++;
			}
		}
		nPts = i;

		for(int j = 0; j < DIM; j++)
			meanFeatureValues[j] /= nPts;
	}
	else
	{
		logger->Log(ERROR, (format("File %1% at line %2%: Unable to open the training data file at %3%")%__LINE__%__FILE__%globalConfig->getPathTrainingFile()).str());
	}
	myfile.close();

	//Normalize the data points

	//Foreach feature within the data point
	for(int j = 0;j < enabledFeatures;j++)
	{
		//Foreach data point
		for(int i=0;i < nPts;i++)
		{
			normalizedDataPts[i][j] = Normalize(normalization[j], dataPts[i][j], minFeatureValues[j], maxFeatureValues[j]);
		}
	}

	kdTree = new ANNkd_tree(					// build search structure
			normalizedDataPts,					// the data points
					nPts,						// number of points
					enabledFeatures);						// dimension of space
}

double Nova::Normalize(normalizationType type, double value, double min, double max)
{
	switch (type)
	{
		case LINEAR:
		{
			return value / max;
		}
		case LINEAR_SHIFT:
		{
			return (value -min) / (max - min);
		}
		case LOGARITHMIC:
		{
			if(!value || !max)
				return 0;
			else return(log(value)/log(max));
			//return (log(value - min + 1)) / (log(max - min + 1));
		}
		case NONORM:
		{
			return value;
		}
		default:
		{
			//logger->Logging(ERROR, "Normalization failed: Normalization type unkown");
			return 0;
		}

		// TODO: A sigmoid normalization function could be very useful,
		// especially if we could somehow use it interactively to set the center and smoothing
		// while looking at the data visualizations to see what works best for a feature
	}
}


void Nova::WriteDataPointsToFile(string outFilePath, ANNkd_tree* tree)
{
	ofstream myfile (outFilePath.data());

	if (myfile.is_open())
	{
		for(int i = 0; i < tree->nPoints(); i++ )
		{
			for(int j=0; j < tree->theDim(); j++)
			{
				myfile << tree->thePoints()[i][j] << " ";
			}
			myfile << dataPtsWithClass[i]->m_classification;
			myfile << "\n";
		}
	}
	else
	{
		logger->Log(ERROR, (format("File %1% at line %2%: Unable to open the training data file at %3%")%__LINE__%__FILE__%outFilePath).str());

	}
	myfile.close();

}*/

void Nova::SilentAlarm(Suspect *suspect)
{
	char suspectAddr[INET_ADDRSTRLEN];
	string commandLine;
	string hostAddrString = GetLocalIP(Config::Inst()->getInterface().c_str());
	u_char serializedBuffer[MAX_MSG_SIZE];

	uint dataLen = suspect->SerializeSuspect(serializedBuffer);

	//If the hostility hasn't changed don't bother the DM
	if(oldClassification != suspect->GetIsHostile())
	{
		if(suspect->GetIsHostile() && Config::Inst()->getIsDmEnabled())
		{
			in_addr temp = suspect->GetInAddr();
			inet_ntop(AF_INET, &(temp), suspectAddr, INET_ADDRSTRLEN);

			commandLine = "sudo iptables -t nat -A PREROUTING -d ";
			commandLine += hostAddrString;
			commandLine += " -s ";
			commandLine += suspectAddr;
			commandLine += " -j DNAT --to-destination ";
			commandLine += Config::Inst()->getDoppelIp();

			system(commandLine.c_str());
		}
		else
		{
			in_addr temp = suspect->GetInAddr();
			inet_ntop(AF_INET, &(temp), suspectAddr, INET_ADDRSTRLEN);

			commandLine = "sudo iptables -t nat -D PREROUTING -d ";
			commandLine += hostAddrString;
			commandLine += " -s ";
			commandLine += suspectAddr;
			commandLine += " -j DNAT --to-destination ";
			commandLine += Config::Inst()->getDoppelIp();

			system(commandLine.c_str());
		}
	}
	if(suspect->GetFeatureSet().m_unsentData->m_packetCount)
	{
		do
		{
			dataLen = suspect->SerializeSuspect(serializedBuffer);

			// Serialize the unsent data
			dataLen += suspect->GetFeatureSet().m_unsentData->SerializeFeatureData(serializedBuffer+dataLen);
			// Move the unsent data to the sent side
			FeatureSet fs = suspect->GetFeatureSet();
			fs.UpdateFeatureData(true);
			suspect->SetFeatureSet(&fs);
			// Clear the unsent data
			suspect->ClearUnsentData();

			//Update other Nova Instances with latest suspect Data
			for(uint i = 0; i < Config::Inst()->getNeighbors().size(); i++)
			{
				serv_addr.sin_addr.s_addr = Config::Inst()->getNeighbors()[i];

				stringstream ss;
				string commandLine;

				ss << "sudo iptables -I INPUT -s " << string(inet_ntoa(serv_addr.sin_addr)) << " -p tcp -j ACCEPT";
				commandLine = ss.str();

				if(system(commandLine.c_str()) == -1)
				{
					LOG(ERROR, "Failed to update iptables.", "Failed to update iptables.");
				}


				int i;
				for(i = 0; i < Config::Inst()->getSaMaxAttempts(); i++)
				{
					if(KnockPort(OPEN))
					{
						//Send Silent Alarm to other Nova Instances with feature Data
						if ((sockfd = socket(AF_INET,SOCK_STREAM,6)) == -1)
						{
							LOG(ERROR, (format("File %1% at line %2%: Unable to open socket to send silent alarm. Errno: %3%")
									%__LINE__%__FILE__%strerror(errno)).str());
							close(sockfd);
							continue;
						}
						if (connect(sockfd, serv_addrPtr, inSocketSize) == -1)
						{
							//If the host isn't up we stop trying
							if(errno == EHOSTUNREACH)
							{
								//set to max attempts to hit the failed connect condition
								i = Config::Inst()->getSaMaxAttempts();
								LOG(ERROR, (format("File %1% at line %2%: Unable to connect to host to send silent alarm. Errno: %3%")
										%__LINE__%__FILE__%strerror(errno)).str());
								break;
							}
							LOG(ERROR, (format("File %1% at line %2%: Unable to open socket to send silent alarm. Errno: %3%")
									%__LINE__%__FILE__%strerror(errno)).str());
							close(sockfd);
							continue;
						}
						break;
					}
				}
				//If connecting failed
				if(i == Config::Inst()->getSaMaxAttempts() )
				{
					close(sockfd);
					ss.str("");
					ss << "sudo iptables -D INPUT -s " << string(inet_ntoa(serv_addr.sin_addr)) << " -p tcp -j ACCEPT";
					commandLine = ss.str();
					if(system(commandLine.c_str()) == -1)
					{
						LOG(ERROR, "Failed to update iptables.", "Failed to update iptables.");
					}
					continue;
				}

				if( send(sockfd, serializedBuffer, dataLen, 0) == -1)
				{
					LOG(ERROR, (format("File %1% at line %2%: Error in TCP Send of silent alarm. Errno: %3%")%__LINE__%__FILE__%strerror(errno)).str());
					close(sockfd);
					ss.str("");
					ss << "sudo iptables -D INPUT -s " << string(inet_ntoa(serv_addr.sin_addr)) << " -p tcp -j ACCEPT";
					commandLine = ss.str();
					if(system(commandLine.c_str()) == -1)
					{
						LOG(ERROR, "Failed to update iptables.", "Failed to update iptables.");
					}
					continue;
				}
				close(sockfd);
				KnockPort(CLOSE);
				ss.str("");
				ss << "sudo iptables -D INPUT -s " << string(inet_ntoa(serv_addr.sin_addr)) << " -p tcp -j ACCEPT";
				commandLine = ss.str();
				if(system(commandLine.c_str()) == -1)
				{
					LOG(ERROR, "Failed to update iptables.", "Failed to update iptables.");
				}
			}
		}while(dataLen == MORE_DATA);
	}
}


bool Nova::KnockPort(bool mode)
{
	stringstream ss;
	ss << Config::Inst()->getKey();

	//mode == OPEN (true)
	if(mode)
		ss << "OPEN";

	//mode == CLOSE (false)
	else
		ss << "SHUT";

	uint keyDataLen = Config::Inst()->getKey().size() + 4;
	u_char keyBuf[1024];
	bzero(keyBuf, 1024);
	memcpy(keyBuf, ss.str().c_str(), ss.str().size());

	CryptBuffer(keyBuf, keyDataLen, ENCRYPT);

	//Send Port knock to other Nova Instances
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 17)) == -1)
	{
		LOG(ERROR, (format("File %1% at line %2%:  Error in port knocking. Can't create socket: %s")%__FILE__%__LINE__%strerror(errno)).str());
		close(sockfd);
		return false;
	}

	if( sendto(sockfd,keyBuf,keyDataLen, 0,serv_addrPtr, inSocketSize) == -1)
	{
		LOG(ERROR, (format("File %1% at line %2%:  Error in UDP Send for port knocking: %s")%__FILE__%__LINE__%strerror(errno)).str());
		close(sockfd);
		return false;
	}

	close(sockfd);
	sleep(Config::Inst()->getSaSleepDuration());
	return true;
}


bool Nova::Start_Packet_Handler()
{
	char errbuf[PCAP_ERRBUF_SIZE];

	int ret;
	usePcapFile = Config::Inst()->getReadPcap();
	string haystackAddresses_csv = "";

	struct bpf_program fp;			/* The compiled filter expression */
	char filter_exp[64];


	haystackAddresses = GetHaystackAddresses(Config::Inst()->getPathConfigHoneydHs());
	haystackDhcpAddresses = GetHaystackDhcpAddresses(dhcpListFile);
	haystackAddresses_csv = ConstructFilterString();

	//If we're reading from a packet capture file
	if(usePcapFile)
	{
		sleep(1); //To allow time for other processes to open
		handle = pcap_open_offline(Config::Inst()->getPathPcapFile().c_str(), errbuf);

		if(handle == NULL)
		{
			LOG(CRITICAL, (format("File %1% at line %2%: Couldn't open pcapc file: %3%: %4%")%__FILE__%__LINE__%Config::Inst()->getPathPcapFile().c_str()%errbuf).str());
			exit(EXIT_FAILURE);
		}
		if (pcap_compile(handle, &fp, haystackAddresses_csv.data(), 0, maskp) == -1)
		{
			LOG(CRITICAL, (format("File %1% at line %2%: Couldn't parse pcap filter: %3%: %4%")%__LINE__%filter_exp%pcap_geterr(handle)).str());
			exit(EXIT_FAILURE);
		}

		if (pcap_setfilter(handle, &fp) == -1)
		{
			LOG(CRITICAL, (format("File %1% at line %2%: Couldn't install pcap filter: %3%: %4%")% __FILE__%__LINE__%filter_exp%pcap_geterr(handle)).str());
			exit(EXIT_FAILURE);
		}
		//First process any packets in the file then close all the sessions
		pcap_loop(handle, -1, Packet_Handler,NULL);

		TCPTimeout(NULL);
		if (!Config::Inst()->getIsTraining())
			ClassificationLoop(NULL);
		else
			TrainingLoop(NULL);

		if(Config::Inst()->getGotoLive()) usePcapFile = false; //If we are going to live capture set the flag.
	}


	if(!usePcapFile)
	{
		//Open in non-promiscuous mode, since we only want traffic destined for the host machine
		handle = pcap_open_live(Config::Inst()->getInterface().c_str(), BUFSIZ, 0, 1000, errbuf);

		if(handle == NULL)
		{
			LOG(ERROR, (format("File %1% at line %2%:  Unable to open the network interface for live capture: %3% %4%")% __FILE__%__LINE__%Config::Inst()->getInterface().c_str()%errbuf).str());
			exit(EXIT_FAILURE);
		}

		/* ask pcap for the network address and mask of the device */
		ret = pcap_lookupnet(Config::Inst()->getInterface().c_str(), &netp, &maskp, errbuf);

		if(ret == -1)
		{
			LOG(ERROR, (format("File %1% at line %2%: Unable to get the network address and mask of the interface. Error: %3%")% __FILE__%__LINE__%errbuf).str());
			exit(EXIT_FAILURE);
		}

		if (pcap_compile(handle, &fp, haystackAddresses_csv.data(), 0, maskp) == -1)
		{
			LOG(ERROR, (format("File %1% at line %2%:  Couldn't parse filter: %3% %4%")% __FILE__%__LINE__% filter_exp%pcap_geterr(handle)).str());
			exit(EXIT_FAILURE);
		}

		if (pcap_setfilter(handle, &fp) == -1)
		{
			LOG(ERROR, (format("File %1% at line %2%:  Couldn't install filter:%3% %4%")% __FILE__%__LINE__% filter_exp%pcap_geterr(handle)).str());
			exit(EXIT_FAILURE);
		}
		//"Main Loop"
		//Runs the function "Packet_Handler" every time a packet is received
		pthread_create(&TCP_timeout_thread, NULL, TCPTimeout, NULL);

	    pcap_loop(handle, -1, Packet_Handler, NULL);
	}
	return false;
}

void Nova::Packet_Handler(u_char *useless,const struct pcap_pkthdr* pkthdr,const u_char* packet)
{
	//Memory assignments moved outside packet handler to increase performance
	int dest_port;
	Packet packet_info;
	struct ether_header *ethernet;  	/* net/ethernet.h */
	struct ip *ip_hdr; 					/* The IP header */
	char tcp_socket[55];

	if(packet == NULL)
	{
		LOG(ERROR, (format("File %1% at line %2%:  Failed to capture packet!")% __FILE__%__LINE__).str());
		return;
	}


	/* let's start with the ether header... */
	ethernet = (struct ether_header *) packet;

	/* Do a couple of checks to see what packet type we have..*/
	if (ntohs (ethernet->ether_type) == ETHERTYPE_IP)
	{
		ip_hdr = (struct ip*)(packet + sizeof(struct ether_header));

		//Prepare Packet structure
		packet_info.ip_hdr = *ip_hdr;
		packet_info.pcap_header = *pkthdr;
		//If this is to the host
		if(packet_info.ip_hdr.ip_dst.s_addr == hostAddr.sin_addr.s_addr)
			packet_info.fromHaystack = FROM_LTM;
		else
			packet_info.fromHaystack = FROM_HAYSTACK_DP;

		//IF UDP or ICMP
		if(ip_hdr->ip_p == 17 )
		{
			packet_info.udp_hdr = *(struct udphdr*) ((char *)ip_hdr + sizeof(struct ip));
			UpdateSuspect(packet_info);
		}
		else if(ip_hdr->ip_p == 1)
		{
			packet_info.icmp_hdr = *(struct icmphdr*) ((char *)ip_hdr + sizeof(struct ip));
			UpdateSuspect(packet_info);
		}
		//If TCP...
		else if(ip_hdr->ip_p == 6)
		{
			packet_info.tcp_hdr = *(struct tcphdr*)((char*)ip_hdr + sizeof(struct ip));
			dest_port = ntohs(packet_info.tcp_hdr.dest);

			bzero(tcp_socket, 55);
			snprintf(tcp_socket, 55, "%d-%d-%d", ip_hdr->ip_dst.s_addr, ip_hdr->ip_src.s_addr, dest_port);

			pthread_rwlock_wrlock(&sessionLock);
			//If this is a new entry...
			if(SessionTable[tcp_socket].session.size() == 0)
			{
				//Insert packet into Hash Table
				SessionTable[tcp_socket].session.push_back(packet_info);
				SessionTable[tcp_socket].fin = false;
			}

			//If there is already a session in progress for the given LogEntry
			else
			{
				//If Session is ending
				//TODO: The session may continue a few packets after the FIN. Account for this case.
				//See ticket #15
				if(packet_info.tcp_hdr.fin)// Runs appendToStateFile before exiting
				{
					SessionTable[tcp_socket].session.push_back(packet_info);
					SessionTable[tcp_socket].fin = true;
				}
				else
				{
					//Add this new packet to the session vector
					SessionTable[tcp_socket].session.push_back(packet_info);
				}
			}
			pthread_rwlock_unlock(&sessionLock);
		}

		// Allow for continuous classification
		if(!Config::Inst()->getClassificationTimeout())
		{
			if (!Config::Inst()->getIsTraining())
				ClassificationLoop(NULL);
			else
				TrainingLoop(NULL);
		}
	}
	else if(ntohs(ethernet->ether_type) == ETHERTYPE_ARP)
	{
		return;
	}
	else
	{
		LOG(ERROR, (format("File %1% at line %2%:  Unknown Non-IP Packet Received. Nova is ignoring it.")% __FILE__%__LINE__).str());
		return;
	}
}


void Nova::ReceiveGUICommand()
{
    struct sockaddr_un msgRemote;
	int socketSize, msgSocket;
	int bytesRead;
	GUIMsg msg = GUIMsg();
	in_addr_t suspectKey = 0;
	u_char msgBuffer[MAX_GUIMSG_SIZE];

	bzero(msgBuffer, MAX_GUIMSG_SIZE);

	socketSize = sizeof(msgRemote);

	//Blocking call
	if ((msgSocket = accept(GUISocket, (struct sockaddr *)&msgRemote, (socklen_t*)&socketSize)) == -1)
	{
		LOG(ERROR, (format("File %1% at line %2%:  Unable to get GUI commands, accept on the socket failed: %s")% __FILE__%__LINE__% strerror(errno)).str());
		close(msgSocket);
	}
	if((bytesRead = recv(msgSocket, msgBuffer, MAX_GUIMSG_SIZE, MSG_WAITALL )) == -1)
	{
		LOG(ERROR, (format("File %1% at line %2%:  Unable to get GUI commands, recv the socket failed: %s")% __FILE__%__LINE__% strerror(errno)).str());
		close(msgSocket);
	}

	msg.DeserializeMessage(msgBuffer);
	switch(msg.GetType())
	{
		case EXIT:
		{
    		system("sudo iptables -F");
			SaveAndExit(0);
		}
		case CLEAR_ALL:
		{
			suspects.Wrlock();
			suspectsSinceLastSave.Wrlock();
			suspects.Clear();
			suspectsSinceLastSave.Clear();
			string delString = "rm -f " + globalConfig->getPathCESaveFile();
			if(system(delString.c_str()) == -1)
				logger->Log(ERROR, (format("File %1% at line %2%:  Unable to delete CE state file. System call to rm failed.")% __FILE__%__LINE__).str());
			suspectsSinceLastSave.Unlock();
			suspects.Unlock();
			break;
		}
		case CLEAR_SUSPECT:
		{
			suspects.Wrlock();
			suspectsSinceLastSave.Wrlock();
			suspectKey = inet_addr(msg.GetValue().c_str());
			suspectsSinceLastSave[suspectKey] = suspects[suspectKey];
			suspects.Erase(suspectKey);
			suspectsSinceLastSave.Erase(suspectKey);
			RefreshStateFile();
			suspectsSinceLastSave.Unlock();
			suspects.Unlock();
			break;
		}
		case WRITE_SUSPECTS:
		{
			SaveSuspectsToFile(msg.GetValue());
			break;
		}
		case RELOAD:
		{
			Reload();
			break;
		}
		default:
		{
			break;
		}
	}
	close(msgSocket);
}


void Nova::SaveSuspectsToFile(string filename)
{
	LOG(NOTICE, (format("File %1% at line %2%:  Got request to save file to %3%")% __FILE__%__LINE__% filename).str());

	ofstream out(filename.c_str());

	if(!out.is_open())
	{
		LOG(ERROR, (format("File %1% at line %2%:  Error: Unable to open file %3% to save suspect data.")% __FILE__%__LINE__% filename).str());
		return;
	}
	suspects.Rdlock();
	for(SuspectTableIterator it = suspects.Begin(); it.GetIndex() != suspects.Size(); ++it)
	{
		out << it.Current().ToString() << endl;
	}
	suspects.Unlock();
	out.close();
}


void Nova::SendToUI(Suspect *suspect)
{
	if (!enableGUI)
		return;

	u_char GUIData[MAX_MSG_SIZE];

	uint GUIDataLen = suspect->SerializeSuspect(GUIData);

	if ((GUISendSocket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		LOG(ERROR, "Unable to connect to GUI", (format("File %1% at line %2%:  Unable to create GUI socket: %3%")% __FILE__%__LINE__% strerror(errno)).str());
		close(GUISendSocket);
		return;
	}

	if (connect(GUISendSocket, GUISendPtr, GUILen) == -1)
	{
		LOG(ERROR, "Unable to connect to GUI", (format("File %1% at line %2%:  Unable to connect to GUI: %3%")% __FILE__%__LINE__% strerror(errno)).str());
		close(GUISendSocket);
		return;
	}

	if (send(GUISendSocket, GUIData, GUIDataLen, 0) == -1)
	{
		LOG(ERROR, "Unable to connect to GUI", (format("File %1% at line %2%:  Unable to send to GUI: %3%")% __FILE__%__LINE__% strerror(errno)).str());
		close(GUISendSocket);
		return;
	}
	close(GUISendSocket);
}


void Nova::LoadConfiguration()
{
	string hostAddrString = GetLocalIP(Config::Inst()->getInterface().c_str());

	if(hostAddrString.size() == 0)
	{
		LOG(ERROR, (format("File %1% at line %2%:  Bad ethernet interface, no IP's associated!")% __FILE__%__LINE__).str());
		exit(EXIT_FAILURE);
	}

	inet_pton(AF_INET,hostAddrString.c_str(),&(hostAddr.sin_addr));


	string enabledFeatureMask = Config::Inst()->getEnabledFeatures();
}


string Nova::ConstructFilterString()
{
	//Flatten out the vectors into a csv string
	string filterString = "";

	for(uint i = 0; i < haystackAddresses.size(); i++)
	{
		filterString += "dst host ";
		filterString += haystackAddresses[i];

		if(i+1 != haystackAddresses.size())
			filterString += " || ";
	}

	if (!haystackDhcpAddresses.empty() && !haystackAddresses.empty())
		filterString += " || ";

	for(uint i = 0; i < haystackDhcpAddresses.size(); i++)
	{
		filterString += "dst host ";
		filterString += haystackDhcpAddresses[i];

		if(i+1 != haystackDhcpAddresses.size())
			filterString += " || ";
	}

	if (filterString == "")
	{
		filterString = "dst host 0.0.0.0";
	}

	LOG(DEBUG, "Pcap filter string is " + filterString);
	return filterString;
}

void *Nova::UpdateIPFilter(void *ptr)
{
	while (true)
	{
		if (watch > 0)
		{
			int BUF_LEN =  (1024 * (sizeof (struct inotify_event)) + 16);
			char buf[BUF_LEN];
			struct bpf_program fp;			/* The compiled filter expression */
			char filter_exp[64];

			// Blocking call, only moves on when the kernel notifies it that file has been changed
			int readLen = read (notifyFd, buf, BUF_LEN);
			if (readLen > 0) {
				watch = inotify_add_watch (notifyFd, dhcpListFile.c_str(), IN_CLOSE_WRITE | IN_MOVED_TO | IN_MODIFY | IN_DELETE);
				haystackDhcpAddresses = GetHaystackDhcpAddresses(dhcpListFile);
				string haystackAddresses_csv = ConstructFilterString();

				if (pcap_compile(handle, &fp, haystackAddresses_csv.data(), 0, maskp) == -1)
					LOG(ERROR, "Unable to enable packet capture", (format("File %1% at line %2%: Couldn't parse pcap filter: %3% %4%")% __FILE__%__LINE__% filter_exp%pcap_geterr(handle)).str());

				if (pcap_setfilter(handle, &fp) == -1)
					LOG(ERROR, "Unable to enable packet capture", (format("File %1% at line %2%:  Couldn't install pcap filter: %3% %4%")% __FILE__%__LINE__% filter_exp%pcap_geterr(handle)).str());
			}
		}
		else
		{
			// This is the case when there's no file to watch, just sleep and wait for it to
			// be created by honeyd when it starts up.
			sleep(2);
			watch = inotify_add_watch (notifyFd, dhcpListFile.c_str(), IN_CLOSE_WRITE | IN_MOVED_TO | IN_MODIFY | IN_DELETE);
		}
	}

	return NULL;
}

vector <string> Nova::GetHaystackDhcpAddresses(string dhcpListFile)
{
	ifstream dhcpFile(dhcpListFile.data());
	vector<string> haystackDhcpAddresses;

	if (dhcpFile.is_open())
	{
		while ( dhcpFile.good() )
		{
			string line;
			getline (dhcpFile,line);
			if (strcmp(line.c_str(), ""))
				haystackDhcpAddresses.push_back(line);
		}
		dhcpFile.close();
	}
	else cout << "Unable to open file";

	return haystackDhcpAddresses;
}

vector <string> Nova::GetHaystackAddresses(string honeyDConfigPath)
{
	//Path to the main log file
	ifstream honeydConfFile (honeyDConfigPath.c_str());
	vector <string> retAddresses;
	retAddresses.push_back(GetLocalIP(Config::Inst()->getInterface().c_str()));

	if( honeydConfFile == NULL)
	{
		LOG(ERROR, (format("File %1% at line %2%:  Error opening log file. Does it exist?")% __FILE__%__LINE__).str());
		exit(EXIT_FAILURE);
	}

	string LogInputLine;

	while(!honeydConfFile.eof())
	{
		stringstream LogInputLineStream;

		//Get the next line
		getline(honeydConfFile, LogInputLine);

		//Load the line into a stringstream for easier tokenizing
		LogInputLineStream << LogInputLine;
		string token;

		//Is the first word "bind"?
		getline(LogInputLineStream, token, ' ');

		if(token.compare( "bind" ) != 0)
		{
			continue;
		}

		//The next token will be the IP address
		getline(LogInputLineStream, token, ' ');
		retAddresses.push_back(token);
	}
	return retAddresses;
}

void *Nova::TCPTimeout(void *ptr)
{
	do
	{
		pthread_rwlock_wrlock(&sessionLock);

		time_t currentTime = time(NULL);
		time_t packetTime;

		for(TCPSessionHashTable::iterator it = SessionTable.begin(); it != SessionTable.end(); it++ )
		{

			if(it->second.session.size() > 0)
			{
				packetTime = it->second.session.back().pcap_header.ts.tv_sec;
				//If were reading packets from a file, assume all packets have been loaded and go beyond timeout threshhold
				if(usePcapFile)
				{
					currentTime = packetTime+3+Config::Inst()->getTcpTimout();
				}
				// If it exists)
				if(packetTime + 2 < currentTime)
				{
					//If session has been finished for more than two seconds
					if(it->second.fin == true)
					{
						for(uint p = 0; p < (SessionTable[it->first].session).size(); p++)
						{
							pthread_rwlock_unlock(&sessionLock);
							UpdateSuspect(SessionTable[it->first].session[p]);
							pthread_rwlock_wrlock(&sessionLock);
						}

						// Allow for continuous classification
						if(!Config::Inst()->getClassificationTimeout())
						{
							pthread_rwlock_unlock(&sessionLock);
							if (!Config::Inst()->getIsTraining())
								ClassificationLoop(NULL);
							else
								TrainingLoop(NULL);
							pthread_rwlock_wrlock(&sessionLock);
						}

						SessionTable[it->first].session.clear();
						SessionTable[it->first].fin = false;
					}
					//If this session is timed out
					else if(packetTime + Config::Inst()->getTcpTimout() < currentTime)
					{
						for(uint p = 0; p < (SessionTable[it->first].session).size(); p++)
						{
							pthread_rwlock_unlock(&sessionLock);
							UpdateSuspect(SessionTable[it->first].session[p]);
							pthread_rwlock_wrlock(&sessionLock);
						}

						// Allow for continuous classification
						if(!Config::Inst()->getClassificationTimeout())
						{
							pthread_rwlock_unlock(&sessionLock);
							if (!Config::Inst()->getIsTraining())
								ClassificationLoop(NULL);
							else
								TrainingLoop(NULL);
							pthread_rwlock_wrlock(&sessionLock);
						}

						SessionTable[it->first].session.clear();
						SessionTable[it->first].fin = false;
					}
				}
			}
		}
		pthread_rwlock_unlock(&sessionLock);
		//Check only once every TCP_CHECK_FREQ seconds
		sleep(Config::Inst()->getTcpCheckFreq());
	}while(!usePcapFile);

	//After a pcap file is read we do one iteration of this function to clear out the sessions
	//This is return is to prevent an error being thrown when there isn't one.
	if(usePcapFile) return NULL;


	LOG(CRITICAL, "The code should never get here, something went very wrong.", (format("File %1% at line %2%: Should never get here")%__LINE__%__FILE__).str());
	return NULL;
}

void Nova::UpdateSuspect(Packet packet)
{
	in_addr_t addr = packet.ip_hdr.ip_src.s_addr;
	suspects.Wrlock();
	//If our suspect is new
	if((suspects.Peek(addr).GetIpAddress()) != addr)
	{
		Suspect sRef = Suspect(packet);
		suspects.AddNewSuspect(&sRef);
	}
	//Else our suspect exists
	else
	{
		suspects[addr].AddEvidence(packet);
	}

	suspects[addr].SetIsLive(usePcapFile);
	suspects.Unlock();
}

