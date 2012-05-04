//============================================================================
// Name        : ProtocolHandler.h
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
// Description : Manages the message sending protocol to and from the Nova UI
//============================================================================

#ifndef PROTOCOLHANDLER_H_
#define PROTOCOLHANDLER_H_

#include "messaging/messages/UI_Message.h"
#include "messaging/messages/ControlMessage.h"
#include "messaging/messages/RequestMessage.h"
#include "Suspect.h"

namespace Nova
{

//This is the only thread Novad needs to call to set up a UI Message Handler
//Launches a UI Handling thread
//	returns - true if listening successfully, false on error
bool Spawn_UI_Handler();

//Helper thread launched by Spawn_UI_Handler() so that it can return
//	Loops, listening on the main IPC socket for new connections. When one is found, spawn a new thread to handle it (Handle_UI_Thread)
void *Handle_UI_Helper(void *ptr);

//Looping thread which receives UI messages and handles them
//	NOTE: Must manually free() the socketPtr after using it.
//		This eliminates a race condition on passing the socket parameter
void *Handle_UI_Thread(void *socketVoidPtr);

//Processes a single ControlMessage received from the UI
//	controlMessage - A reference to the received ControlMessage
//	socketFD - The socket on which to contact the UI
void HandleControlMessage(ControlMessage &controlMessage, int socketFD);
void HandleRequestMessage(RequestMessage &requestMessage, int socketFD);


//Commands and Updates to UI:

//Sends (updates) a single suspect to all UIs for display to the user
//	suspect - The suspect to send
//	socket - The socket of the UI to send to
void SendSuspectToUIs(Suspect *suspect);

}
#endif /* PROTOCOLHANDLER_H_ */
