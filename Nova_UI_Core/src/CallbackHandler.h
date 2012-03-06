//============================================================================
// Name        : CallbackHandler.h
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
// Description : Functions for reading messages on the UI callback socket
//				IE: asynchronous messages received from Novad
//============================================================================


#ifndef CALLBACKHANDLER_H_
#define CALLBACKHANDLER_H_

enum CallbackType: char
{
	CALLBACK_ERROR = 0,		//There was an error in receiving the callback message
};

struct CallbackChange
{
	enum CallbackType type;
};

namespace Nova
{

//Receives a single callback message and returns its details
//	NOTE: Blocking call. Should be run from within its own looping thread
//	returns - A struct describing what Novad is asking
struct CallbackChange ProcessCallbackMessage();

}


#endif /* CALLBACKHANDLER_H_ */
