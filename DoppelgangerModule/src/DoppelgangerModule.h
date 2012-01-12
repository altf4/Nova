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

// Returns a string representation of the specified device's IP address
//		dev - Device name, ie "eth0"
string GetLocalIP(const char *dev);

// Listens over IPC for a Silent Alarm, blocking on no answer
void ReceiveAlarm();

// Startup routine for pthread that listens for GUI comamnds
//		ptr - Requires for pthread startup routines
void *GUILoop(void *ptr);

// Receives input commands from the GUI
// Note: This is a blocking function. If nothing is received, then wait on this thread for an answer
void ReceiveGUICommand();

// Returns tips for command line usage
string Usage();

// Loads configuration variables
//		configFilePath - Location of configuration file
void LoadConfig(char* configFilePath);

}
}
