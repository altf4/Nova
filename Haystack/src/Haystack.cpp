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

#include "Haystack.h"
#include "NOVAConfiguration.h"

using namespace std;
using namespace Nova;
using namespace Haystack;

static TCPSessionHashTable SessionTable;
static SuspectHashTable	suspects;

pthread_rwlock_t sessionLock;
pthread_rwlock_t suspectLock;

string dev; //Interface name, read from config file
string honeydConfigPath;
string pcapPath; //Pcap file to read from instead of live packet capture.
bool usePcapFile; //Specify if reading from PCAP file or capturing live, true uses file
bool goToLiveCap; //Specify if go to live capture mode after reading from a pcap file
int tcpTime; //TCP_TIMEOUT measured in seconds
int tcpFreq; //TCP_CHECK_FREQ measured in seconds
uint classificationTimeout; //Time between checking suspects for updated data

//Memory assignments moved outside packet handler to increase performance
int len, dest_port;
struct sockaddr_un remote;
Packet packet_info;
struct ether_header *ethernet;  	/* net/ethernet.h */
struct ip *ip_hdr; 					/* The IP header */
char tcp_socket[55];

u_char data[MAX_MSG_SIZE];
uint dataLen;

char * pathsFile = (char*)PATHS_FILE;
string homePath;
bool useTerminals;

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

	vector <string> haystackAddresses;
	string haystackAddresses_csv = "";

	string novaConfig;

	string line, prefix; //used for input checking

	//Get locations of nova files
	homePath = GetHomePath();
	novaConfig = homePath + "/Config/NOVAConfig.txt";
	chdir(homePath.c_str());

	//Runs the configuration loader
	LoadConfig((char*)novaConfig.c_str());

	if(!useTerminals)
	{
		openlog("Haystack", NO_TERM_SYSL, LOG_AUTHPRIV);
	}

	else
	{
		openlog("Haystack", OPEN_SYSL, LOG_AUTHPRIV);
	}

	pthread_create(&GUIListenThread, NULL, GUILoop, NULL);

	struct bpf_program fp;			/* The compiled filter expression */
	char filter_exp[64];
	pcap_t *handle;



	haystackAddresses = GetHaystackAddresses(honeydConfigPath);

	if(haystackAddresses.empty())
	{
		syslog(SYSL_ERR, "Line: %d Error: No honeyd haystack nodes were found", __LINE__);
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

	syslog(SYSL_INFO, "%s", haystackAddresses_csv.c_str());

	//Preform the socket address for faster run time
	//Builds the key path
	string key = KEY_FILENAME;
	key = homePath+key;

	remote.sun_family = AF_UNIX;
	strcpy(remote.sun_path, key.c_str());
	len = strlen(remote.sun_path) + sizeof(remote.sun_family);

	//If we're reading from a packet capture file
	if(usePcapFile)
	{
		sleep(1); //To allow time for other processes to open
		handle = pcap_open_offline(pcapPath.c_str(), errbuf);

		if(handle == NULL)
		{
			syslog(SYSL_ERR, "Line: %d Couldn't open file: %s: %s", __LINE__, pcapPath.c_str(), errbuf);
			return(2);
		}
		if (pcap_compile(handle, &fp, haystackAddresses_csv.data(), 0, maskp) == -1)
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
		//Open in non-promiscuous mode, since we only want traffic destined for the host machine
		handle = pcap_open_live(dev.c_str(), BUFSIZ, 0, 1000, errbuf);

		if(handle == NULL)
		{
			syslog(SYSL_ERR, "Line: %d Couldn't open device: %s %s", __LINE__, dev.c_str(), errbuf);
			return(2);
		}

		/* ask pcap for the network address and mask of the device */
		ret = pcap_lookupnet(dev.c_str(), &netp, &maskp, errbuf);

		if(ret == -1)
		{
			syslog(SYSL_ERR, "Line: %d %s", __LINE__, errbuf);
			exit(1);
		}

		if (pcap_compile(handle, &fp, haystackAddresses_csv.data(), 0, maskp) == -1)
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


void Nova::Haystack::Packet_Handler(u_char *useless,const struct pcap_pkthdr* pkthdr,const u_char* packet)
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


void *Nova::Haystack::GUILoop(void *ptr)
{
	struct sockaddr_un localIPCAddress;
	int IPCsock, len;

	if((IPCsock = socket(AF_UNIX,SOCK_STREAM,0)) == -1)
	{
		syslog(SYSL_ERR, "Line: %d socket: %s", __LINE__, strerror(errno));
		close(IPCsock);
		exit(1);
	}

	localIPCAddress.sun_family = AF_UNIX;

	//Builds the key path
	string key = HS_GUI_FILENAME;
	key = homePath + key;

	strcpy(localIPCAddress.sun_path, key.c_str());
	unlink(localIPCAddress.sun_path);
	len = strlen(localIPCAddress.sun_path) + sizeof(localIPCAddress.sun_family);

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


void Haystack::ReceiveGUICommand(int socket)
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


void *Nova::Haystack::TCPTimeout(void *ptr)
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
						//tempEvent = new TrafficEvent( &(SessionTable[it->first].session), FROM_HAYSTACK_DP);

						for (uint p = 0; p < (SessionTable[it->first].session).size(); p++)
						{
							(SessionTable[it->first].session).at(p).fromHaystack = FROM_HAYSTACK_DP;
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

						//delete tempEvent;
						//tempEvent = NULL;
					}
					//If this session is timed out
					else if(packetTime + tcpTime < currentTime)
					{
						//tempEvent = new TrafficEvent( &(SessionTable[it->first].session), FROM_HAYSTACK_DP);
						for (uint p = 0; p < (SessionTable[it->first].session).size(); p++)
						{
							(SessionTable[it->first].session).at(p).fromHaystack = FROM_HAYSTACK_DP;
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

						//delete tempEvent;
						//tempEvent = NULL;
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
	syslog(SYSL_ERR, "Line: %d TCP Timeout Thread has halted", __LINE__);
	return NULL;
}


bool Nova::Haystack::SendToCE(Suspect *suspect)
{
	int socketFD;

	do
	{
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


void Nova::Haystack::UpdateSuspect(Packet packet)
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


void *Nova::Haystack::SuspectLoop(void *ptr)
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


vector <string> Nova::Haystack::GetHaystackAddresses(string honeyDConfigPath)
{
	//Path to the main log file
	ifstream honeydConfFile (honeyDConfigPath.c_str());
	vector <string> retAddresses;

	if( honeydConfFile == NULL)
	{
		syslog(SYSL_ERR, "Line: %d Error opening log file. Does it exist?", __LINE__);
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


void Haystack::LoadConfig(char* configFilePath)
{
	NOVAConfiguration * NovaConfig = new NOVAConfiguration();
	NovaConfig->LoadConfig(configFilePath, homePath, __FILE__);

	int confCheck = NovaConfig->SetDefaults();

	string prefix;

	openlog("Haystack", OPEN_SYSL, LOG_AUTHPRIV);

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
	honeydConfigPath = NovaConfig->options["HS_HONEYD_CONFIG"].data;
	tcpTime = atoi(NovaConfig->options["TCP_TIMEOUT"].data.c_str());
	tcpFreq = atoi(NovaConfig->options["TCP_CHECK_FREQ"].data.c_str());
	usePcapFile = atoi(NovaConfig->options["READ_PCAP"].data.c_str());
	pcapPath = NovaConfig->options["PCAP_FILE"].data;
	goToLiveCap = atoi(NovaConfig->options["GO_TO_LIVE"].data.c_str());
	useTerminals = atoi(NovaConfig->options["USE_TERMINALS"].data.c_str());
	classificationTimeout = atoi(NovaConfig->options["CLASSIFICATION_TIMEOUT"].data.c_str());
}
