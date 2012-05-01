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

#include "messaging/MessageManager.h"
#include "ClassificationEngine.h"
#include "ProtocolHandler.h"
#include "SuspectTable.h"
#include "FeatureSet.h"
#include "NovaUtil.h"
#include "Threads.h"
#include "Control.h"
#include "Config.h"
#include "Logger.h"
#include "Point.h"
#include "Novad.h"

#include <vector>
#include <math.h>
#include <time.h>
#include <errno.h>
#include <fstream>
#include <sstream>
#include <sys/un.h>
#include <signal.h>
#include <iostream>
#include <sys/file.h>
#include <sys/inotify.h>
#include <netinet/if_ether.h>

using namespace std;
using namespace Nova;

// Maintains a list of suspects and information on network activity
SuspectTable suspects;

// Suspects not yet written to the state file
SuspectTable suspectsSinceLastSave;
pthread_mutex_t suspectsSinceLastSaveLock;

// TCP session tracking table
TCPSessionHashTable SessionTable;
pthread_rwlock_t sessionLock;

//** Silent Alarm **
struct sockaddr_in serv_addr;
struct sockaddr* serv_addrPtr = (struct sockaddr *) &serv_addr;
struct sockaddr_in hostAddr;

// Timestamps for the CE state file exiration of data
time_t lastLoadTime;
time_t lastSaveTime;

// Time novad started, used for uptime and pcap capture names
time_t startTime;

string trainingCapFile;

ofstream trainingFileStream;

//HS Vars
string dhcpListFile = "/var/log/honeyd/ipList";
vector<string> haystackAddresses;
vector<string> haystackDhcpAddresses;
pcap_t *handle;
bpf_u_int32 maskp; /* subnet mask */
bpf_u_int32 netp; /* ip          */

int notifyFd;
int watch;


ClassificationEngine *engine;

pthread_t classificationLoopThread;
pthread_t trainingLoopThread;
pthread_t silentAlarmListenThread;
pthread_t ipUpdateThread;
pthread_t TCP_timeout_thread;

namespace Nova
{

int RunNovaD()
{
	Config::Inst();
	MessageManager::Initialize(DIRECTION_TO_UI);

	if (!LockNovad())
	{
		cout << "ERROR: Novad is already running. Please close all other instances before continuing." << endl;
		exit(EXIT_FAILURE);
	}

	suspects.Resize(INIT_SIZE_SMALL);
	suspectsSinceLastSave.Resize(INIT_SIZE_SMALL);

	SessionTable.set_empty_key("");
	SessionTable.resize(INIT_SIZE_HUGE);

	pthread_mutex_init(&suspectsSinceLastSaveLock, NULL);
	pthread_rwlock_init(&sessionLock, NULL);

	// Let the logger initialize before we have multiple threads going
	Logger::Inst();

	// Change our working folder into the config folder so our relative paths are correct
	if(chdir(Config::Inst()->GetPathHome().c_str()) == -1)
	{
		LOG(INFO, "Failed to change directory to " + Config::Inst()->GetPathHome(),"");
	}

	// Set up our signal handlers
	signal(SIGKILL, SaveAndExit);
	signal(SIGINT, SaveAndExit);
	signal(SIGTERM, SaveAndExit);
	signal(SIGPIPE, SIG_IGN);

	lastLoadTime = time(NULL);
	if(lastLoadTime == ((time_t)-1))
	{
		LOG(ERROR, "Unable to get timestamp, call to time() failed", "");
	}

	lastSaveTime = time(NULL);
	if(lastSaveTime == ((time_t)-1))
	{
		LOG(ERROR, "Unable to get timestamp, call to time() failed", "");
	}

	startTime = time(NULL);
	if(startTime == ((time_t)-1))
	{
		LOG(ERROR, "Unable to get timestamp, call to time() failed", "");
	}

	//Need to load the configuration before making the Classification Engine for setting up the DM
	//Reload requires a CE object so we do a partial config load here.
	LoadConfiguration();
	//Loads the configuration file
	Config::Inst()->LoadConfig();

	engine = new ClassificationEngine(suspects);

	Spawn_UI_Handler();

	Reload();

	//Are we Training or Classifying?
	if(Config::Inst()->GetIsTraining())
	{
		// We suffix the training capture files with the date/time
		time_t rawtime;
		time(&rawtime);
		struct tm * timeinfo = localtime(&rawtime);
		char buffer[40];
		strftime(buffer, 40, "%m-%d-%y_%H-%M-%S", timeinfo);

		trainingCapFile = Config::Inst()->GetPathHome() + "/"
				+ Config::Inst()->GetPathTrainingCapFolder() + "/training" + buffer
				+ ".dump";


		if (Config::Inst()->GetReadPcap())
		{
			Config::Inst()->SetClassificationThreshold(0);
			Config::Inst()->SetClassificationTimeout(0);

			trainingFileStream.open(trainingCapFile.data(), ios::app);

			if(!trainingFileStream.is_open())
			{
				LOG(CRITICAL, "Unable to open the training capture file.",
					"Unable to open training capture file at: "+trainingCapFile);
				exit(EXIT_FAILURE);
			}
		}
		else
		{
			pthread_create(&trainingLoopThread,NULL,TrainingLoop,NULL);
		}
	}
	else
	{
		pthread_create(&classificationLoopThread,NULL,ClassificationLoop, NULL);
		pthread_create(&silentAlarmListenThread,NULL,SilentAlarmLoop, NULL);
	}

	// If we're not reading from a pcap, monitor for IP changes in the honeyd file
	if (!Config::Inst()->GetReadPcap())
	{
		notifyFd = inotify_init ();

		if(notifyFd > 0)
		{
			watch = inotify_add_watch (notifyFd, dhcpListFile.c_str(), IN_CLOSE_WRITE | IN_MOVED_TO | IN_MODIFY | IN_DELETE);
			pthread_create(&ipUpdateThread, NULL, UpdateIPFilter,NULL);
		}
		else
		{
			LOG(ERROR, "Unable to set up file watcher for the honeyd IP list file.","");
		}
	}

	Start_Packet_Handler();


	if (!Config::Inst()->GetIsTraining())
	{
		//Shouldn't get here!
		LOG(CRITICAL, "Main thread ended. This should never happen, something went very wrong.", "");
		return EXIT_FAILURE;
	}
	else
	{
		return EXIT_SUCCESS;
	}
}

bool LockNovad()
{
	int lockFile = open((Config::Inst()->GetPathHome() + "/novad.lock").data(), O_CREAT | O_RDWR, 0666);
	int rc = flock(lockFile, LOCK_EX | LOCK_NB);
	if(rc)
	{
		// Someone else has this file open
		return false;
	}
	else
	{
		// We have the file open, it will be released if the process dies for any reason
		return true;
	}
}

void MaskKillSignals()
{
	sigset_t x;
	sigemptyset (&x);
	sigaddset(&x, SIGKILL);
	sigaddset(&x, SIGTERM);
	sigaddset(&x, SIGINT);
	sigaddset(&x, SIGPIPE);
	sigprocmask(SIG_BLOCK, &x, NULL);
}

void AppendToStateFile()
{
	pthread_mutex_lock(&suspectsSinceLastSaveLock);
	lastSaveTime = time(NULL);
	if(lastSaveTime == ((time_t)-1))
	{
		LOG(ERROR, "Problem with CE State File", "Unable to get timestamp, call to time() failed");
	}

	// Don't bother saving if no new suspect data, just confuses deserialization
	if(suspectsSinceLastSave.Size() <= 0)
	{
		pthread_mutex_unlock(&suspectsSinceLastSaveLock);
		return;
	}

	u_char tableBuffer[MAX_MSG_SIZE];
	uint32_t dataSize = 0;

	// Compute the total dataSize
	for(SuspectTableIterator it = suspectsSinceLastSave.Begin(); it.GetIndex() <  suspectsSinceLastSave.Size(); ++it)
	{
		Suspect currentSuspect = suspectsSinceLastSave.CheckOut(it.GetKey());
		currentSuspect.UpdateEvidence();
		currentSuspect.UpdateFeatureData(true);
		currentSuspect.CalculateFeatures();
		suspectsSinceLastSave.CheckIn(&currentSuspect);
		if(!currentSuspect.GetFeatureSet().m_packetCount)
		{
			continue;
		}
		else
		{
			dataSize += currentSuspect.Serialize(tableBuffer, true);
		}
	}
	// No suspects with packets to update
	if(dataSize == 0)
	{
		pthread_mutex_unlock(&suspectsSinceLastSaveLock);
		return;
	}

	ofstream out(Config::Inst()->GetPathCESaveFile().data(), ofstream::binary | ofstream::app);
	if(!out.is_open())
	{
		LOG(ERROR, "Problem with CE State File",
			"Unable to open the CE state file at "+ Config::Inst()->GetPathCESaveFile());
		pthread_mutex_unlock(&suspectsSinceLastSaveLock);
		return;
	}

	out.write((char*)&lastSaveTime, sizeof lastSaveTime);
	out.write((char*)&dataSize, sizeof dataSize);

	stringstream ss;
	ss << "Appending " << dataSize << " bytes to the CE state file at " << Config::Inst()->GetPathCESaveFile();
	LOG(DEBUG,ss.str(),"");

	Suspect suspectCopy;
	// Serialize our suspect table
	for(SuspectTableIterator it = suspectsSinceLastSave.Begin(); it.GetIndex() <  suspectsSinceLastSave.Size(); ++it)
	{
		suspectCopy = suspectsSinceLastSave.CheckOut(it.GetKey());

		if(!suspectCopy.GetFeatureSet().m_packetCount)
		{
			continue;
		}
		dataSize = suspectCopy.Serialize(tableBuffer, true);
		suspectsSinceLastSave.CheckIn(&suspectCopy);
		out.write((char*) tableBuffer, dataSize);
	}
	out.close();
	suspectsSinceLastSave.Clear();

	pthread_mutex_unlock(&suspectsSinceLastSaveLock);
}

void LoadStateFile()
{
	time_t timeStamp;
	uint32_t dataSize;

	lastLoadTime = time(NULL);
	if(lastLoadTime == ((time_t)-1))
	{
		LOG(ERROR, "Problem with CE State File", "Unable to get timestamp, call to time() failed");
	}

	// Open input file
	ifstream in(Config::Inst()->GetPathCESaveFile().data(), ios::binary | ios::in);
	if(!in.is_open())
	{
		LOG(WARNING, "Unable to open CE state file.", "");
		return;
	}

	// get length of input for error checking of partially written files
	in.seekg (0, ios::end);
	uint lengthLeft = in.tellg();
	in.seekg (0, ios::beg);

	while (in.is_open() && !in.eof() && lengthLeft)
	{
		// Bytes left, but not enough to make a header (timestamp + size)?
		if(lengthLeft < (sizeof timeStamp + sizeof dataSize))
		{
			LOG(ERROR, "The CE state file may be corruput", "");
			break;
		}

		in.read((char*) &timeStamp, sizeof timeStamp);
		lengthLeft -= sizeof timeStamp;

		in.read((char*) &dataSize, sizeof dataSize);
		lengthLeft -= sizeof dataSize;

		if(Config::Inst()->GetDataTTL() && (timeStamp < lastLoadTime - Config::Inst()->GetDataTTL()))
		{
			stringstream ss;
			ss << "Throwing out old CE state at time: " << timeStamp << ".";
			LOG(DEBUG,"Throwing out old CE state.", ss.str());

			in.seekg(dataSize, ifstream::cur);
			lengthLeft -= dataSize;
			continue;
		}

		// Not as many bytes left as the size of the entry?
		if(lengthLeft < dataSize)
		{
			LOG(ERROR, "The CE state file may be corruput", "");
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
			suspectBytes += newSuspect->Deserialize(tableBuffer + bytesSoFar + suspectBytes);

			FeatureSet fs = newSuspect->GetFeatureSet();
			suspectBytes += fs.DeserializeFeatureData(tableBuffer + bytesSoFar + suspectBytes);
			newSuspect->SetFeatureSet(&fs);
			bytesSoFar += suspectBytes;

			// If our suspect has no evidence, throw it out
			if(newSuspect->GetFeatureSet().m_packetCount == 0)
			{
				LOG(WARNING,"Discarding invalid suspect.",
					"A suspect containing no evidence was detected and discarded");
			}
			else
			{
				if(!suspects.IsValidKey(newSuspect->GetIpAddress()))
				{
					newSuspect->SetNeedsClassificationUpdate(true);
					suspects.AddNewSuspect(newSuspect);
				}
				else
				{
					suspectCopy = suspects.CheckOut(newSuspect->GetIpAddress());
					FeatureSet fs = newSuspect->GetFeatureSet();
					suspectCopy.AddFeatureSet(&fs);
					suspectCopy.SetNeedsClassificationUpdate(true);
					suspects.CheckIn(&suspectCopy);
					delete newSuspect;
				}
			}
		}
	}

	in.close();
}

void RefreshStateFile()
{
	time_t timeStamp;
	uint32_t dataSize;
	vector<in_addr_t> deletedKeys;

	lastLoadTime = time(NULL);
	if(lastLoadTime == ((time_t)-1))
	{
		LOG(ERROR, "Problem with CE State File", "Unable to get timestamp, call to time() failed");
	}

	// Open input file
	ifstream in(Config::Inst()->GetPathCESaveFile().data(), ios::binary | ios::in);
	if(!in.is_open())
	{
		LOG(ERROR,"Problem with CE State File",
			"Unable to open the CE state file at "+Config::Inst()->GetPathCESaveFile());
		return;
	}

	// Open the tmp file
	string tmpFile = Config::Inst()->GetPathCESaveFile() + ".tmp";
	ofstream out(tmpFile.data(), ios::binary);
	if(!out.is_open())
	{
		LOG(ERROR, "Problem with CE State File", "Unable to open the temporary CE state file.");
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
		if(lengthLeft < (sizeof timeStamp + sizeof dataSize))
		{
			LOG(ERROR, "Problem with CE State File","The CE state file may be corrupt");
			break;
		}

		in.read((char*) &timeStamp, sizeof timeStamp);
		lengthLeft -= sizeof timeStamp;

		in.read((char*) &dataSize, sizeof dataSize);
		lengthLeft -= sizeof dataSize;

		if(Config::Inst()->GetDataTTL() && (timeStamp < lastLoadTime - Config::Inst()->GetDataTTL()))
		{
			stringstream ss;
			ss << "Throwing out old CE state at time: " << timeStamp << ".";
			LOG(DEBUG,"Throwing out old CE state.", ss.str());
			in.seekg(dataSize, ifstream::cur);
			lengthLeft -= dataSize;
			continue;
		}

		// Not as many bytes left as the size of the entry?
		if(lengthLeft < dataSize)
		{
			LOG(ERROR, "The CE state file may be corrupt","");
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
			suspectBytes += newSuspect->Deserialize(tableBuffer + bytesSoFar + suspectBytes);

			FeatureSet fs = newSuspect->GetFeatureSet();
			suspectBytes += fs.DeserializeFeatureData(tableBuffer + bytesSoFar + suspectBytes);
			newSuspect->SetFeatureSet(&fs);

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
		if(dataSize > 0)
		{
			out.write((char*) &timeStamp, sizeof timeStamp);
			out.write((char*) &dataSize, sizeof dataSize);
			out.write((char*) tableBuffer, dataSize);
		}
	}

	out.close();
	in.close();

	string copyCommand = "cp -f " + tmpFile + " " + Config::Inst()->GetPathCESaveFile();
	if(system(copyCommand.c_str()) == -1)
	{
		LOG(ERROR, "Problem with CE State File", "System Call: " + copyCommand +" has failed.");
	}
}

void Reload()
{
	LoadConfiguration();

	// Reload the configuration file
	Config::Inst()->LoadConfig();

	engine->LoadDataPointsFromFile(Config::Inst()->GetPathTrainingFile());
	Suspect suspectCopy;
	// Set everyone to be reclassified
	for(SuspectTableIterator it = suspects.Begin(); it.GetIndex() < suspects.Size(); ++it)
	{
		suspectCopy = suspects.CheckOut(it.GetKey());
		suspectCopy.SetNeedsClassificationUpdate(true);
		suspects.CheckIn(&suspectCopy);
	}
}


void SilentAlarm(Suspect *suspect, int oldClassification)
{
	int sockfd = 0;
	string commandLine;
	string hostAddrString = GetLocalIP(Config::Inst()->GetInterface().c_str());

	uint32_t dataLen = suspect->GetSerializeLength(true);
	u_char serializedBuffer[dataLen];

	if(suspect->GetUnsentFeatureSet().m_packetCount)
	{
		do
		{
			dataLen = suspect->Serialize(serializedBuffer, true);
			// Move the unsent data to the sent side
			suspect->UpdateFeatureData(INCLUDE);
			// Clear the unsent data
			suspect->ClearUnsentData();

			//Update other Nova Instances with latest suspect Data
			for(uint i = 0; i < Config::Inst()->GetNeighbors().size(); i++)
			{
				serv_addr.sin_addr.s_addr = Config::Inst()->GetNeighbors()[i];

				stringstream ss;
				string commandLine;

				ss << "sudo iptables -I INPUT -s " << string(inet_ntoa(serv_addr.sin_addr)) << " -p tcp -j ACCEPT";
				commandLine = ss.str();

				if(system(commandLine.c_str()) == -1)
				{
					LOG(ERROR, "Failed to update iptables.", "");
				}

				int i;
				for(i = 0; i < Config::Inst()->GetSaMaxAttempts(); i++)
				{
					if(KnockPort(OPEN))
					{
						//Send Silent Alarm to other Nova Instances with feature Data
						if((sockfd = socket(AF_INET,SOCK_STREAM,6)) == -1)
						{
							LOG(ERROR, "Unable to open socket to send silent alarm: "+ string(strerror(errno)), "");
							close(sockfd);
							continue;
						}
						if(connect(sockfd, serv_addrPtr, sizeof(serv_addr)) == -1)
						{
							//If the host isn't up we stop trying
							if(errno == EHOSTUNREACH)
							{
								//set to max attempts to hit the failed connect condition
								i = Config::Inst()->GetSaMaxAttempts();
								LOG(ERROR, "Unable to connect to host to send silent alarm: "
									+ string(strerror(errno)), "");
								break;
							}
							LOG(ERROR, "Unable to open socket to send silent alarm: "+ string(strerror(errno)), "");
							close(sockfd);
							continue;
						}
						break;
					}
				}
				//If connecting failed
				if(i == Config::Inst()->GetSaMaxAttempts())
				{
					close(sockfd);
					ss.str("");
					ss << "sudo iptables -D INPUT -s " << string(inet_ntoa(serv_addr.sin_addr)) << " -p tcp -j ACCEPT";
					commandLine = ss.str();
					if(system(commandLine.c_str()) == -1)
					{
						LOG(ERROR, "Failed to update iptables.", "");
					}
					continue;
				}

				if( send(sockfd, serializedBuffer, dataLen, 0) == -1)
				{
					LOG(ERROR, "Error in TCP Send of silent alarm: "+string(strerror(errno)), "");

					close(sockfd);
					ss.str("");
					ss << "sudo iptables -D INPUT -s " << string(inet_ntoa(serv_addr.sin_addr)) << " -p tcp -j ACCEPT";
					commandLine = ss.str();
					if(system(commandLine.c_str()) == -1)
					{
						LOG(ERROR, "Failed to update iptables.", "");
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
					LOG(ERROR, "Failed to update iptables.", "");
				}
			}
		}while(dataLen == MORE_DATA);
	}
}


bool KnockPort(bool mode)
{
	int sockfd;
	stringstream ss;
	ss << Config::Inst()->GetKey();

	//mode == OPEN (true)
	if(mode)
	{
		ss << "OPEN";
	}

	//mode == CLOSE (false)
	else
	{
		ss << "SHUT";
	}

	uint keyDataLen = Config::Inst()->GetKey().size() + 4;
	u_char keyBuf[1024];
	bzero(keyBuf, 1024);
	memcpy(keyBuf, ss.str().c_str(), ss.str().size());

	CryptBuffer(keyBuf, keyDataLen, ENCRYPT);

	//Send Port knock to other Nova Instances
	if((sockfd = socket(AF_INET, SOCK_DGRAM, 17)) == -1)
	{
			LOG(ERROR, "Error in port knocking. Can't create socket: "+string(strerror(errno)), "");

		close(sockfd);
		return false;
	}

	if(sendto(sockfd,keyBuf,keyDataLen, 0,serv_addrPtr, sizeof(serv_addr)) == -1)
	{
		LOG(ERROR, "Error in UDP Send for port knocking: "+string(strerror(errno)), "");
		close(sockfd);
		return false;
	}

	close(sockfd);
	sleep(Config::Inst()->GetSaSleepDuration());
	return true;
}


bool Start_Packet_Handler()
{
	char errbuf[PCAP_ERRBUF_SIZE];

	int ret;
	string haystackAddresses_csv = "";

	struct bpf_program fp;			/* The compiled filter expression */
	char filter_exp[64];


	haystackAddresses = GetHaystackAddresses(Config::Inst()->GetPathConfigHoneydHS());
	haystackDhcpAddresses = GetHaystackDhcpAddresses(dhcpListFile);
	haystackAddresses_csv = ConstructFilterString();

	//If we're reading from a packet capture file
	if(Config::Inst()->GetReadPcap())
	{
		sleep(1); //To allow time for other processes to open
		handle = pcap_open_offline(Config::Inst()->GetPathPcapFile().c_str(), errbuf);

		if(handle == NULL)
		{
			LOG(CRITICAL, "Unable to start packet capture.",
				"Couldn't open pcap file: "+Config::Inst()->GetPathPcapFile()+": "+string(errbuf)+".");
			exit(EXIT_FAILURE);
		}
		if(pcap_compile(handle, &fp, haystackAddresses_csv.data(), 0, maskp) == -1)
		//if(pcap_compile(handle, &fp, "dst net 192.168.10 && !dst host 192.168.10.255" , 0, maskp) == -1)
		{
			LOG(CRITICAL, "Unable to start packet capture.",
				"Couldn't parse filter: "+string(filter_exp)+ " " + pcap_geterr(handle) +".");
			exit(EXIT_FAILURE);
		}

		if(pcap_setfilter(handle, &fp) == -1)
		{
			LOG(CRITICAL, "Unable to start packet capture.",
				"Couldn't install filter: "+string(filter_exp)+ " " + pcap_geterr(handle) +".");
			exit(EXIT_FAILURE);
		}
		//First process any packets in the file then close all the sessions
		pcap_loop(handle, -1, Packet_Handler,NULL);

		TCPTimeout(NULL);

		if(Config::Inst()->GetGotoLive()) Config::Inst()->SetReadPcap(false); //If we are going to live capture set the flag.

		trainingFileStream.close();
		LOG(DEBUG, "Done processing PCAP file", "");
	}


	if(!Config::Inst()->GetReadPcap())
	{
		LoadStateFile();

		//Open in non-promiscuous mode, since we only want traffic destined for the host machine
		handle = pcap_open_live(Config::Inst()->GetInterface().c_str(), BUFSIZ, 0, 1000, errbuf);

		if(handle == NULL)
		{
			LOG(ERROR, "Unable to start packet capture.",
				"Unable to open network interface "+Config::Inst()->GetInterface()+" for live capture: "+string(errbuf));
			exit(EXIT_FAILURE);
		}

		/* ask pcap for the network address and mask of the device */
		ret = pcap_lookupnet(Config::Inst()->GetInterface().c_str(), &netp, &maskp, errbuf);
		if(ret == -1)
		{
			LOG(ERROR, "Unable to start packet capture.",
				"Unable to get the network address and mask: "+string(strerror(errno)));
			exit(EXIT_FAILURE);
		}

		if(pcap_compile(handle, &fp, haystackAddresses_csv.data(), 0, maskp) == -1)
		{
			LOG(ERROR, "Unable to start packet capture.",
				"Couldn't parse filter: "+string(filter_exp)+ " " + pcap_geterr(handle) +".");
			exit(EXIT_FAILURE);
		}

		if(pcap_setfilter(handle, &fp) == -1)
		{
			LOG(ERROR, "Unable to start packet capture.",
				"Couldn't install filter: "+string(filter_exp)+ " " + pcap_geterr(handle) +".");
			exit(EXIT_FAILURE);
		}
		//"Main Loop"
		//Runs the function "Packet_Handler" every time a packet is received
		pthread_create(&TCP_timeout_thread, NULL, TCPTimeout, NULL);

	    pcap_loop(handle, -1, Packet_Handler, NULL);
	}
	return false;
}

void Packet_Handler(u_char *useless,const struct pcap_pkthdr* pkthdr,const u_char* packet)
{
	//Memory assignments moved outside packet handler to increase performance
	int dest_port;
	Packet packet_info;
	struct ether_header *ethernet;  	/* net/ethernet.h */
	struct ip *ip_hdr; 					/* The IP header */
	char tcp_socket[55];

	if(packet == NULL)
	{
		LOG(ERROR, "Failed to capture packet!","");
		return;
	}


	/* let's start with the ether header... */
	ethernet = (struct ether_header *) packet;

	/* Do a couple of checks to see what packet type we have..*/
	if(ntohs (ethernet->ether_type) == ETHERTYPE_IP)
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
		if(!Config::Inst()->GetClassificationTimeout())
		{
			if(!Config::Inst()->GetIsTraining())
			{
				UpdateAndClassify(packet_info.ip_hdr.ip_src.s_addr);
			}
			else
			{
				UpdateAndStore(packet_info.ip_hdr.ip_src.s_addr);
			}
		}
	}
	else if(ntohs(ethernet->ether_type) == ETHERTYPE_ARP)
	{
		return;
	}
	else
	{
		LOG(ERROR, "Unknown Non-IP Packet Received. Nova is ignoring it.","");
		return;
	}
}

void LoadConfiguration()
{
	string hostAddrString = GetLocalIP(Config::Inst()->GetInterface().c_str());

	if(hostAddrString.size() == 0)
	{
		LOG(ERROR, "Bad ethernet interface, no IP's associated!","");
		exit(EXIT_FAILURE);
	}

	inet_pton(AF_INET,hostAddrString.c_str(),&(hostAddr.sin_addr));
}


string ConstructFilterString()
{
	//Flatten out the vectors into a csv string
	string filterString = "";

	for(uint i = 0; i < haystackAddresses.size(); i++)
	{
		filterString += "dst host ";
		filterString += haystackAddresses[i];

		if(i+1 != haystackAddresses.size())
		{
			filterString += " || ";
		}
	}

	if(!haystackDhcpAddresses.empty() && !haystackAddresses.empty())
	{
		filterString += " || ";
	}

	for(uint i = 0; i < haystackDhcpAddresses.size(); i++)
	{
		filterString += "dst host ";
		filterString += haystackDhcpAddresses[i];

		if(i+1 != haystackDhcpAddresses.size())
		{
			filterString += " || ";
		}
	}

	if(filterString == "")
	{
		filterString = "dst host 0.0.0.0";
	}

	LOG(DEBUG, "Pcap filter string is "+filterString,"");
	return filterString;
}


vector <string> GetHaystackDhcpAddresses(string dhcpListFile)
{
	ifstream dhcpFile(dhcpListFile.data());
	vector<string> haystackDhcpAddresses;

	if(dhcpFile.is_open())
	{
		while(dhcpFile.good())
		{
			string line;
			getline (dhcpFile,line);
			if(strcmp(line.c_str(), ""))
			{
				haystackDhcpAddresses.push_back(line);
			}
		}
		dhcpFile.close();
	}
	else
	{
		LOG(ERROR,"Unable to open file: " + dhcpListFile, "");
	}

	return haystackDhcpAddresses;
}

vector <string> GetHaystackAddresses(string honeyDConfigPath)
{
	//Path to the main log file
	ifstream honeydConfFile(honeyDConfigPath.c_str());
	vector <string> retAddresses;
	retAddresses.push_back(GetLocalIP(Config::Inst()->GetInterface().c_str()));

	if( honeydConfFile == NULL)
	{
		LOG(ERROR, "Error opening log file. Does it exist?", "");
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

void UpdateSuspect(Packet packet)
{
	Suspect * newSuspect = new Suspect(packet);
	in_addr_t addr = newSuspect->GetIpAddress();
	//If our suspect is new
	if(!suspects.IsValidKey(addr))
	{
		newSuspect->SetNeedsClassificationUpdate(true);
		newSuspect->SetIsLive(Config::Inst()->GetReadPcap());

		// If we're in training mode, we only send new suspects to the GUI
		if (Config::Inst()->GetIsTraining())
		{
			if(SendSuspectToUI(newSuspect))
			{
				LOG(DEBUG, "Sent a suspect to the UI: " + string(inet_ntoa(newSuspect->GetInAddr())), "");
			}
			else
			{
				LOG(DEBUG, "Failed to send a suspect to the UI: "+ string(inet_ntoa(newSuspect->GetInAddr())), "");
			}
		}

		// Make a copy to add to the other table
		Suspect *newSuspectCopy = new Suspect(*newSuspect);

		suspects.AddNewSuspect(newSuspect);

		pthread_mutex_lock(&suspectsSinceLastSaveLock);
		suspectsSinceLastSave.AddNewSuspect(newSuspectCopy);
		pthread_mutex_unlock(&suspectsSinceLastSaveLock);
	}
	//Else our suspect exists
	else
	{
		Suspect suspectCopy = suspects.CheckOut(addr);
		suspectCopy.AddEvidence(packet);
		suspectCopy.SetIsLive(Config::Inst()->GetReadPcap());
		suspects.CheckIn(&suspectCopy);

		// Don't care about the state file if we're training
		if (!Config::Inst()->GetIsTraining())
		{
			pthread_mutex_lock(&suspectsSinceLastSaveLock);
			Suspect suspectCopy2 = suspectsSinceLastSave.CheckOut(addr);
			suspectCopy2.AddEvidence(packet);
			suspectCopy2.SetIsLive(Config::Inst()->GetReadPcap());
			suspectsSinceLastSave.CheckIn(&suspectCopy2);
			pthread_mutex_unlock(&suspectsSinceLastSaveLock);
		}

		delete newSuspect;
	}
}


void UpdateAndStore(uint64_t suspect)
{
	Suspect suspectCopy = suspects.CheckOut(suspect);

	// If the checkout failed and we got the empty suspect
	if (suspectCopy.GetIpAddress() == 0)
	{
		return;
	}

	if(suspectCopy.GetNeedsClassificationUpdate())
	{

		suspectCopy.UpdateEvidence();
		suspectCopy.CalculateFeatures();

		trainingFileStream << string(inet_ntoa(suspectCopy.GetInAddr())) << " ";
		for (int j = 0; j < DIM; j++)
		{
			trainingFileStream << suspectCopy.GetFeatureSet().m_features[j] << " ";
		}
		trainingFileStream << "\n";

		suspectCopy.SetNeedsClassificationUpdate(false);
	}

	suspects.CheckIn(&suspectCopy);
}


void UpdateAndClassify(uint64_t suspect)
{
	Suspect suspectCopy = suspects.CheckOut(suspect);

	// If the checkout failed and we got the empty suspect
	if (suspectCopy.GetIpAddress() == 0)
	{
		return;
	}

	if(suspectCopy.GetNeedsClassificationUpdate())
	{
		suspectCopy.UpdateEvidence();
		suspectCopy.CalculateFeatures();
		int oldClassification = suspectCopy.GetIsHostile();

		engine->NormalizeDataPoint(&suspectCopy);
		engine->Classify(&suspectCopy);

		//If suspect is hostile and this Nova instance has unique information
		// 			(not just from silent alarms)
		if(suspectCopy.GetIsHostile() || oldClassification)
		{
			if(suspectCopy.GetIsLive())
			{
				SilentAlarm(&suspectCopy, oldClassification);
			}
		}

		if(SendSuspectToUI(&suspectCopy))
		{
			LOG(DEBUG, string("Sent a suspect to the UI: ")+ inet_ntoa(suspectCopy.GetInAddr()), "");
		}
		else
		{
			LOG(DEBUG, string("Failed to send a suspect to the UI: ")+ inet_ntoa(suspectCopy.GetInAddr()), "");
		}
	}
	suspects.CheckIn(&suspectCopy);
}

}
