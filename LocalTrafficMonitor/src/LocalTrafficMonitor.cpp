//============================================================================
// Name        : LocalTrafficMonitor.cpp
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Monitors local network traffic and sends detailed TrafficEvents
//					to the Classification Engine.
//============================================================================/*

#include <errno.h>
#include <arpa/inet.h>
#include <fstream>
#include "LocalTrafficMonitor.h"
#include <net/if.h>
#include <sys/un.h>

using namespace log4cxx;
using namespace log4cxx::xml;
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
TrafficEvent *event, *tempEvent;
struct Packet packet_info;
char tcp_socket[55];
struct sockaddr_un remote;

u_char data[MAX_MSG_SIZE];
uint dataLen;

LoggerPtr m_logger(Logger::getLogger("main"));

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

	string novaConfig, logConfig;

	string line, prefix; //used for input checking

	//Get locations of nova files
	homePath = getHomePath();
	novaConfig = homePath + "/Config/NOVAConfig.txt";
	logConfig = homePath + "/Config/Log4cxxConfig_Console.xml";

	DOMConfigurator::configure(logConfig.c_str());

	//Runs the configuration loader
	LoadConfig((char*)novaConfig.c_str());

	if(!useTerminals)
	{
		logConfig = homePath +"/Config/Log4cxxConfig.xml";
		DOMConfigurator::configure(logConfig.c_str());
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

	hostAddress = getLocalIP(dev.c_str());

	if(hostAddress.empty())
	{
		LOG4CXX_ERROR(m_logger, "Invalid interface given");
		exit(1);
	}

	//Form the Filter Expression String
	bzero(filter_exp, 64);
	snprintf(filter_exp, 64, "dst host %s", hostAddress.data());
	LOG4CXX_INFO(m_logger, filter_exp);

	//If we are reading from a packet capture file
	if(usePcapFile)
	{
		sleep(1); //To allow time for other processes to open
		handle = pcap_open_offline(pcapPath.c_str(), errbuf);

		if(handle == NULL)
		{
			LOG4CXX_ERROR(m_logger, "Couldn't open file: " << pcapPath << ": " << errbuf);
			return(2);
		}
		if (pcap_compile(handle, &fp,  filter_exp, 0, maskp) == -1)
		{
			LOG4CXX_ERROR(m_logger, "Couldn't parse filter: " << filter_exp << " " << pcap_geterr(handle));
			exit(1);
		}

		if (pcap_setfilter(handle, &fp) == -1)
		{
			LOG4CXX_ERROR(m_logger, "Couldn't install filter: " << filter_exp << " " << pcap_geterr(handle));
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
		//Open in non-promiscuous mode, since we only want traffic destined for the host machine
		handle = pcap_open_live(dev.c_str(), BUFSIZ, 0, 1000, errbuf);

		if(handle == NULL)
		{
			LOG4CXX_ERROR(m_logger, "Couldn't open device: " << dev << ": " << errbuf);
			return(2);
		}

		/* ask pcap for the network address and mask of the device */
		ret = pcap_lookupnet(dev.c_str(), &netp, &maskp, errbuf);

		if(ret == -1)
		{
			LOG4CXX_ERROR(m_logger, errbuf);
			exit(1);
		}

		if (pcap_compile(handle, &fp,  filter_exp, 0, maskp) == -1)
		{
			LOG4CXX_ERROR(m_logger, "Couldn't parse filter: " << filter_exp << " " << pcap_geterr(handle));
			exit(1);
		}

		if (pcap_setfilter(handle, &fp) == -1)
		{
			LOG4CXX_ERROR(m_logger, "Couldn't install filter: " << filter_exp << " " << pcap_geterr(handle));
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

/// Callback function that is passed to pcap_loop(..) and called each time
/// a packet is recieved
void LocalTrafficMonitor::Packet_Handler(u_char *useless,const struct pcap_pkthdr* pkthdr,const u_char* packet)
{
	if(packet == NULL)
	{
		LOG4CXX_ERROR(m_logger, "Didn't grab packet!");
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

		//IF UDP or ICMP
		if(ip_hdr->ip_p == 17 )
		{
			packet_info.udp_hdr = *(struct udphdr*) ((char *)ip_hdr + sizeof(struct ip));
			event = new Nova::TrafficEvent(packet_info, FROM_HAYSTACK_DP);
			if(event->dst_port ==  sAlarmPort)
			{
				//if we receive a udp packet on the silent alarm port, see if it is a port knock request
				knockRequest(event, (((u_char *)ip_hdr + sizeof(struct ip)) + sizeof(struct udphdr)));
			}
			updateSuspect(event);
			delete event;
			event = NULL;
		}
		else if(ip_hdr->ip_p == 1)
		{
			packet_info.icmp_hdr = *(struct icmphdr*) ((char *)ip_hdr + sizeof(struct ip));
			event = new Nova::TrafficEvent(packet_info, FROM_HAYSTACK_DP);
			updateSuspect(event);
			delete event;
			event = NULL;
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
	}
	else if(ntohs(ethernet->ether_type) == ETHERTYPE_ARP)
	{
		return;
	}
	else
	{
		LOG4CXX_ERROR(m_logger, "Unknown Non-IP Packet Received");
		return;
	}
}

void *Nova::LocalTrafficMonitor::GUILoop(void *ptr)
{
	struct sockaddr_un localIPCAddress;
	int IPCsock, length;

	if((IPCsock = socket(AF_UNIX,SOCK_STREAM,0)) == -1)
	{
		LOG4CXX_ERROR(m_logger, "socket: " << strerror(errno));
		close(IPCsock);
		exit(1);
	}

	localIPCAddress.sun_family = AF_UNIX;

	//Builds the key path
	string key = LTM_GUI_FILENAME;
	key = homePath+key;

	strcpy(localIPCAddress.sun_path, key.c_str());
	unlink(localIPCAddress.sun_path);
	length = strlen(localIPCAddress.sun_path) + sizeof(localIPCAddress.sun_family);

	if(bind(IPCsock,(struct sockaddr *)&localIPCAddress,len) == -1)
	{
		LOG4CXX_ERROR(m_logger, "bind: " << strerror(errno));
		close(IPCsock);
		exit(1);
	}

	if(listen(IPCsock, SOCKET_QUEUE_SIZE) == -1)
	{
		LOG4CXX_ERROR(m_logger, "listen: " << strerror(errno));
		close(IPCsock);
		exit(1);
	}
	while(true)
	{
		ReceiveGUICommand(IPCsock);
	}
}

/// This is a blocking function. If nothing is received, then wait on this thread for an answer
void LocalTrafficMonitor::ReceiveGUICommand(int socket)
{
	struct sockaddr_un msgRemote;
    int socketSize, msgSocket;
    int bytesRead;
    u_char msgBuffer[MAX_GUIMSG_SIZE];
    GUIMsg msg = GUIMsg();

    socketSize = sizeof(msgRemote);
    //Blocking call
    if ((msgSocket = accept(socket, (struct sockaddr *)&msgRemote, (socklen_t*)&socketSize)) == -1)
    {
		LOG4CXX_ERROR(m_logger,"accept: " << strerror(errno));
		close(msgSocket);
    }
    if((bytesRead = recv(msgSocket, msgBuffer, MAX_GUIMSG_SIZE, 0 )) == -1)
    {
		LOG4CXX_ERROR(m_logger,"recv: " << strerror(errno));
		close(msgSocket);
    }

    msg.deserializeMessage(msgBuffer);
    switch(msg.getType())
    {
    	case EXIT:
    		exit(1);
    	default:
    		break;
    }
    close(msgSocket);
}

/// Thread for periodically checking for TCP timeout.
///	IE: Not all TCP sessions get torn down properly. Sometimes they just end midstram
///	This thread looks for old tcp sessions and declares them terminated
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
						tempEvent = new TrafficEvent( &(SessionTable[it->first].session), FROM_LTM);
						updateSuspect(tempEvent);

						pthread_rwlock_unlock(&sessionLock);
						pthread_rwlock_wrlock(&sessionLock);
						SessionTable[it->first].session.clear();
						SessionTable[it->first].fin = false;
						pthread_rwlock_unlock(&sessionLock);
						pthread_rwlock_rdlock(&sessionLock);

						delete tempEvent;
						tempEvent = NULL;
					}
					//If this session is timed out
					else if(packetTime + tcpTime < currentTime)
					{

						tempEvent = new TrafficEvent( &(SessionTable[it->first].session), FROM_LTM);
						updateSuspect(tempEvent);

						pthread_rwlock_unlock(&sessionLock);
						pthread_rwlock_wrlock(&sessionLock);
						SessionTable[it->first].session.clear();
						SessionTable[it->first].fin = false;
						pthread_rwlock_unlock(&sessionLock);
						pthread_rwlock_rdlock(&sessionLock);

						delete tempEvent;
						tempEvent = NULL;
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
	LOG4CXX_ERROR(m_logger, "TCP Timeout Thread has halted!");
	return NULL;
}

///Sends the given TrafficEvent to the Classification Engine
///	Returns success or failure
bool LocalTrafficMonitor::SendToCE(Suspect *suspect)
{
	int socketFD;

	do{
		dataLen = suspect->serializeSuspect(data);
		dataLen += suspect->features.serializeFeatureDataLocal(data+dataLen);

		if ((socketFD = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		{
			LOG4CXX_ERROR(m_logger,"socket: " << strerror(errno));
			close(socketFD);
			return false;
		}

		if (connect(socketFD, (struct sockaddr *)&remote, len) == -1)
		{
			LOG4CXX_ERROR(m_logger,"connect: " << strerror(errno));
			close(socketFD);
			return false;
		}

		if (send(socketFD, data, dataLen, 0) == -1)
		{
			LOG4CXX_ERROR(m_logger,"send: " << strerror(errno));
			close(socketFD);
			return false;
		}
		bzero(data,dataLen);
		close(socketFD);

	}while(dataLen == MORE_DATA);

    return true;
}

//Stores events to be processed before sending
void LocalTrafficMonitor::updateSuspect(TrafficEvent *event)
{
	in_addr_t addr = event->src_IP.s_addr;
	pthread_rwlock_wrlock(&suspectLock);
	//If our suspect is new
	if(suspects.find(addr) == suspects.end())
		suspects[addr] = new Suspect(event);
	//Else our suspect exists
	else
		suspects[addr]->AddEvidence(event);

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
					it->second->features.UpdateEvidence(&it->second->evidence[i]);
				}
				it->second->evidence.clear();
				SendToCE(it->second);
				it->second->needs_feature_update = false;
				pthread_rwlock_unlock(&suspectLock);
				pthread_rwlock_rdlock(&suspectLock);
			}
		}
		pthread_rwlock_unlock(&suspectLock);
	} while(!usePcapFile);

	//After a pcap file is read we do one iteration of this function to clear out the sessions
	//This is return is to prevent an error being thrown when there isn't one.
	if(usePcapFile) return NULL;

	//Shouldn't get here
	LOG4CXX_ERROR(m_logger, "SuspectLoop Thread has halted!");
	return NULL;
}

///Returns a string representation of the specified device's IP address
string LocalTrafficMonitor::getLocalIP(const char *dev)
{
	static struct ifreq ifreqs[20];
	struct ifconf ifconf;
	uint nifaces, i;

	memset(&ifconf,0,sizeof(ifconf));
	ifconf.ifc_buf = (char*) (ifreqs);
	ifconf.ifc_len = sizeof(ifreqs);

	int sock, rval;
	sock = socket(AF_INET,SOCK_STREAM,0);

	if(sock < 0)
	{
    	LOG4CXX_ERROR(m_logger, "socket: " << strerror(errno));
		close(sock);
		return (NULL);
	}

	if((rval = ioctl(sock, SIOCGIFCONF , (char*) &ifconf  )) < 0)
	{
    	LOG4CXX_ERROR(m_logger, "ioctl(SIOGIFCONF): " << strerror(errno));
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
	return NULL;
}

void LocalTrafficMonitor::LoadConfig(char* input)
{
	string line;
	string prefix;
	uint i = 0;

	string settingsPath = homePath +"/settings";
	ifstream settings(settingsPath.c_str());

	LOG4CXX_INFO(m_logger,"Starting to load configuration file");

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
					LOG4CXX_ERROR(m_logger, "Invalid Key parsed on line " << i << " of the settings file.");
			}
		}
	}
	settings.close();



	NOVAConfiguration * NovaConfig = new NOVAConfiguration();
	NovaConfig->LoadConfig(input, homePath);

	bool v = true;

	const string prefixes[] = {"INTERFACE", "TCP_TIMEOUT",
			"TCP_CHECK_FREQ", "READ_PCAP",
			"PCAP_FILE", "GO_TO_LIVE","USE_TERMINALS", "CLASSIFICATION_TIMEOUT", "SILENT_ALARM_PORT"};


	for (i = 0; i < 9; i++) {
		prefix = prefixes[i];

		NovaConfig->options[prefix];
		if (!NovaConfig->options[prefix].isValid) {
			LOG4CXX_ERROR(m_logger, i + " The configuration variable # " + prefixes[i] + " was not set in configuration file " + input);
			v = false;
		}
	}

	//Checks to make sure all values have been set.
	if(v == false)
	{
		LOG4CXX_ERROR(m_logger,"One or more values have not been set.");
		exit(1);
	}
	else
	{
		LOG4CXX_INFO(m_logger,"All configuration values appear valid.");
	}


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

//Checks the udp packet payload associated with event for a port knocking request,
// opens/closes the port for the sender depending on the payload
void LocalTrafficMonitor::knockRequest(TrafficEvent * event, u_char * payload)
{
	stringstream ss;
	string commandLine;

	uint len = key.size() + 4;
	cryptBuffer(payload, len, DECRYPT);
	string sentKey = (char*)payload;

	sentKey = sentKey.substr(0,key.size());
	if(!sentKey.compare(key))
	{
		sentKey = (char*)(payload+key.size());
		if(!sentKey.compare("OPEN"))
		{
			ss << "iptables -I INPUT -s " << string(inet_ntoa(event->src_IP)) << " -p tcp --dport " << sAlarmPort << " -j ACCEPT";
			commandLine = ss.str();
			system(commandLine.c_str());
		}
		else if(!sentKey.compare("SHUT"))
		{
			ss << "iptables -D INPUT -s " << string(inet_ntoa(event->src_IP)) << " -p tcp --dport " << sAlarmPort << " -j ACCEPT";
			commandLine = ss.str();
			system(commandLine.c_str());
		}
	}
}
