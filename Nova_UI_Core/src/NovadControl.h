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

namespace Nova
{

//Runs the Novad process
//	returns - True upon successfully running the novad process, false on error
bool StartNovad();

//Kills the Novad process
//	returns - True upon successfully stopping the novad process, false on error
bool StopNovad();


}

#endif /* NOVADCONTROL_H_ */
