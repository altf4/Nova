//============================================================================
// Name        : Connection.cpp
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
// Description : Manages connections out to Novad, initializes and closes them
//============================================================================

#include "messaging/messages/ControlMessage.h"
#include "messaging/messages/ErrorMessage.h"
#include "messaging/MessageManager.h"
#include "Commands.h"
#include "NovaUtil.h"
#include "Logger.h"
#include "Config.h"
#include "Lock.h"

#include <unistd.h>
#include <string>
#include <cerrno>
#include <sys/un.h>
#include <sys/socket.h>
#include "event.h"
#include "event2/thread.h"

using namespace std;
using namespace Nova;
//Socket communication variables
int IPCSocketFD = -1;

static struct event_base *libeventBase = NULL;
static struct bufferevent *bufferevent = NULL;
pthread_mutex_t bufferevent_mutex;				//Mutex for the bufferevent pointer (not object)
pthread_t eventDispatchThread;

namespace Nova
{

void *EventDispatcherThread(void *arg)
{
	int ret = event_base_dispatch(libeventBase);
	if(ret != 0)
	{
		stringstream ss;
		ss << ret;
		//LOG(DEBUG, "Message loop ended. Error code: " + ss.str(), "");
	}
	else
	{
		LOG(DEBUG, "Message loop ended cleanly.", "");
	}

	DisconnectFromNovad();
	return NULL;
}

bool ConnectToNovad()
{
	if(IsNovadUp(false))
	{
		return true;
	}

	DisconnectFromNovad();

	//Create new base
	if(libeventBase == NULL)
	{
		evthread_use_pthreads();
		libeventBase = event_base_new();
		pthread_mutex_init(&bufferevent_mutex, NULL);
	}

	//Builds the key path
	string key = Config::Inst()->GetPathHome();
	key += "/config/keys";
	key += NOVAD_LISTEN_FILENAME;

	//Builds the address
	struct sockaddr_un novadAddress;
	novadAddress.sun_family = AF_UNIX;
	memset(novadAddress.sun_path, '\0', sizeof(novadAddress.sun_path));
	strncpy(novadAddress.sun_path, key.c_str(), sizeof(novadAddress.sun_path));

	{
		Lock buffereventLock(&bufferevent_mutex);
		bufferevent = bufferevent_socket_new(libeventBase, -1,
				BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE |  BEV_OPT_UNLOCK_CALLBACKS | BEV_OPT_DEFER_CALLBACKS );
		if(bufferevent == NULL)
		{
			LOG(ERROR, "Unable to create a socket to Nova", "");
			DisconnectFromNovad();
			return false;
		}

		bufferevent_setcb(bufferevent, MessageManager::MessageDispatcher, NULL, MessageManager::ErrorDispatcher, NULL);

		if(bufferevent_enable(bufferevent, EV_READ) == -1)
		{
			LOG(ERROR, "Unable to enable socket events", "");
			return false;
		}

		if(bufferevent_socket_connect(bufferevent, (struct sockaddr *)&novadAddress, sizeof(novadAddress)) == -1)
		{
			bufferevent = NULL;
			LOG(DEBUG, "Unable to connect to NOVAD: "+string(strerror(errno))+".", "");
			return false;
		}

		IPCSocketFD = bufferevent_getfd(bufferevent);
		if(IPCSocketFD == -1)
		{
			LOG(DEBUG, "Unable to connect to Novad: "+string(strerror(errno))+".", "");
			bufferevent_free(bufferevent);
			bufferevent = NULL;
			return false;
		}

		if(evutil_make_socket_nonblocking(IPCSocketFD) == -1)
		{
			LOG(DEBUG, "Unable to connect to Novad", "Could not set socket to non-blocking mode");
			bufferevent_free(bufferevent);
			bufferevent = NULL;
			return false;
		}

		MessageManager::Instance().DeleteEndpoint(IPCSocketFD);
		MessageManager::Instance().StartSocket(IPCSocketFD, bufferevent);
	}
	pthread_create(&eventDispatchThread, NULL, EventDispatcherThread, NULL);

	//Send a connection request
	Ticket ticket = MessageManager::Instance().StartConversation(IPCSocketFD);

	ControlMessage connectRequest(CONTROL_CONNECT_REQUEST);
	if(!MessageManager::Instance().WriteMessage(ticket, &connectRequest))
	{
		return false;
	}

	Message *reply = MessageManager::Instance().ReadMessage(ticket);
	if(reply->m_messageType == ERROR_MESSAGE && ((ErrorMessage*)reply)->m_errorType == ERROR_TIMEOUT)
	{
		LOG(ERROR, "Timeout error when waiting for message reply", "");
		delete reply;
		return false;
	}

	if(reply->m_messageType != CONTROL_MESSAGE)
	{
		stringstream s;
		s << "Expected message type of CONTROL_MESSAGE but got " << reply->m_messageType;
		LOG(ERROR, s.str(), "");

		reply->DeleteContents();
		delete reply;
		return false;
	}
	ControlMessage *connectionReply = (ControlMessage*)reply;
	if(connectionReply->m_contents.m_controltype() != CONTROL_CONNECT_REPLY)
	{
		stringstream s;
		s << "Expected control type of CONTROL_CONNECT_REPLY but got " <<connectionReply->m_contents.m_controltype();
		LOG(ERROR, s.str(), "");

		reply->DeleteContents();
		delete reply;
		return false;
	}
	bool replySuccess = connectionReply->m_contents.m_success();
	delete connectionReply;

	return replySuccess;
}

void DisconnectFromNovad()
{
	//Close out any possibly remaining socket artifacts
	if(libeventBase != NULL)
	{
		if(eventDispatchThread != 0)
		{
			if(event_base_loopbreak(libeventBase) == -1)
			{
				LOG(WARNING, "Unable to exit event loop", "");
			}
			pthread_join(eventDispatchThread, NULL);
			eventDispatchThread = 0;
		}
	}

	{
		Lock buffereventLock(&bufferevent_mutex);
		if(bufferevent != NULL)
		{
			//bufferevent_free(bufferevent);
			shutdown(IPCSocketFD, 2);
			bufferevent = NULL;
		}
	}

	MessageManager::Instance().DeleteEndpoint(IPCSocketFD);

	IPCSocketFD = -1;
}

bool TryWaitConnectToNovad(int timeout_ms)
{
	if(ConnectToNovad())
	{
		return true;
	}
	else
	{
		//usleep takes in microsecond argument. Convert to milliseconds
		usleep(timeout_ms *1000);
		return ConnectToNovad();
	}
}

bool CloseNovadConnection()
{
	bool success = true;
	//Keep the scope of the following Ticket out of the call to Disconnect
	{
		Ticket ticket = MessageManager::Instance().StartConversation(IPCSocketFD);

		if(IPCSocketFD == -1)
		{
			return true;
		}

		ControlMessage disconnectNotice(CONTROL_DISCONNECT_NOTICE);
		if(!MessageManager::Instance().WriteMessage(ticket, &disconnectNotice))
		{
			success = false;
		}

		Message *reply = MessageManager::Instance().ReadMessage(ticket);
		if(reply->m_messageType == ERROR_MESSAGE && ((ErrorMessage*)reply)->m_errorType == ERROR_TIMEOUT)
		{
			LOG(ERROR, "Timeout error when waiting for message reply", "");
			reply->DeleteContents();
			delete reply;
			success = false;
		}
		else if(reply->m_messageType != CONTROL_MESSAGE)
		{
			reply->DeleteContents();
			delete reply;
			success = false;
		}
		else
		{
			ControlMessage *connectionReply = (ControlMessage*)reply;
			if(connectionReply->m_contents.m_controltype() != CONTROL_DISCONNECT_ACK)
			{
				success = false;
			}
			connectionReply->DeleteContents();
			delete connectionReply;
		}
	}
	DisconnectFromNovad();
	LOG(DEBUG, "Call to CloseNovadConnection complete", "");
	return success;
}

}
