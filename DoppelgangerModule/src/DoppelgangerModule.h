//============================================================================
// Name        : DoppelgangerModule.h
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Nova utility for disguising a real system
//============================================================================

#include <NovaUtil.h>
#include <Suspect.h>

using namespace std;

//Hash table for keeping track of suspects
//	the bool represents if the suspect is hostile or not
typedef google::dense_hash_map<in_addr_t, bool, tr1::hash<in_addr_t>, eqaddr > SuspectHashTable;

namespace Nova{
namespace DoppelgangerModule{

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
