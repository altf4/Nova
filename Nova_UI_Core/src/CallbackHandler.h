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

#include "Suspect.h"

enum CallbackChangeType: char
{
	CALLBACK_ERROR = 0,				//There was an error in receiving the callback message
	CALLBACK_HUNG_UP,				//Novad hung up on us
	CALLBACK_NEW_SUSPECT,			//Received a new suspect from Novad
	CALLBACK_ALL_SUSPECTS_CLEARED,	//Another UI cleared the suspects list
	CALLBACK_SUSPECT_CLEARED,		//A single specified suspect was cleared
};

struct CallbackChange
{
	enum CallbackChangeType m_type;
	Nova::Suspect *m_suspect;			//Used in type: CALLBACK_NEW_SUSPECT
	in_addr_t m_suspectIP;				//Used in CALLBACK_SUSPECT_CLEARED
};

namespace Nova
{

//Receives a single callback message and returns its details
//	NOTE: Blocking call. Should be run from within its own looping thread
//	returns - A struct describing what Novad is asking the UI to change
struct CallbackChange ProcessCallbackMessage();

}


#endif /* CALLBACKHANDLER_H_ */
