//============================================================================
// Name        : Haystack.cpp
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
// Description : Nova utility for transforming Honeyd log files into
//					TrafficEvents usable by Nova's Classification Engine.
//============================================================================

#include "NOVAConfiguration.h"
#include "HashMapStructs.h"
#include "SuspectTable.h"
#include "NovaUtil.h"
#include "Haystack.h"
#include "Logger.h"

#include <netinet/if_ether.h>
#include <sys/un.h>
#include <sys/inotify.h>
#include <syslog.h>
#include <errno.h>
#include <sstream>
#include <fstream>

using namespace std;
using namespace Nova;

static TCPSessionHashTable SessionTable;
static SuspectHashTable	suspects;

pthread_rwlock_t HSsessionLock;
pthread_rwlock_t HSsuspectLock;

//Memory assignments moved outside packet handler to increase performance
int HSlen, HSdest_port;
struct sockaddr_un HSremote;
Packet HSpacket_info;
struct ether_header *HSethernet;  	/* net/ethernet.h */
struct ip *HSip_hdr; 					/* The IP header */
char HStcp_socket[55];

u_char HSdata[MAX_MSG_SIZE];
uint HSdataLen;

char * HSpathsFile = (char*)PATHS_FILE;
extern string userHomePath;
extern string novaConfigPath;
extern NOVAConfiguration *globalConfig;

bool HSusePcapcFile;

extern Logger *logger;

string dhcpListFile = "/var/log/honeyd/ipList";
vector <string> haystackAddresses;
vector <string> haystackDhcpAddresses;
pcap_t *handle;
bpf_u_int32 maskp;				/* subnet mask */
bpf_u_int32 netp; 				/* ip          */

int notifyFd;
int watch;

void *Nova::HaystackMain(void *ptr)
{
	pthread_rwlock_init(&HSsessionLock, NULL);
	pthread_rwlock_init(&HSsuspectLock, NULL);
	pthread_t TCP_timeout_thread;
	pthread_t GUIListenThread;
	pthread_t SuspectUpdateThread;
	pthread_t ipUpdateThread;

	SessionTable.set_empty_key("");
	SessionTable.resize(INIT_SIZE_HUGE);
	suspects.set_empty_key(0);
	suspects.resize(INIT_SIZE_SMALL);

	char errbuf[PCAP_ERRBUF_SIZE];
	bzero(HSdata, MAX_MSG_SIZE);

	int ret;
	HSusePcapcFile = globalConfig->getReadPcap();


	string haystackAddresses_csv = "";

	if(chdir(userHomePath.c_str()) == -1)
		logger->Logging("Haystack", INFO, "Failed to change directory to " + userHomePath, "Failed to change directory to " + userHomePath);

	if(!globalConfig->getUseTerminals())
	{
		openlog("Haystack", NO_TERM_SYSL, LOG_AUTHPRIV);
	}

	else
	{
		openlog("Haystack", OPEN_SYSL, LOG_AUTHPRIV);
	}

	pthread_create(&GUIListenThread, NULL, HS_GUILoop, NULL);

	struct bpf_program fp;			/* The compiled filter expression */
	char filter_exp[64];


	haystackAddresses = GetHaystackAddresses(globalConfig->getPathConfigHoneydHs());
	haystackDhcpAddresses = GetHaystackDhcpAddresses(dhcpListFile);
	haystackAddresses_csv = ConstructFilterString();


	notifyFd = inotify_init ();

	if (notifyFd > 0)
	{
		watch = inotify_add_watch (notifyFd, dhcpListFile.c_str(), IN_CLOSE_WRITE | IN_MOVED_TO | IN_MODIFY | IN_DELETE);
		pthread_create(&ipUpdateThread, NULL, UpdateIPFilter,NULL);
	}
	else
	{
		syslog(SYSL_ERR, "Unable to set up 'file modified/moved' watcher for the IP list file\n");
	}


	//Preform the socket address for faster run time
	//Builds the key path
	string key = KEY_FILENAME;
	key = userHomePath+key;

	HSremote.sun_family = AF_UNIX;
	strcpy(HSremote.sun_path, key.c_str());
	HSlen = strlen(HSremote.sun_path) + sizeof(HSremote.sun_family);

	//If we're reading from a packet capture file
	if(HSusePcapcFile)
	{
		sleep(1); //To allow time for other processes to open
		handle = pcap_open_offline(globalConfig->getPathPcapFile().c_str(), errbuf);

		if(handle == NULL)
		{
			syslog(SYSL_ERR, "Line: %d Couldn't open file: %s: %s", __LINE__, globalConfig->getPathPcapFile().c_str(), errbuf);
			exit(EXIT_FAILURE);
		}
		if (pcap_compile(handle, &fp, haystackAddresses_csv.data(), 0, maskp) == -1)
		{
			syslog(SYSL_ERR, "Line: %d Couldn't parse filter: %s %s", __LINE__, filter_exp, pcap_geterr(handle));
			exit(EXIT_FAILURE);
		}

		if (pcap_setfilter(handle, &fp) == -1)
		{
			syslog(SYSL_ERR, "Line: %d Couldn't install filter: %s %s", __LINE__, filter_exp, pcap_geterr(handle));
			exit(EXIT_FAILURE);
		}
		//First process any packets in the file then close all the sessions
		pcap_loop(handle, -1, HSPacket_Handler,NULL);

		HSTCPTimeout(NULL);
		HSSuspectLoop(NULL);


		if(globalConfig->getGotoLive()) HSusePcapcFile = false; //If we are going to live capture set the flag.
	}


	if(!HSusePcapcFile)
	{
		//Open in non-promiscuous mode, since we only want traffic destined for the host machine
		handle = pcap_open_live(globalConfig->getInterface().c_str(), BUFSIZ, 0, 1000, errbuf);

		if(handle == NULL)
		{
			syslog(SYSL_ERR, "Line: %d Couldn't open device: %s %s", __LINE__, globalConfig->getInterface().c_str(), errbuf);
			exit(EXIT_FAILURE);
		}

		/* ask pcap for the network address and mask of the device */
		ret = pcap_lookupnet(globalConfig->getInterface().c_str(), &netp, &maskp, errbuf);

		if(ret == -1)
		{
			syslog(SYSL_ERR, "Line: %d %s", __LINE__, errbuf);
			exit(EXIT_FAILURE);
		}

		if (pcap_compile(handle, &fp, haystackAddresses_csv.data(), 0, maskp) == -1)
		{
			syslog(SYSL_ERR, "Line: %d Couldn't parse filter: %s %s", __LINE__, filter_exp, pcap_geterr(handle));
			exit(EXIT_FAILURE);
		}

		if (pcap_setfilter(handle, &fp) == -1)
		{
			syslog(SYSL_ERR, "Line: %d Couldn't install filter: %s %s", __LINE__, filter_exp, pcap_geterr(handle));
			exit(EXIT_FAILURE);
		}

		pthread_create(&TCP_timeout_thread, NULL, HSTCPTimeout, NULL);
		pthread_create(&SuspectUpdateThread, NULL, HSSuspectLoop, NULL);

		//"Main Loop"
		//Runs the function "Packet_Handler" every time a packet is received
	    pcap_loop(handle, -1, HSPacket_Handler, NULL);
	}
	return 0;
}


void Nova::HSPacket_Handler(u_char *useless,const struct pcap_pkthdr* pkthdr,const u_char* packet)
{
	if(packet == NULL)
	{
		syslog(SYSL_ERR, "Line: %d Didn't grab packet!", __LINE__);
		return;
	}


	/* let's start with the ether header... */
	HSethernet = (struct ether_header *) packet;

	/* Do a couple of checks to see what packet type we have..*/
	if (ntohs (HSethernet->ether_type) == ETHERTYPE_IP)
	{
		HSip_hdr = (struct ip*)(packet + sizeof(struct ether_header));

		//Prepare Packet structure
		HSpacket_info.ip_hdr = *HSip_hdr;
		HSpacket_info.pcap_header = *pkthdr;
		HSpacket_info.fromHaystack = FROM_HAYSTACK_DP;

		//IF UDP or ICMP
		if(HSip_hdr->ip_p == 17 )
		{
			HSpacket_info.udp_hdr = *(struct udphdr*) ((char *)HSip_hdr + sizeof(struct ip));
			HSUpdateSuspect(HSpacket_info);
		}
		else if(HSip_hdr->ip_p == 1)
		{
			HSpacket_info.icmp_hdr = *(struct icmphdr*) ((char *)HSip_hdr + sizeof(struct ip));
			HSUpdateSuspect(HSpacket_info);
		}
		//If TCP...
		else if(HSip_hdr->ip_p == 6)
		{
			HSpacket_info.tcp_hdr = *(struct tcphdr*)((char*)HSip_hdr + sizeof(struct ip));
			HSdest_port = ntohs(HSpacket_info.tcp_hdr.dest);

			bzero(HStcp_socket, 55);
			snprintf(HStcp_socket, 55, "%d-%d-%d", HSip_hdr->ip_dst.s_addr, HSip_hdr->ip_src.s_addr, HSdest_port);

			pthread_rwlock_wrlock(&HSsessionLock);
			//If this is a new entry...
			if( SessionTable[HStcp_socket].session.size() == 0)
			{
				//Insert packet into Hash Table
				SessionTable[HStcp_socket].session.push_back(HSpacket_info);
				SessionTable[HStcp_socket].fin = false;
			}

			//If there is already a session in progress for the given LogEntry
			else
			{
				//If Session is ending
				//TODO: The session may continue a few packets after the FIN. Account for this case.
				//See ticket #15
				if(HSpacket_info.tcp_hdr.fin)
				{
					SessionTable[HStcp_socket].session.push_back(HSpacket_info);
					SessionTable[HStcp_socket].fin = true;
				}
				else
				{
					//Add this new packet to the session vector
					SessionTable[HStcp_socket].session.push_back(HSpacket_info);
				}
			}
			pthread_rwlock_unlock(&HSsessionLock);
		}

		// Allow for continuous classification
		if (!globalConfig->getClassificationTimeout())
			HSSuspectLoop(NULL);
	}
	else if(ntohs(HSethernet->ether_type) == ETHERTYPE_ARP)
	{
		return;
	}
	else
	{
		syslog(SYSL_ERR, "Line: %d Unknown Non-IP Packet Received", __LINE__);
		return;
	}
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

	syslog(SYSL_INFO, "Pcap filter string is: %s", filterString.c_str());
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
					syslog(SYSL_ERR, "Line: %d Couldn't parse filter: %s %s", __LINE__, filter_exp, pcap_geterr(handle));

				if (pcap_setfilter(handle, &fp) == -1)
					syslog(SYSL_ERR, "Line: %d Couldn't install filter: %s %s", __LINE__, filter_exp, pcap_geterr(handle));
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

void *Nova::HS_GUILoop(void *ptr)
{
	struct sockaddr_un localIPCAddress;
	int IPCsock, len;

	if((IPCsock = socket(AF_UNIX,SOCK_STREAM,0)) == -1)
	{
		syslog(SYSL_ERR, "Line: %d socket: %s", __LINE__, strerror(errno));
		close(IPCsock);
		exit(EXIT_FAILURE);
	}

	localIPCAddress.sun_family = AF_UNIX;

	//Builds the key path
	string key = HS_GUI_FILENAME;
	key = userHomePath + key;

	strcpy(localIPCAddress.sun_path, key.c_str());
	unlink(localIPCAddress.sun_path);
	len = strlen(localIPCAddress.sun_path) + sizeof(localIPCAddress.sun_family);

	if(bind(IPCsock,(struct sockaddr *)&localIPCAddress,len) == -1)
	{
		syslog(SYSL_ERR, "Line: %d bind: %s", __LINE__, strerror(errno));
		close(IPCsock);
		exit(EXIT_FAILURE);
	}

	if(listen(IPCsock, SOCKET_QUEUE_SIZE) == -1)
	{
		syslog(SYSL_ERR, "Line: %d listen: %s", __LINE__, strerror(errno));
		close(IPCsock);
		exit(EXIT_FAILURE);
	}
	while(true)
	{
		HSReceiveGUICommand(IPCsock);
	}
}


void Nova::HSReceiveGUICommand(int socket)
{
	struct sockaddr_un msgRemote;
    int socketSize, msgSocket;
    int bytesRead;
    u_char msgBuffer[MAX_GUIMSG_SIZE];
    GUIMsg msg = GUIMsg();
    in_addr_t suspectKey = 0;

    socketSize = sizeof(msgRemote);
    //Blocking call
    if ((msgSocket = accept(socket, (struct sockaddr *)&msgRemote, (socklen_t*)&socketSize)) == -1)
    {
    	syslog(SYSL_ERR, "Line: %d accept: %s", __LINE__, strerror(errno));
		close(msgSocket);
    }
    if((bytesRead = recv(msgSocket, msgBuffer, MAX_GUIMSG_SIZE, 0 )) == -1)
    {
    	syslog(SYSL_ERR, "Line: %d recv: %s", __LINE__, strerror(errno));
		close(msgSocket);
    }

    msg.DeserializeMessage(msgBuffer);
    switch(msg.GetType())
    {
    	case CLEAR_ALL:
			pthread_rwlock_wrlock(&HSsuspectLock);
    		suspects.clear();
			pthread_rwlock_unlock(&HSsuspectLock);
    		break;
    	case CLEAR_SUSPECT:
			suspectKey = inet_addr(msg.GetValue().c_str());
			pthread_rwlock_wrlock(&HSsuspectLock);
			suspects.set_deleted_key(5);
			suspects.erase(suspectKey);
			suspects.clear_deleted_key();
			pthread_rwlock_unlock(&HSsuspectLock);
			break;
    	case EXIT:
    		exit(EXIT_SUCCESS);
    	default:
    		break;
    }
    close(msgSocket);
}


void *Nova::HSTCPTimeout(void *ptr)
{
	do
	{
		time_t currentTime = time(NULL);
		time_t packetTime;

		pthread_rwlock_rdlock(&HSsessionLock);
		for ( TCPSessionHashTable::iterator it = SessionTable.begin() ; it != SessionTable.end(); it++ )
		{

			if(it->second.session.size() > 0)
			{
				packetTime = it->second.session.back().pcap_header.ts.tv_sec;
				//If were reading packets from a file, assume all packets have been loaded and go beyond timeout threshhold
				if(HSusePcapcFile)
				{
					currentTime = packetTime+3+globalConfig->getTcpTimout();
				}
				// If it exists)
				if(packetTime + 2 < currentTime)
				{
					//If session has been finished for more than two seconds
					if(it->second.fin == true)
					{
						//tempEvent = new TrafficEvent( &(SessionTable[it->first].session), FROM_HAYSTACK_DP);

						for (uint p = 0; p < (SessionTable[it->first].session).size(); p++)
						{
							(SessionTable[it->first].session).at(p).fromHaystack = FROM_HAYSTACK_DP;
							HSUpdateSuspect((SessionTable[it->first].session).at(p));
						}

						// Allow for continuous classification
						if (!globalConfig->getClassificationTimeout())
							HSSuspectLoop(NULL);

						pthread_rwlock_unlock(&HSsessionLock);
						pthread_rwlock_wrlock(&HSsessionLock);
						SessionTable[it->first].session.clear();
						SessionTable[it->first].fin = false;
						pthread_rwlock_unlock(&HSsessionLock);
						pthread_rwlock_rdlock(&HSsessionLock);

						//delete tempEvent;
						//tempEvent = NULL;
					}
					//If this session is timed out
					else if(packetTime + globalConfig->getTcpTimout() < currentTime)
					{
						//tempEvent = new TrafficEvent( &(SessionTable[it->first].session), FROM_HAYSTACK_DP);
						for (uint p = 0; p < (SessionTable[it->first].session).size(); p++)
						{
							(SessionTable[it->first].session).at(p).fromHaystack = FROM_HAYSTACK_DP;
							HSUpdateSuspect((SessionTable[it->first].session).at(p));
						}

						// Allow for continuous classification
						if (!globalConfig->getClassificationTimeout())
							HSSuspectLoop(NULL);

						pthread_rwlock_unlock(&HSsessionLock);
						pthread_rwlock_wrlock(&HSsessionLock);
						SessionTable[it->first].session.clear();
						SessionTable[it->first].fin = false;
						pthread_rwlock_unlock(&HSsessionLock);
						pthread_rwlock_rdlock(&HSsessionLock);

						//delete tempEvent;
						//tempEvent = NULL;
					}
				}
			}
		}
		pthread_rwlock_unlock(&HSsessionLock);
		//Check only once every TCP_CHECK_FREQ seconds
		sleep(globalConfig->getTcpCheckFreq());
	}while(!HSusePcapcFile);

	//After a pcap file is read we do one iteration of this function to clear out the sessions
	//This is return is to prevent an error being thrown when there isn't one.
	if(HSusePcapcFile) return NULL;

	//Shouldn't get here
	syslog(SYSL_ERR, "Line: %d TCP Timeout Thread has halted", __LINE__);
	return NULL;
}


bool Nova::HSSendToCE(Suspect *suspect)
{
	int socketFD;

	do
	{
		HSdataLen = suspect->SerializeSuspect(HSdata);
		HSdataLen += suspect->features.unsentData->SerializeFeatureData(HSdata+HSdataLen);
		suspect->features.unsentData->clearFeatureData();

		if ((socketFD = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		{
			syslog(SYSL_ERR, "Line: %d Unable to make socket to ClassificationEngine: %s", __LINE__, strerror(errno));
			close(socketFD);
			return false;
		}

		if (connect(socketFD, (struct sockaddr *)&HSremote, HSlen) == -1)
		{
			syslog(SYSL_ERR, "Line: %d Unable to connect to ClassificationEngine: %s", __LINE__, strerror(errno));
			close(socketFD);
			return false;
		}

		if (send(socketFD, HSdata, HSdataLen, 0) == -1)
		{
			syslog(SYSL_ERR, "Line: %d Unable to send to ClassificationEngine: %s", __LINE__, strerror(errno));
			close(socketFD);
			return false;
		}
		bzero(HSdata,HSdataLen);
		close(socketFD);

	}while(HSdataLen == MORE_DATA);

	return true;
}


void Nova::HSUpdateSuspect(Packet packet)
{
	in_addr_t addr = packet.ip_hdr.ip_src.s_addr;
	pthread_rwlock_wrlock(&HSsuspectLock);
	//If our suspect is new
	if(suspects.find(addr) == suspects.end())
		suspects[addr] = new Suspect(packet);
	//Else our suspect exists
	else
		suspects[addr]->AddEvidence(packet);

	suspects[addr]->isLive = HSusePcapcFile;
	pthread_rwlock_unlock(&HSsuspectLock);
}


void *Nova::HSSuspectLoop(void *ptr)
{
	do
	{
		sleep(globalConfig->getClassificationTimeout());
		pthread_rwlock_rdlock(&HSsuspectLock);
		for(SuspectHashTable::iterator it = suspects.begin(); it != suspects.end(); it++)
		{
			if(it->second->needs_feature_update)
			{
				pthread_rwlock_unlock(&HSsuspectLock);
				pthread_rwlock_wrlock(&HSsuspectLock);
				for(uint i = 0; i < it->second->evidence.size(); i++)
				{
					it->second->features.unsentData->UpdateEvidence(it->second->evidence[i]);
				}
				it->second->evidence.clear();
				HSSendToCE(it->second);
				it->second->needs_feature_update = false;
				pthread_rwlock_unlock(&HSsuspectLock);
				pthread_rwlock_rdlock(&HSsuspectLock);
			}
		}
		pthread_rwlock_unlock(&HSsuspectLock);
	} while(!HSusePcapcFile && globalConfig->getClassificationTimeout());

	//After a pcap file is read we do one iteration of this function to clear out the sessions
	//This is return is to prevent an error being thrown when there isn't one.
	// Also return if continuous classification is enabled by setting classificationTimeout to 0
	if(HSusePcapcFile || !globalConfig->getClassificationTimeout()) return NULL;

	//Shouldn't get here
	syslog(SYSL_ERR, "Line: %d SuspectLoop Thread has halted!", __LINE__);
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

	if( honeydConfFile == NULL)
	{
		syslog(SYSL_ERR, "Line: %d Error opening log file. Does it exist?", __LINE__);
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
