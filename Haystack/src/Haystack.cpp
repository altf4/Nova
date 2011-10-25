//============================================================================
// Name        : Haystack.cpp
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Nova utility for transforming Honeyd log files into
//					TrafficEvents usable by Nova's Classification Engine.
//============================================================================

#include "Haystack.h"
#include <errno.h>
#include <fstream>
#include <sys/un.h>
#include <log4cxx/xml/domconfigurator.h>
#include <boost/archive/text_oarchive.hpp>

using namespace log4cxx;
using namespace log4cxx::xml;
using namespace std;
using namespace Nova;
using namespace Haystack;

int tcpTime; //TCP_TIMEOUT measured in seconds
int tcpFreq; //TCP_CHECK_FREQ measured in seconds
static TCPSessionHashTable SessionTable;

pthread_rwlock_t lock;

LoggerPtr m_logger(Logger::getLogger("main"));

//These variables used to be in the main function, changed to global to allow LoadConfig to set them
string dev; //Interface name, read from config file
string honeydConfigPath;
string pcapPath; //Pcap file to read from instead of live packet capture.
bool usePcapFile; //Specify if reading from PCAP file or capturing live, true uses file

/// Callback function that is passed to pcap_loop(..) and called each time
/// a packet is recieved
void Nova::Haystack::Packet_Handler(u_char *useless,const struct pcap_pkthdr* pkthdr,const u_char* packet)
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
	pthread_rwlock_init(&lock, NULL);
	pthread_t TCP_timeout_thread;
	char errbuf[PCAP_ERRBUF_SIZE];

	int ret;
	bpf_u_int32 maskp;				/* subnet mask */
	bpf_u_int32 netp; 				/* ip          */
	int c;

	//Path name variable for config file, set to a default
	char* nConfig = (char*)"Config/NOVAConfig_HS.txt";
	string line; //used for input checking

	vector <string> haystackAddresses;
	string haystackAddresses_csv = "";

	while ((c = getopt (argc, argv, ":n:l:")) != -1)
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

	struct bpf_program fp;			/* The compiled filter expression */
	char filter_exp[64];
	pcap_t *handle;
	pcap_t *phandle;

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

	haystackAddresses = GetHaystackAddresses(honeydConfigPath);

	if(haystackAddresses.empty())
	{
		LOG4CXX_ERROR(m_logger, "Invalid interface given");
		exit(1);
	}

	//Flatten out the vector into a csv string
	for(uint i = 0; i < haystackAddresses.size(); i++)
	{
		haystackAddresses_csv += "dst host ";
		haystackAddresses_csv += haystackAddresses[i];

		if(i+1 != haystackAddresses.size())
		{
			haystackAddresses_csv += " || ";
		}
	}

	LOG4CXX_INFO(m_logger, haystackAddresses_csv);

	if (pcap_compile(handle, &fp, haystackAddresses_csv.data(), 0, maskp) == -1)
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

	//If we're reading from a packet capture file
	if(usePcapFile)
	{
		phandle = pcap_open_offline(pcapPath.c_str(), errbuf);

		if(phandle == NULL)
		{
			LOG4CXX_ERROR(m_logger, "Couldn't open file: " << pcapPath << ": " << errbuf);
			return(2);
		}
		if (pcap_compile(phandle, &fp, haystackAddresses_csv.data(), 0, maskp) == -1)
		{
			LOG4CXX_ERROR(m_logger, "Couldn't parse filter: " << filter_exp << " " << pcap_geterr(handle));
			exit(1);
		}

		if (pcap_setfilter(phandle, &fp) == -1)
		{
			LOG4CXX_ERROR(m_logger, "Couldn't install filter: " << filter_exp << " " << pcap_geterr(handle));
			exit(1);
		}
	}

	//First process any packets in the file
	pcap_loop(phandle, -1, Packet_Handler,NULL);
	//Set the boolean to live capture mode so it won't interfere with live capture
	usePcapFile = false;

	//"Main Loop"
	//Runs the function "Packet_Handler" every time a packet is received
    pcap_loop(handle, -1, Packet_Handler, NULL);
	return 0;
}

/// Thread for periodically checking for TCP timeout.
///	IE: Not all TCP sessions get torn down properly. Sometimes they just end midstram
///	This thread looks for old tcp sessions and declares them terminated
void *Nova::Haystack::TCPTimeout(void *ptr)
{
	while(1)
	{
		//Check only once every TCP_CHECK_FREQ seconds
		sleep(tcpFreq);
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
						TrafficEvent *event = new TrafficEvent( &(SessionTable[it->first].session), FROM_HAYSTACK_DP);
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
						TrafficEvent *event = new TrafficEvent( &(SessionTable[it->first].session), FROM_HAYSTACK_DP);
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
	}
	//Shouldn't get here
	LOG4CXX_ERROR(m_logger, "TCP Timeout Thread has halted");
	return NULL;
}

//Sends the given TrafficEvent to the Classification Engine
//	Returns success or failure
bool Nova::Haystack::SendToCE(TrafficEvent *event)
{
	stringstream ss;
	boost::archive::text_oarchive oa(ss);

	int socketFD, len;
	struct sockaddr_un remote;

	//Serialize the data into a simple char buffer
	oa << event;
	string temp = ss.str();

	const char* data = temp.c_str();
	int dataLen = temp.size();

	if ((socketFD = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
    	LOG4CXX_ERROR(m_logger,"socket: " << strerror(errno));
		close(socketFD);
		return false;
	}

	remote.sun_family = AF_UNIX;
	strcpy(remote.sun_path, KEY_FILENAME);
	len = strlen(remote.sun_path) + sizeof(remote.sun_family);

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
	close(socketFD);
    return true;
}

//Prints usage tips
string Nova::Haystack::Usage()
{
	string usage_tips = "Nova Haystack Module\n";
	usage_tips += "\tUsage:Haystack Module -l <log config file> -n <NOVA config file> \n";
	usage_tips += "\t-l: Path to LOG4CXX config xml file.\n";
	usage_tips += "\t-n: Path to NOVA config txt file. (Config/NOVAConfig_HS.txt by default)\n";

	return usage_tips;
}

//Parse through the honeyd config file and get the list of IP addresses used.
vector <string> Nova::Haystack::GetHaystackAddresses(string honeyDConfigPath)
{
	//Path to the main log file
	ifstream honeydConfFile (honeyDConfigPath.c_str());
	vector <string> retAddresses;

	if( honeydConfFile == NULL)
	{
		LOG4CXX_ERROR(m_logger, "Error opening log file. Does it exist?");
		exit(1);
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
void Haystack::LoadConfig(char* input)
{
	//Used to verify all values have been loaded
	bool verify[6];
	for(uint i = 0; i < 6; i++)
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
			prefix = "HONEYD_CONFIG";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				if(line.size() > 0)
				{
					honeydConfigPath = line;
					verify[1]=true;
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
					verify[2]=true;
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
					verify[3]=true;
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
					verify[4]=true;
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
					verify[5]=true;
				}
				continue;
			}
			prefix = "#";
			if(line.substr(0,prefix.size()).compare(prefix) && line.compare(""))
			{
				LOG4CXX_INFO(m_logger,"Unexpected entry in NOVA configuration file");
				continue;
			}
		}

		//Checks to make sure all values have been set.
		bool v = true;
		for(uint i = 0; i < 6; i++)
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
		LOG4CXX_INFO(m_logger, "No configuration file detected.");
		exit(1);
	}
}
