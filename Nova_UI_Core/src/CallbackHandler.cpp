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

using namespace Nova;

extern int UI_ListenSocket;
extern int novadListenSocket;


struct CallbackChange ProcessCallbackMessage()
{
	struct CallbackChange change;
	change.type = CALLBACK_ERROR;

	UI_Message *message = UI_Message::ReadMessage(UI_ListenSocket);
	if( message == NULL)
	{
		return change;
	}
	if( message->m_messageType )
	MatchLobbyMessage *match_message = (MatchLobbyMessage*)message;

	switch(message->type)
	{
		case TEAM_CHANGED_NOTIFICATION:
		{

		}
		default:
		{
			return change;
		}
	}
}
