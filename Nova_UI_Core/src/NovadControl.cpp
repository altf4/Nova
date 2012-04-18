//============================================================================
// Name        : NovadControl.cpp
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
// Description : Controls the Novad process itself, in terms of stopping and starting
//============================================================================

#include "messages/UI_Message.h"
#include "messages/ControlMessage.h"
#include "messages/ErrorMessage.h"
#include "Commands.h"
#include "Logger.h"
#include "Socket.h"
#include "Lock.h"

#include <iostream>
#include <stdio.h>
#include <unistd.h>

using namespace Nova;
using namespace std;

extern Socket novadListenSocket;

namespace Nova
{
bool StartNovad()
{
	if(IsNovadUp(false))
	{
		return true;
	}

	if(system("nohup novad > /dev/null&") != 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool StopNovad()
{
	Lock lock(&novadListenSocket.m_mutex);

	ControlMessage killRequest(CONTROL_EXIT_REQUEST);
	if(!UI_Message::WriteMessage(&killRequest, novadListenSocket.m_socketFD) )
	{
		//There was an error in sending the message
		//TODO: Log this fact
		return false;
	}

	UI_Message *reply = UI_Message::ReadMessage(novadListenSocket.m_socketFD, REPLY_TIMEOUT);
	if (reply->m_messageType == ERROR_MESSAGE && ((ErrorMessage*)reply)->m_errorType == ERROR_TIMEOUT)
	{
		LOG(ERROR, "Timeout error when waiting for message reply", "");
		delete ((ErrorMessage*)reply);
		return false;
	}

	if(reply->m_messageType == ERROR_MESSAGE )
	{
		ErrorMessage *error = (ErrorMessage*)reply;
		if(error->m_errorType == ERROR_SOCKET_CLOSED)
		{
			CloseNovadConnection();
		}
		delete error;
		return false;
	}
	if(reply->m_messageType != CONTROL_MESSAGE )
	{
		//Received the wrong kind of message
		delete reply;
		return false;
	}

	ControlMessage *killReply = (ControlMessage*)reply;
	if( killReply->m_controlType != CONTROL_EXIT_REPLY )
	{
		//Received the wrong kind of control message
		delete killReply;
		return false;
	}
	bool retSuccess = killReply->m_success;
	delete killReply;

	CloseNovadConnection();

	return retSuccess;
}

bool SaveAllSuspects(std::string file)
{
	Lock lock(&novadListenSocket.m_mutex);

	ControlMessage saveRequest(CONTROL_SAVE_SUSPECTS_REQUEST);
	strcpy(saveRequest.m_filePath, file.c_str());

	if(!UI_Message::WriteMessage(&saveRequest, novadListenSocket.m_socketFD) )
	{
		//There was an error in sending the message
		//TODO: Log this fact
		return false;
	}

	UI_Message *reply = UI_Message::ReadMessage(novadListenSocket.m_socketFD, REPLY_TIMEOUT);
	if (reply->m_messageType == ERROR_MESSAGE && ((ErrorMessage*)reply)->m_errorType == ERROR_TIMEOUT)
	{
		LOG(ERROR, "Timeout error when waiting for message reply", "");
		delete ((ErrorMessage*)reply);
		return false;
	}

	if(reply->m_messageType == ERROR_MESSAGE )
	{
		ErrorMessage *error = (ErrorMessage*)reply;
		if(error->m_errorType == ERROR_SOCKET_CLOSED)
		{
			CloseNovadConnection();
		}
		delete error;
		return false;
	}
	if(reply->m_messageType != CONTROL_MESSAGE )
	{
		//Received the wrong kind of message
		delete reply;
		return false;
	}

	ControlMessage *saveReply = (ControlMessage*)reply;
	if( saveReply->m_controlType != CONTROL_SAVE_SUSPECTS_REPLY )
	{
		//Received the wrong kind of control message
		delete saveReply;
		return false;
	}
	bool retSuccess = saveReply->m_success;
	delete saveReply;
	return retSuccess;
}

bool ClearAllSuspects()
{
	Lock lock(&novadListenSocket.m_mutex);

	ControlMessage clearRequest(CONTROL_CLEAR_ALL_REQUEST);
	if(!UI_Message::WriteMessage(&clearRequest, novadListenSocket.m_socketFD) )
	{
		//There was an error in sending the message
		//TODO: Log this fact
		return false;
	}

	UI_Message *reply = UI_Message::ReadMessage(novadListenSocket.m_socketFD, REPLY_TIMEOUT);
	if (reply->m_messageType == ERROR_MESSAGE && ((ErrorMessage*)reply)->m_errorType == ERROR_TIMEOUT)
	{
		LOG(ERROR, "Timeout error when waiting for message reply", "");
		delete ((ErrorMessage*)reply);
		return false;
	}

	if(reply->m_messageType == ERROR_MESSAGE )
	{
		ErrorMessage *error = (ErrorMessage*)reply;
		if(error->m_errorType == ERROR_SOCKET_CLOSED)
		{
			CloseNovadConnection();
		}
		delete error;
		return false;
	}
	if(reply->m_messageType != CONTROL_MESSAGE )
	{
		//Received the wrong kind of message
		delete reply;
		return false;
	}

	ControlMessage *clearReply = (ControlMessage*)reply;
	if( clearReply->m_controlType != CONTROL_CLEAR_ALL_REPLY )
	{
		//Received the wrong kind of control message
		delete clearReply;
		return false;
	}
	bool retSuccess = clearReply->m_success;
	delete clearReply;
	return retSuccess;
}

bool ClearSuspect(in_addr_t suspectAddress)
{
	Lock lock(&novadListenSocket.m_mutex);

	ControlMessage clearRequest(CONTROL_CLEAR_SUSPECT_REQUEST);
	clearRequest.m_suspectAddress = suspectAddress;
	if(!UI_Message::WriteMessage(&clearRequest, novadListenSocket.m_socketFD) )
	{
		//There was an error in sending the message
		//TODO: Log this fact
		return false;
	}

	UI_Message *reply = UI_Message::ReadMessage(novadListenSocket.m_socketFD, REPLY_TIMEOUT);
	if (reply->m_messageType == ERROR_MESSAGE && ((ErrorMessage*)reply)->m_errorType == ERROR_TIMEOUT)
	{
		LOG(ERROR, "Timeout error when waiting for message reply", "");
		delete ((ErrorMessage*)reply);
		return false;
	}

	if(reply->m_messageType == ERROR_MESSAGE )
	{
		ErrorMessage *error = (ErrorMessage*)reply;
		if(error->m_errorType == ERROR_SOCKET_CLOSED)
		{
			CloseNovadConnection();
		}
		delete error;
		return false;
	}
	if(reply->m_messageType != CONTROL_MESSAGE )
	{
		//Received the wrong kind of message
		delete reply;
		return false;
	}

	ControlMessage *clearReply = (ControlMessage*)reply;
	if( clearReply->m_controlType != CONTROL_CLEAR_SUSPECT_REPLY )
	{
		//Received the wrong kind of control message
		delete clearReply;
		return false;
	}
	bool retSuccess = clearReply->m_success;
	delete clearReply;
	return retSuccess;
}

bool ReclassifyAllSuspects()
{
	Lock lock(&novadListenSocket.m_mutex);

	ControlMessage reclassifyRequest(CONTROL_RECLASSIFY_ALL_REQUEST);
	if(!UI_Message::WriteMessage(&reclassifyRequest, novadListenSocket.m_socketFD) )
	{
		//There was an error in sending the message
		//TODO: Log this fact
		return false;
	}

	UI_Message *reply = UI_Message::ReadMessage(novadListenSocket.m_socketFD, REPLY_TIMEOUT);
	if (reply->m_messageType == ERROR_MESSAGE && ((ErrorMessage*)reply)->m_errorType == ERROR_TIMEOUT)
	{
		LOG(ERROR, "Timeout error when waiting for message reply", "");
		delete ((ErrorMessage*)reply);
		return false;
	}

	if(reply->m_messageType == ERROR_MESSAGE )
	{
		ErrorMessage *error = (ErrorMessage*)reply;
		if(error->m_errorType == ERROR_SOCKET_CLOSED)
		{
			CloseNovadConnection();
		}
		delete error;
		return false;
	}
	if(reply->m_messageType != CONTROL_MESSAGE )
	{
		//Received the wrong kind of message
		delete reply;
		return false;
	}

	ControlMessage *reclassifyReply = (ControlMessage*)reply;
	if( reclassifyReply->m_controlType != CONTROL_RECLASSIFY_ALL_REPLY )
	{
		//Received the wrong kind of control message
		delete reclassifyReply;
		return false;
	}
	bool retSuccess = reclassifyReply->m_success;
	delete reclassifyReply;
	return retSuccess;
}
}
