//============================================================================
// Name        : DoppelgangerModule.h
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Nova utility for disguising a real system
//============================================================================

#include "Suspect.h"
#include <ANN/ANN.h>

using std::string;

///	Filename of the file to be used as an IPC key
#define KEY_ALARM_FILENAME "/etc/NovaDoppIPCKey"
///The maximum message, as defined in /proc/sys/kernel/msgmax
#define MAX_MSG_SIZE 65535
//Number of messages to queue in a listening socket before ignoring requests until the queue is open
#define SOCKET_QUEUE_SIZE 50

namespace Nova{
namespace DoppelgangerModule{

using namespace ClassificationEngine;

///Returns a string representation of the specified device's IP address
string getLocalIP(const char *dev);

///Listens over IPC for a Silent Alarm, blocking on no answer
Suspect *ReceiveAlarm(int alarmSock);

//Returns usage tips
string Usage();

//Loads configuration variables from NOVAConfig_DM.txt or specified config file
void LoadConfig(char* input);

}
}
