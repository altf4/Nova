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

#include "messaging/messages/Message.h"
#include "messaging/messages/ControlMessage.h"
#include "messaging/messages/RequestMessage.h"
#include "messaging/messages/UpdateMessage.h"
#include "Suspect.h"

namespace Nova
{

struct UI_NotificationPackage
{
	UpdateMessage *m_updateMessage;
	enum UpdateType m_ackType;
	int m_socketFD_sender;
};

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

//Sends notification messages to all UIs (except one), expecting an ACK message in response from each
//	notificationType - The update message type to send
//	updateMessage - The message to send to each UI. Transfers control of life cycle to this function!
//	socketFD_sender - The sender's socket. Don't send a notification to this socket
//	NOTE: Spawns a separate thread and returns immediately. NOT blocking
void NotifyUIs(UpdateMessage *updateMessage, enum UpdateType ackType, int socketFD_sender);

//Helper function to NotifyUIs. Runs the actual pthread
//	ptr - Pointer to a UI_NotificationPackage struct containing the actual arguments needed
void *NotifyUIsHelper(void *ptr);

}
#endif /* PROTOCOLHANDLER_H_ */
