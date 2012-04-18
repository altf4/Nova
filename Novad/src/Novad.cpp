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
	suspectsSinceLastSave.UpdateAllSuspects();
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
		if((numBytes = suspects.ReadContents(&in, expirationTime)) == 0)
		{
			//No need to log, ReadContents already does so
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
	uint bytesRead = 0, cur = 0, lengthLeft = 0;
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
		//Save our position
		cur = in.tellg();

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

void Reload()
{
	LoadConfiguration();

	// Reload the configuration file
	Config::Inst()->LoadConfig();

	engine->LoadDataPointsFromFile(Config::Inst()->GetPathTrainingFile());
	Suspect suspectCopy;
	vector<uint64_t> keys = suspects.GetAllKeys();
	// Set everyone to be reclassified
	for(uint i = 0;i < keys.size(); i++)
	{
		suspectCopy = suspects.CheckOut(keys[i]);
		suspectCopy.SetNeedsClassificationUpdate(true);
		suspects.CheckIn(&suspectCopy);
	}
}


void SilentAlarm(Suspect *suspect, int oldClassification)
{
	int sockfd = 0;
	string commandLine;
	string hostAddrString = GetLocalIP(Config::Inst()->GetInterface().c_str());

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
			if(dataLen != suspectCopy.Serialize(serializedBuffer, UNSENT_FEATURE_DATA))
			{
				stringstream ss;
				ss << "Serialization of Suspect with key: " << suspectCopy.GetIpAddress();
				ss << " returned a size != " << dataLen;
				LOG(ERROR, "Unable to Serialize Suspect", ss.str());
				return;
			}
			suspectCopy.UpdateFeatureData(INCLUDE);
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
		{
			packet_info.fromHaystack = FROM_LTM;
		}
		else
		{
			packet_info.fromHaystack = FROM_HAYSTACK_DP;
		}

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
	//Attempt to add the evidence into the table, if it fails the suspect doesn't exist in this table
	in_addr_t key = packet.ip_hdr.ip_src.s_addr;
	if(!suspects.AddEvidenceToSuspect(key, packet))
	{
		suspects.AddNewSuspect(packet);
	}
	//Attempt to add the evidence into the table, if it fails the suspect doesn't exist in this table
	if(!Config::Inst()->GetIsTraining() && !suspectsSinceLastSave.AddEvidenceToSuspect(key, packet))
	{
		suspectsSinceLastSave.AddNewSuspect(packet);
	}
}


void UpdateAndStore(in_addr_t key)
{
	// If the checkout failed and we got the empty suspect
	Suspect suspectCopy = suspects.GetSuspect(key);
	if(suspects.IsEmptySuspect(&suspectCopy))
	{
		return;
	}
	suspects.UpdateSuspect(key);
	suspectCopy = suspects.GetSuspect(key);
	if(suspects.IsEmptySuspect(&suspectCopy))
	{
		return;
	}
	trainingFileStream << string(inet_ntoa(suspectCopy.GetInAddr())) << " ";
	for (int j = 0; j < DIM; j++)
	{
		trainingFileStream << suspectCopy.GetFeatureSet(MAIN_FEATURES).m_features[j] << " ";
	}
	trainingFileStream << "\n";
	if(SendSuspectToUI(&suspectCopy))
	{
		LOG(DEBUG, string("Sent a suspect to the UI: ")+ inet_ntoa(suspectCopy.GetInAddr()), "");
	}
	else
	{
		LOG(DEBUG, string("Failed to send a suspect to the UI: ")+ inet_ntoa(suspectCopy.GetInAddr()), "");
	}
}


void UpdateAndClassify(in_addr_t key)
{
	// If the checkout failed and we got the empty suspect
	Suspect suspectCopy = suspects.GetSuspect(key);
	if(suspects.IsEmptySuspect(&suspectCopy))
	{
		return;
	}

	//Get the old hostility bool
	bool oldIsHostile = suspectCopy.GetIsHostile();
	suspects.UpdateSuspect(key);
	suspectCopy = suspects.GetSuspect(key);
	if(suspects.IsEmptySuspect(&suspectCopy))
	{
		return;
	}

	if(suspectCopy.GetIsHostile() || oldIsHostile)
	{
		if(suspectCopy.GetIsLive())
		{
			SilentAlarm(&suspectCopy, oldIsHostile);
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

}
