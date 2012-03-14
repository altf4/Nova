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
#include "Control.h"
#include "ClassificationEngine.h"
#include "ProtocolHandler.h"
#include "FeatureSet.h"

#include <vector>
#include <math.h>
#include <string>
#include <errno.h>
#include <fstream>
#include <sstream>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <signal.h>
#include <sys/inotify.h>
#include <netinet/if_ether.h>
#include <iostream>
#include <time.h>

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

int Nova::RunNovaD()
{
	suspects.Resize(INIT_SIZE_SMALL);
	suspectsSinceLastSave.Resize(INIT_SIZE_SMALL);

	SessionTable.set_empty_key("");
	SessionTable.resize(INIT_SIZE_HUGE);

	// Let both of these initialize before we have multiple threads going
	Config::Inst();
	Logger::Inst();
	userHomePath = Config::Inst()->getPathHome();
	novaConfigPath = userHomePath + "/Config/NOVAConfig.txt";
	if(chdir(userHomePath.c_str()) == -1)
		LOG(INFO, "Failed to change directory to " + userHomePath, "Failed to change directory to " + userHomePath);

	pthread_rwlock_init(&sessionLock, NULL);

	signal(SIGINT, SaveAndExit);

	lastLoadTime = time(NULL);
	if (lastLoadTime == ((time_t)-1))
		LOG(ERROR, (format("File %1% at line %2%: Unable to get system time with time()")%__FILE__%__LINE__).str());

	lastSaveTime = time(NULL);
	if (lastSaveTime == ((time_t)-1))
		LOG(ERROR, (format("File %1% at line %2%: Unable to get system time with time()")%__FILE__%__LINE__).str());

	pthread_t classificationLoopThread;
	pthread_t trainingLoopThread;
	pthread_t silentAlarmListenThread;
	pthread_t ipUpdateThread;

	Spawn_UI_Handler();

	engine = new ClassificationEngine(suspects);

	Reload();

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
		LOG(ERROR, (format("File %1% at line %2%: Unable to set up file watcher for the honeyd IP"
				" list file. DHCP addresse in honeyd will not be read")%__FILE__%__LINE__).str());
	}

	Start_Packet_Handler();

	//Shouldn't get here!
	LOG(CRITICAL, (format("File %1% at line %2%: Main thread ended. This should never happen,"
			" something went very wrong.")%__FILE__%__LINE__).str());
	close(IPCsock);

	return EXIT_FAILURE;
}

void Nova::AppendToStateFile()
{
	lastSaveTime = time(NULL);
	if (lastSaveTime == ((time_t)-1))
		LOG(ERROR, (format("File %1% at line %2%: Unable to get timestamp, call to time()"
				" failed")%__FILE__%__LINE__).str());

	// Don't bother saving if no new suspect data, just confuses deserialization
	if (suspectsSinceLastSave.Size() <= 0)
		return;

	u_char tableBuffer[MAX_MSG_SIZE];
	uint32_t dataSize = 0;

	// Compute the total dataSize
	for(SuspectTableIterator it = suspectsSinceLastSave.Begin(); it.GetIndex() <  suspectsSinceLastSave.Size(); ++it)
	{
		if (!it.Current().GetFeatureSet().m_packetCount)
		{
			continue;
		}
		else
		{
			dataSize += it.Current().SerializeSuspectWithData(tableBuffer);
		}
	}
	// No suspects with packets to update
	if (dataSize == 0)
		return;

	ofstream out(Config::Inst()->getPathCESaveFile().data(), ofstream::binary | ofstream::app);
	if(!out.is_open())
	{
		LOG(ERROR, (format("File %1% at line %2%: Unable to open the CE state file %3%")
				%__FILE__%__LINE__%Config::Inst()->getPathCESaveFile()).str());
		return;
	}

	out.write((char*)&lastSaveTime, sizeof lastSaveTime);
	out.write((char*)&dataSize, sizeof dataSize);

	LOG(DEBUG, (format("File %1% at line %2%: Appending %3% bytes to the CE state file")
			%__FILE__%__LINE__%dataSize).str());
	Suspect suspectCopy;
	// Serialize our suspect table
	for(SuspectTableIterator it = suspectsSinceLastSave.Begin(); it.GetIndex() < suspectsSinceLastSave.Size(); ++it)
	{
		if (!it.Current().GetFeatureSet().m_packetCount)
			continue;
		suspectCopy = suspectsSinceLastSave.CheckOut(it.GetKey());
		dataSize = suspectCopy.SerializeSuspectWithData(tableBuffer);
		suspectsSinceLastSave.CheckIn(&suspectCopy);
		out.write((char*) tableBuffer, dataSize);
	}
	out.close();
	suspectsSinceLastSave.Clear();
}

void Nova::LoadStateFile()
{
	time_t timeStamp;
	uint32_t dataSize;

	lastLoadTime = time(NULL);
	if (lastLoadTime == ((time_t)-1))
		LOG(ERROR, (format("File %1% at line %2%: Unable to get timestamp, call to time() failed")
				%__FILE__%__LINE__).str());

	// Open input file
	ifstream in(Config::Inst()->getPathCESaveFile().data(), ios::binary | ios::in);
	if(!in.is_open())
	{
		LOG(ERROR, (format("File %1% at line %2%: Unable to open CE state file. This is normal for "
				"the first run.")%__FILE__%__LINE__).str());
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
			LOG(ERROR, "The CE state file may be corrupt",(format("File %1% at line %2%: CE state file "
				"should have another entry, but only contains %3% more bytes")%__FILE__%__LINE__%lengthLeft).str());
			break;
		}

		in.read((char*) &timeStamp, sizeof timeStamp);
		lengthLeft -= sizeof timeStamp;

		in.read((char*) &dataSize, sizeof dataSize);
		lengthLeft -= sizeof dataSize;

		if (Config::Inst()->getDataTTL() && (timeStamp < lastLoadTime - Config::Inst()->getDataTTL()))
		{
			LOG(DEBUG, (format("File %1% at line %2%: Throwing out old CE state with timestamp of %3%")
					%__FILE__%__LINE__%(int)timeStamp).str());

			in.seekg(dataSize, ifstream::cur);
			lengthLeft -= dataSize;
			continue;
		}

		// Not as many bytes left as the size of the entry?
		if (lengthLeft < dataSize)
		{
			LOG(ERROR, "The CE state file may be corruput, unable to read all data from it",
				(format("File %1% at line %2%: CE state file should have another entry of size %3% "
				"but only has %4% bytes left")%__FILE__%__LINE__%dataSize%lengthLeft).str());

			break;
		}

		u_char tableBuffer[dataSize];
		in.read((char*) tableBuffer, dataSize);
		lengthLeft -= dataSize;

		// Read each suspect
		uint32_t bytesSoFar = 0;
		Suspect suspectCopy;
		while (bytesSoFar < dataSize)
		{
			Suspect* newSuspect = new Suspect();
			uint32_t suspectBytes = 0;
			suspectBytes += newSuspect->DeserializeSuspect(tableBuffer + bytesSoFar + suspectBytes);
			suspectBytes += newSuspect->GetFeatureSet().DeserializeFeatureData(tableBuffer + bytesSoFar + suspectBytes);
			bytesSoFar += suspectBytes;

			if(!suspects.IsValidKey(newSuspect->GetIpAddress()))
			{
				newSuspect->SetNeedsClassificationUpdate(true);
				newSuspect->SetNeedsFeatureUpdate(true);
				suspects.AddNewSuspect(newSuspect);
			}
			else
			{
				suspectCopy = suspects.CheckOut(newSuspect->GetIpAddress());
				FeatureSet fs = newSuspect->GetFeatureSet();
				suspectCopy.AddFeatureSet(&fs);
				suspectCopy.SetNeedsFeatureUpdate(true);
				suspectCopy.SetNeedsClassificationUpdate(true);
				suspects.CheckIn(&suspectCopy);
				delete newSuspect;
			}
		}
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
		LOG(ERROR, (format("File %1% at line %2%: Unable to get timestamp, call to time() failed")%__FILE__
				%__LINE__).str());

	// Open input file
	ifstream in(Config::Inst()->getPathCESaveFile().data(), ios::binary | ios::in);
	if(!in.is_open())
	{
		LOG(ERROR, (format("File %1% at line %2%: Unable to open the CE state file at %3%")%__FILE__
				%__LINE__%Config::Inst()->getPathCESaveFile()).str());
		return;
	}

	// Open the tmp file
	string tmpFile = Config::Inst()->getPathCESaveFile() + ".tmp";
	ofstream out(tmpFile.data(), ios::binary);
	if(!out.is_open())
	{
		LOG(ERROR, (format("File %1% at line %2%: Unable to open the temporary CE state file at %3%"
				)%__FILE__%__LINE__%tmpFile).str());
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
			LOG(ERROR, "The CE state file may be corrupt", (format("File %1% at line %2%: CE state file "
					"should have another entry, but only contains %3% more bytes")%__FILE__%__LINE__%lengthLeft).str());
			break;
		}

		in.read((char*) &timeStamp, sizeof timeStamp);
		lengthLeft -= sizeof timeStamp;

		in.read((char*) &dataSize, sizeof dataSize);
		lengthLeft -= sizeof dataSize;

		if (Config::Inst()->getDataTTL() && (timeStamp < lastLoadTime - Config::Inst()->getDataTTL()))
		{
			LOG(DEBUG, (format("File %1% at line %2%: Throwing out old CE state with timestamp of %3%")
					%__FILE__%__LINE__%(int)timeStamp).str());

			in.seekg(dataSize, ifstream::cur);
			lengthLeft -= dataSize;
			continue;
		}

		// Not as many bytes left as the size of the entry?
		if (lengthLeft < dataSize)
		{
			LOG(ERROR, "The CE state file may be corrupt",
				(format("File %1% at line %2%: Data file should have another entry of size %3%, "
				"but contains only %4% bytes left")%__FILE__%__LINE__%dataSize%lengthLeft).str());

			break;
		}

		u_char tableBuffer[dataSize];
		in.read((char*) tableBuffer, dataSize);
		lengthLeft -= dataSize;

		// Read each suspect
		uint32_t bytesSoFar = 0;
		while (bytesSoFar < dataSize)
		{
			Suspect* newSuspect = new Suspect();
			uint32_t suspectBytes = 0;
			suspectBytes += newSuspect->DeserializeSuspect(tableBuffer + bytesSoFar + suspectBytes);
			suspectBytes += newSuspect->GetFeatureSet().DeserializeFeatureData(tableBuffer + bytesSoFar + suspectBytes);

			if(!suspects.IsValidKey(newSuspect->GetIpAddress())
					&& suspectsSinceLastSave.IsValidKey(newSuspect->GetIpAddress()))
			{
				in_addr_t key = newSuspect->GetIpAddress();
				suspectsSinceLastSave.Erase(key);
				// Shift the rest of the data over on top of our bad suspect
				memmove(tableBuffer + bytesSoFar, tableBuffer + bytesSoFar + suspectBytes,
						(dataSize - bytesSoFar - suspectBytes) * sizeof(tableBuffer[0]));
				dataSize -= suspectBytes;
			}
			else
			{
				bytesSoFar += suspectBytes;
			}
			delete newSuspect;
		}

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
			(format("File %1% at line %2%: Unable to copy CE state tmp file to CE state file."
			" System call to '%3' failed")%__FILE__%__LINE__%copyCommand).str());
}

void Nova::Reload()
{
	LoadConfiguration();

	// Reload the configuration file
	Config::Inst()->LoadConfig();

	// Did our data file move?
	Config::Inst()->getPathTrainingFile() = userHomePath + "/" +Config::Inst()->getPathTrainingFile();

	engine->LoadDataPointsFromFile(Config::Inst()->getPathTrainingFile());
	Suspect suspectCopy;
	// Set everyone to be reclassified
	for(SuspectTableIterator it = suspects.Begin(); it.GetIndex() < suspects.Size(); ++it)
	{
		suspectCopy = suspects.CheckOut(it.GetKey());
		suspectCopy.SetNeedsFeatureUpdate(true);
		suspectCopy.SetNeedsClassificationUpdate(true);
		suspects.CheckIn(&suspectCopy);
	}
}

void *Nova::ClassificationLoop(void *ptr)
{
	//Builds the GUI address
	string GUIKey = userHomePath + NOVAD_LISTEN_FILENAME;
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
		sleep(Config::Inst()->getClassificationTimeout());
		//Calculate the "true" Feature Set for each Suspect
		// XXX 'suspects' SuspectTableIterator todo
		for(SuspectTableIterator it = suspects.Begin(); it.GetIndex() < suspects.Size(); ++it)
		{
			if(it.Current().GetNeedsFeatureUpdate())
			{
				Suspect suspectCopy = suspects.CheckOut(it.GetKey());
				suspectCopy.UpdateEvidence();
				suspectCopy.CalculateFeatures();
				suspects.CheckIn(&suspectCopy);
			}
		}
		//Calculate the normalized feature sets, actually used by ANN
		//	Writes into Suspect ANNPoints
		engine->NormalizeDataPoints();

		//Perform classification on each suspect
		for(SuspectTableIterator it = suspects.Begin(); it.GetIndex() < suspects.Size(); ++it)
		{
			if(it.Current().GetNeedsClassificationUpdate())
			{
				Suspect suspectCopy = suspects.CheckOut(it.GetKey());
				oldClassification = suspectCopy.GetIsHostile();
				engine->Classify(&suspectCopy);

				//If suspect is hostile and this Nova instance has unique information
				// 			(not just from silent alarms)
				if(suspectCopy.GetIsHostile() || oldClassification)
				{
					if(suspectCopy.GetIsLive())
					{
						SilentAlarm(&suspectCopy);
					}
				}
				if(SendSuspectToUI(&suspectCopy))
				{
					LOG(DEBUG, "Sent a suspect to the UI",
							(format("File %1% at line %2%: Successfully sent suspect")%__FILE__%__LINE__).str());
				}
				else
				{
					LOG(NOTICE, "Failed to send suspect to UI",
							(format("File %1% at line %2%: Sending suspect to UI failed")%__FILE__%__LINE__).str());
				}
				suspects.CheckIn(&suspectCopy);
			}
		}
		if (Config::Inst()->getSaveFreq() > 0)
		{
			if ((time(NULL) - lastSaveTime) > Config::Inst()->getSaveFreq())
			{
				AppendToStateFile();
			}
		}

		if (Config::Inst()->getDataTTL() > 0)
		{
			if((time(NULL) - lastLoadTime) > Config::Inst()->getDataTTL())
			{
				AppendToStateFile();
				RefreshStateFile();
				suspects.Clear();
				suspectsSinceLastSave.Clear();
				LoadStateFile();
			}
		}
	} while(Config::Inst()->getClassificationTimeout() && !usePcapFile);

	if (usePcapFile)
	{
		return NULL;
	}

	//Shouldn't get here!!
	if(Config::Inst()->getClassificationTimeout())
	{
		LOG(CRITICAL, "The code should never get here, something went very wrong.", (format("File %1% "
				"at line %2%: Should never get here")%__FILE__%__LINE__).str());
	}

	return NULL;
}


void *Nova::TrainingLoop(void *ptr)
{
	//Builds the GUI address
	string GUIKey = userHomePath + NOVAD_LISTEN_FILENAME;
	GUISendRemote.sun_family = AF_UNIX;
	strcpy(GUISendRemote.sun_path, GUIKey.c_str());
	GUILen = strlen(GUISendRemote.sun_path) + sizeof(GUISendRemote.sun_family);

	// We suffix the training capture files with the date/time
	time_t rawtime;
	time ( &rawtime );
	struct tm * timeinfo = localtime(&rawtime);
	char buffer [40];
	strftime (buffer,40,"%m-%d-%y_%H-%M-%S",timeinfo);


	string trainingCapFile = userHomePath + "/" + Config::Inst()->getPathTrainingCapFolder() + "/training"
			+ buffer + ".dump";
	Suspect suspectCopy;
	//Training Loop
	do
	{
		sleep(Config::Inst()->getClassificationTimeout());
		ofstream myfile (trainingCapFile.data(), ios::app);

		if (myfile.is_open())
		{
			//Calculate the "true" Feature Set for each Suspect
			for(SuspectTableIterator it = suspects.Begin(); it.GetIndex() < suspects.Size(); ++it)
			{
				if(it.Current().GetNeedsFeatureUpdate())
				{
					suspectCopy = suspects.CheckOut(it.GetKey());
					ANNpoint aNN = annAllocPt(Config::Inst()->getEnabledFeatureCount());
					aNN = suspectCopy.GetAnnPoint();
					suspectCopy.CalculateFeatures();
					if(aNN == NULL)
						aNN = annAllocPt(DIM);

					for(int j=0; j < DIM; j++)
					{
						aNN[j] = suspectCopy.GetFeatureSet().m_features[j];

						myfile << string(inet_ntoa(suspectCopy.GetInAddr())) << " ";
						for(int j=0; j < DIM; j++)
						{
							myfile << aNN[j] << " ";
						}
						myfile << "\n";
					}
					suspectCopy.SetAnnPoint(aNN);
					suspectCopy.SetNeedsFeatureUpdate(false);
					if(SendSuspectToUI(&suspectCopy))
					{
						LOG(DEBUG, "Sent a suspect to the UI",
								(format("File %1% at line %2%: Successfully sent suspect")%__FILE__%__LINE__).str());
					}
					else
					{
						LOG(NOTICE, "Failed to send suspect to UI",
								(format("File %1% at line %2%: Sending suspect to UI failed")%__FILE__%__LINE__).str());
					}
					suspects.CheckIn(&suspectCopy);
					annDeallocPt(aNN);
				}
			}
		}
		else
		{
			LOG(CRITICAL, (format("File %1% at line %2%: Unable to open the training capture file "
					"%3% for writing. Can not save training data.")%__FILE__%__LINE__%trainingCapFile).str());
		}
		myfile.close();
	} while(Config::Inst()->getClassificationTimeout());

	//Shouldn't get here!
	if (Config::Inst()->getClassificationTimeout())
		LOG(CRITICAL, "The code should never get here, something went very wrong.", (format(
				"File %1% at line %2%: Should never get here")%__FILE__%__LINE__).str());

	return NULL;
}

void *Nova::SilentAlarmLoop(void *ptr)
{
	int sockfd;
	u_char buf[MAX_MSG_SIZE];
	struct sockaddr_in sendaddr;

	if((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1)
	{
		LOG(CRITICAL, (format("File %1% at line %2%: Unable to create the silent alarm socket."
				" Errno: %3%")%__FILE__%__LINE__%strerror(errno)).str());
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	sendaddr.sin_family = AF_INET;
	sendaddr.sin_port = htons(Config::Inst()->getSaPort());
	sendaddr.sin_addr.s_addr = INADDR_ANY;

	memset(sendaddr.sin_zero,'\0', sizeof sendaddr.sin_zero);
	struct sockaddr* sockaddrPtr = (struct sockaddr*) &sendaddr;
	socklen_t sendaddrSize = sizeof sendaddr;


	if(::bind(sockfd,sockaddrPtr,sendaddrSize) == -1)
	{
		LOG(CRITICAL, (format("File %1% at line %2%: Unable to bind to the silent alarm socket."
				" Errno: %3%")%__FILE__%__LINE__%strerror(errno)).str());
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	stringstream ss;
	ss << "sudo iptables -A INPUT -p udp --dport " << Config::Inst()->getSaPort() << " -j REJECT"
			" --reject-with icmp-port-unreachable";
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
    	LOG(CRITICAL, (format("File %1% at line %2%: Unable to listen on the silent alarm socket."
    			" Errno: %3%")%__FILE__%__LINE__%strerror(errno)).str());
		close(sockfd);
        exit(EXIT_FAILURE);
    }

	int connectionSocket, bytesRead;
	Suspect suspectCopy;

	//Accept incoming Silent Alarm TCP Connections
	while(1)
	{

		bzero(buf, MAX_MSG_SIZE);

		//Blocking call
		if((connectionSocket = accept(sockfd, sockaddrPtr, &sendaddrSize)) == -1)
		{
			LOG(ERROR, (format("File %1% at line %2%: Problem when accepting incoming silent alarm connection."
					" Errno: %3%")%__FILE__%__LINE__%strerror(errno)).str());
			close(connectionSocket);
			continue;
		}

		if((bytesRead = recv(connectionSocket, buf, MAX_MSG_SIZE, MSG_WAITALL)) == -1)
		{
			LOG(CRITICAL, (format("File %1% at line %2%: Problem when receiving incoming silent alarm connection."
					" Errno: %3%")%__FILE__%__LINE__%strerror(errno)).str());
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

		in_addr_t addr = 0;
		memcpy(&addr, buf, 4);
		uint64_t key = addr;
		Suspect * newSuspect = new Suspect();
		newSuspect->DeserializeSuspectWithData(buf, BROADCAST_DATA);
		//If this suspect exists, update the information
		if(suspects.IsValidKey(key))
		{
			suspectCopy = suspects.CheckOut(key);
			suspectCopy.SetFlaggedByAlarm(true);
			FeatureSet fs = newSuspect->GetFeatureSet();
			suspectCopy.AddFeatureSet(&fs);
			suspects.CheckIn(&suspectCopy);
		}
		//If this is a new suspect put it in the table
		else
		{
			newSuspect->SetIsHostile(false);
			newSuspect->SetFlaggedByAlarm(true);
			//We set isHostile to false so that when we classify the first time
			// the suspect will go from benign to hostile and be sent to the doppelganger module
			suspects.AddNewSuspect(newSuspect);
		}

		//We need to move host traffic data from broadcast into the bin for this host, and remove the old bin
		LOG(CRITICAL, (format("File %1% at line %2%: Got a silent alarm!. Suspect: %3%")
				%__FILE__%__LINE__%suspects[addr].ToString()).str());

		if(!Config::Inst()->getClassificationTimeout())
			ClassificationLoop(NULL);

		close(connectionSocket);
	}
	close(sockfd);
	LOG(CRITICAL, "The code should never get here, something went very wrong.",
			(format("File %1% at line %2%: Should never get here")%__FILE__%__LINE__).str());
	return NULL;
}

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

			if(system(commandLine.c_str()) != 0)
			{
				//TODO Logging
			}
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

			if(system(commandLine.c_str()) != 0)
			{
				//TODO Logging
			}
		}
	}
	if(suspect->GetUnsentFeatureSet().m_packetCount)
	{
		do
		{
			dataLen = suspect->SerializeSuspectWithData(serializedBuffer);
			// Move the unsent data to the sent side
			suspect->UpdateFeatureData(INCLUDE);
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
							LOG(ERROR, (format("File %1% at line %2%: Unable to open socket to send silent alarm."
									" Errno: %3%")%__FILE__%__LINE__%strerror(errno)).str());
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
								LOG(ERROR, (format("File %1% at line %2%: Unable to connect to host to send silent alarm."
										" Errno: %3%")%__FILE__%__LINE__%strerror(errno)).str());
								break;
							}
							LOG(ERROR, (format("File %1% at line %2%: Unable to open socket to send silent alarm."
									" Errno: %3%")%__FILE__%__LINE__%strerror(errno)).str());
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
					LOG(ERROR, (format("File %1% at line %2%: Error in TCP Send of silent alarm."
							" Errno: %3%")%__FILE__%__LINE__%strerror(errno)).str());

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
		LOG(ERROR, (format("File %1% at line %2%:  Error in port knocking. Can't create socket: %3%")
				%__FILE__%__LINE__%strerror(errno)).str());
		close(sockfd);
		return false;
	}

	if( sendto(sockfd,keyBuf,keyDataLen, 0,serv_addrPtr, inSocketSize) == -1)
	{
		LOG(ERROR, (format("File %1% at line %2%:  Error in UDP Send for port knocking: %3%")
				%__FILE__%__LINE__%strerror(errno)).str());
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
			LOG(CRITICAL, (format("File %1% at line %2%: Couldn't open pcapc file: %3%: %4%")
					%__FILE__%__LINE__%Config::Inst()->getPathPcapFile().c_str()%errbuf).str());
			exit(EXIT_FAILURE);
		}
		if (pcap_compile(handle, &fp, haystackAddresses_csv.data(), 0, maskp) == -1)
		{
			LOG(CRITICAL, (format("File %1% at line %2%: Couldn't parse pcap filter: %3%: %4%")
					%__LINE__%filter_exp%pcap_geterr(handle)).str());
			exit(EXIT_FAILURE);
		}

		if (pcap_setfilter(handle, &fp) == -1)
		{
			LOG(CRITICAL, (format("File %1% at line %2%: Couldn't install pcap filter: %3%: %4%")
					% __FILE__%__LINE__%filter_exp%pcap_geterr(handle)).str());
			exit(EXIT_FAILURE);
		}
		//First process any packets in the file then close all the sessions
		pcap_loop(handle, -1, Packet_Handler,NULL);

		TCPTimeout(NULL);

		if(Config::Inst()->getGotoLive()) usePcapFile = false; //If we are going to live capture set the flag.

		LOG(DEBUG, "Done processing PCAP file");
		exit(EXIT_SUCCESS);
	}


	if(!usePcapFile)
	{
		//Open in non-promiscuous mode, since we only want traffic destined for the host machine
		handle = pcap_open_live(Config::Inst()->getInterface().c_str(), BUFSIZ, 0, 1000, errbuf);

		if(handle == NULL)
		{
			LOG(ERROR, (format("File %1% at line %2%:  Unable to open the network interface for live capture:"
					" %3% %4%")% __FILE__%__LINE__%Config::Inst()->getInterface().c_str()%errbuf).str());
			exit(EXIT_FAILURE);
		}

		/* ask pcap for the network address and mask of the device */
		ret = pcap_lookupnet(Config::Inst()->getInterface().c_str(), &netp, &maskp, errbuf);

		if(ret == -1)
		{
			LOG(ERROR, (format("File %1% at line %2%: Unable to get the network address and mask "
					"of the interface. Error: %3%")% __FILE__%__LINE__%errbuf).str());
			exit(EXIT_FAILURE);
		}

		if (pcap_compile(handle, &fp, haystackAddresses_csv.data(), 0, maskp) == -1)
		{
			LOG(ERROR, (format("File %1% at line %2%:  Couldn't parse filter: %3% %4%")
					% __FILE__%__LINE__% filter_exp%pcap_geterr(handle)).str());
			exit(EXIT_FAILURE);
		}

		if (pcap_setfilter(handle, &fp) == -1)
		{
			LOG(ERROR, (format("File %1% at line %2%:  Couldn't install filter:%3% %4%")
					% __FILE__%__LINE__% filter_exp%pcap_geterr(handle)).str());
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
		LOG(ERROR, (format("File %1% at line %2%:  Unknown Non-IP Packet Received. Nova is ignoring it.")
				% __FILE__%__LINE__).str());
		return;
	}
}

void Nova::SaveSuspectsToFile(string filename)
{
	LOG(NOTICE, (format("File %1% at line %2%:  Got request to save file to %3%")% __FILE__%__LINE__% filename).str());

	ofstream out(filename.c_str());

	if(!out.is_open())
	{
		LOG(ERROR, (format("File %1% at line %2%:  Error: Unable to open file %3% to save suspect data.")
				% __FILE__%__LINE__% filename).str());
		return;
	}
	for(SuspectTableIterator it = suspects.Begin(); it.GetIndex() < suspects.Size(); ++it)
	{
		out << it.Current().ToString() << endl;
	}
	out.close();
}

void Nova::LoadConfiguration()
{
	string hostAddrString = GetLocalIP(Config::Inst()->getInterface().c_str());

	if(hostAddrString.size() == 0)
	{
		LOG(ERROR, (format("File %1% at line %2%:  Bad ethernet interface, no IP's associated!")
				% __FILE__%__LINE__).str());
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
				watch = inotify_add_watch (notifyFd, dhcpListFile.c_str(), IN_CLOSE_WRITE | IN_MOVED_TO
						| IN_MODIFY | IN_DELETE);
				haystackDhcpAddresses = GetHaystackDhcpAddresses(dhcpListFile);
				string haystackAddresses_csv = ConstructFilterString();

				if (pcap_compile(handle, &fp, haystackAddresses_csv.data(), 0, maskp) == -1)
					LOG(ERROR, "Unable to enable packet capture", (format("File %1% at line %2%: "
							"Couldn't parse pcap filter: %3% %4%")% __FILE__%__LINE__% filter_exp%
							pcap_geterr(handle)).str());

				if (pcap_setfilter(handle, &fp) == -1)
					LOG(ERROR, "Unable to enable packet capture", (format("File %1% at line %2%:  "
							"Couldn't install pcap filter: %3% %4%")% __FILE__%__LINE__% filter_exp%
							pcap_geterr(handle)).str());
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
				//If were reading packets from a file, assume all packets have been loaded and go beyond
				// timeout threshhold
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
							//pthread_rwlock_unlock(&sessionLock);
							UpdateSuspect(SessionTable[it->first].session[p]);
							//pthread_rwlock_wrlock(&sessionLock);
						}

						// Allow for continuous classification
						if(!Config::Inst()->getClassificationTimeout())
						{
							//pthread_rwlock_unlock(&sessionLock);
							if (!Config::Inst()->getIsTraining())
								ClassificationLoop(NULL);
							else
								TrainingLoop(NULL);
							//pthread_rwlock_wrlock(&sessionLock);
						}

						SessionTable[it->first].session.clear();
						SessionTable[it->first].fin = false;
					}
					//If this session is timed out
					else if(packetTime + Config::Inst()->getTcpTimout() < currentTime)
					{
						for(uint p = 0; p < (SessionTable[it->first].session).size(); p++)
						{
							//pthread_rwlock_unlock(&sessionLock);
							UpdateSuspect(SessionTable[it->first].session[p]);
							//pthread_rwlock_wrlock(&sessionLock);
						}

						// Allow for continuous classification
						if(!Config::Inst()->getClassificationTimeout())
						{
							//pthread_rwlock_unlock(&sessionLock);
							if (!Config::Inst()->getIsTraining())
								ClassificationLoop(NULL);
							else
								TrainingLoop(NULL);
							//pthread_rwlock_wrlock(&sessionLock);
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


	LOG(CRITICAL, "The code should never get here, something went very wrong.", (format("File %1% at line %2%:"
			" Should never get here")%__FILE__%__LINE__).str());
	return NULL;
}

void Nova::UpdateSuspect(Packet packet)
{
	Suspect * newSuspect = new Suspect(packet);
	in_addr_t addr = newSuspect->GetIpAddress();
	//If our suspect is new
	if(!suspects.IsValidKey(addr))
	{
		newSuspect->SetNeedsClassificationUpdate(true);
		newSuspect->SetNeedsFeatureUpdate(true);
		newSuspect->SetIsLive(usePcapFile);
		suspects.AddNewSuspect(newSuspect);
	}
	//Else our suspect exists
	else
	{
		Suspect suspectCopy = suspects.CheckOut(addr);
		suspectCopy.AddEvidence(packet);
		suspectCopy.SetIsLive(usePcapFile);
		suspects.CheckIn(&suspectCopy);
		delete newSuspect;
	}
}
string Nova::GetLocalIP(const char *dev)
{
	static struct ifreq ifreqs[20];
	struct ifconf ifconf;
	uint  nifaces, i;

	memset(&ifconf,0,sizeof(ifconf));
	ifconf.ifc_buf = (char*) (ifreqs);
	ifconf.ifc_len = sizeof(ifreqs);

	int sock, rval;
	sock = socket(AF_INET,SOCK_STREAM,0);

	if(sock < 0)
	{
		LOG(ERROR, "Unable to determine the local interface's IP address", (format("File %1% at line %2%:  Error creating socket to check interface IP: %3%")% __FILE__%__LINE__%strerror(errno)).str());
	}

	if((rval = ioctl(sock, SIOCGIFCONF , (char*) &ifconf)) < 0 )
	{
		LOG(ERROR, "Unable to determine the local interface's IP address", (format("File %1% at line %2%:  Error with getLocalIP socket ioctl(SIOGIFCONF): %3%")% __FILE__%__LINE__%strerror(errno)).str());
	}

	close(sock);
	nifaces =  ifconf.ifc_len/sizeof(struct ifreq);

	for(i = 0; i < nifaces; i++)
	{
		if( strcmp(ifreqs[i].ifr_name, dev) == 0 )
		{
			char ip_addr [ INET_ADDRSTRLEN ];
			struct sockaddr_in *b = (struct sockaddr_in *) &(ifreqs[i].ifr_addr);

			inet_ntop(AF_INET, &(b->sin_addr), ip_addr, INET_ADDRSTRLEN);
			return string(ip_addr);
		}
	}
	return string("");
}
