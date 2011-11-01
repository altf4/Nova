//============================================================================
// Name        : LocalTrafficMonitor.cpp
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Monitors local network traffic and sends detailed TrafficEvents
//					to the Classification Engine.
//============================================================================/*

#include <errno.h>
#include <arpa/inet.h>
#include <TrafficEvent.h>
#include <fstream>
#include "LocalTrafficMonitor.h"
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/un.h>
#include <log4cxx/xml/domconfigurator.h>
#include <boost/archive/text_oarchive.hpp>


using namespace log4cxx;
using namespace log4cxx::xml;
using namespace std;
using namespace Nova;

int tcpTime; //TCP_TIMEOUT Measured in seconds
int tcpFreq; //TCP_CHECK_FREQ Measured in seconds
static LocalTrafficMonitor::TCPSessionHashTable SessionTable;

string dev; //Interface name, read from config file
string pcapPath; //Pcap file to read from instead of live packet capture.
bool usePcapFile; //Specify if reading from PCAP file or capturing live, true uses file
bool goToLiveCap; //Specify if go to live capture mode after reading from a pcap file

pthread_rwlock_t lock;
LoggerPtr m_logger(Logger::getLogger("main"));

/// Callback function that is passed to pcap_loop(..) and called each time
/// a packet is recieved
void Nova::LocalTrafficMonitor::Packet_Handler(u_char *useless,const struct pcap_pkthdr* pkthdr,const u_char* packet)
{
	struct ether_header *ethernet;  	/* net/ethernet.h */
		struct ip *ip_hdr; 					/* The IP header */

		TrafficEvent *event;
		struct Packet temp;

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
			struct Packet *packet_info = &temp;
			packet_info->ip_hdr = *ip_hdr;
			packet_info->pcap_header = *pkthdr;

			//IF UDP or ICMP
			if(ip_hdr->ip_p == 17 )
			{
				packet_info->udp_hdr = *(struct udphdr*) ((char *)ip_hdr + sizeof(struct ip));
				event = new Nova::TrafficEvent(*packet_info, FROM_HAYSTACK_DP);
				SendToCE(event);
				delete event;
				event = NULL;
			}
			else if(ip_hdr->ip_p == 1)
			{
				packet_info->icmp_hdr = *(struct icmphdr*) ((char *)ip_hdr + sizeof(struct ip));
				event = new Nova::TrafficEvent(*packet_info, FROM_HAYSTACK_DP);
				SendToCE(event);
				delete event;
				event = NULL;
			}
			//If TCP...
			else if(ip_hdr->ip_p == 6)
			{
				packet_info->tcp_hdr = *(struct tcphdr*)((char*)ip_hdr + sizeof(struct ip));
				int dest_port = ntohs(packet_info->tcp_hdr.dest);
				char tcp_socket[55];

				bzero(tcp_socket, 55);
				snprintf(tcp_socket, 55, "%d-%d-%d", ip_hdr->ip_dst.s_addr, ip_hdr->ip_src.s_addr, dest_port);
				pthread_rwlock_wrlock(&lock);

				//If this is a new entry...
				if( SessionTable[tcp_socket].session.size() == 0)
				{
					//Insert packet into Hash Table
					SessionTable[tcp_socket].session.push_back(*packet_info);
					SessionTable[tcp_socket].fin = false;
				}

				//If there is already a session in progress for the given LogEntry
				else
				{
					//If Session is ending
					//TODO: The session may continue a few packets after the FIN. Account for this case.
					//See ticket #15
					if(packet_info->tcp_hdr.fin)
					{
						SessionTable[tcp_socket].session.push_back(*packet_info);
						SessionTable[tcp_socket].fin = true;
					}
					else
					{
						//Add this new packet to the session vector
						SessionTable[tcp_socket].session.push_back(*packet_info);
					}
				}
				pthread_rwlock_unlock(&lock);
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

int main(int argc, char *argv[])
{
	using namespace LocalTrafficMonitor;
	pthread_rwlock_init(&lock, NULL);
	pthread_t TCP_timeout_thread;
	pthread_t GUIListenThread;

	char errbuf[PCAP_ERRBUF_SIZE];
	int ret;

	//Path name variable for config file, set to a default
	char* nConfig = (char*)"Config/NOVAConfig_LTM.txt";
	string line; //used for input checking

	bpf_u_int32 maskp;				/* subnet mask */
	bpf_u_int32 netp; 				/* ip          */

	string hostAddress;
	int c;

	while((c = getopt (argc, argv, ":n:l:")) != -1)
	{
		switch(c)
		{

			//"NOVA Config"
			case 'n':
				if(optarg != NULL)
				{
					line = string(optarg);
					if(line.size() > 4 && !line.substr(line.size()-4, line.size()).compare(".txt"))
					{
						nConfig = (char *)optarg;
					}
				}
				else
				{
					cerr << "Bad Input File Path" << endl;
					cout << Usage();
					exit(1);
				}
				break;

			//Log config file
			case 'l':
				if(optarg != NULL)
				{
					line = string(optarg);
					if(line.size() > 4 && !line.substr(line.size()-4, line.size()).compare(".xml"))
					{
						DOMConfigurator::configure(optarg);
					}
				}
				else
				{
					cerr << "Bad Output File Path" << endl;
					cout << Usage();
					exit(1);
				}
				break;

			case '?':
				cerr << "You entered an unrecognized input flag: " << (char)optopt << endl;
				cout << Usage();
				exit(1);
				break;

			case ':':
				cerr << "You're missing an argument after the flag: " << (char)optopt << endl;
				cout << Usage();
				exit(1);
				break;

			default:
			{
				cerr << "Sorry, I didn't recognize the option: " << (char)c << endl;
				cout << Usage();
				exit(1);
			}
		}
	}

	//Runs the configuration loader
	LoadConfig(nConfig);

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
		sleep(tcpFreq); //To allow time for other processes to open
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


		//"Main Loop"
		//Runs the function "Packet_Handler" every time a packet is received
	    pcap_loop(handle, -1, Packet_Handler, NULL);
	}
	return 0;
}

void *Nova::LocalTrafficMonitor::GUILoop(void *ptr)
{
	struct sockaddr_un localIPCAddress;
	int IPCsock, len;

	if((IPCsock = socket(AF_UNIX,SOCK_STREAM,0)) == -1)
	{
		LOG4CXX_ERROR(m_logger, "socket: " << strerror(errno));
		close(IPCsock);
		exit(1);
	}

	localIPCAddress.sun_family = AF_UNIX;

	//Builds the key path
	string path = getenv("HOME");
	string key = GUI_FILENAME;
	path += key;

	strcpy(localIPCAddress.sun_path, path.c_str());
	unlink(localIPCAddress.sun_path);
	len = strlen(localIPCAddress.sun_path) + sizeof(localIPCAddress.sun_family);

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
	struct sockaddr_un remote;
    int socketSize, connectionSocket;
    int bytesRead;
    char buffer[MAX_MSG_SIZE];

    socketSize = sizeof(remote);

    //Blocking call
    if ((connectionSocket = accept(socket, (struct sockaddr *)&remote, (socklen_t*)&socketSize)) == -1)
    {
		LOG4CXX_ERROR(m_logger,"accept: " << strerror(errno));
		close(connectionSocket);
    }
    if((bytesRead = recv(connectionSocket, buffer, MAX_MSG_SIZE, 0 )) == -1)
    {
		LOG4CXX_ERROR(m_logger,"recv: " << strerror(errno));
		close(connectionSocket);
    }

    string line = string(buffer);

    if(!line.compare("EXIT"))
    {
    	exit(1);
    }
	close(connectionSocket);
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

		pthread_rwlock_rdlock(&lock);
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
						TrafficEvent *event = new TrafficEvent( &(SessionTable[it->first].session), FROM_LTM);
						SendToCE(event);

						pthread_rwlock_unlock(&lock);
						pthread_rwlock_wrlock(&lock);
						SessionTable[it->first].session.clear();
						SessionTable[it->first].fin = false;
						pthread_rwlock_unlock(&lock);
						pthread_rwlock_rdlock(&lock);

						delete event;
						event = NULL;
					}
					//If this session is timed out
					else if(packetTime + tcpTime < currentTime)
					{

						TrafficEvent *event = new TrafficEvent( &(SessionTable[it->first].session), FROM_LTM);
						SendToCE(event);

						pthread_rwlock_unlock(&lock);
						pthread_rwlock_wrlock(&lock);
						SessionTable[it->first].session.clear();
						SessionTable[it->first].fin = false;
						pthread_rwlock_unlock(&lock);
						pthread_rwlock_rdlock(&lock);

						delete event;
						event = NULL;
					}
				}
			}
		}
		pthread_rwlock_unlock(&lock);
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
bool Nova::LocalTrafficMonitor::SendToCE( TrafficEvent *event )
{
	stringstream ss;
	boost::archive::text_oarchive oa(ss);

	//Builds the key path
	string path = getenv("HOME");
	string key = KEY_FILENAME;
	path += key;

	int socketFD, len;
	struct sockaddr_un remote;

	//Serialize the data into a simple char buffer
	oa << event;
	string temp = ss.str();

	const char* data = temp.c_str();
	int dataLen = temp.size();

	if((socketFD = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
    	LOG4CXX_ERROR(m_logger, "socket: " << strerror(errno));
		close(socketFD);
		return false;
	}

	remote.sun_family = AF_UNIX;
	strcpy(remote.sun_path, path.c_str());
	len = strlen(remote.sun_path) + sizeof(remote.sun_family);

	if(connect(socketFD, (struct sockaddr *)&remote, len) == -1)
	{
    	LOG4CXX_ERROR(m_logger, "connect: " << strerror(errno));
		close(socketFD);
		return false;
	}
	if(send(socketFD, data, dataLen, 0) == -1)
	{
    	LOG4CXX_ERROR(m_logger, "send: " << strerror(errno));
		close(socketFD);
		return false;
	}
	close(socketFD);
    return true;
}

///Returns a string representation of the specified device's IP address
string Nova::LocalTrafficMonitor::getLocalIP(const char *dev)
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

///Returns usage tips
string Nova::LocalTrafficMonitor::Usage()
{
	string usage_tips = "Local Traffic Monitor Module\n";
	usage_tips += "\tUsage:Local Traffic Monitor Module -l <log config file> -n <NOVA config file> \n";
	usage_tips += "\t-l: Path to LOG4CXX config xml file.\n";
	usage_tips += "\t-n: Path to NOVA config txt file. (Config/NOVAConfig_LTM.txt by default)\n";
	return usage_tips;
}

void Nova::LocalTrafficMonitor::LoadConfig(char* input)
{
	//Used to verify all values have been loaded
	bool verify[CONFIG_FILE_LINE_COUNT];
	for(uint i = 0; i < CONFIG_FILE_LINE_COUNT; i++)
		verify[i] = false;

	string line;
	string prefix;
	ifstream config(input);

	if(config.is_open())
	{
		while(config.good())
		{
			getline(config,line);
			prefix = "INTERFACE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				if(line.size() > 0)
				{
					dev = line;
					verify[0]=true;
				}
				continue;

			}
			prefix = "TCP_TIMEOUT";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				if(atoi(line.c_str()) > 0)
				{
					tcpTime = atoi(line.c_str());
					verify[1]=true;
				}
				continue;
			}
			prefix = "TCP_CHECK_FREQ";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				if(atoi(line.c_str()) > 0)
				{
					tcpFreq = atoi(line.c_str());
					verify[2]=true;
				}
				continue;

			}
			prefix = "READ_PCAP";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				if(atoi(line.c_str()) == 0 || atoi(line.c_str()) == 1)
				{
					usePcapFile = atoi(line.c_str());
					verify[3]=true;
				}
				continue;
			}
			prefix = "PCAP_FILE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				if(line.size() > 0)
				{
					pcapPath = line;
					verify[4]=true;
				}
				continue;
			}
			prefix = "GO_TO_LIVE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				if(line.size() > 0)
				{
					goToLiveCap = atoi(line.c_str());
					verify[5]=true;
				}
				continue;
			}
		}

		//Checks to make sure all values have been set.
		bool v = true;
		for(uint i = 0; i < CONFIG_FILE_LINE_COUNT; i++)
		{
			v &= verify[i];
		}

		if(v == false)
		{
			LOG4CXX_ERROR(m_logger,"One or more values have not been set.");
			exit(1);
		}
		else
		{
			LOG4CXX_INFO(m_logger,"Config loaded successfully.");
		}
	}
	else
	{
		LOG4CXX_INFO(m_logger, "No configuration file detected." );
		exit(1);
	}
	config.close();
}
