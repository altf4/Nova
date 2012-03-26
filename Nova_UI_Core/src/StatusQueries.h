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
// Description : Handles requests for information from Novad
//============================================================================

#ifndef STATUSQUERIES_H_
#define STATUSQUERIES_H_

#include <vector>
#include <arpa/inet.h>

#include "messages/RequestMessage.h"

namespace Nova
{

//Queries Novad to see if it is currently up or down
//	tryToConnect - Optional boolean to indicate whether this function should also try to connect to Novad before testing IsUp()
//		You should generally just leave this alone as true.
//	returns - True if Novad is up, false if down
bool IsNovadUp(bool tryToConnect = true);

// Gets a list of suspect addresses currently classified
//	 listType: Type of list to get (all, just hostile, just benign)
//	Returns: list of addresses
std::vector<in_addr_t> *GetSuspectList(enum SuspectListType listType);

// Gets a suspect from the daemon
// address: IP address of the suspect
// Returns: Pointer to the suspect
Suspect *GetSuspect(in_addr_t address);

}
#endif /* STATUSQUERIES_H_ */
