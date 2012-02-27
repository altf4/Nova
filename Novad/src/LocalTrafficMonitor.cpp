//============================================================================
// Name        : LocalTrafficMonitor.cpp
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
// Description : Monitors local network traffic and sends detailed TrafficEvents
//					to the Classification Engine.
//============================================================================/*

#include "LocalTrafficMonitor.h"
#include "NOVAConfiguration.h"
#include "HashMapStructs.h"
#include "SuspectTable.h"
#include "NovaUtil.h"
#include "Logger.h"

#include <netinet/if_ether.h>
#include <sys/un.h>
#include <syslog.h>
#include <errno.h>
#include <sstream>
#include <fstream>

using namespace std;
using namespace Nova;

static TCPSessionHashTable SessionTable;
static SuspectHashTable	suspects;

pthread_rwlock_t LTMsessionLock;
pthread_rwlock_t LTMsuspectLock;

//Global memory assignments to improve packet handler performance
int LTMlen, LTMdest_port;
struct ether_header *LTMethernet;  	/* net/ethernet.h */
struct ip *LTMip_hdr; 					/* The IP header */
Packet LTMpacket_info;
char LTMtcp_socket[55];
struct sockaddr_un LTMremote;

u_char LTMdata[MAX_MSG_SIZE];
uint LTMdataLen;
bool LTMusePcapFile;

char * LTMpathsFile = (char*)PATHS_FILE;
extern string userHomePath;
extern string novaConfigPath;
extern NOVAConfiguration *globalConfig;


string LTMkey;

extern Logger * logger;

void *Nova::LocalTrafficMonitorMain(void *ptr)
{
	pthread_rwlock_init(&LTMsessionLock, NULL);
	pthread_rwlock_init(&LTMsuspectLock, NULL);
	pthread_t TCP_timeout_thread;
	pthread_t GUIListenThread;
	pthread_t SuspectUpdateThread;

	SessionTable.set_empty_key("");
	SessionTable.resize(INIT_SIZE_HUGE);
	suspects.set_empty_key(0);
	suspects.resize(INIT_SIZE_SMALL);

	char errbuf[PCAP_ERRBUF_SIZE];
	bzero(LTMdata, MAX_MSG_SIZE);

	LTMusePcapFile = globalConfig->getReadPcap();
	int ret;

	bpf_u_int32 maskp;				/* subnet mask */
	bpf_u_int32 netp; 				/* ip          */

	string hostAddress;


	LTMLoadConfig((char*)novaConfigPath.c_str());

	if(!globalConfig->getUseTerminals())
	{
		openlog("LocalTrafficMonitor", NO_TERM_SYSL, LOG_AUTHPRIV);
	}

	else
	{
		openlog("LocalTrafficMonitor", OPEN_SYSL, LOG_AUTHPRIV);
	}

	//Pre-Forms the socket address to improve performance
	//Builds the key path
	string key = KEY_FILENAME;
	key = userHomePath+key;

	LTMremote.sun_family = AF_UNIX;
	strcpy(LTMremote.sun_path, key.c_str());
	LTMlen = strlen(LTMremote.sun_path) + sizeof(LTMremote.sun_family);

	pthread_create(&GUIListenThread, NULL, LTM_GUILoop, NULL);

	struct bpf_program fp;			/* The compiled filter expression */
	char filter_exp[64];
	pcap_t *handle;

	hostAddress = GetLocalIP(globalConfig->getInterface().c_str());

	if(hostAddress.empty())
	{
		syslog(SYSL_ERR, "Line: %d Invalid interface given", __LINE__);
		exit(EXIT_FAILURE);
	}

	//Form the Filter Expression String
	bzero(filter_exp, 64);
	snprintf(filter_exp, 64, "dst host %s", hostAddress.data());
	syslog(SYSL_INFO, "%s", filter_exp);

	//If we are reading from a packet capture file
	if(globalConfig->getReadPcap())
	{
		sleep(1); //To allow time for other processes to open
		handle = pcap_open_offline(globalConfig->getPathPcapFile().c_str(), errbuf);

		if(handle == NULL)
		{
			syslog(SYSL_ERR, "Line: %d Couldn't open file: %s: %s", __LINE__, globalConfig->getPathPcapFile().c_str(), errbuf);
			exit(EXIT_FAILURE);
		}
		if (pcap_compile(handle, &fp,  filter_exp, 0, maskp) == -1)
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
		pcap_loop(handle, -1, LTMPacket_Handler,NULL);

		LTMTCPTimeout(NULL);
		LTMSuspectLoop(NULL);

		if(globalConfig->getGotoLive()) LTMusePcapFile = false; //If we are going to live capture set the flag.
	}
	if(!LTMusePcapFile)
	{
		//system("setcap cap_net_raw,cap_net_admin=eip /usr/local/bin/LocalTrafficMonitor");
		//Open in non-promiscuous mode, since we only want traffic destined for the host machine
		handle = pcap_open_live(globalConfig->getInterface().c_str(), BUFSIZ, 0, 1000, errbuf);

		if(handle == NULL)
		{
			syslog(SYSL_ERR, "Line: %d Couldn't open device: %s: %s", __LINE__, globalConfig->getInterface().c_str(), errbuf);
			exit(EXIT_FAILURE);
		}

		/* ask pcap for the network address and mask of the device */
		ret = pcap_lookupnet(globalConfig->getInterface().c_str(), &netp, &maskp, errbuf);

		if(ret == -1)
		{
			syslog(SYSL_ERR, "Line: %d %s", __LINE__, errbuf);
			exit(EXIT_FAILURE);
		}

		if (pcap_compile(handle, &fp,  filter_exp, 0, maskp) == -1)
		{
			syslog(SYSL_ERR, "Line: %d Couldn't parse filter: %s %s", __LINE__, filter_exp, pcap_geterr(handle));
			exit(EXIT_FAILURE);
		}

		if (pcap_setfilter(handle, &fp) == -1)
		{
			syslog(SYSL_ERR, "Line: %d Couldn't install filter: %s %s", __LINE__, filter_exp, pcap_geterr(handle));
			exit(EXIT_FAILURE);
		}

		pthread_create(&TCP_timeout_thread, NULL, LTMTCPTimeout, NULL);
		pthread_create(&SuspectUpdateThread, NULL, LTMSuspectLoop, NULL);

		//"Main Loop"
		//Runs the function "LTM Packet_Handler" every time a packet is received
	    pcap_loop(handle, -1, LTMPacket_Handler, NULL);
	}
	return 0;
}


void Nova::LTMPacket_Handler(u_char *useless,const struct pcap_pkthdr* pkthdr,const u_char* packet)
{
	if(packet == NULL)
	{
		syslog(SYSL_ERR, "Line: %d Didn't grab packet!", __LINE__);
		return;
	}

	/* let's start with the ether header... */
	LTMethernet = (struct ether_header *) packet;

	/* Do a couple of checks to see what packet type we have..*/
	if (ntohs (LTMethernet->ether_type) == ETHERTYPE_IP)
	{
		LTMip_hdr = (struct ip*)(packet + sizeof(struct ether_header));

		//Prepare Packet structure
		LTMpacket_info.ip_hdr = *LTMip_hdr;
		LTMpacket_info.pcap_header = *pkthdr;
		LTMpacket_info.fromHaystack = FROM_HAYSTACK_DP;

		//IF UDP or ICMP
		if(LTMip_hdr->ip_p == 17 )
		{
			LTMpacket_info.udp_hdr = *(struct udphdr*) ((char *)LTMip_hdr + sizeof(struct ip));

			if((in_port_t)ntohs(LTMpacket_info.udp_hdr.dest) ==  globalConfig->getSaPort())
			{
				//if we receive a udp packet on the silent alarm port, see if it is a port knock request
				LTMKnockRequest(LTMpacket_info, (((u_char *)LTMip_hdr + sizeof(struct ip)) + sizeof(struct udphdr)));
			}
			LTMUpdateSuspect(LTMpacket_info);
		}
		else if(LTMip_hdr->ip_p == 1)
		{
			LTMpacket_info.icmp_hdr = *(struct icmphdr*) ((char *)LTMip_hdr + sizeof(struct ip));
			LTMUpdateSuspect(LTMpacket_info);
		}
		//If TCP...
		else if(LTMip_hdr->ip_p == 6)
		{
			LTMpacket_info.tcp_hdr = *(struct tcphdr*)((char*)LTMip_hdr + sizeof(struct ip));
			LTMdest_port = ntohs(LTMpacket_info.tcp_hdr.dest);

			bzero(LTMtcp_socket, 55);
			snprintf(LTMtcp_socket, 55, "%d-%d-%d", LTMip_hdr->ip_dst.s_addr, LTMip_hdr->ip_src.s_addr, LTMdest_port);
			pthread_rwlock_wrlock(&LTMsessionLock);
			//If this is a new entry...
			if( SessionTable[LTMtcp_socket].session.size() == 0)
			{
				//Insert packet into Hash Table
				SessionTable[LTMtcp_socket].session.push_back(LTMpacket_info);
				SessionTable[LTMtcp_socket].fin = false;
			}

			//If there is already a session in progress for the given LogEntry
			else
			{
				//If Session is ending
				//TODO: The session may continue a few packets after the FIN. Account for this case.
				//See ticket #15
				if(LTMpacket_info.tcp_hdr.fin)
				{
					SessionTable[LTMtcp_socket].session.push_back(LTMpacket_info);
					SessionTable[LTMtcp_socket].fin = true;
				}
				else
				{
					//Add this new packet to the session vector
					SessionTable[LTMtcp_socket].session.push_back(LTMpacket_info);
				}
			}
			pthread_rwlock_unlock(&LTMsessionLock);
		}

		// Allow for continuous classification
		if (!globalConfig->getClassificationTimeout())
			LTMSuspectLoop(NULL);
	}
	else if(ntohs(LTMethernet->ether_type) == ETHERTYPE_ARP)
	{
		return;
	}
	else
	{
		syslog(SYSL_ERR, "Line: %d Unknown Non-IP Packet Received", __LINE__);
		return;
	}
}


void *Nova::LTM_GUILoop(void *ptr)
{
	struct sockaddr_un localIPCAddress;
	int IPCsock;

	if((IPCsock = socket(AF_UNIX,SOCK_STREAM,0)) == -1)
	{
		syslog(SYSL_ERR, "Line: %d socket: %s", __LINE__, strerror(errno));
		close(IPCsock);
		exit(EXIT_FAILURE);
	}

	localIPCAddress.sun_family = AF_UNIX;

	//Builds the key path
	string key = LTM_GUI_FILENAME;
	key = userHomePath+key;

	strcpy(localIPCAddress.sun_path, key.c_str());
	unlink(localIPCAddress.sun_path);
	// length = strlen(localIPCAddress.sun_path) + sizeof(localIPCAddress.sun_family);

	if(bind(IPCsock,(struct sockaddr *)&localIPCAddress,LTMlen) == -1)
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
		LTMReceiveGUICommand(IPCsock);
	}
}


void Nova::LTMReceiveGUICommand(int socket)
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
			pthread_rwlock_wrlock(&LTMsuspectLock);
			suspects.clear();
			pthread_rwlock_unlock(&LTMsuspectLock);
			break;
		case CLEAR_SUSPECT:
			suspectKey = inet_addr(msg.GetValue().c_str());
			pthread_rwlock_wrlock(&LTMsuspectLock);
			suspects.set_deleted_key(5);
			suspects.erase(suspectKey);
			suspects.clear_deleted_key();
			pthread_rwlock_unlock(&LTMsuspectLock);
			break;
    	case EXIT:
    		exit(EXIT_SUCCESS);
    	default:
    		break;
    }
    close(msgSocket);
}


void *Nova::LTMTCPTimeout( void *ptr )
{
	do
	{
		time_t currentTime = time(NULL);
		time_t packetTime;

		pthread_rwlock_rdlock(&LTMsessionLock);
		for ( TCPSessionHashTable::iterator it = SessionTable.begin() ; it != SessionTable.end(); it++ )
		{
			if(it->second.session.size() > 0)
			{
				packetTime = it->second.session.back().pcap_header.ts.tv_sec;
				//If were reading packets from a file, assume all packets have been loaded and go beyond timeout threshhold
				if(LTMusePcapFile)
				{
					currentTime = packetTime+3+globalConfig->getTcpTimout();
				}
				// If it exists)
				if(packetTime + 2 < currentTime)
				{
					//If session has been finished for more than two seconds
					if(it->second.fin == true)
					{
						for (uint p = 0; p < (SessionTable[it->first].session).size(); p++)
						{
							(SessionTable[it->first].session).at(p).fromHaystack = FROM_LTM;
							LTMUpdateSuspect((SessionTable[it->first].session).at(p));
						}

						// Allow for continuous classification
						if (!globalConfig->getClassificationTimeout())
							LTMSuspectLoop(NULL);

						pthread_rwlock_unlock(&LTMsessionLock);
						pthread_rwlock_wrlock(&LTMsessionLock);
						SessionTable[it->first].session.clear();
						SessionTable[it->first].fin = false;
						pthread_rwlock_unlock(&LTMsessionLock);
						pthread_rwlock_rdlock(&LTMsessionLock);


					}
					//If this session is timed out
					else if(packetTime + globalConfig->getTcpTimout() < currentTime)
					{

						for (uint p = 0; p < (SessionTable[it->first].session).size(); p++)
						{
							(SessionTable[it->first].session).at(p).fromHaystack = FROM_LTM;
							LTMUpdateSuspect((SessionTable[it->first].session).at(p));
						}

						// Allow for continuous classification
						if (!globalConfig->getClassificationTimeout())
							LTMSuspectLoop(NULL);

						pthread_rwlock_unlock(&LTMsessionLock);
						pthread_rwlock_wrlock(&LTMsessionLock);
						SessionTable[it->first].session.clear();
						SessionTable[it->first].fin = false;
						pthread_rwlock_unlock(&LTMsessionLock);
						pthread_rwlock_rdlock(&LTMsessionLock);
					}
				}
			}
		}
		pthread_rwlock_unlock(&LTMsessionLock);
		//Check only once every TCP_CHECK_FREQ seconds
		sleep(globalConfig->getTcpCheckFreq());
	}while(!LTMusePcapFile);

	//After a pcap file is read we do one iteration of this function to clear out the sessions
	//This is return is to prevent an error being thrown when there isn't one.
	if(LTMusePcapFile) return NULL;

	//Shouldn't get here
	syslog(SYSL_ERR, "Line: %d TCP Timeout Thread has halted!", __LINE__);
	return NULL;
}


bool Nova::LTMSendToCE(Suspect *suspect)
{
	int socketFD;

	do{
		LTMdataLen = suspect->SerializeSuspect(LTMdata);
		LTMdataLen += suspect->features.unsentData->SerializeFeatureData(LTMdata+LTMdataLen);
		suspect->features.unsentData->clearFeatureData();

		if ((socketFD = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		{
			syslog(SYSL_ERR, "Line: %d Unable to create socket to ClassificationEngine: %s", __LINE__, strerror(errno));
			close(socketFD);
			return false;
		}

		if (connect(socketFD, (struct sockaddr *)&LTMremote, LTMlen) == -1)
		{
			syslog(SYSL_ERR, "Line: %d Unable to connect to ClassificationEngine: %s", __LINE__, strerror(errno));
			close(socketFD);
			return false;
		}

		if (send(socketFD, LTMdata, LTMdataLen, 0) == -1)
		{
			syslog(SYSL_ERR, "Line: %d Unable to send to ClassificationEngine: %s", __LINE__, strerror(errno));
			close(socketFD);
			return false;
		}
		bzero(LTMdata,LTMdataLen);
		close(socketFD);

	}while(LTMdataLen == MORE_DATA);

    return true;
}


void Nova::LTMUpdateSuspect(Packet packet)
{
	in_addr_t addr = packet.ip_hdr.ip_src.s_addr;
	pthread_rwlock_wrlock(&LTMsuspectLock);
	//If our suspect is new
	if(suspects.find(addr) == suspects.end())
		suspects[addr] = new Suspect(packet);
	//Else our suspect exists
	else
		suspects[addr]->AddEvidence(packet);

	suspects[addr]->isLive = !LTMusePcapFile;
	pthread_rwlock_unlock(&LTMsuspectLock);
}


void *Nova::LTMSuspectLoop(void *ptr)
{
	do
	{
		sleep(globalConfig->getClassificationTimeout());
		pthread_rwlock_rdlock(&LTMsuspectLock);
		for(SuspectHashTable::iterator it = suspects.begin(); it != suspects.end(); it++)
		{
			if(it->second->needs_feature_update)
			{
				pthread_rwlock_unlock(&LTMsuspectLock);
				pthread_rwlock_wrlock(&LTMsuspectLock);
				for(uint i = 0; i < it->second->evidence.size(); i++)
				{
					it->second->features.unsentData->UpdateEvidence(it->second->evidence[i]);
				}
				it->second->evidence.clear();
				LTMSendToCE(it->second);
				it->second->needs_feature_update = false;
				pthread_rwlock_unlock(&LTMsuspectLock);
				pthread_rwlock_rdlock(&LTMsuspectLock);
			}
		}
		pthread_rwlock_unlock(&LTMsuspectLock);
	} while(!globalConfig->getReadPcap() && globalConfig->getClassificationTimeout());

	//After a pcap file is read we do one iteration of this function to clear out the sessions
	//This is return is to prevent an error being thrown when there isn't one.
	// Also return if continuous classification is enabled by setting classificationTimeout to 0
	if(globalConfig->getReadPcap() || !globalConfig->getClassificationTimeout()) return NULL;

	//Shouldn't get here
	syslog(SYSL_ERR, "Line: %d LTMSuspectLoop Thread has halted!", __LINE__);
	return NULL;
}

void Nova::LTMLoadConfig(char* configFilePath)
{
	string line;
	string prefix;
	uint i = 0;

	string settingsPath = userHomePath +"/settings";
	ifstream settings(settingsPath.c_str());

	openlog("LocalTrafficMonitor", OPEN_SYSL, LOG_AUTHPRIV);

	syslog(SYSL_INFO, "Line: %d Starting to load configuration file", __LINE__);

	if(settings.is_open())
	{
		while(settings.good())
		{
			getline(settings, line);
			i++;

			prefix = "key";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				//TODO Key should be 256 characters, hard check for this once implemented
				if((line.size() > 0) && (line.size() < 257))
					LTMkey = line;
				else
					syslog(SYSL_ERR, "Line: %d Invalid Key parsed on line %d of the settings file.", __LINE__, i);
			}
		}
		settings.close();
	}
	else
	{
		syslog(SYSL_ERR, "Line %d ERROR: Unable to open settings file", __LINE__);
	}


	closelog();
}


void Nova::LTMKnockRequest(Packet packet, u_char * payload)
{
	stringstream ss;
	string commandLine;

	uint len = LTMkey.size() + 4;
	CryptBuffer(payload, len, DECRYPT);
	string sentKey = (char*)payload;

	sentKey = sentKey.substr(0,LTMkey.size());
	if(!sentKey.compare(LTMkey))
	{
		sentKey = (char*)(payload+LTMkey.size());
		if(!sentKey.compare("OPEN"))
		{
			ss << "iptables -I INPUT -s " << string(inet_ntoa(packet.ip_hdr.ip_src)) << " -p tcp --dport " << globalConfig->getSaPort() << " -j ACCEPT";
			commandLine = ss.str();
			if(system(commandLine.c_str()) == -1)
				logger->Logging("LocalTrafficMonitor", INFO, "Failed to open port with port knocking.", "Failed to open port with port knocking.");

		}
		else if(!sentKey.compare("SHUT"))
		{
			ss << "iptables -D INPUT -s " << string(inet_ntoa(packet.ip_hdr.ip_src)) << " -p tcp --dport " << globalConfig->getSaPort() << " -j ACCEPT";
			commandLine = ss.str();
			if(system(commandLine.c_str()) == -1)
				logger->Logging("LocalTrafficMonitor", INFO, "Failed to shut port after knock request.", "Failed to shut port after knock request.");
		}
	}
}
