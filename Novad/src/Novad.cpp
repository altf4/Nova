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
#include "WhitelistConfiguration.h"
#include "ClassificationEngine.h"
#include "ProtocolHandler.h"
#include "EvidenceTable.h"
#include "SuspectTable.h"
#include "FeatureSet.h"
#include "NovaUtil.h"
#include "Commands.h"
#include "Threads.h"
#include "Control.h"
#include "Config.h"
#include "Logger.h"
#include "Point.h"
#include "Novad.h"


#include <pcap.h>
#include <vector>
#include <math.h>
#include <time.h>
#include <errno.h>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/un.h>
#include <signal.h>
#include <iostream>
#include <ifaddrs.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <netinet/if_ether.h>

using namespace std;
using namespace Nova;

// Maintains a list of suspects and information on network activity
SuspectTable suspects;
// Suspects not yet written to the state file
SuspectTable suspectsSinceLastSave;
//Contains packet evidence yet to be included in a suspect
EvidenceTable suspectEvidence;

//-- Silent Alarm --
struct sockaddr_in serv_addr;
struct sockaddr *serv_addrPtr = (struct sockaddr *) &serv_addr;
vector<struct sockaddr_in> hostAddrs;
vector<uint> dropCounts;

// Timestamps for the CE state file exiration of data
time_t lastLoadTime;
time_t lastSaveTime;

// Time novad started, used for uptime and pcap capture names
time_t startTime;

string trainingFolder;
string trainingCapFile;


pcap_dumper_t *trainingFileStream;

//HS Vars
// TODO: Don't hard code this path. Might also be in NovaTrainer.
string dhcpListFile = "/var/log/honeyd/ipList";
vector<string> haystackAddresses;
vector<string> haystackDhcpAddresses;
vector<string> whitelistIpAddresses;
vector<string> whitelistIpRanges;
vector<pcap_t *> handles;

bpf_u_int32 maskp;
bpf_u_int32 netp;

int honeydDHCPNotifyFd;
int honeydDHCPWatch;

int whitelistNotifyFd;
int whitelistWatch;

ClassificationEngine *engine;

pthread_t classificationLoopThread;
pthread_t trainingLoopThread;
pthread_t silentAlarmListenThread;
pthread_t ipUpdateThread;
pthread_t ipWhitelistUpdateThread;

vector<uint32_t> localIPs;

namespace Nova
{

int RunNovaD()
{
	Config::Inst();

	MessageManager::Initialize(DIRECTION_TO_UI);

	if(!LockNovad())
	{
		exit(EXIT_FAILURE);
	}

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

	LOG(ALERT, "Starting NOVA version " + Config::Inst()->GetVersionString(), "");

	engine = new ClassificationEngine(suspects);
	engine->LoadDataPointsFromFile(Config::Inst()->GetPathTrainingFile());

	Spawn_UI_Handler();



	//Are we Training or Classifying?
	if(Config::Inst()->GetIsTraining())
	{
		if(system(string("mkdir " + Config::Inst()->GetPathTrainingCapFolder()).c_str()))
		{
			// Not really an problem, throws compiler warning if we don't catch the system call though
		}

		trainingFolder = Config::Inst()->GetPathHome() + "/" + Config::Inst()->GetPathTrainingCapFolder() + "/" + Config::Inst()->GetTrainingSession();
		if(system(string("mkdir " + trainingFolder).c_str()))
		{
			// Not really an problem, throws compiler warning if we don't catch the system call though
		}

		trainingCapFile = trainingFolder + "/capture.pcap";

		if(Config::Inst()->GetReadPcap())
		{
			Config::Inst()->SetClassificationThreshold(0);
			Config::Inst()->SetClassificationTimeout(0);
		}
		else
		{
			pthread_create(&trainingLoopThread,NULL,TrainingLoop,NULL);
		}
	}
	else
	{
		whitelistNotifyFd = inotify_init ();
		if(whitelistNotifyFd > 0)
		{
			whitelistWatch = inotify_add_watch (whitelistNotifyFd, Config::Inst()->GetPathWhitelistFile().c_str(), IN_CLOSE_WRITE | IN_MOVED_TO | IN_MODIFY | IN_DELETE);
			pthread_create(&ipWhitelistUpdateThread, NULL, UpdateWhitelistIPFilter,NULL);
			pthread_detach(ipWhitelistUpdateThread);
		}
		else
		{
			LOG(ERROR, "Unable to set up file watcher for the Whitelist IP file.","");
		}
	}

	// If we're not reading from a pcap, monitor for IP changes in the honeyd file
	if(!Config::Inst()->GetReadPcap())
	{
		honeydDHCPNotifyFd = inotify_init ();

		if(honeydDHCPNotifyFd > 0)
		{
			honeydDHCPWatch = inotify_add_watch (honeydDHCPNotifyFd, dhcpListFile.c_str(), IN_CLOSE_WRITE | IN_MOVED_TO | IN_MODIFY | IN_DELETE);
			pthread_create(&ipUpdateThread, NULL, UpdateIPFilter,NULL);
			pthread_detach(ipUpdateThread);
		}
		else
		{
			LOG(ERROR, "Unable to set up file watcher for the honeyd IP list file.","");
		}
	}

	return Start_Packet_Handler();
}

bool LockNovad()
{
	int lockFile = open((Config::Inst()->GetPathHome() + "/novad.lock").data(), O_CREAT | O_RDWR, 0666);
	int rc = flock(lockFile, LOCK_EX | LOCK_NB);
	if(rc != 0)
	{
		if(errno == EAGAIN)
		{
			cerr << "ERROR: Novad is already running. Please close all other instances before continuing." << endl;
		}
		else
		{
			cerr << "ERROR: Unable to open the novad.lock file. This could be due to bad file permissions on it. Error was: " << strerror(errno) << endl;
		}
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

//Appends the evidence gathered on suspects since the last save to the statefile
void AppendToStateFile()
{
	//Store the time we started the save for ref later.
	lastSaveTime = time(NULL);
	//if time(NULL) failed
	if(lastSaveTime == ~0)
	{
		LOG(ERROR, "Problem with CE State File", "Unable to get timestamp, call to time() failed");
	}

	//If there are no suspects to save
	if(suspectsSinceLastSave.Size() <= 0)
	{
		return;
	}

	//Open the output file
	ofstream out(Config::Inst()->GetPathCESaveFile().data(), ofstream::binary | ofstream::app);

	//Update the feature set and dump the contents of the table to the output file
	uint32_t dataSize;
	dataSize = suspectsSinceLastSave.DumpContents(&out, lastSaveTime);

	//Check that the dump was successful, print the results as a debug message.
	stringstream ss;
	if(dataSize)
	{
		ss << "Appending " << dataSize << " bytes to the CE state file at " << Config::Inst()->GetPathCESaveFile();
	}
	else
	{
		ss << "Unable to write any Suspects to the state file. func: 'DumpContents' returned 0.";
	}
	LOG(DEBUG,ss.str(),"");
	out.close();
	//Clear the table;
	suspectsSinceLastSave.EraseAllSuspects();
}

//Loads the statefile by reading table state dumps that haven't expired yet.
void LoadStateFile()
{

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
	uint expirationTime = lastLoadTime - Config::Inst()->GetDataTTL();
	if(Config::Inst()->GetDataTTL() == 0)
	{
		expirationTime = 0;
	}
	uint numBytes = 0;
	while(in.is_open() && !in.eof() && lengthLeft)
	{
		numBytes = suspects.ReadContents(&in, expirationTime);
		if(numBytes == 0)
		{

			in.close();

			// Back up the possibly corrupt state file
			int suffix = 0;
			string stateFileBackup = Config::Inst()->GetPathCESaveFile() + ".Corrupt";
			struct stat st;

			// Find an unused filename to move it to
			string fileName;
			do {
				suffix++;
				stringstream ss;
				ss << stateFileBackup;
				ss << suffix;
				fileName = ss.str();
			} while(stat(fileName.c_str(),&st) != -1);

			LOG(WARNING, "Backing up possibly corrupt state file to file " + fileName, "");

			// Copy the file
			stringstream copyCommand;
			copyCommand << "mv " << Config::Inst()->GetPathCESaveFile() << " " << fileName;
			if(system(copyCommand.str().c_str()) == -1) {
				LOG(ERROR, "There was a problem when attempting to move the corrupt state file. System call failed: " + copyCommand.str(), "");
			}

			// Recreate an empty file
			stringstream touchCommand;
			touchCommand << "touch " << Config::Inst()->GetPathCESaveFile();
			if(system(touchCommand.str().c_str()) == -1) {
				LOG(ERROR, "There was a problem when attempting to recreate the state file. System call to 'touch' failed:" + touchCommand.str(), "");
			}

			break;
		}
		lengthLeft -= numBytes;
	}
	in.close();
}

//Refreshes the state file by parsing the contents and writing un-expired state dumps to the new state file
void RefreshStateFile()
{
	//Variable assignments
	uint32_t dataSize = 0;
	vector<in_addr_t> deletedKeys;
	uint bytesRead = 0, lengthLeft = 0;
	time_t saveTime = 0;

	string ceFile = Config::Inst()->GetPathCESaveFile();
	string tmpFile = Config::Inst()->GetPathCESaveFile() + ".tmp";

	lastLoadTime = time(NULL);
	time_t timestamp = lastLoadTime - Config::Inst()->GetDataTTL();

	//Check that we have a valid timestamp for the last load time.
	if(lastLoadTime == (~0)) // == -1 in signed vals
	{
		LOG(ERROR, "Problem with CE State File", "Unable to get timestamp, call to time() failed");
	}
	if(system(string("cp -f -p "+ceFile+" "+tmpFile).c_str()) != 0)
	{
		LOG(ERROR, "Unable to refresh CE State File.",
				string("Unable to refresh CE State File: cp -f -p "+ceFile+" "+tmpFile+ " failed."));
		return;
	}
	// Open input file
	ifstream in(tmpFile.data(), ios::binary | ios::in);
	if(!in.is_open())
	{
		LOG(ERROR,"Problem with CE State File",
			"Unable to open the CE state file at "+Config::Inst()->GetPathCESaveFile());
		return;
	}

	ofstream out(ceFile.data(), ios::binary | ios::trunc);
	if(!out.is_open())
	{
		LOG(ERROR, "Problem with CE State File", "Unable to open the temporary CE state file.");
		in.close();
		return;
	}

	//Get some variables about the input size for error checking.
	in.seekg (0, ios::end);
	lengthLeft = in.tellg();
	in.seekg (0, ios::beg);

	while(in.is_open() && !in.eof() && lengthLeft)
	{
		//If we can get the timestamp and data size
		if(lengthLeft < (sizeof timestamp + sizeof dataSize))
		{
			LOG(ERROR, "The CE state file may be corruput", "");
			return;
		}

		//Read the timestamp and dataSize
		in.read((char*)&saveTime, sizeof saveTime);
		in.read((char*)&dataSize, sizeof dataSize);
		bytesRead = dataSize + sizeof dataSize + sizeof saveTime;

		//If the data has expired, skip past
		if(saveTime < timestamp)
		{
			//Print debug msg
			stringstream ss;
			ss << "Throwing out old CE state at time: " << saveTime << ".";
			LOG(DEBUG,"Throwing out old CE state.", ss.str());

			//Skip past expired data
			in.seekg(dataSize, ios::cur);
		}
		else
		{
			char buf[dataSize];
			in.read(buf, dataSize);
			out.write((char*)&saveTime, sizeof saveTime);
			out.write((char*)&dataSize, sizeof dataSize);
			out.write(buf, dataSize);
		}
		lengthLeft -= bytesRead;
	}
	in.close();
	out.close();
}

void CloseTrainingCapture()
{
	pcap_dump_close(trainingFileStream);
}

void Reload()
{
	LoadConfiguration();

	// Reload the configuration file
	Config::Inst()->LoadConfig();

	engine->LoadDataPointsFromFile(Config::Inst()->GetPathTrainingFile());
	Suspect suspectCopy;

	suspects.UpdateAllSuspects();
}


void SilentAlarm(Suspect *suspect, int oldClassification)
{
	int sockfd = 0;
	string commandLine;

	Suspect suspectCopy = suspects.CheckOut(suspect->GetIpAddress());
	if(suspects.IsEmptySuspect(&suspectCopy))
	{
		LOG(WARNING, "Attempted to broadcast Silent Alarm for an Invalid suspect","");
		return;
	}

	uint32_t dataLen = suspectCopy.GetSerializeLength(UNSENT_FEATURE_DATA);
	u_char serializedBuffer[dataLen];

	if(suspectCopy.GetFeatureSet(UNSENT_FEATURES).m_packetCount)
	{
		do
		{
			if(dataLen != suspectCopy.Serialize(serializedBuffer, dataLen, UNSENT_FEATURE_DATA))
			{
				stringstream ss;
				ss << "Serialization of Suspect with key: " << suspectCopy.GetIpAddress();
				ss << " returned a size != " << dataLen;
				LOG(ERROR, "Unable to Serialize Suspect", ss.str());
				return;
			}
			//suspectCopy.UpdateFeatureData(INCLUDE);
			suspectCopy.ClearFeatureData(UNSENT_FEATURES);
			suspects.CheckIn(&suspectCopy);

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

				int j;
				for(j = 0; j < Config::Inst()->GetSaMaxAttempts(); j++)
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
								j = Config::Inst()->GetSaMaxAttempts();
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
				if(j == Config::Inst()->GetSaMaxAttempts())
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
	string haystackAddresses_csv = "";
	struct bpf_program fp;
	char filter_exp[64];
	bpf_u_int32 maskp;
	bpf_u_int32 netp;

	haystackAddresses = Config::GetHaystackAddresses(Config::Inst()->GetPathConfigHoneydHS());
	haystackDhcpAddresses = Config::GetIpAddresses(dhcpListFile);
	whitelistIpAddresses = WhitelistConfiguration::GetIps();
	whitelistIpRanges = WhitelistConfiguration::GetIpRanges();
	haystackAddresses_csv = ConstructFilterString();

	UpdateHaystackFeatures();

	//If we're reading from a packet capture file
	if(Config::Inst()->GetReadPcap())
	{
		LOG(DEBUG, "Loading PCAP file", "");
		string pcapFilePath = Config::Inst()->GetPathPcapFile() + "/capture.pcap";
		string ipAddressFile = Config::Inst()->GetPathPcapFile() + "/localIps.txt";

		handles.push_back(pcap_open_offline(pcapFilePath.c_str(), errbuf));


		vector<string> ips = Config::GetIpAddresses(ipAddressFile);
		for (uint i = 0; i < ips.size(); i++)
		{
			localIPs.push_back(inet_addr(ips.at(i).c_str()));
		}

		if(handles[0] == NULL)
		{
			LOG(CRITICAL, "Unable to start packet capture.",
				"Couldn't open pcap file: "+Config::Inst()->GetPathPcapFile()+": "+string(errbuf)+".");
			exit(EXIT_FAILURE);
		}

		if(pcap_compile(handles[0], &fp, haystackAddresses_csv.data(), 0, PCAP_NETMASK_UNKNOWN) == -1)
		{
			LOG(CRITICAL, "Unable to start packet capture.",
				"Couldn't parse filter: "+string(filter_exp)+ " " + pcap_geterr(handles[0]) +".");
			exit(EXIT_FAILURE);
		}

		if(pcap_setfilter(handles[0], &fp) == -1)
		{
			LOG(CRITICAL, "Unable to start packet capture.",
				"Couldn't install filter: "+string(filter_exp)+ " " + pcap_geterr(handles[0]) +".");
			exit(EXIT_FAILURE);
		}

		pcap_freecode(&fp);
		//First process any packets in the file then close all the sessions
		u_char index = 0;
		pcap_loop(handles[0], -1, Packet_Handler,&index);

		LOG(DEBUG, "Done reading PCAP file. Processing...", "");
		ClassificationLoop(NULL);
		LOG(DEBUG, "Done processing PCAP file.", "");

		if(Config::Inst()->GetGotoLive())
		{
			Config::Inst()->SetReadPcap(false); //If we are going to live capture set the flag.
		}
		else
		{
			// Just sleep this thread forever so the UI responds still but no packet capture
			while(true)
			{
				sleep(1000000);
			}
		}
	}

	if(!Config::Inst()->GetReadPcap())
	{
		pthread_create(&classificationLoopThread,NULL,ClassificationLoop, NULL);
		pthread_create(&silentAlarmListenThread,NULL,SilentAlarmLoop, NULL);
		pthread_detach(classificationLoopThread);
		pthread_detach(silentAlarmListenThread);

		vector<string> ifList = Config::Inst()->GetInterfaces();
		if(!Config::Inst()->GetIsTraining())
		{
			LoadStateFile();
		}

		for(uint i = 0; i < ifList.size(); i++)
		{
			dropCounts.push_back(0);
			string ipAddr = GetLocalIP(ifList.back().c_str());
			struct in_addr tempAddr;
			inet_aton(ipAddr.c_str(), &tempAddr);
			localIPs.push_back(ntohl(tempAddr.s_addr));
			string temp = haystackAddresses_csv;
			temp.append(" || ");
			temp.append(ipAddr);
			handles.push_back(pcap_create(ifList[i].c_str(), errbuf));

			if(handles[i] == NULL)
			{
				LOG(ERROR, "Unable to start packet capture.",
					"Unable to open network interfaces for live capture: "+string(errbuf));
				exit(EXIT_FAILURE);
			}

			if(pcap_set_promisc(handles[i], 1) != 0)
			{
				LOG(ERROR, string("Unable to set interface mode to promisc due to error: ") + pcap_geterr(handles[i]), "");
			}

			// Set a 20MB buffer
			// TODO Make this a user configurable option. Too small will cause dropped packets under high load.
			if(pcap_set_buffer_size(handles[i], 1024*1024) != 0)
			{
				LOG(ERROR, string("Unable to set pcap capture buffer size due to error: ") + pcap_geterr(handles[i]), "");
			}

			//Set a capture length of 1Kb. Should be more than enough to get the packet headers
			// 88 == Ethernet header (14 bytes) + max IP header size (60 bytes)  + 4 bytes to extract the destination port for udp and tcp packets
			if(pcap_set_snaplen(handles[i], 88) != 0)
			{
				LOG(ERROR, string("Unable to set pcap capture length due to error: ") + pcap_geterr(handles[i]), "");
			}

			if(pcap_set_timeout(handles[i], 1000) != 0)
			{
				LOG(ERROR, string("Unable to set pcap timeout value due to error: ") + pcap_geterr(handles[i]), "");
			}

			if(pcap_activate(handles[i]) != 0)
			{
				LOG(CRITICAL, string("Unable to activate packet capture due to error: ") + pcap_geterr(handles[i]), "");
				exit(EXIT_FAILURE);
			}

			// ask pcap for the network address and mask of the device
			int ret = pcap_lookupnet(Config::Inst()->GetInterface(i).c_str(), &netp, &maskp, errbuf);
			if(ret == -1)
			{
				LOG(ERROR, "Unable to start packet capture.",
					"Unable to get the network address and mask: "+string(strerror(errno)));
				exit(EXIT_FAILURE);
			}

			if(pcap_compile(handles[i], &fp, haystackAddresses_csv.data(), 0, maskp) == -1)
			{
				LOG(ERROR, "Unable to start packet capture.",
					"Couldn't parse filter: "+string(filter_exp)+ " " + pcap_geterr(handles[i]) +".");
				exit(EXIT_FAILURE);
			}

			if(pcap_setfilter(handles[i], &fp) == -1)
			{
				LOG(ERROR, "Unable to start packet capture.",
					"Couldn't install filter: "+string(filter_exp)+ " " + pcap_geterr(handles[i]) +".");
				exit(EXIT_FAILURE);
			}
			pcap_freecode(&fp);
		}

		if((handles.empty()) || (handles[0] == NULL))
		{
			LOG(CRITICAL, "Invalid pcap handle provided, unable to start pcap loop!", "");
			exit(EXIT_FAILURE);
		}

		trainingFileStream = pcap_dump_open(handles[0], trainingCapFile.c_str());

		ofstream localIpsStream(trainingFolder + "/localIps.txt");
		for(uint i = 0; i < localIPs.size(); i++)
		{
			in_addr temp;
			temp.s_addr = ntohl(localIPs.at(i));
			localIpsStream << inet_ntoa(temp) << endl;
		}
		localIpsStream.close();

		if(IsHaystackUp())
		{
			ofstream haystackIpStream(trainingFolder + "/haystackIps.txt");
			for(uint i = 0; i < haystackDhcpAddresses.size(); i++)
			{
				haystackIpStream << haystackDhcpAddresses.at(i) << endl;
			}
			for(uint i = 0; i < haystackAddresses.size(); i++)
			{
				haystackIpStream << haystackAddresses.at(i) << endl;
			}
		}

		for(u_char i = 1; i < handles.size(); i++)
		{
			pthread_t readThread;
			u_char temp = i;
			pthread_create(&readThread, NULL, StartPcapLoop, &temp);
			pthread_detach(readThread);
		}

		//"Main Loop"
		//Runs the function "Packet_Handler" every time a packet is received
		u_char index = 0;
		pthread_t consumer;
		pthread_create(&consumer, NULL, ConsumerLoop, NULL);
		pthread_detach(consumer);
		pcap_loop(handles[0], -1, Packet_Handler, &index);
	}
	return false;
}

void Packet_Handler(u_char *index,const struct pcap_pkthdr *pkthdr,const u_char *packet)
{
	if(Config::Inst()->GetIsTraining())
	{
		pcap_dump((u_char *)trainingFileStream, pkthdr, packet);
	}

	if(packet == NULL)
	{
		LOG(ERROR, "Failed to capture packet!","");
		return;
	}
	switch(ntohs(*(uint16_t *)(packet+12)))
	{
		//IPv4, currently the only handled case
		case ETHERTYPE_IP:
		{

			//Prepare Packet structure
			Evidence *evidencePacket = new Evidence(packet + sizeof(struct ether_header), pkthdr);
			if(localIPs[*index] == evidencePacket->m_evidencePacket.ip_dst)
			{
				//manually setting dst ip to 0.0.0.1 designates the packet was to a real host not a haystack node
				evidencePacket->m_evidencePacket.ip_dst = 1;
			}

			if (!Config::Inst()->GetReadPcap())
			{
				suspectEvidence.InsertEvidence(evidencePacket);
			}
			else
			{
				// If reading from pcap file no Consumer threads, so process the evidence right away
				suspectsSinceLastSave.ProcessEvidence(evidencePacket, true);
				suspects.ProcessEvidence(evidencePacket, false);
			}
			return;
		}
		//Ignore IPV6
		case ETHERTYPE_IPV6:
		{
			return;
		}
		//Ignore ARP
		case ETHERTYPE_ARP:
		{
			return;
		}
		default:
		{
			//stringstream ss;
			//ss << "Ignoring a packet with unhandled protocol #" << (uint16_t)(ntohs(((struct ether_header *)packet)->ether_type));
			//LOG(DEBUG, ss.str(), "");
			return;
		}
	}
}

void LoadConfiguration()
{
	vector<string> ifList = Config::Inst()->GetInterfaces();
	hostAddrs.clear();
	for(uint i = 0; i < ifList.size(); i++)
	{
		struct sockaddr_in hostAddr;
		string hostAddrString = GetLocalIP(Config::Inst()->GetInterface(i).c_str());
		if(hostAddrString.size() == 0)
		{
			LOG(ERROR, "Bad ethernet interface, no IP's associated!","");
			exit(EXIT_FAILURE);
		}
		inet_pton(AF_INET, hostAddrString.c_str(),&(hostAddr.sin_addr));
		hostAddrs.push_back(hostAddr);
	}
}

//Convert monitored ip address into a csv string
string ConstructFilterString()
{
	string filterString = "not src host 0.0.0.0 && ";
	if(Config::Inst()->GetCustomPcapString() != "") {
		if(Config::Inst()->GetOverridePcapString())
		{
			filterString = Config::Inst()->GetCustomPcapString();
			LOG(DEBUG, "Pcap filter string is "+filterString,"");
			return filterString;
		}
		else
		{
			filterString += Config::Inst()->GetCustomPcapString() + " && ";
		}
	}


	struct ifaddrs *devices = NULL;
	struct ifaddrs *curIf = NULL;
	stringstream ss;

	//Get a list of interfaces
	if(getifaddrs(&devices))
	{
		LOG(ERROR, string("Ethernet Interface Auto-Detection failed: ") + string(strerror(errno)), "");
	}

	vector<string> networkFilderBuilder;
	vector<string> enabledInterfaces = Config::Inst()->GetInterfaces();
	for(curIf = devices; curIf != NULL; curIf = curIf->ifa_next)
	{
		for(uint i = 0; i < enabledInterfaces.size(); i++)
		{
			if(!strcmp(curIf->ifa_name, enabledInterfaces[i].c_str()) && ((int)curIf->ifa_addr->sa_family == AF_INET))
			{
				// TODO ipv6 support
				in_addr interfaceIp;
				interfaceIp.s_addr = ((sockaddr_in*)curIf->ifa_addr)->sin_addr.s_addr & ((sockaddr_in*)curIf->ifa_netmask)->sin_addr.s_addr;
				string interfaceIpBase = inet_ntoa(interfaceIp);
				string interfaceMask = inet_ntoa(((sockaddr_in*)curIf->ifa_netmask)->sin_addr);
				LOG(DEBUG, "Listening on network " + interfaceIpBase + "/" + interfaceMask, "");
				networkFilderBuilder.push_back("dst net " + interfaceIpBase + " mask " + interfaceMask);
			}
		}
	}

	if(networkFilderBuilder.size() == 0) {
		// Nothing to do
	}
	else if(networkFilderBuilder.size() == 1)
	{
		filterString += networkFilderBuilder.at(0) + " && ";
	} 
	else
	{
		filterString += "(" + networkFilderBuilder.at(0);
		for(uint i = 1; i < networkFilderBuilder.size(); i++)
		{
			filterString += "|| " + networkFilderBuilder.at(i);
		}
		filterString += ") && ";
	}


	{
		//Get list of interfaces and insert associated local IP's
		vector<string> ifList = Config::Inst()->GetInterfaces();
		while(ifList.size())
		{
			//Remove and add the host entry
			filterString += "not src host ";
			//Look up associated IP with the interface
			filterString += GetLocalIP(ifList.back().c_str());
			ifList.pop_back();

			//If we have another local IP or there is at least one haystack/whitelist ip, add 'or' conditional
			if(ifList.size() || haystackAddresses.size() || haystackDhcpAddresses.size()
				|| whitelistIpRanges.size() || whitelistIpAddresses.size())
			{
				filterString += " && ";
			}
		}
	}

	//Insert static haystack IP's
	vector<string> hsAddresses = haystackAddresses;
	while(hsAddresses.size())
	{
		//Remove and add the haystack host entry
		filterString += "not src host ";
		filterString += hsAddresses.back();
		hsAddresses.pop_back();

		//If there is at least one more haystack/whitelist ip, add 'or' conditional
		if(hsAddresses.size() || haystackDhcpAddresses.size() || whitelistIpRanges.size() || whitelistIpAddresses.size())
		{
			filterString += " && ";
		}
	}

	// Whitelist the DHCP haystack node IP addresses
	hsAddresses = haystackDhcpAddresses;
	while(hsAddresses.size())
	{
		//Remove and add the haystack host entry
		filterString += "not src host ";
		filterString += hsAddresses.back();
		hsAddresses.pop_back();

		//If there is at least one more haystack/whitelist ip, add 'or' conditional
		if(hsAddresses.size() || whitelistIpRanges.size() || whitelistIpAddresses.size())
		{
			filterString += " && ";
		}
	}

	hsAddresses = whitelistIpAddresses;
	while(hsAddresses.size())
	{
		//Remove and add the haystack host entry
		filterString += "not src host ";
		filterString += hsAddresses.back();
		hsAddresses.pop_back();

		//If there is at least one more whitelist ip, add 'or' conditional
		if(hsAddresses.size() || whitelistIpRanges.size())
		{
			filterString += " && ";
		}
	}

	hsAddresses = whitelistIpRanges;
	while(hsAddresses.size())
	{
		filterString += "not src net ";
		filterString += WhitelistConfiguration::GetIp(hsAddresses.back());
		filterString += " mask ";
		filterString += WhitelistConfiguration::GetSubnet(hsAddresses.back());
		hsAddresses.pop_back();

		if(hsAddresses.size())
		{
			filterString += " && ";
		}
	}

	LOG(DEBUG, "Pcap filter string is "+filterString,"");
	return filterString;
}

void UpdateAndStore(const in_addr_t& key)
{
	//Check for a valid suspect
	Suspect suspectCopy = suspects.GetSuspect(key);
	if(suspects.IsEmptySuspect(&suspectCopy))
	{
		return;
	}

	//Classify
	suspects.ClassifySuspect(key);

	//Check that we updated correctly
	suspectCopy = suspects.GetSuspect(key);
	if(suspects.IsEmptySuspect(&suspectCopy))
	{
		return;
	}

	//Send to UI
	SendSuspectToUIs(&suspectCopy);
}


void UpdateAndClassify(const in_addr_t& key)
{
	//Check for a valid suspect
	Suspect suspectCopy = suspects.GetSuspect(key);
	if(suspects.IsEmptySuspect(&suspectCopy))
	{
		return;
	}

	//Get the old hostility bool
	bool oldIsHostile = suspectCopy.GetIsHostile();

	//Classify
	suspects.ClassifySuspect(key);

	//Check that we updated correctly
	suspectCopy = suspects.GetSuspect(key);
	if(suspects.IsEmptySuspect(&suspectCopy))
	{
		return;
	}

	//Send silent alarm if needed
	if(suspectCopy.GetIsHostile() || oldIsHostile)
	{
		if(suspectCopy.GetIsLive())
		{
			SilentAlarm(&suspectCopy, oldIsHostile);
		}
	}
	//Send to UI
	SendSuspectToUIs(&suspectCopy);
}

void CheckForDroppedPackets()
{
	// Quick check for libpcap dropping packets
	for(uint i = 0; i < handles.size(); i++)
	{
		if(handles[i] == NULL)
		{
			continue;
		}

		pcap_stat captureStats;
		int result = pcap_stats(handles[i], &captureStats);
		if((result == 0) && (captureStats.ps_drop != dropCounts[i]))
		{
			if(captureStats.ps_drop > dropCounts[i])
			{
				stringstream ss;
				ss << "Libpcap has dropped " << captureStats.ps_drop - dropCounts[i] << " packets. Try increasing the capture buffer." << endl;
				LOG(WARNING, ss.str(), "");
			}
			dropCounts[i] = captureStats.ps_drop;
		}
	}
}

void UpdateHaystackFeatures()
{
	vector<uint32_t> haystackNodes;
	for(uint i = 0; i < haystackAddresses.size(); i++)
	{
		haystackNodes.push_back(htonl(inet_addr(haystackAddresses[i].c_str())));
	}

	for(uint i = 0; i < haystackDhcpAddresses.size(); i++)
	{
		haystackNodes.push_back(htonl(inet_addr(haystackDhcpAddresses[i].c_str())));
	}

	suspects.SetHaystackNodes(haystackNodes);
}

}
