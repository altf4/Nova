//============================================================================
// Name        : StatusQueries.h
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
// Description : Controls the Novad process itself, in terms of stopping and starting
//============================================================================

#ifndef NOVADCONTROL_H_
#define NOVADCONTROL_H_

#include <string>
#include <cstring>
#include <arpa/inet.h>

namespace Nova
{

//Runs the Novad process
//	returns - True upon successfully running the novad process, false on error
//	NOTE: This function will return true if Novad was already running
bool StartNovad();

//Kills the Novad process
//	returns - True upon successfully stopping the novad process, false on error
//	NOTE: This function will return true if Novad was already dead
bool StopNovad();

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


}

#endif /* NOVADCONTROL_H_ */
