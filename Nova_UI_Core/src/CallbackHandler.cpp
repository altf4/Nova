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
#include "messages/UI_Message.h"
#include "messages/CallbackMessage.h"

using namespace Nova;

extern int UI_parentSocket;
extern int UI_ListenSocket;
extern int novadListenSocket;


struct CallbackChange Nova::ProcessCallbackMessage()
{
	struct CallbackChange change;
	change.type = CALLBACK_ERROR;

	UI_Message *message = UI_Message::ReadMessage(UI_ListenSocket);
	if( message == NULL)
	{
		change.type = CALLBACK_HUNG_UP;
		return change;
	}
	if( message->m_messageType != CALLBACK_MESSAGE)
	{
		return change;
	}
	CallbackMessage *callbackMessage = (CallbackMessage*)message;

	switch(callbackMessage->m_callbackType)
	{
		case CALLBACK_SUSPECT_UDPATE:
		{
			change.type = CALLBACK_NEW_SUSPECT;
			change.suspect = callbackMessage->m_suspect;

			CallbackMessage *callbackAck = new CallbackMessage();
			callbackAck->m_callbackType = CALLBACK_SUSPECT_UDPATE_ACK;
			if(!UI_Message::WriteMessage(callbackAck, UI_ListenSocket))
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
