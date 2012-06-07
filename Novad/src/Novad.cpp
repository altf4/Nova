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

//** Silent Alarm **
struct sockaddr_in serv_addr;
struct sockaddr* serv_addrPtr = (struct sockaddr *) &serv_addr;
vector<struct sockaddr_in> hostAddrs;
vector<uint> dropCounts;

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
vector<string> whitelistIpAddresses;
vector<string> whitelistIpRanges;
vector<pcap_t *> handles;
bpf_u_int32 maskp; /* subnet mask */
bpf_u_int32 netp; /* ip          */

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

	engine = new ClassificationEngine(suspects);

	Spawn_UI_Handler();

	Reload();

	//Are we Training or Classifying?
	if(Config::Inst()->GetIsTraining())
	{
		// We suffix the training capture files with the date/time
		time_t rawtime;
		time(&rawtime);
		struct tm *timeinfo = localtime(&rawtime);
		char buffer[40];
		strftime(buffer, 40, "%m-%d-%y_%H-%M-%S", timeinfo);

		if(system(string("mkdir " + Config::Inst()->GetPathTrainingCapFolder()).c_str()))
		{
			// Not really an problem, throws compiler warning if we don't catch the system call though
		}
		trainingCapFile = Config::Inst()->GetPathHome() + "/"
				+ Config::Inst()->GetPathTrainingCapFolder() + "/training" + buffer
				+ ".dump";

		trainingFileStream.open(trainingCapFile.data(), ios::app);

		if(!trainingFileStream.is_open())
		{
			LOG(CRITICAL, "Unable to open the training capture file.",
				"Unable to open training capture file at: "+trainingCapFile);
			exit(EXIT_FAILURE);
		}

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
		pthread_create(&classificationLoopThread,NULL,ClassificationLoop, NULL);
		pthread_create(&silentAlarmListenThread,NULL,SilentAlarmLoop, NULL);
		pthread_detach(classificationLoopThread);
		pthread_detach(silentAlarmListenThread);

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

	Start_Packet_Handler();

	if(!Config::Inst()->GetIsTraining())
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
	if(rc != 0)
	{
		if (errno == EAGAIN)
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
	while (in.is_open() && !in.eof() && lengthLeft)
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

	while (in.is_open() && !in.eof() && lengthLeft)
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
	trainingFileStream.close();
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

	string haystackAddresses_csv = "";

	struct bpf_program fp;			/* The compiled filter expression */
	char filter_exp[64];
	bpf_u_int32 maskp; /* subnet mask */
	bpf_u_int32 netp; /* ip          */

	haystackAddresses = GetHaystackAddresses(Config::Inst()->GetPathConfigHoneydHS());
	haystackDhcpAddresses = GetIpAddresses(dhcpListFile);
	whitelistIpAddresses = WhitelistConfiguration::GetIps();
	whitelistIpRanges = WhitelistConfiguration::GetIpRanges();
	haystackAddresses_csv = ConstructFilterString();

	UpdateHaystackFeatures();

	//If we're reading from a packet capture file
	if(Config::Inst()->GetReadPcap())
	{
		sleep(1); //To allow time for other processes to open
		handles[0] = pcap_open_offline(Config::Inst()->GetPathPcapFile().c_str(), errbuf);

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
		pcap_dispatch(handles[0], -1, Packet_Handler,NULL);

		if(Config::Inst()->GetGotoLive()) Config::Inst()->SetReadPcap(false); //If we are going to live capture set the flag.

		trainingFileStream.close();
		LOG(DEBUG, "Done processing PCAP file", "");

		pcap_close(handles[0]);
	}
	if(!Config::Inst()->GetReadPcap())
	{
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

			/* ask pcap for the network address and mask of the device */
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
		for(u_char i = 1; i < handles.size(); i++)
		{
			pthread_t readThread;
			u_char temp = i;
			pthread_create(&readThread, NULL, StartPcapLoop, &temp);
			pthread_detach(readThread);
		}
		if((handles.empty()) || (handles[0] == NULL))
		{
			LOG(CRITICAL, "Invalid pcap handle provided, unable to start pcap loop!", "");
			exit(EXIT_FAILURE);
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

void Packet_Handler(u_char *index,const struct pcap_pkthdr* pkthdr,const u_char* packet)
{

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
			Evidence * evidencePacket = new Evidence(packet + sizeof(struct ether_header), pkthdr);
			if(localIPs[*index] == evidencePacket->m_evidencePacket.ip_dst)
			{
				//manually setting dst ip to 0.0.0.1 designates the packet was to a real host not a haystack node
				evidencePacket->m_evidencePacket.ip_dst = 1;
			}
			suspectEvidence.InsertEvidence(evidencePacket);
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
	// Whitelist local traffic
	string filterString = "not src 0.0.0.0 && ";
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


vector <string> GetIpAddresses(string ipListFile)
{
	ifstream ipListFileStream(ipListFile.data());
	vector<string> whitelistedAddresses;

	if(ipListFileStream.is_open())
	{
		while(ipListFileStream.good())
		{
			string line;
			getline (ipListFileStream,line);
			if(strcmp(line.c_str(), "")&& line.at(0) != '#' )
			{
				whitelistedAddresses.push_back(line);
			}
		}
		ipListFileStream.close();
	}
	else
	{
		LOG(ERROR,"Unable to open file: " + ipListFile, "");
	}

	return whitelistedAddresses;
}


vector <string> GetHaystackAddresses(string honeyDConfigPath)
{
	//Path to the main log file
	ifstream honeydConfFile(honeyDConfigPath.c_str());
	vector<string> retAddresses;

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
		string honeydTemplate;

		//Is the first word "bind"?
		getline(LogInputLineStream, token, ' ');

		if(token.compare( "bind" ) != 0)
		{
			continue;
		}

		//The next token will be the IP address
		getline(LogInputLineStream, token, ' ');

		// Get the template
		getline(LogInputLineStream, honeydTemplate, ' ');

		if (honeydTemplate != "DoppelgangerReservedTemplate")
		{
			retAddresses.push_back(token);
		}
	}
	return retAddresses;
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

	//Store in training file
	trainingFileStream << string(inet_ntoa(suspectCopy.GetInAddr())) << " ";
	for (int j = 0; j < DIM; j++)
	{
		trainingFileStream << suspectCopy.GetFeatureSet(MAIN_FEATURES).m_features[j] << " ";
	}
	trainingFileStream << "\n";

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
	for (uint i = 0; i < haystackAddresses.size(); i++)
	{
		cout << "Address is " << haystackAddresses[i] << endl;
		haystackNodes.push_back(htonl(inet_addr(haystackAddresses[i].c_str())));
	}

	for (uint i = 0; i < haystackDhcpAddresses.size(); i++)
	{
		cout << "Address is " << haystackDhcpAddresses[i] << endl;
		haystackNodes.push_back(htonl(inet_addr(haystackDhcpAddresses[i].c_str())));
	}

	suspects.SetHaystackNodes(haystackNodes);
}

}
