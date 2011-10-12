//============================================================================
// Name        : DoppelgangerModule.h
// Author      : DataSoft Corporation
// Copyright   :
// Description : 	Nova utility for disguising a real system
//============================================================================

#include "Suspect.h"
#include <ANN/ANN.h>

using std::string;

///	Filename of the file to be used as an IPC key
#define KEY_ALARM_FILENAME "/etc/NovaDoppIPCKey"
///The maximum message, as defined in /proc/sys/kernel/msgmax
#define MAX_MSG_SIZE 65535

namespace Nova{
namespace DoppelgangerModule{

using namespace ClassificationEngine;

///Returns a string representation of the specified device's IP address
string getLocalIP(const char *dev);

///Listens over IPC for a Silent Alarm, blocking on no answer
Suspect *ReceiveAlarm(int alarmSock);

//Returns usage tips
string Usage();

//Calculates if the current suspect should be marked hostile or not
bool IsSuspectHostile(Suspect *suspect);

//Loads configuration variables from NOVAConfig_DM.txt or specified config file
void LoadConfig(char* input);

}
}
