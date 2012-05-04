//============================================================================
// Name        : nova_ui_core.h
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
// Description : Set of command API functions available for use as a UI implementation
//============================================================================

#ifndef COMMANDS_H_
#define COMMANDS_H_

#include <vector>
#include "string"

#include "Suspect.h"
#include "messaging/messages/RequestMessage.h"

namespace Nova
{
//************************************************************************
//**						Initialization								**
//************************************************************************

//Initializes internal objects and states. MUST be run once first, before any other functions
void InitializeUI();

//************************************************************************
//**						Process Operations							**
//************************************************************************

//Starts the Honeyd Haystack process
//	returns - True if haystack successfully started, false on error
//	NOTE: If the haystack is already running, this function does nothing and returns true
bool StartHaystack();

//Stops the Honeyd Haystack process
//	returns - True if haystack successfully stopped, false on error
//	NOTEL if the haystack is already dead, this function does nothing and returns true
bool StopHaystack();

//Returns whether the Haystack is running or not
//	returns - True if honeyd haystack is running, false if not running
bool IsHaystackUp();

//Runs the Novad process
//	returns - True upon successfully running the novad process, false on error
//	NOTE: This function will return true if Novad was already running
bool StartNovad();

//Kills the Novad process
//	returns - True upon successfully stopping the novad process, false on error
//	NOTE: This function will return true if Novad was already dead
bool StopNovad();

//Queries Novad to see if it is currently up or down
//	tryToConnect - Optional boolean to indicate whether this function should also try to connect to Novad before testing IsUp()
//		You should generally just leave this alone as true.
//	returns - True if Novad is up, false if down
bool IsNovadUp(bool tryToConnect = false);


//************************************************************************
//**						Connection Operations						**
//************************************************************************

//Initializes a connection out to Novad over IPC
//	NOTE: Must be called before any message can be sent to Novad (but after InitCallbackSocket())
//	returns - true if a successful connection is established, false if no connection (error)
//	NOTE: If a connection already exists, then the function does nothing and returns true
bool ConnectToNovad();

//Tries to connect to Novad, waiting for at most timeout_ms milliseconds
//	timeout_ms - The amount of time in milliseconds at maximum to wait for a connection
//	NOTE: Blocks for at most timeout_ms milliseconds
//	returns - true if a successful connection is established, false if no connection (error)
//	NOTE: If a connection already exists, then the function does nothing and returns true
bool TryWaitConnectToNovad(int timeout_ms);

//Closes any connection Novad over IPC
//	returns - true if no connections to Novad exists, false if there is a connection (error)
//	NOTE: If there was already no connection, then the function does nothing and returns true
bool CloseNovadConnection();


//************************************************************************
//**						Suspect Operations							**
//************************************************************************

// Gets a list of suspect addresses currently classified
//	 listType: Type of list to get (all, just hostile, just benign)
//	Returns: list of addresses
std::vector<in_addr_t> *GetSuspectList(enum SuspectListType listType);

// Gets a suspect from the daemon
// address: IP address of the suspect
// Returns: Pointer to the suspect
Suspect *GetSuspect(in_addr_t address);

//Asks Novad to save the suspect list to persistent storage
//	returns - true if saved correctly, false on error
bool SaveAllSuspects(std::string file);

//Asks Novad to forget all data on all suspects that it has
//	returns - true if all suspects cleared successfully, false on error
bool ClearAllSuspects();

//Asks Novad to forget data on the specified suspect
//	suspectAddress - The IP address (unique identifier) of the suspect to forget
//	returns - true is suspect has been cleared successfully, false on error
bool ClearSuspect(in_addr_t suspectAddress);

//Asks Novad to reclassify all suspects
//	returns - true if all suspects have been reclassified, false on error
bool ReclassifyAllSuspects();

// Asks novad for it's uptime
//  returns - uptime in seconds (0 if error)
int GetUptime();

}



#endif /* COMMANDS_H_ */
