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

#include <errno.h>
#include <arpa/inet.h>
#include <fstream>
#include "LocalTrafficMonitor.h"
#include "NOVAConfiguration.h"
#include <net/if.h>
#include <sys/un.h>

using namespace std;
using namespace Nova;
using namespace LocalTrafficMonitor;

static TCPSessionHashTable SessionTable;
static SuspectHashTable	suspects;

pthread_rwlock_t sessionLock;
pthread_rwlock_t suspectLock;

string dev; //Interface name, read from config file
string pcapPath; //Pcap file to read from instead of live packet capture.
bool usePcapFile; //Specify if reading from PCAP file or capturing live, true uses file
bool goToLiveCap; //Specify if go to live capture mode after reading from a pcap file
int tcpTime; //TCP_TIMEOUT Measured in seconds
int tcpFreq; //TCP_CHECK_FREQ Measured in seconds
uint classificationTimeout; //Time between checking suspects for updated data

//Global memory assignments to improve packet handler performance
int len, dest_port;
struct ether_header *ethernet;  	/* net/ethernet.h */
struct ip *ip_hdr; 					/* The IP header */
Packet packet_info;
char tcp_socket[55];
struct sockaddr_un remote;

u_char data[MAX_MSG_SIZE];
uint dataLen;

char * pathsFile = (char*)PATHS_FILE;
string homePath;
bool useTerminals;

string key;
in_port_t sAlarmPort;

int main(int argc, char *argv[])
{
	pthread_rwlock_init(&sessionLock, NULL);
	pthread_rwlock_init(&suspectLock, NULL);
	pthread_t TCP_timeout_thread;
	pthread_t GUIListenThread;
	pthread_t SuspectUpdateThread;

	SessionTable.set_empty_key("");
	SessionTable.resize(INIT_SIZE_HUGE);
	suspects.set_empty_key(0);
	suspects.resize(INIT_SIZE_SMALL);

	char errbuf[PCAP_ERRBUF_SIZE];
	bzero(data, MAX_MSG_SIZE);

	int ret;

	bpf_u_int32 maskp;				/* subnet mask */
	bpf_u_int32 netp; 				/* ip          */

	string hostAddress;

	string novaConfig;

	string line, prefix; //used for input checking

	//Get locations of Nova files
	homePath = GetHomePath();
	novaConfig = homePath + "/Config/NOVAConfig.txt";
	chdir(homePath.c_str());

	//Runs the configuration loader
	LoadConfig((char*)novaConfig.c_str());

	if(!useTerminals)
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
	key = homePath+key;

	remote.sun_family = AF_UNIX;
	strcpy(remote.sun_path, key.c_str());
	len = strlen(remote.sun_path) + sizeof(remote.sun_family);

	pthread_create(&GUIListenThread, NULL, GUILoop, NULL);

	struct bpf_program fp;			/* The compiled filter expression */
	char filter_exp[64];
	pcap_t *handle;

	hostAddress = GetLocalIP(dev.c_str());

	if(hostAddress.empty())
	{
		syslog(SYSL_ERR, "Line: %d Invalid interface given", __LINE__);
		exit(1);
	}

	//Form the Filter Expression String
	bzero(filter_exp, 64);
	snprintf(filter_exp, 64, "dst host %s", hostAddress.data());
	syslog(SYSL_INFO, "%s", filter_exp);

	//If we are reading from a packet capture file
	if(usePcapFile)
	{
		sleep(1); //To allow time for other processes to open
		handle = pcap_open_offline(pcapPath.c_str(), errbuf);

		if(handle == NULL)
		{
			syslog(SYSL_ERR, "Line: %d Couldn't open file: %s: %s", __LINE__, pcapPath.c_str(), errbuf);
			return(2);
		}
		if (pcap_compile(handle, &fp,  filter_exp, 0, maskp) == -1)
		{
			syslog(SYSL_ERR, "Line: %d Couldn't parse filter: %s %s", __LINE__, filter_exp, pcap_geterr(handle));
			exit(1);
		}

		if (pcap_setfilter(handle, &fp) == -1)
		{
			syslog(SYSL_ERR, "Line: %d Couldn't install filter: %s %s", __LINE__, filter_exp, pcap_geterr(handle));
			exit(1);
		}
		//First process any packets in the file then close all the sessions
		pcap_loop(handle, -1, Packet_Handler,NULL);

		TCPTimeout(NULL);
		SuspectLoop(NULL);

		if(goToLiveCap) usePcapFile = false; //If we are going to live capture set the flag.
	}
	if(!usePcapFile)
	{
		//system("setcap cap_net_raw,cap_net_admin=eip /usr/local/bin/LocalTrafficMonitor");
		//Open in non-promiscuous mode, since we only want traffic destined for the host machine
		handle = pcap_open_live(dev.c_str(), BUFSIZ, 0, 1000, errbuf);

		if(handle == NULL)
		{
			syslog(SYSL_ERR, "Line: %d Couldn't open device: %s: %s", __LINE__, dev.c_str(), errbuf);
			return(2);
		}

		/* ask pcap for the network address and mask of the device */
		ret = pcap_lookupnet(dev.c_str(), &netp, &maskp, errbuf);

		if(ret == -1)
		{
			syslog(SYSL_ERR, "Line: %d %s", __LINE__, errbuf);
			exit(1);
		}

		if (pcap_compile(handle, &fp,  filter_exp, 0, maskp) == -1)
		{
			syslog(SYSL_ERR, "Line: %d Couldn't parse filter: %s %s", __LINE__, filter_exp, pcap_geterr(handle));
			exit(1);
		}

		if (pcap_setfilter(handle, &fp) == -1)
		{
			syslog(SYSL_ERR, "Line: %d Couldn't install filter: %s %s", __LINE__, filter_exp, pcap_geterr(handle));
			exit(1);
		}

		pthread_create(&TCP_timeout_thread, NULL, TCPTimeout, NULL);
		pthread_create(&SuspectUpdateThread, NULL, SuspectLoop, NULL);

		//"Main Loop"
		//Runs the function "Packet_Handler" every time a packet is received
	    pcap_loop(handle, -1, Packet_Handler, NULL);
	}
	return 0;
}


void LocalTrafficMonitor::Packet_Handler(u_char *useless,const struct pcap_pkthdr* pkthdr,const u_char* packet)
{
	if(packet == NULL)
	{
		syslog(SYSL_ERR, "Line: %d Didn't grab packet!", __LINE__);
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
		packet_info.fromHaystack = FROM_HAYSTACK_DP;

		//IF UDP or ICMP
		if(ip_hdr->ip_p == 17 )
		{
			packet_info.udp_hdr = *(struct udphdr*) ((char *)ip_hdr + sizeof(struct ip));

			if((in_port_t)ntohs(packet_info.udp_hdr.dest) ==  sAlarmPort)
			{
				//if we receive a udp packet on the silent alarm port, see if it is a port knock request
				KnockRequest(packet_info, (((u_char *)ip_hdr + sizeof(struct ip)) + sizeof(struct udphdr)));
			}
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
			if( SessionTable[tcp_socket].session.size() == 0)
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
				if(packet_info.tcp_hdr.fin)
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
		if (!classificationTimeout)
			SuspectLoop(NULL);
	}
	else if(ntohs(ethernet->ether_type) == ETHERTYPE_ARP)
	{
		return;
	}
	else
	{
		syslog(SYSL_ERR, "Line: %d Unknown Non-IP Packet Received", __LINE__);
		return;
	}
}


void *Nova::LocalTrafficMonitor::GUILoop(void *ptr)
{
	struct sockaddr_un localIPCAddress;
	int IPCsock;

	if((IPCsock = socket(AF_UNIX,SOCK_STREAM,0)) == -1)
	{
		syslog(SYSL_ERR, "Line: %d socket: %s", __LINE__, strerror(errno));
		close(IPCsock);
		exit(1);
	}

	localIPCAddress.sun_family = AF_UNIX;

	//Builds the key path
	string key = LTM_GUI_FILENAME;
	key = homePath+key;

	strcpy(localIPCAddress.sun_path, key.c_str());
	unlink(localIPCAddress.sun_path);
	// length = strlen(localIPCAddress.sun_path) + sizeof(localIPCAddress.sun_family);

	if(bind(IPCsock,(struct sockaddr *)&localIPCAddress,len) == -1)
	{
		syslog(SYSL_ERR, "Line: %d bind: %s", __LINE__, strerror(errno));
		close(IPCsock);
		exit(1);
	}

	if(listen(IPCsock, SOCKET_QUEUE_SIZE) == -1)
	{
		syslog(SYSL_ERR, "Line: %d listen: %s", __LINE__, strerror(errno));
		close(IPCsock);
		exit(1);
	}
	while(true)
	{
		ReceiveGUICommand(IPCsock);
	}
}


void LocalTrafficMonitor::ReceiveGUICommand(int socket)
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
			pthread_rwlock_wrlock(&suspectLock);
			suspects.clear();
			pthread_rwlock_unlock(&suspectLock);
			break;
		case CLEAR_SUSPECT:
			suspectKey = inet_addr(msg.GetValue().c_str());
			pthread_rwlock_wrlock(&suspectLock);
			suspects.set_deleted_key(5);
			suspects.erase(suspectKey);
			suspects.clear_deleted_key();
			pthread_rwlock_unlock(&suspectLock);
			break;
    	case EXIT:
    		exit(1);
    	default:
    		break;
    }
    close(msgSocket);
}


void *Nova::LocalTrafficMonitor::TCPTimeout( void *ptr )
{
	do
	{
		time_t currentTime = time(NULL);
		time_t packetTime;

		pthread_rwlock_rdlock(&sessionLock);
		for ( TCPSessionHashTable::iterator it = SessionTable.begin() ; it != SessionTable.end(); it++ )
		{
			if(it->second.session.size() > 0)
			{
				packetTime = it->second.session.back().pcap_header.ts.tv_sec;
				//If were reading packets from a file, assume all packets have been loaded and go beyond timeout threshhold
				if(usePcapFile)
				{
					currentTime = packetTime+3+tcpTime;
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
							UpdateSuspect((SessionTable[it->first].session).at(p));
						}

						// Allow for continuous classification
						if (!classificationTimeout)
							SuspectLoop(NULL);

						pthread_rwlock_unlock(&sessionLock);
						pthread_rwlock_wrlock(&sessionLock);
						SessionTable[it->first].session.clear();
						SessionTable[it->first].fin = false;
						pthread_rwlock_unlock(&sessionLock);
						pthread_rwlock_rdlock(&sessionLock);


					}
					//If this session is timed out
					else if(packetTime + tcpTime < currentTime)
					{

						for (uint p = 0; p < (SessionTable[it->first].session).size(); p++)
						{
							(SessionTable[it->first].session).at(p).fromHaystack = FROM_LTM;
							UpdateSuspect((SessionTable[it->first].session).at(p));
						}

						// Allow for continuous classification
						if (!classificationTimeout)
							SuspectLoop(NULL);

						pthread_rwlock_unlock(&sessionLock);
						pthread_rwlock_wrlock(&sessionLock);
						SessionTable[it->first].session.clear();
						SessionTable[it->first].fin = false;
						pthread_rwlock_unlock(&sessionLock);
						pthread_rwlock_rdlock(&sessionLock);
					}
				}
			}
		}
		pthread_rwlock_unlock(&sessionLock);
		//Check only once every TCP_CHECK_FREQ seconds
		sleep(tcpFreq);
	}while(!usePcapFile);

	//After a pcap file is read we do one iteration of this function to clear out the sessions
	//This is return is to prevent an error being thrown when there isn't one.
	if(usePcapFile) return NULL;

	//Shouldn't get here
	syslog(SYSL_ERR, "Line: %d TCP Timeout Thread has halted!", __LINE__);
	return NULL;
}


bool LocalTrafficMonitor::SendToCE(Suspect *suspect)
{
	int socketFD;

	do{
		dataLen = suspect->SerializeSuspect(data);
		dataLen += suspect->features.SerializeFeatureDataLocal(data+dataLen);

		if ((socketFD = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		{
			syslog(SYSL_ERR, "Line: %d socket: %s", __LINE__, strerror(errno));
			close(socketFD);
			return false;
		}

		if (connect(socketFD, (struct sockaddr *)&remote, len) == -1)
		{
			syslog(SYSL_ERR, "Line: %d connect: %s", __LINE__, strerror(errno));
			close(socketFD);
			return false;
		}

		if (send(socketFD, data, dataLen, 0) == -1)
		{
			syslog(SYSL_ERR, "Line: %d send: %s", __LINE__, strerror(errno));
			close(socketFD);
			return false;
		}
		bzero(data,dataLen);
		close(socketFD);

	}while(dataLen == MORE_DATA);

    return true;
}


void LocalTrafficMonitor::UpdateSuspect(Packet packet)
{
	in_addr_t addr = packet.ip_hdr.ip_src.s_addr;
	pthread_rwlock_wrlock(&suspectLock);
	//If our suspect is new
	if(suspects.find(addr) == suspects.end())
		suspects[addr] = new Suspect(packet);
	//Else our suspect exists
	else
		suspects[addr]->AddEvidence(packet);

	suspects[addr]->isLive = !usePcapFile;
	pthread_rwlock_unlock(&suspectLock);
}


void *Nova::LocalTrafficMonitor::SuspectLoop(void *ptr)
{
	do
	{
		sleep(classificationTimeout);
		pthread_rwlock_rdlock(&suspectLock);
		for(SuspectHashTable::iterator it = suspects.begin(); it != suspects.end(); it++)
		{
			if(it->second->needs_feature_update)
			{
				pthread_rwlock_unlock(&suspectLock);
				pthread_rwlock_wrlock(&suspectLock);
				for(uint i = 0; i < it->second->evidence.size(); i++)
				{
					it->second->features.UpdateEvidence(it->second->evidence[i]);
				}
				it->second->evidence.clear();
				SendToCE(it->second);
				it->second->needs_feature_update = false;
				pthread_rwlock_unlock(&suspectLock);
				pthread_rwlock_rdlock(&suspectLock);
			}
		}
		pthread_rwlock_unlock(&suspectLock);
	} while(!usePcapFile && classificationTimeout);

	//After a pcap file is read we do one iteration of this function to clear out the sessions
	//This is return is to prevent an error being thrown when there isn't one.
	// Also return if continuous classification is enabled by setting classificationTimeout to 0
	if(usePcapFile || !classificationTimeout) return NULL;

	//Shouldn't get here
	syslog(SYSL_ERR, "Line: %d SuspectLoop Thread has halted!", __LINE__);
	return NULL;
}

void LocalTrafficMonitor::LoadConfig(char* configFilePath)
{
	string line;
	string prefix;
	uint i = 0;
	int confCheck = 0;

	string settingsPath = homePath +"/settings";
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
					key = line;
				else
					syslog(SYSL_ERR, "Line: %d Invalid Key parsed on line %d of the settings file.", __LINE__, i);
			}
		}
	}
	settings.close();



	NOVAConfiguration * NovaConfig = new NOVAConfiguration();
	NovaConfig->LoadConfig(configFilePath, homePath, __FILE__);

	confCheck = NovaConfig->SetDefaults();

	//Checks to make sure all values have been set.
	if(confCheck == 2)
	{
		syslog(SYSL_ERR, "Line: %d One or more values have not been set, and have no default.", __LINE__);
		exit(1);
	}
	else if(confCheck == 1)
	{
		syslog(SYSL_INFO, "Line: %d INFO Config loaded successfully with defaults; some variables in NOVAConfig.txt were incorrectly set, not present, or not valid!", __LINE__);
	}
	else if (confCheck == 0)
	{
		syslog(SYSL_INFO, "Line: %d INFO Config loaded successfully.", __LINE__);
	}

	closelog();

	dev = NovaConfig->options["INTERFACE"].data;
	tcpTime = atoi(NovaConfig->options["TCP_TIMEOUT"].data.c_str());
	tcpFreq = atoi(NovaConfig->options["TCP_CHECK_FREQ"].data.c_str());
	usePcapFile = atoi(NovaConfig->options["READ_PCAP"].data.c_str());
	pcapPath = NovaConfig->options["PCAP_FILE"].data;
	goToLiveCap = atoi(NovaConfig->options["GO_TO_LIVE"].data.c_str());
	useTerminals = atoi(NovaConfig->options["USE_TERMINALS"].data.c_str());
	classificationTimeout = atoi(NovaConfig->options["CLASSIFICATION_TIMEOUT"].data.c_str());
	sAlarmPort = atoi(NovaConfig->options["SILENT_ALARM_PORT"].data.c_str());
}


void LocalTrafficMonitor::KnockRequest(Packet packet, u_char * payload)
{
	stringstream ss;
	string commandLine;

	uint len = key.size() + 4;
	CryptBuffer(payload, len, DECRYPT);
	string sentKey = (char*)payload;

	sentKey = sentKey.substr(0,key.size());
	if(!sentKey.compare(key))
	{
		sentKey = (char*)(payload+key.size());
		if(!sentKey.compare("OPEN"))
		{
			ss << "iptables -I INPUT -s " << string(inet_ntoa(packet.ip_hdr.ip_src)) << " -p tcp --dport " << sAlarmPort << " -j ACCEPT";
			commandLine = ss.str();
			system(commandLine.c_str());
		}
		else if(!sentKey.compare("SHUT"))
		{
			ss << "iptables -D INPUT -s " << string(inet_ntoa(packet.ip_hdr.ip_src)) << " -p tcp --dport " << sAlarmPort << " -j ACCEPT";
			commandLine = ss.str();
			system(commandLine.c_str());
		}
	}
}
