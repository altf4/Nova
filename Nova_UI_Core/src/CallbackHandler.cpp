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
#include "messaging/messages/Message.h"
#include "messaging/messages/UpdateMessage.h"
#include "messaging/messages/ErrorMessage.h"
#include "messaging/MessageManager.h"
#include "Lock.h"

using namespace Nova;

extern int IPCSocketFD;

struct CallbackChange Nova::ProcessCallbackMessage()
{
	struct CallbackChange change;
	change.m_type = CALLBACK_ERROR;

	//Wait for a callback to occur
	//	If it comes back false, then that means the socket is dead
	if(!MessageManager::Instance().RegisterCallback(IPCSocketFD))
	{
		change.m_type = CALLBACK_HUNG_UP;
		return change;
	}

	Message *message = Message::ReadMessage(IPCSocketFD, DIRECTION_TO_UI, REPLY_TIMEOUT);
	if( message->m_messageType == ERROR_MESSAGE)
	{
		ErrorMessage *errorMessage = (ErrorMessage*)message;
		if(errorMessage->m_errorType == ERROR_SOCKET_CLOSED)
		{
			change.m_type = CALLBACK_ERROR;
		}
		//TODO: Do we care about the other error message types here?

		delete errorMessage;
		return change;
	}
	if( message->m_messageType != UPDATE_MESSAGE)
	{
		delete message;
		return change;
	}
	UpdateMessage *updateMessage = (UpdateMessage*)message;

	switch(updateMessage->m_updateType)
	{
		case UPDATE_SUSPECT:
		{
			change.m_type = CALLBACK_NEW_SUSPECT;
			change.m_suspect = updateMessage->m_suspect;

			UpdateMessage callbackAck(UPDATE_SUSPECT_ACK, DIRECTION_TO_UI);
			if(!Message::WriteMessage(&callbackAck, IPCSocketFD))
			{
				//TODO: log this? We failed to send the ack
			}
			break;
		}
		case UPDATE_ALL_SUSPECTS_CLEARED:
		{
			change.m_type = CALLBACK_ALL_SUSPECTS_CLEARED;

			UpdateMessage callbackAck(UPDATE_ALL_SUSPECTS_CLEARED_ACK, DIRECTION_TO_UI);
			if(!Message::WriteMessage(&callbackAck, IPCSocketFD))
			{
				//TODO: log this? We failed to send the ack
			}
			break;
		}
		case UPDATE_SUSPECT_CLEARED:
		{
			change.m_type = CALLBACK_SUSPECT_CLEARED;
			change.m_suspectIP = updateMessage->m_IPAddress;

			UpdateMessage callbackAck(UPDATE_SUSPECT_CLEARED_ACK, DIRECTION_TO_UI);
			if(!Message::WriteMessage(&callbackAck, IPCSocketFD))
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
	delete updateMessage;
	return change;
}
