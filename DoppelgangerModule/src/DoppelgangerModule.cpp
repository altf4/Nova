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
#include <sys/msg.h>
#include <errno.h>
#include <signal.h>
#include <log4cxx/xml/domconfigurator.h>
#include <fstream>

using namespace log4cxx;
using namespace log4cxx::xml;
using namespace std;
using namespace Nova;
using namespace DoppelgangerModule;

LoggerPtr m_logger(Logger::getLogger("main"));

//These variables used to be in the main function, changed to global to allow LoadConfig to set them
string hostAddrString, doppelgangerAddrString;
struct sockaddr_in hostAddr, loopbackAddr;

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
	char* nConfig = (char*)"Config/NovaConfig_DM.txt";

	//Hash table for keeping track of suspects
	//	the bool represents if the suspect is hostile or not
	typedef std::tr1::unordered_map<in_addr_t, bool> SuspectHashTable;
	SuspectHashTable SuspectTable;

	while ((c = getopt (argc, argv, ":i:d:l:")) != -1)
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
	Suspect *suspect;
	//"Main Loop"
	while(true)
	{
		string commandLine;
		suspect  = ReceiveAlarm();

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
		if(isBadGuy)
		{
			inet_ntop(AF_INET, &(suspect->IP_address), suspectAddr, INET_ADDRSTRLEN);

			commandLine += "iptables -t nat -A PREROUTING -d ";
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

			commandLine += "iptables -t nat -D PREROUTING -d ";
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
	int  nifaces, i;

	memset(&ifconf,0,sizeof(ifconf));
	ifconf.ifc_buf = (char*) (ifreqs);
	ifconf.ifc_len = sizeof(ifreqs);

	int sock, rval;
	sock = socket(AF_INET,SOCK_STREAM,0);

	if(sock < 0)
	{
		perror("socket");
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
Suspect *Nova::DoppelgangerModule::ReceiveAlarm()
{
	// ugly struct hack required by msgrcv
	struct RawMessage
	{
		long mtype;
		char mdata[MAX_MSG_SIZE];
	};

	struct RawMessage msg;
	ssize_t bytes_read;
	key_t key;

	int msqid;
	size_t data_offset = sizeof(msg.mtype);

	// Allocate a buffer of the correct size for message
	//vector<char> msgbuf(MAX_MSG_SIZE + data_offset);
	key = ftok(KEY_ALARM_FILENAME, 'c');

	if(key == -1)
	{
		LOG4CXX_ERROR(m_logger,"Obtaining an IPC key returned error!");
		exit(1);

	}

	msqid = msgget(key, 0666 | IPC_CREAT);

	if(msqid == -1)
	{
		LOG4CXX_ERROR(m_logger,"Obtaining an IPC message Queue ID returned error!");
		exit(1);
	}

	// Read raw message
	if((bytes_read = msgrcv(msqid,&msg,MAX_MSG_SIZE - data_offset, 0, 0)) < 0)
	{
		LOG4CXX_ERROR(m_logger,"Error in IPC Queue" << errno);
		perror("msgsnd");
	}

	msg.mdata[bytes_read] = 0;
	Suspect *suspect = new Suspect();

	try
	{
		// create and open an archive for input
		stringstream ss;
		ss << msg.mdata;
		boost::archive::text_iarchive ia(ss);

		// read class state from archive
		ia >> suspect;
		// archive and stream closed when destructors are called
	}
	catch(...)
	{
		LOG4CXX_ERROR(m_logger, "Error interpreting received Silent Alarm");
		if(suspect != NULL)
		{
			delete suspect;
			suspect = NULL;
		}
	}
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
	bool verify[2];
	for(int i = 0; i < 2; i++)
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
			prefix = "#";
			if(line.substr(0,prefix.size()).compare(prefix))
			{
				LOG4CXX_INFO(m_logger,"Unexpected entry in NOVA configuration file" << errno);
				continue;
			}
		}

		//Checks to make sure all values have been set.
		bool v = true;
		for(int i = 0; i < 2; i++)
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
