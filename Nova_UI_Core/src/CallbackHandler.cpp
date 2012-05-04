//============================================================================
// Name        : CallbackHandler.cpp
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

#include "CallbackHandler.h"
#include "messaging/messages/UI_Message.h"
#include "messaging/messages/CallbackMessage.h"
#include "messaging/messages/ErrorMessage.h"
#include "messaging/MessageManager.h"
#include "Lock.h"

using namespace Nova;

extern int IPCSocketFD;

struct CallbackChange Nova::ProcessCallbackMessage()
{
	struct CallbackChange change;
	change.type = CALLBACK_ERROR;

	//Wait for a callback to occur
	//	If it comes back false, then that means the socket is dead
	if(!MessageManager::Instance().RegisterCallback(IPCSocketFD))
	{
		change.type = CALLBACK_HUNG_UP;
		return change;
	}

	//Claim the socket's mutex, so another protocol doesn't get mixed up in between
	Lock lock = MessageManager::Instance().UseSocket(IPCSocketFD);

	UI_Message *message = UI_Message::ReadMessage(IPCSocketFD, DIRECTION_TO_UI, REPLY_TIMEOUT);
	if( message->m_messageType == ERROR_MESSAGE)
	{
		ErrorMessage *errorMessage = (ErrorMessage*)message;
		if(errorMessage->m_errorType == ERROR_SOCKET_CLOSED)
		{
			change.type = CALLBACK_HUNG_UP;
		}
		//TODO: Do we care about the other error message types here?

		delete errorMessage;
		return change;
	}
	if( message->m_messageType != CALLBACK_MESSAGE)
	{
		delete message;
		return change;
	}
	CallbackMessage *callbackMessage = (CallbackMessage*)message;

	switch(callbackMessage->m_callbackType)
	{
		case CALLBACK_SUSPECT_UDPATE:
		{
			change.type = CALLBACK_NEW_SUSPECT;
			change.suspect = callbackMessage->m_suspect;

			CallbackMessage callbackAck(CALLBACK_SUSPECT_UDPATE_ACK, DIRECTION_TO_UI);
			if(!UI_Message::WriteMessage(&callbackAck, IPCSocketFD))
			{
				//TODO: log this? We failed to send the ack
			}
			break;
		}
		default:
		{
			break;
		}
	}
	delete callbackMessage;
	return change;
}
