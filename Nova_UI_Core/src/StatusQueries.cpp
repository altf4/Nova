//============================================================================
// Name        : StatusQueries.cpp
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
// Description : Handles requests for information from Novad
//============================================================================

#include "Commands.h"
#include "messaging/MessageManager.h"
#include "messaging/messages/ControlMessage.h"
#include "messaging/messages/RequestMessage.h"
#include "messaging/messages/ErrorMessage.h"
#include "Logger.h"
#include "Lock.h"

#include <iostream>

using namespace Nova;
using namespace std;

extern int IPCSocketFD;

namespace Nova
{
bool IsNovadUp(bool tryToConnect)
{

	if(tryToConnect)
	{
		//If we couldn't connect, then it's definitely not up
		if(!ConnectToNovad())
		{
			return false;
		}
	}

	Lock lock = MessageManager::Instance().UseSocket(IPCSocketFD);

	RequestMessage ping(REQUEST_PING, DIRECTION_TO_NOVAD);
	if(!Message::WriteMessage(&ping, IPCSocketFD) )
	{
		//There was an error in sending the message
		return false;
	}

	Message *reply = Message::ReadMessage(IPCSocketFD, DIRECTION_TO_NOVAD, REPLY_TIMEOUT);
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
			// This was breaking things during the mess of isNovadUp calls
			// when the QT GUi starts and connects to novad. If there was some
			// important reason for it being here that I don't know about, we
			// might need to put it back and track down why exactly it was
			// causing problems.
			//CloseNovadConnection();
		}
		delete error;
		return false;
	}
	if(reply->m_messageType != REQUEST_MESSAGE )
	{
		//Received the wrong kind of message
		delete reply;
		return false;
	}

	RequestMessage *pong = (RequestMessage*)reply;
	if(pong->m_requestType != REQUEST_PONG)
	{
		//Received the wrong kind of control message
		delete pong;
		return false;
	}
	delete pong;
	return true;
}

int GetUptime()
{
	Lock lock = MessageManager::Instance().UseSocket(IPCSocketFD);

	RequestMessage request(REQUEST_UPTIME, DIRECTION_TO_NOVAD);

	if(!Message::WriteMessage(&request, IPCSocketFD) )
	{
		return 0;
	}

	Message *reply = Message::ReadMessage(IPCSocketFD, DIRECTION_TO_NOVAD, REPLY_TIMEOUT);
	if (reply->m_messageType == ERROR_MESSAGE && ((ErrorMessage*)reply)->m_errorType == ERROR_TIMEOUT)
	{
		LOG(ERROR, "Timeout error when waiting for message reply", "");
		delete ((ErrorMessage*)reply);
		return 0;
	}

	if(reply->m_messageType != REQUEST_MESSAGE )
	{
		//Received the wrong kind of message
		delete reply;
		return 0;
	}

	RequestMessage *requestReply = (RequestMessage*)reply;
	if(requestReply->m_requestType != REQUEST_UPTIME_REPLY)
	{
		//Received the wrong kind of control message
		delete requestReply;
		return 0;
	}

	int ret = requestReply->m_uptime;

	delete requestReply;
	return ret;
}

vector<in_addr_t> *GetSuspectList(enum SuspectListType listType)
{
	Lock lock = MessageManager::Instance().UseSocket(IPCSocketFD);

	RequestMessage request(REQUEST_SUSPECTLIST, DIRECTION_TO_NOVAD);
	request.m_listType = listType;

	if(!Message::WriteMessage(&request, IPCSocketFD) )
	{
		//There was an error in sending the message
		return NULL;
	}

	Message *reply = Message::ReadMessage(IPCSocketFD, DIRECTION_TO_NOVAD, REPLY_TIMEOUT);
	if (reply->m_messageType == ERROR_MESSAGE && ((ErrorMessage*)reply)->m_errorType == ERROR_TIMEOUT)
	{
		LOG(ERROR, "Timeout error when waiting for message reply", "");
		delete ((ErrorMessage*)reply);
		return NULL;
	}

	if(reply->m_messageType != REQUEST_MESSAGE )
	{
		//Received the wrong kind of message
		delete reply;
		return NULL;
	}

	RequestMessage *requestReply = (RequestMessage*)reply;
	if(requestReply->m_requestType != REQUEST_SUSPECTLIST_REPLY)
	{
		//Received the wrong kind of control message
		delete requestReply;
		return NULL;
	}

	vector<in_addr_t> *ret = new vector<in_addr_t>;
	*ret = requestReply->m_suspectList;

	delete requestReply;
	return ret;
}

Suspect *GetSuspect(in_addr_t address)
{
	Lock lock = MessageManager::Instance().UseSocket(IPCSocketFD);

	RequestMessage request(REQUEST_SUSPECT, DIRECTION_TO_NOVAD);
	request.m_suspectAddress = address;


	if(!Message::WriteMessage(&request, IPCSocketFD) )
	{
		//There was an error in sending the message
		return NULL;
	}


	Message *reply = Message::ReadMessage(IPCSocketFD, DIRECTION_TO_NOVAD, REPLY_TIMEOUT);
	if (reply->m_messageType == ERROR_MESSAGE && ((ErrorMessage*)reply)->m_errorType == ERROR_TIMEOUT)
	{
		LOG(ERROR, "Timeout error when waiting for message reply", "");
		delete ((ErrorMessage*)reply);
		return NULL;
	}

	if(reply->m_messageType != REQUEST_MESSAGE)
	{
		//Received the wrong kind of message
		delete reply;
		return NULL;
	}

	RequestMessage *requestReply = (RequestMessage*)reply;
	if(requestReply->m_requestType != REQUEST_SUSPECT_REPLY)
	{
		//Received the wrong kind of control message
		delete requestReply;
		return NULL;
	}

	Suspect *returnSuspect = new Suspect();
	*returnSuspect = *requestReply->m_suspect;
	delete requestReply;

	return returnSuspect;
}
}
