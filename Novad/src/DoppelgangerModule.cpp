//============================================================================
// Name        : DoppelgangerModule.cpp
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
// Description : Nova utility for disguising a real system
//============================================================================

#include "DoppelgangerModule.h"
#include "NOVAConfiguration.h"
#include "NovaUtil.h"
#include "Suspect.h"
#include "Logger.h"

#include <errno.h>
#include <signal.h>
#include <sys/un.h>
#include <syslog.h>

using namespace std;
using namespace Nova;

DMSuspectHashTable DMSuspectTable;
pthread_rwlock_t DMlock;

//These variables used to be in the main function, changed to global to allow LoadConfig to set them
string hostAddrString, doppelgangerAddrString, DMhoneydConfigPath;
struct sockaddr_in hostAddr, loopbackAddr;
bool DMisEnabled, DMuseTerminals;

char * DMpathsFile = (char*)PATHS_FILE;
extern string userHomePath;
extern string novaConfigPath;

//Alarm IPC globals to improve performance
struct sockaddr_un DMremote;
struct sockaddr * DMremoteAddrPtr = (struct sockaddr *)&DMremote;
int DMconnectionSocket;
int DMbytesRead;
u_char DMbuf[MAX_MSG_SIZE];
Suspect * DMsuspect = NULL;
int DMalarmSocket;

//GUI IPC globals to improve performance
struct sockaddr_un localIPCAddress;
struct sockaddr *localIPCAddressPtr = (struct sockaddr *)&localIPCAddress;
int DM_IPCsock;

//constants that can be re-used
int DMsocketSize = sizeof(DMremote);
socklen_t *DMsocketSizePtr = (socklen_t*)&DMsocketSize;

in_port_t DMsAlarmPort;

extern Logger *logger;

//Called when process receives a SIGINT, like if you press ctrl+c
void siginthandler(int param)
{
	//Clear any existing DNAT routes on exit
	//	Otherwise susepcts will keep getting routed into a black hole
	system("sudo iptables -F");
	exit(EXIT_SUCCESS);
}

void *Nova::DoppelgangerModuleMain(void *ptr)
{
	char suspectAddr[INET_ADDRSTRLEN];
	bzero(DMbuf, MAX_MSG_SIZE);
	pthread_t GUIListenThread;

	DMSuspectTable.set_empty_key(0);
	DMSuspectTable.resize(INIT_SIZE_SMALL);

	signal(SIGINT, siginthandler);
	loopbackAddr.sin_addr.s_addr = INADDR_LOOPBACK;

	chdir(userHomePath.c_str());

	//Runs the configuration loaders
	DMLoadConfig((char*)novaConfigPath.c_str());

	if(!DMuseTerminals)
	{
		//DOMConfigurator::configure(logConfig.c_str());
		//openlog opens a stream to syslog for logging. The parameters are as follows:
		//1st: identity; a string representing where the call is coming from.
		//2nd: options; OR all of the flags you want together to generate the correct argument
		//     LOG_CONS    = write to console IF there is an error writing to syslog
		//     LOG_PID     = Log PID with other information
		//     LOG_NDELAY  = Open connection immediately
		//     LOG_PERROR  = Print to stderror as well
		//3rd: facility; essentially what facility you want to log to. In our case Local0, other examples
		//     are AUTH or CRON, etc.
		openlog("DoppelgangerModule", NO_TERM_SYSL, LOG_AUTHPRIV);
	}

	else
	{
		openlog("DoppelgangerModule", OPEN_SYSL, LOG_AUTHPRIV);
	}

	pthread_create(&GUIListenThread, NULL, DM_GUILoop, NULL);
	string commandLine;
	//system commands to allow DM to function.
	commandLine = "sudo iptables -A FORWARD -i lo -j DROP";
	system(commandLine.c_str());

	commandLine = "sudo route add -host "+ doppelgangerAddrString+" dev lo";
	system(commandLine.c_str());

	commandLine = "sudo iptables -t nat -F";
	system(commandLine.c_str());

    int len;
    struct sockaddr_un remote;

	//Builds the key path
	string key = KEY_ALARM_FILENAME;
	key = userHomePath+key;

    if((DMalarmSocket = socket(AF_UNIX,SOCK_STREAM,0)) == -1)
    {
    	// syslog takes 2+ arguments.
    	// 1st: where and what; OR together facility and level. note that
    	//      if you elect to use openlog, facilty can be left out.
    	// 2nd: String that will be logged
    	// 3rd: arguments for formatted string, much like printf
		syslog(SYSL_ERR, "ERROR: socket: %s", strerror(errno));
		close(DMalarmSocket);
		exit(EXIT_FAILURE);
    }
    remote.sun_family = AF_UNIX;

    strcpy(remote.sun_path, key.c_str());
    unlink(remote.sun_path);

    len = strlen(remote.sun_path) + sizeof(remote.sun_family);

    if(bind(DMalarmSocket,(struct sockaddr *)&remote,len) == -1)
    {
		syslog(SYSL_ERR, "Line: %d ERROR: bind: %s", __LINE__, strerror(errno));
		close(DMalarmSocket);
        exit(EXIT_FAILURE);
    }

    if(listen(DMalarmSocket, SOCKET_QUEUE_SIZE) == -1)
    {
		syslog(SYSL_ERR, "Line: %d ERROR: listen: %s", __LINE__, strerror(errno));
		close(DMalarmSocket);
        exit(EXIT_FAILURE);
    }

	//"Main Loop"
	while(true)
	{
		DMReceiveAlarm();

		if(DMsuspect == NULL)
		{
			continue;
		}

		//If this is from us, then ignore it!
		if((hostAddr.sin_addr.s_addr == DMsuspect->IP_address.s_addr)
				||(hostAddr.sin_addr.s_addr == loopbackAddr.sin_addr.s_addr))
		{
			delete DMsuspect;
			DMsuspect = NULL;
			continue;
		}

		//cout << "Alarm!\n";
		//cout << suspect->ToString();
		//Keep track of what suspects we've swapped and their current state
		// Otherwise rules can accumulate in IPtables. Then when you go to delete one,
		// there will still be more left over and it won't seem to have worked.



		pthread_rwlock_rdlock(&DMlock);
		//If the suspect already exists in our table
		if(DMSuspectTable.find(DMsuspect->IP_address.s_addr) != DMSuspectTable.end())
		{
			//If hostility hasn't changed
			if(DMSuspectTable[DMsuspect->IP_address.s_addr] == DMsuspect->isHostile)
			{
				//Do nothing. This means no change has happened since last alarm
				delete DMsuspect;
				DMsuspect = NULL;
				pthread_rwlock_unlock(&DMlock);
				continue;
			}
		}

		pthread_rwlock_unlock(&DMlock);
		pthread_rwlock_wrlock(&DMlock);

		DMSuspectTable[DMsuspect->IP_address.s_addr] = DMsuspect->isHostile;

		pthread_rwlock_unlock(&DMlock);

		if(DMsuspect->isHostile && DMisEnabled)
		{
			inet_ntop(AF_INET, &(DMsuspect->IP_address), suspectAddr, INET_ADDRSTRLEN);

			commandLine = "sudo iptables -t nat -A PREROUTING -d ";
			commandLine += hostAddrString;
			commandLine += " -s ";
			commandLine += suspectAddr;
			commandLine += " -j DNAT --to-destination ";
			commandLine += doppelgangerAddrString;

			system(commandLine.c_str());
		}
		else
		{
			inet_ntop(AF_INET, &(DMsuspect->IP_address), suspectAddr, INET_ADDRSTRLEN);

			commandLine = "sudo iptables -t nat -D PREROUTING -d ";
			commandLine += hostAddrString;
			commandLine += " -s ";
			commandLine += suspectAddr;
			commandLine += " -j DNAT --to-destination ";
			commandLine += doppelgangerAddrString;

			system(commandLine.c_str());
		}
		delete DMsuspect;
		DMsuspect = NULL;
	}
}

void *Nova::DM_GUILoop(void *ptr)
{
	if((DM_IPCsock = socket(AF_UNIX,SOCK_STREAM,0)) == -1)
	{
		syslog(SYSL_ERR, "Line: %d ERROR: socket: %s", __LINE__, strerror(errno));
		close(DM_IPCsock);
		exit(EXIT_FAILURE);
	}

	localIPCAddress.sun_family = AF_UNIX;

	//Builds the key path
	string key = userHomePath + DM_GUI_FILENAME;

	strcpy(localIPCAddress.sun_path, key.c_str());
	unlink(localIPCAddress.sun_path);

	int GUILen = strlen(localIPCAddress.sun_path) + sizeof(localIPCAddress.sun_family);

	if(bind(DM_IPCsock,localIPCAddressPtr,GUILen) == -1)
	{
		syslog(SYSL_ERR, "Line %d ERROR: bind: %s", __LINE__, strerror(errno));
		close(DM_IPCsock);
		exit(EXIT_FAILURE);
	}

	if(listen(DM_IPCsock, SOCKET_QUEUE_SIZE) == -1)
	{
		syslog(SYSL_ERR, "Line: %d ERROR: listen: %s", __LINE__, strerror(errno));
		close(DM_IPCsock);
		exit(EXIT_FAILURE);
	}
	while(true)
	{
		DMReceiveGUICommand();
	}
}

void Nova::DMReceiveGUICommand()
{
	struct sockaddr_un msgRemote;
    int socketSize, msgSocket;
    int bytesRead;
    GUIMsg msg = GUIMsg();
    u_char msgBuffer[MAX_GUIMSG_SIZE];
    in_addr_t suspectKey = 0;

    socketSize = sizeof(msgRemote);
    //Blocking call
    if ((msgSocket = accept(DM_IPCsock, (struct sockaddr *)&msgRemote, (socklen_t*)&socketSize)) == -1)
    {
		syslog(SYSL_ERR, "Line: %d ERROR: accept: %s", __LINE__, strerror(errno));
		close(msgSocket);
    }
    if((bytesRead = recv(msgSocket, msgBuffer, MAX_GUIMSG_SIZE, 0 )) == -1)
    {
		syslog(SYSL_ERR, "Line: %d ERROR: recv: %s", __LINE__, strerror(errno));
		close(msgSocket);
    }

    msg.DeserializeMessage(msgBuffer);
    switch(msg.GetType())
    {
    	case EXIT:
    		system("sudo iptables -F");
    		exit(EXIT_SUCCESS);
    	case CLEAR_ALL:
    		pthread_rwlock_wrlock(&DMlock);
			DMSuspectTable.clear();
			pthread_rwlock_unlock(&DMlock);
			break;
    	case CLEAR_SUSPECT:
			suspectKey = inet_addr(msg.GetValue().c_str());
			pthread_rwlock_wrlock(&DMlock);
			DMSuspectTable.set_deleted_key(5);
			DMSuspectTable.erase(suspectKey);
			DMSuspectTable.clear_deleted_key();
			pthread_rwlock_unlock(&DMlock);
			break;
    	default:
    		break;
    }
    close(msgSocket);
}


void Nova::DMReceiveAlarm()
{
    //Blocking call
    if ((DMconnectionSocket = accept(DMalarmSocket, DMremoteAddrPtr, DMsocketSizePtr)) == -1)
    {
		syslog(SYSL_ERR, "Line: %d ERROR: accept: %s", __LINE__, strerror(errno));
		close(DMconnectionSocket);
        return;
    }
    if((DMbytesRead = recv(DMconnectionSocket, DMbuf, MAX_MSG_SIZE, 0 )) == -1)
    {
    	syslog(SYSL_ERR, "Line: %d ERROR: recv: %s", __LINE__, strerror(errno));
		close(DMconnectionSocket);
        return;
    }
	DMsuspect = new Suspect();
	try
	{
		DMsuspect->DeserializeSuspect(DMbuf);
		bzero(DMbuf, DMbytesRead);
	}
	catch(std::exception e)
	{
		syslog(SYSL_ERR, "Line: %d ERROR: Error interpreting received Silent Alarm: %s", __LINE__, string(e.what()).c_str());
		delete DMsuspect;
		DMsuspect = NULL;
	}
	close(DMconnectionSocket);
	return;
}


string Nova::DMUsage()
{
	string usage_tips = "Nova Doppelganger Module\n";
	usage_tips += "\tUsage: DoppelgangerModule -l <log config file> -n <NOVA config file>\n";
	usage_tips += "\t-n: Path to NOVA config txt file.\n";

	return usage_tips;
}


void Nova::DMLoadConfig(char* configFilePath)
{
	string prefix;
	int confCheck = 0;

	NOVAConfiguration * NovaConfig = new NOVAConfiguration();
	NovaConfig->LoadConfig(configFilePath, userHomePath, __FILE__);

	confCheck = NovaConfig->SetDefaults();

	openlog("DoppelgangerModule", OPEN_SYSL, LOG_AUTHPRIV);

	//Checks to make sure all values have been set.
	if(confCheck == 2)
	{
		syslog(SYSL_ERR, "Line: %d One or more values have not been set, and have no default.", __LINE__);
		exit(EXIT_FAILURE);
	}
	else if(confCheck == 1)
	{
		syslog(SYSL_INFO, "Line: %d INFO Config loaded successfully with defaults; some variables in NOVAConfig.txt were incorrectly set, not present, or not valid!", __LINE__);
	}
	else if (confCheck == 0)
	{
		syslog(SYSL_INFO, "Line: %d INFO Config loaded successfully.", __LINE__);
	}

	hostAddrString = GetLocalIP(NovaConfig->options["INTERFACE"].data.c_str());

	if(hostAddrString.size() == 0)
	{
		syslog(SYSL_ERR, "Line: %d ERROR: Bad interface, no IP's associated", __LINE__);
		exit(EXIT_FAILURE);
	}

	inet_pton(AF_INET,hostAddrString.c_str(),&(hostAddr.sin_addr));

	doppelgangerAddrString = NovaConfig->options["DOPPELGANGER_IP"].data;
	struct in_addr *tempr = NULL;

	if( inet_aton(doppelgangerAddrString.c_str(), tempr) == 0)
	{
		syslog(SYSL_ERR, "Line: %d ERROR: Invalid doppelganger IP address", __LINE__);
		exit(EXIT_FAILURE);
	}

	closelog();

	DMuseTerminals = atoi(NovaConfig->options["USE_TERMINALS"].data.c_str());
	DMsAlarmPort = atoi(NovaConfig->options["SILENT_ALARM_PORT"].data.c_str());
	DMisEnabled = atoi(NovaConfig->options["DM_ENABLED"].data.c_str());
}
