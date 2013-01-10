//============================================================================
// Name        : NovadCallback.cpp
// Copyright   : DataSoft Corporation 2011-2013
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
// Description : Child class of ServerCallback. Specifies a function to be run
//		as the callback thread for incoming connections
//============================================================================

#include "messaging/MessageManager.h"
#include "messaging/messages/Message.h"
#include "messaging/messages/ErrorMessage.h"
#include "messaging/messages/ControlMessage.h"
#include "messaging/messages/RequestMessage.h"
#include "messaging/messages/UpdateMessage.h"
#include "NovadCallback.h"
#include "Logger.h"
#include "ProtocolHandler.h"

namespace Nova
{

void NovadCallback::CallbackThread(int socketFD)
{
	bool keepLooping = true;

	while(keepLooping)
	{
		//Wait for a callback to occur
		//If register comes back false, then the socket was closed. So exit the thread
		Ticket ticket;
		if(!MessageManager::Instance().RegisterCallback(socketFD, ticket))
		{
			keepLooping = false;
			continue;
		}
		Message *message = MessageManager::Instance().ReadMessage(ticket);
		switch(message->m_messageType)
		{
			case CONTROL_MESSAGE:
			{
				ControlMessage *controlMessage = (ControlMessage*)message;
				HandleControlMessage(*controlMessage, ticket);
				delete controlMessage;
				break;
			}
			case REQUEST_MESSAGE:
			{
				RequestMessage *msg = (RequestMessage*)message;
				HandleRequestMessage(*msg, ticket);
				delete msg;
				break;
			}
			case ERROR_MESSAGE:
			{
				ErrorMessage *errorMessage = (ErrorMessage*)message;
				switch(errorMessage->m_errorType)
				{
					case ERROR_SOCKET_CLOSED:
					{
						LOG(DEBUG, "The UI hung up","UI socket closed uncleanly, exiting this thread");
						break;
					}
					case ERROR_MALFORMED_MESSAGE:
					{
						LOG(DEBUG, "There was an error reading a message from the UI", "Got a message but it was not deserialized correctly");
						break;
					}
					case ERROR_UNKNOWN_MESSAGE_TYPE:
					{
						LOG(DEBUG, "There was an error reading a message from the UI", "Received an unknown message type.");
						break;
					}
					case ERROR_PROTOCOL_MISTAKE:
					{
						LOG(DEBUG, "We sent a bad message to the UI", "Received an ERROR_PROTOCOL_MISTAKE.");
						break;
					}
					default:
					{
						LOG(DEBUG, "There was an error reading a message from the UI", "Unknown error type. Should see this!");
						break;
					}
				}
				delete errorMessage;
				break;

			}
			default:
			{
				//There was an error reading this message
				LOG(DEBUG, "There was an error reading a message from the UI", "Invalid message type");
				delete message;
				break;
			}
		}
	}
}

}
