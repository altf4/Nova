//============================================================================
// Name        : DoppelgangerModule.h
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

/*#include "HashMapStructs.h"

using namespace std;

//Hash table for keeping track of suspects
//	the bool represents if the suspect is hostile or not
typedef google::dense_hash_map<in_addr_t, bool, tr1::hash<in_addr_t>, eqaddr > DMSuspectHashTable;

namespace Nova{

//The main thread for the Doppelganger Module
// ptr - Unused pointer required by pthread
// returns - Does not return (main loop)
void *DoppelgangerModuleMain(void *ptr);

// Listens over IPC for a Silent Alarm, blocking on no answer
void DMReceiveAlarm();

// Startup routine for pthread that listens for GUI comamnds
//		ptr - Requires for pthread startup routines
void *DM_GUILoop(void *ptr);

// Receives input commands from the GUI
// Note: This is a blocking function. If nothing is received, then wait on this thread for an answer
void DMReceiveGUICommand();

// Returns tips for command line usage
string DMUsage();

// Loads configuration variables
//		configFilePath - Location of configuration file
void DMLoadConfig(char* configFilePath);

}*/

