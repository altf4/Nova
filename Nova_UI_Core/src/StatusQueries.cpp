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

	Ticket ticket = MessageManager::Instance().StartConversation(IPCSocketFD);

	RequestMessage ping(REQUEST_PING);
	if(!MessageManager::Instance().WriteMessage(ticket, &ping))
	{
		//There was an error in sending the message
		return false;
	}

	Message *reply = MessageManager::Instance().ReadMessage(ticket);
	if(reply->m_messageType == ERROR_MESSAGE && ((ErrorMessage*)reply)->m_errorType == ERROR_TIMEOUT)
	{
		LOG(ERROR, "Timeout error when waiting for message reply", "");
		reply->DeleteContents();
		delete reply;
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
		reply->DeleteContents();
		delete reply;
		return false;
	}

	RequestMessage *pong = (RequestMessage*)reply;
	if(pong->m_requestType != REQUEST_PONG)
	{
		//Received the wrong kind of control message
		pong->DeleteContents();
		delete pong;
		return false;
	}
	delete pong;
	return true;
}

uint64_t GetStartTime()
{
	Ticket ticket = MessageManager::Instance().StartConversation(IPCSocketFD);

	RequestMessage request(REQUEST_UPTIME);

	if(!MessageManager::Instance().WriteMessage(ticket, &request))
	{
		return 0;
	}

	Message *reply = MessageManager::Instance().ReadMessage(ticket);
	if(reply->m_messageType == ERROR_MESSAGE && ((ErrorMessage*)reply)->m_errorType == ERROR_TIMEOUT)
	{
		LOG(ERROR, "Timeout error when waiting for message reply", "");
		reply->DeleteContents();
		delete reply;
		return 0;
	}

	if(reply->m_messageType != REQUEST_MESSAGE )
	{
		//Received the wrong kind of message
		reply->DeleteContents();
		delete reply;
		return 0;
	}

	RequestMessage *requestReply = (RequestMessage*)reply;
	if(requestReply->m_requestType != REQUEST_UPTIME_REPLY)
	{
		//Received the wrong kind of control message
		requestReply->DeleteContents();
		delete requestReply;
		return 0;
	}

	uint64_t ret = requestReply->m_startTime;

	delete requestReply;
	return ret;
}

vector<in_addr_t> *GetSuspectList(enum SuspectListType listType)
{
	Ticket ticket = MessageManager::Instance().StartConversation(IPCSocketFD);

	RequestMessage request(REQUEST_SUSPECTLIST);
	request.m_listType = listType;

	if(!MessageManager::Instance().WriteMessage(ticket, &request))
	{
		//There was an error in sending the message
		return NULL;
	}

	Message *reply = MessageManager::Instance().ReadMessage(ticket);
	if(reply->m_messageType == ERROR_MESSAGE && ((ErrorMessage*)reply)->m_errorType == ERROR_TIMEOUT)
	{
		LOG(ERROR, "Timeout error when waiting for message reply", "");
		delete ((ErrorMessage*)reply);
		return NULL;
	}

	if(reply->m_messageType != REQUEST_MESSAGE )
	{
		//Received the wrong kind of message
		delete reply;
		reply->DeleteContents();
		return NULL;
	}

	RequestMessage *requestReply = (RequestMessage*)reply;
	if(requestReply->m_requestType != REQUEST_SUSPECTLIST_REPLY)
	{
		//Received the wrong kind of control message
		reply->DeleteContents();
		delete reply;
		return NULL;
	}

	vector<in_addr_t> *ret = new vector<in_addr_t>;
	*ret = requestReply->m_suspectList;

	delete requestReply;
	return ret;
}

Suspect *GetSuspect(in_addr_t address)
{
	Ticket ticket = MessageManager::Instance().StartConversation(IPCSocketFD);

	RequestMessage request(REQUEST_SUSPECT);
	request.m_suspectAddress = address;

	if(!MessageManager::Instance().WriteMessage(ticket, &request))
	{
		//There was an error in sending the message
		return NULL;
	}

	Message *reply = MessageManager::Instance().ReadMessage(ticket);
	if(reply->m_messageType == ERROR_MESSAGE && ((ErrorMessage*)reply)->m_errorType == ERROR_TIMEOUT)
	{
		LOG(ERROR, "Timeout error when waiting for message reply", "");
		reply->DeleteContents();
		delete reply;
		return NULL;
	}

	if(reply->m_messageType != REQUEST_MESSAGE)
	{
		//Received the wrong kind of message
		reply->DeleteContents();
		delete reply;
		return NULL;
	}

	RequestMessage *requestReply = (RequestMessage*)reply;
	if(requestReply->m_requestType != REQUEST_SUSPECT_REPLY)
	{
		//Received the wrong kind of control message
		reply->DeleteContents();
		delete requestReply;
		return NULL;
	}

	Suspect *returnSuspect = new Suspect();
	*returnSuspect = *requestReply->m_suspect;
	delete requestReply;

	return returnSuspect;
}
}
