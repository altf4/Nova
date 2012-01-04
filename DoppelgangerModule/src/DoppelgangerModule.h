//============================================================================
// Name        : DoppelgangerModule.h
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Nova utility for disguising a real system
//============================================================================

#include <NovaUtil.h>
#include <Suspect.h>

using namespace std;

///	Filename of the file to be used as an IPC key
#define KEY_ALARM_FILENAME "/keys/NovaDoppIPCKey"
/// File name of the file to be used as GUI Input IPC key.
#define GUI_FILENAME "/keys/GUI_DMKey"
//Number of lines read in the NOVAConfig file
#define CONFIG_FILE_LINE_COUNT 6
//Sets the Initial Table size for faster operations
#define INITIAL_TABLESIZE 256

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
