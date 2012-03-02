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

#include "messages/UI_Message.h"
#include "messages/ControlMessage.h"

///	Filename of the file to be used as an Classification Engine IPC key
#define NOVAD_IN_FILENAME "/keys/CEKey"
/// File name of the file to be used as CE_GUI Input IPC key.
#define NOVAD_OUT_FILENAME "/keys/NovaIPCKey"

namespace Nova
{

//Launches a UI Handling thread, and returns
void Spawn_UI_Handler();

//Looping thread which receives UI messages and handles them
//	NOTE: Must manually free() the socketPtr after using it.
//		This eliminates a race condition on passing the socket parameter
void *Handle_UI_Thread(void *socketVoidPtr);

//Processes a single ControlMessage received from the UI
//	controlMessage - A reference to the received ControlMessage
//	socketFD - The socket on which to contact the UI
void HandleControlMessage(ControlMessage &controlMessage, int socketFD);

}
#endif /* PROTOCOLHANDLER_H_ */
