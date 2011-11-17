//============================================================================
// Name        : DoppelgangerModule.cpp
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Nova utility for disguising a real system
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
#include <boost/archive/binary_iarchive.hpp>

using namespace log4cxx;
using namespace log4cxx::xml;
using namespace std;
using namespace Nova;
using namespace DoppelgangerModule;

LoggerPtr m_logger(Logger::getLogger("main"));

SuspectHashTable SuspectTable;
pthread_rwlock_t lock;

//These variables used to be in the main function, changed to global to allow LoadConfig to set them
string hostAddrString, doppelgangerAddrString, honeydConfigPath;
struct sockaddr_in hostAddr, loopbackAddr;
bool isEnabled, useTerminals;

char * pathsFile = (char*)"/etc/nova/paths";
string homePath;

//Alarm IPC globals to improve performance
struct sockaddr_un remote;
struct sockaddr * remoteAddrPtr = (struct sockaddr *)&remote;
int connectionSocket;
int bytesRead;
char buf[MAX_MSG_SIZE];
Suspect * suspect = NULL;
int alarmSocket;

//GUI IPC globals to improve performance
struct sockaddr_un localIPCAddress;
struct sockaddr * localIPCAddressPtr = (struct sockaddr *)&localIPCAddress;
struct sockaddr_un GUIRemote;
struct sockaddr * GUIRemotePtr = (struct sockaddr *)&GUIRemote;
int IPCsock;
int GUISocket;
int numBytes;
char buffer[MAX_MSG_SIZE];

//constants that can be re-used
int socketSize = sizeof(remote);
socklen_t * socketSizePtr = (socklen_t*)&socketSize;

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
	char suspectAddr[INET_ADDRSTRLEN];
	pthread_t GUIListenThread;

	signal(SIGINT, siginthandler);
	loopbackAddr.sin_addr.s_addr = INADDR_LOOPBACK;
	string novaConfig, logConfig;

	string line, prefix; //used for input checking

	//Get locations of nova files
	ifstream *paths =  new ifstream(pathsFile);

	if(paths->is_open())
	{
		while(paths->good())
		{
			getline(*paths,line);

			prefix = "NOVA_HOME";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				homePath = line;
				break;
			}
		}
	}
	paths->close();
	delete paths;
	paths = NULL;

	//Resolves environment variables
	int start = 0;
	int end = 0;
	string var;

	while((start = homePath.find("$",end)) != -1)
	{
		end = homePath.find("/", start);
		//If no path after environment var
		if(end == -1)
		{

			var = homePath.substr(start+1, homePath.size());
			var = getenv(var.c_str());
			homePath = homePath.substr(0,start) + var;
		}
		else
		{
			var = homePath.substr(start+1, end-1);
			var = getenv(var.c_str());
			var = var + homePath.substr(end, homePath.size());
			if(start > 0)
			{
				homePath = homePath.substr(0,start)+var;
			}
			else
			{
				homePath = var;
			}
		}
	}

	if(homePath == "")
	{
		exit(1);
	}

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

	pthread_create(&GUIListenThread, NULL, GUILoop, NULL);
	string commandLine;

	//system commands to allow DM to function.
	commandLine = "iptables -A FORWARD -i lo -j DROP";
	system(commandLine.c_str());
	commandLine = "route add -host "+doppelgangerAddrString+" dev lo";
	system(commandLine.c_str());
	commandLine = "iptables -t nat -F";
	system(commandLine.c_str());

    int len;
    struct sockaddr_un remote;

	//Builds the key path
	string key = KEY_ALARM_FILENAME;
	key = homePath+key;

    if((alarmSocket = socket(AF_UNIX,SOCK_STREAM,0)) == -1)
    {
		LOG4CXX_ERROR(m_logger,"socket: " << strerror(errno));
		close(alarmSocket);
		exit(1);
    }
    remote.sun_family = AF_UNIX;

    strcpy(remote.sun_path, key.c_str());
    unlink(remote.sun_path);

    len = strlen(remote.sun_path) + sizeof(remote.sun_family);

    if(bind(alarmSocket,(struct sockaddr *)&remote,len) == -1)
    {
		LOG4CXX_ERROR(m_logger,"bind: " << strerror(errno));
		close(alarmSocket);
        exit(1);
    }

    if(listen(alarmSocket, SOCKET_QUEUE_SIZE) == -1)
    {
		LOG4CXX_ERROR(m_logger,"listen: " << strerror(errno));
		close(alarmSocket);
        exit(1);
    }

	//"Main Loop"
	while(true)
	{
		ReceiveAlarm();

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



		pthread_rwlock_rdlock(&lock);
		//If the suspect already exists in our table
		if(SuspectTable.find(suspect->IP_address.s_addr) != SuspectTable.end())
		{
			//If hostility hasn't changed
			if(SuspectTable[suspect->IP_address.s_addr] == suspect->isHostile)
			{
				//Do nothing. This means no change has happened since last alarm
				delete suspect;
				suspect = NULL;
				continue;
			}
		}

		pthread_rwlock_unlock(&lock);
		pthread_rwlock_wrlock(&lock);

		SuspectTable[suspect->IP_address.s_addr] = suspect->isHostile;

		pthread_rwlock_unlock(&lock);

		if(suspect->isHostile && isEnabled)
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

void *Nova::DoppelgangerModule::GUILoop(void *ptr)
{
	if((IPCsock = socket(AF_UNIX,SOCK_STREAM,0)) == -1)
	{
		LOG4CXX_ERROR(m_logger, "socket: " << strerror(errno));
		close(IPCsock);
		exit(1);
	}

	localIPCAddress.sun_family = AF_UNIX;

	//Builds the key path
	string key = homePath + GUI_FILENAME;

	strcpy(localIPCAddress.sun_path, key.c_str());
	unlink(localIPCAddress.sun_path);

	int GUILen = strlen(localIPCAddress.sun_path) + sizeof(localIPCAddress.sun_family);

	if(bind(IPCsock,localIPCAddressPtr,GUILen) == -1)
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
		ReceiveGUICommand();
	}
}

/// This is a blocking function. If nothing is received, then wait on this thread for an answer
void DoppelgangerModule::ReceiveGUICommand()
{
    string prefix, line;

    //Blocking call
    if ((connectionSocket = accept(IPCsock, GUIRemotePtr, socketSizePtr)) == -1)
    {
		LOG4CXX_ERROR(m_logger,"accept: " << strerror(errno));
		close(connectionSocket);
    }
    if((bytesRead = recv(connectionSocket, buffer, MAX_MSG_SIZE, 0)) == -1)
    {
		LOG4CXX_ERROR(m_logger,"recv: " << strerror(errno));
		close(connectionSocket);
    }

    line = string(buffer);

    prefix = "EXIT";
    if(!line.substr(0,prefix.size()).compare(prefix))
    {
    	exit(1);
    }

    prefix = "CLEAR";
    if(!line.substr(0,prefix.size()).compare(prefix))
	{
        prefix = "CLEAR ALL";
        if(!line.substr(0,prefix.size()).compare(prefix))
        {
			pthread_rwlock_wrlock(&lock);
			SuspectTable.clear();
			pthread_rwlock_unlock(&lock);
        }
        else
        {
        	//Call clear individual suspect function - removeSuspect(string suspectIP)
        }
	}
    close(connectionSocket);
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
		LOG4CXX_ERROR(m_logger,"socket: " << strerror(errno));
		close(sock);
		return NULL;
	}

	if((rval = ioctl(sock, SIOCGIFCONF , (char*) &ifconf)) < 0 )
	{
		LOG4CXX_ERROR(m_logger,"ioctl(SIOGIFCONF): " << strerror(errno));
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
void Nova::DoppelgangerModule::ReceiveAlarm()
{
    //Blocking call
    if ((connectionSocket = accept(alarmSocket, remoteAddrPtr, socketSizePtr)) == -1)
    {
		LOG4CXX_ERROR(m_logger,"accept: " << strerror(errno));
		close(connectionSocket);
        return;
    }
    if((bytesRead = recv(connectionSocket, buf, MAX_MSG_SIZE, 0 )) == -1)
    {
    	LOG4CXX_ERROR(m_logger,"recv: " << strerror(errno));
		close(connectionSocket);
        return;
    }

	suspect = new Suspect();

	try
	{
		stringbuf ss;
		ss.sputn(buf, bytesRead);
		boost::archive::binary_iarchive ia(ss);
		// create and open an archive for input
		// read class state from archive
		ia >> suspect;
		// archive and stream closed when destructors are called
	}
	catch(boost::archive::archive_exception e)
	{
		LOG4CXX_ERROR(m_logger, "Error interpreting received Silent Alarm: " << string(e.what()));
		if(suspect != NULL)
		{
			delete suspect;
			suspect = NULL;
		}
	}
	close(connectionSocket);
	return;
}

//Returns a string with usage tips
string Nova::DoppelgangerModule::Usage()
{
	string usage_tips = "Nova Doppelganger Module\n";
	usage_tips += "\tUsage: DoppelgangerModule -l <log config file> -n <NOVA config file>\n";
	usage_tips += "\t-l: Path to LOG4CXX config xml file.\n";
	usage_tips += "\t-n: Path to NOVA config txt file.\n";

	return usage_tips;
}


void DoppelgangerModule::LoadConfig(char* input)
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

			prefix = "DM_HONEYD_CONFIG";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				if(line.size() > 0)
				{
					honeydConfigPath = homePath+"/"+line;
					verify[1]=true;
				}
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
						LOG4CXX_ERROR(m_logger,"Invalid doppelganger IP address!");
						exit(1);
					}
				}
				verify[2]=true;
				continue;
			}
			prefix = "DM_ENABLED";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				if(atoi(line.c_str()) == 0 || atoi(line.c_str()) == 1)
				{
					isEnabled = atoi(line.c_str());
					verify[3]=true;
				}
				continue;
			}
			prefix = "USE_TERMINALS";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				if(atoi(line.c_str()) == 0 || atoi(line.c_str()) == 1)
				{
					useTerminals = atoi(line.c_str());
					verify[4]=true;
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
		LOG4CXX_INFO(m_logger, "No configuration file detected!" );
		exit(1);
	}
}
