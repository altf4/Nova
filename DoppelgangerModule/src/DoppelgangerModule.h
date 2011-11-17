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
#define KEY_ALARM_FILENAME "/keys/NovaDoppIPCKey"
/// File name of the file to be used as GUI Input IPC key.
#define GUI_FILENAME "/keys/GUI_DMKey"
///The maximum message, as defined in /proc/sys/kernel/msgmax
#define MAX_MSG_SIZE 65535
//Number of messages to queue in a listening socket before ignoring requests until the queue is open
#define SOCKET_QUEUE_SIZE 50
//Number of lines read in the NOVAConfig file
#define CONFIG_FILE_LINE_COUNT 5

//Hash table for keeping track of suspects
//	the bool represents if the suspect is hostile or not
typedef std::tr1::unordered_map<in_addr_t, bool> SuspectHashTable;

namespace Nova{
namespace DoppelgangerModule{

using namespace ClassificationEngine;

///Returns a string representation of the specified device's IP address
string getLocalIP(const char *dev);

///Listens over IPC for a Silent Alarm, blocking on no answer
void ReceiveAlarm();

/// Thread for listening for GUI commands
void *GUILoop(void *ptr);

/// Receives input commands from the GUI
void ReceiveGUICommand();

//Returns usage tips
string Usage();

//Loads configuration variables from NOVAConfig_DM.txt or specified config file
void LoadConfig(char* input);

}
}
