//============================================================================
// Name        : StatusQueries.cpp
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
	bool isUp = true;
	if(tryToConnect)
	{
		//If we couldn't connect, then it's definitely not up
		if(!ConnectToNovad())
		{
			return false;
		}
	}

	//Funny syntax just so I can break; out of the context
	do
	{
		Ticket ticket = MessageManager::Instance().StartConversation(IPCSocketFD);

		RequestMessage ping(REQUEST_PING);
		if(!MessageManager::Instance().WriteMessage(ticket, &ping))
		{
			//There was an error in sending the message
			isUp = false;
			break;
		}

		Message *reply = MessageManager::Instance().ReadMessage(ticket);
		if(reply->m_messageType == ERROR_MESSAGE && ((ErrorMessage*)reply)->m_errorType == ERROR_TIMEOUT)
		{
			LOG(WARNING, "Timeout error when waiting for message reply", "");
			reply->DeleteContents();
			delete reply;
			isUp = false;
			break;
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
			isUp = false;
			break;
		}
		if(reply->m_messageType != REQUEST_MESSAGE )
		{
			//Received the wrong kind of message
			reply->DeleteContents();
			delete reply;
			isUp = false;
			break;
		}

		RequestMessage *pong = (RequestMessage*)reply;
		if(pong->m_contents.m_requesttype() != REQUEST_PONG)
		{
			//Received the wrong kind of control message
			pong->DeleteContents();
			isUp = false;
		}
		delete pong;
	}while(0);

	if(isUp == false)
	{
		DisconnectFromNovad();
	}
	return isUp;
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
	if(requestReply->m_contents.m_requesttype() != REQUEST_UPTIME_REPLY)
	{
		//Received the wrong kind of control message
		requestReply->DeleteContents();
		delete requestReply;
		return 0;
	}

	uint64_t ret = requestReply->m_contents.m_starttime();

	delete requestReply;
	return ret;
}

vector<SuspectID_pb> GetSuspectList(enum SuspectListType listType)
{
	Ticket ticket = MessageManager::Instance().StartConversation(IPCSocketFD);

	RequestMessage request(REQUEST_SUSPECTLIST);
	request.m_contents.set_m_listtype(listType);

	vector<SuspectID_pb> ret;

	if(!MessageManager::Instance().WriteMessage(ticket, &request))
	{
		//There was an error in sending the message
		return ret;
	}

	Message *reply = MessageManager::Instance().ReadMessage(ticket);
	if(reply->m_messageType == ERROR_MESSAGE && ((ErrorMessage*)reply)->m_errorType == ERROR_TIMEOUT)
	{
		LOG(ERROR, "Timeout error when waiting for message reply", "");
		delete ((ErrorMessage*)reply);
		return ret;
	}

	if(reply->m_messageType != REQUEST_MESSAGE )
	{
		//Received the wrong kind of message
		delete reply;
		reply->DeleteContents();
		LOG(ERROR, "Recieved wrong kind of message", "");
		return ret;
	}

	RequestMessage *requestReply = (RequestMessage*)reply;
	if(requestReply->m_contents.m_requesttype() != REQUEST_SUSPECTLIST_REPLY)
	{
		//Received the wrong kind of control message
		reply->DeleteContents();
		delete reply;
		LOG(ERROR, "Recieved wrong kind of message", "");
		return ret;
	}

	//XXX: Horrible kudge. We should just return the pb object, but this may be hard
	//	this will make a deep copy of the list
	for(int i = 0; i < requestReply->m_contents.m_suspectid_size(); i++)
	{
		ret.push_back(requestReply->m_contents.m_suspectid(i));
	}
	//ret = requestReply->m_suspectList.m_list().;

	delete requestReply;
	return ret;
}

Suspect *GetSuspect(SuspectID_pb address)
{
	Ticket ticket = MessageManager::Instance().StartConversation(IPCSocketFD);

	RequestMessage request(REQUEST_SUSPECT);
	SuspectID_pb *suspectID = request.m_contents.add_m_suspectid();
	*suspectID = address;
	request.m_contents.set_m_featuremode(NO_FEATURE_DATA);

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
	if(requestReply->m_contents.m_requesttype() != REQUEST_SUSPECT_REPLY)
	{
		//Received the wrong kind of control message
		requestReply->DeleteContents();
		delete requestReply;
		return NULL;
	}

	if(requestReply->m_suspects.size() == 1)
	{
		Suspect *suspect = requestReply->m_suspects[0];
		delete requestReply;
		return suspect;
	}
	else
	{
		requestReply->DeleteContents();
		delete requestReply;
		return NULL;
	}
}

Suspect *GetSuspectWithData(SuspectID_pb address)
{
	Ticket ticket = MessageManager::Instance().StartConversation(IPCSocketFD);

	RequestMessage request(REQUEST_SUSPECT);
	SuspectID_pb *ID = request.m_contents.add_m_suspectid();
	*ID = address;
	request.m_contents.set_m_featuremode(MAIN_FEATURE_DATA);

	if(!MessageManager::Instance().WriteMessage(ticket, &request))
	{
		//There was an error in sending the message
		return NULL;
	}


	Message *reply = MessageManager::Instance().ReadMessage(ticket, IPCSocketFD);
	if(reply->m_messageType == ERROR_MESSAGE && ((ErrorMessage*)reply)->m_errorType == ERROR_TIMEOUT)
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
	if(requestReply->m_contents.m_requesttype() != REQUEST_SUSPECT_REPLY)
	{
		//Received the wrong kind of control message
		delete requestReply;
		return NULL;
	}

	if(requestReply->m_suspects.size() == 1)
	{
		Suspect *suspect = requestReply->m_suspects[0];
		delete requestReply;
		return suspect;
	}
	else
	{
		requestReply->DeleteContents();
		delete requestReply;
		return NULL;
	}
}

vector<Suspect*> GetSuspects(enum SuspectListType listType)
{
	vector<Suspect*> suspects;

	Ticket ticket = MessageManager::Instance().StartConversation(IPCSocketFD);

	RequestMessage request(REQUEST_ALL_SUSPECTS);
	request.m_contents.set_m_listtype(listType);

	if(!MessageManager::Instance().WriteMessage(ticket, &request))
	{
		//There was an error in sending the message
		return suspects;
	}

	Message *reply = MessageManager::Instance().ReadMessage(ticket);
	if(reply->m_messageType == ERROR_MESSAGE && ((ErrorMessage*)reply)->m_errorType == ERROR_TIMEOUT)
	{
		LOG(ERROR, "Timeout error when waiting for message reply", "");
		reply->DeleteContents();
		delete reply;
		return suspects;
	}

	if(reply->m_messageType != REQUEST_MESSAGE)
	{
		//Received the wrong kind of message
		reply->DeleteContents();
		delete reply;
		return suspects;
	}

	RequestMessage *requestReply = (RequestMessage*)reply;
	if(requestReply->m_contents.m_requesttype() != REQUEST_ALL_SUSPECTS_REPLY)
	{
		//Received the wrong kind of control message
		requestReply->DeleteContents();
		delete requestReply;
		return suspects;
	}

	suspects = requestReply->m_suspects;
	delete requestReply;
	return suspects;
}

}
