//============================================================================
// Name        : Haystack.h
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
// Description : Controls the Honeyd Haystack and Doppelganger processes
//============================================================================

#ifndef HAYSTACKCONTROL_H_
#define HAYSTACKCONTROL_H_

namespace Nova
{

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

//Starts the Honeyd Doppelganger process
//	returns - True if doppelganger successfully started, false on error
//	NOTE: If the doppelganger is already running, this function does nothing and returns true
//bool StartDoppelganger();

//Stops the Honeyd Doppelganger process
//	returns - True if doppelganger successfully stopped, false on error
//	NOTEL if the doppelganger is already dead, this function does nothing and returns true
//bool StopDoppelganger();

//Returns whether the Doppelganger is running or not
//	returns - True if honeyd doppelganger is running, false if not running
//bool IsDoppelgangerUp();

}


#endif /* HAYSTACKCONTROL_H_ */
