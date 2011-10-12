//============================================================================
// Name        : DoppelgangerModule.cpp
// Author      : DataSoft Corporation
// Copyright   :
// Description : 	Nova utility for disguising a real system
//============================================================================

#include "DoppelgangerModule.h"
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <errno.h>
#include <signal.h>
#include <log4cxx/xml/domconfigurator.h>
#include <fstream>
#include <boost/archive/text_iarchive.hpp>

using namespace log4cxx;
using namespace log4cxx::xml;
using namespace std;
using namespace Nova;
using namespace DoppelgangerModule;

LoggerPtr m_logger(Logger::getLogger("main"));

//These variables used to be in the main function, changed to global to allow LoadConfig to set them
string hostAddrString, doppelgangerAddrString;
struct sockaddr_in hostAddr, loopbackAddr;
bool isEnabled;

//Called when process receives a SIGINT, like if you press ctrl+c
void siginthandler(int param)
{
	//Clear any existing DNAT routes on exit
	//	Otherwise susepcts will keep getting routed into a black hole
	system("iptables -t nat -F");
	exit(1);
}

int main(int argc, char *argv[])
{
	int c;
	char suspectAddr[INET_ADDRSTRLEN];

	signal(SIGINT, siginthandler);
	loopbackAddr.sin_addr.s_addr = INADDR_LOOPBACK;

	//Path name variable for config file, set to a default
	char* nConfig = (char*)"Config/NOVAConfig_DM.txt";

	//Hash table for keeping track of suspects
	//	the bool represents if the suspect is hostile or not
	typedef std::tr1::unordered_map<in_addr_t, bool> SuspectHashTable;
	SuspectHashTable SuspectTable;

	while ((c = getopt (argc, argv, ":n:l:")) != -1)
	{
		switch(c)
		{
			//"NOVA Config"
			case 'n':
				if(optarg != NULL)
				{
					nConfig = (char *)optarg;
				}
				else
				{
					cerr << "Bad Input File Path" << endl;
					cout << Usage();
					exit(1);
				}
				break;

			case 'l':
				if(optarg != NULL)
				{
					DOMConfigurator::configure(optarg);
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
	LoadConfig(nConfig);

	string commandLine;

	//system commands to allow DM to function.
	commandLine = "iptables -A FORWARD -i lo -j DROP";
	system(commandLine.c_str());
	commandLine = "route add -host "+doppelgangerAddrString+" dev lo";
	system(commandLine.c_str());
	commandLine = "iptables -t nat -F";
	system(commandLine.c_str());

	Suspect *suspect;

    int alarmSocket, len;
    struct sockaddr_un remote;

    if((alarmSocket = socket(AF_UNIX,SOCK_STREAM,0)) == -1)
    {
    		perror("socket");
    		close(alarmSocket);
    		exit(1);
    }
    remote.sun_family = AF_UNIX;

    strcpy(remote.sun_path, KEY_ALARM_FILENAME);
    unlink(remote.sun_path);

    len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    if(bind(alarmSocket,(struct sockaddr *)&remote,len) == -1)
    {
        perror("bind");
		close(alarmSocket);
        exit(1);
    }

    if(listen(alarmSocket, 5) == -1)
    {
        perror("listen");
		close(alarmSocket);
        exit(1);
    }

	//"Main Loop"
	while(true)
	{

		suspect  = ReceiveAlarm(alarmSocket);

		if(suspect == NULL)
		{
			continue;
		}

		//If this is from us, then ignore it!
		if((hostAddr.sin_addr.s_addr == suspect->IP_address.s_addr)
				||(hostAddr.sin_addr.s_addr == loopbackAddr.sin_addr.s_addr))
		{
			delete suspect;
			suspect = NULL;
			continue;
		}

		cout << "Alarm!\n";
		cout << suspect->ToString();
		//Keep track of what suspects we've swapped and their current state
		// Otherwise rules can accumulate in IPtables. Then when you go to delete one,
		// there will still be more left over and it won't seem to have worked.

		//Figure out if the suspect is hostile or not
		bool isBadGuy = IsSuspectHostile(suspect);

		//If the suspect already exists in our table
		if(SuspectTable.find(suspect->IP_address.s_addr) != SuspectTable.end())
		{
			//If hostility hasn't changed
			if(SuspectTable[suspect->IP_address.s_addr] == isBadGuy)
			{
				//Do nothing. This means no change has happened since last alarm
				delete suspect;
				suspect = NULL;
				continue;
			}
		}

		SuspectTable[suspect->IP_address.s_addr] = isBadGuy;
		if(isBadGuy&&isEnabled)
		{
			inet_ntop(AF_INET, &(suspect->IP_address), suspectAddr, INET_ADDRSTRLEN);

			commandLine = "iptables -t nat -A PREROUTING -d ";
			commandLine += hostAddrString;
			commandLine += " -s ";
			commandLine += suspectAddr;
			commandLine += " -j DNAT --to-destination ";
			commandLine += doppelgangerAddrString;

			system(commandLine.c_str());
		}
		else
		{
			inet_ntop(AF_INET, &(suspect->IP_address), suspectAddr, INET_ADDRSTRLEN);

			commandLine = "iptables -t nat -D PREROUTING -d ";
			commandLine += hostAddrString;
			commandLine += " -s ";
			commandLine += suspectAddr;
			commandLine += " -j DNAT --to-destination ";
			commandLine += doppelgangerAddrString;

			system(commandLine.c_str());
		}
		delete suspect;
		suspect = NULL;
	}
}

//Returns a string representation of the specified device's IP address
string Nova::DoppelgangerModule::getLocalIP(const char *dev)
{
	static struct ifreq ifreqs[20];
	struct ifconf ifconf;
	uint  nifaces, i;

	memset(&ifconf,0,sizeof(ifconf));
	ifconf.ifc_buf = (char*) (ifreqs);
	ifconf.ifc_len = sizeof(ifreqs);

	int sock, rval;
	sock = socket(AF_INET,SOCK_STREAM,0);

	if(sock < 0)
	{
		perror("socket");
		close(sock);
		return NULL;
	}

	if((rval = ioctl(sock, SIOCGIFCONF , (char*) &ifconf)) < 0 )
	{
		perror("ioctl(SIOGIFCONF)");
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
	return string("");
}

//Listens over IPC for a Silent Alarm, blocking on no answer
Suspect *Nova::DoppelgangerModule::ReceiveAlarm(int alarmSock)
{

	struct sockaddr_un remote;
    int socketSize, connectionSocket;
    int bytesRead;
    char buf[MAX_MSG_SIZE];

    socketSize = sizeof(remote);


    //Blocking call
    if ((connectionSocket = accept(alarmSock, (struct sockaddr *)&remote, (socklen_t*)&socketSize)) == -1)
    {
        perror("accept");
		close(connectionSocket);
        return NULL;
    }
    if((bytesRead = recv(connectionSocket, buf, MAX_MSG_SIZE, 0 )) == -1)
    {
        perror("recv");
		close(connectionSocket);
        return NULL;
    }

	Suspect * suspect = new Suspect();

	try
	{
		stringstream ss;
		ss << buf;
		boost::archive::text_iarchive ia(ss);
		// create and open an archive for input
		// read class state from archive
		ia >> suspect;
		// archive and stream closed when destructors are called
	}
	catch(boost::archive::archive_exception e)
	{
		LOG4CXX_ERROR(m_logger, "Error interpreting received Silent Alarm: "+string(e.what()));
		if(suspect != NULL)
		{
			delete suspect;
			suspect = NULL;
		}
	}
	close(connectionSocket);
	return suspect;
}

//Returns a string with usage tips
string Nova::DoppelgangerModule::Usage()
{
	string usage_tips = "Nova Doppelganger Module\n";
	usage_tips += "\tUsage: DoppelgangerModule -l <log config file> -n <NOVA config file>\n";
	usage_tips += "\t-l: Path to LOG4CXX config xml file.\n";
	usage_tips += "\t-n: Path to NOVA config txt file. (Config/NOVACONFIG_DM.txt by default)\n";

	return usage_tips;
}

//Returns if Suspect is hostile or not
bool Nova::DoppelgangerModule::IsSuspectHostile(Suspect *suspect)
{
	double multiplier = 1;

	if( suspect->flaggedByAlarm )
	{
		multiplier = 2;
	}

	if( (suspect->classification * multiplier) >= .5  )
	{
		return true;
	}
	return false;
}

void DoppelgangerModule::LoadConfig(char* input)
{
	//Used to verify all values have been loaded
	bool verify[3];
	for(uint i = 0; i < 3; i++)
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

				hostAddrString = getLocalIP(line.c_str());
				{
					struct in_addr *temp = NULL;

					if(inet_aton(hostAddrString.c_str(), temp) == 0)
					{
						LOG4CXX_ERROR(m_logger, "Invalid interface IP address!");
						exit(1);
					}
				}
				inet_pton(AF_INET, hostAddrString.c_str(), &(hostAddr.sin_addr));
				verify[0]=true;
				continue;
			}

			prefix = "DOPPELGANGER_IP";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				doppelgangerAddrString = line;
				{
					struct in_addr *tempr = NULL;

					if( inet_aton(doppelgangerAddrString.c_str(), tempr) == 0)
					{
						LOG4CXX_ERROR(m_logger,"Invalid doppelganger IP address!" << errno);
						exit(1);
					}
				}
				verify[1]=true;
				continue;
			}
			prefix = "ENABLED";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				if(atoi(line.c_str()) == 0 || atoi(line.c_str()) == 1)
				{
					isEnabled = atoi(line.c_str());
					verify[2]=true;
				}
				continue;
			}
			prefix = "#";
			if(line.substr(0,prefix.size()).compare(prefix))
			{
				LOG4CXX_INFO(m_logger,"Unexpected entry in NOVA configuration file" << errno);
				continue;
			}
		}

		//Checks to make sure all values have been set.
		bool v = true;
		for(uint i = 0; i < 3; i++)
		{
			v &= verify[i];
		}

		if(v == false)
		{
			LOG4CXX_ERROR(m_logger,"One or more values have not been set" << errno);
			exit(1);
		}
		else
		{
			LOG4CXX_INFO(m_logger,"Config loaded successfully" << errno);
		}
	}
	else
	{
		LOG4CXX_INFO(m_logger, "No configuration file detected!" << errno );
		exit(1);
	}
}
