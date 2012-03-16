//============================================================================
// Name        : Connection.h
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
// Description : Manages connections out to Novad, initializes and closes them
//============================================================================
#ifndef CONNECTION_H_
#define CONNECTION_H_

namespace Nova
{

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
bool TryWaitConenctToNovad(int timeout_ms);

//Closes any connection Novad over IPC
//	returns - true if no connections to Novad exists, false if there is a connection (error)
//	NOTE: If there was already no connection, then the function does nothing and returns true
bool CloseNovadConnection();

}

#endif /* CONNECTION_H_ */
