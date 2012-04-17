//============================================================================
// Name        : Connection.cpp
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
// Description : Manages connections out to Novad, initializes and closes them
//============================================================================

#include "messaging/messages/ControlMessage.h"
#include "messaging/messages/ErrorMessage.h"
#include "messaging/Socket.h"
#include "Commands.h"
#include "NovaUtil.h"
#include "Logger.h"
#include "Config.h"
#include "Lock.h"

#include <string>
#include <cerrno>
#include <sys/un.h>
#include <sys/socket.h>

using namespace std;
using namespace Nova;
//Socket communication variables
Socket UI_parentSocket, UI_ListenSocket, novadListenSocket;
struct sockaddr_un UI_Address, novadAddress;

bool callbackInitialized = false;

//Initializes the Callback socket. IE: The socket the UI listens on
//	NOTE: This is run internally and not meant to be executed by the user
//	returns - true if socket successfully initialized, false on error (such as another UI already listening)
bool InitCallbackSocket()
{
	Lock lock(&UI_parentSocket.m_mutex);

	//Builds the key path
	string homePath = Nova::Config::Inst()->GetPathHome();
	string key = homePath;
	key += "/keys";
	key += UI_LISTEN_FILENAME;

	//Builds the address
	UI_Address.sun_family = AF_UNIX;
	strcpy(UI_Address.sun_path, key.c_str());
	unlink(UI_Address.sun_path);

	if((UI_parentSocket.m_socketFD = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		LOG(ERROR, " socket: "+string(strerror(errno))+".", "");
		close(UI_parentSocket.m_socketFD);
		UI_parentSocket.m_socketFD = -1;
		return false;
	}
	socklen_t len = sizeof(UI_Address);

	if(::bind(UI_parentSocket.m_socketFD,(struct sockaddr *)&UI_Address, len) == -1)
	{
		LOG(ERROR, " bind: "+string(strerror(errno))+".", "");
		close(UI_parentSocket.m_socketFD);
		UI_parentSocket.m_socketFD = -1;
		return false;
	}

	if(listen(UI_parentSocket.m_socketFD, SOCKET_QUEUE_SIZE) == -1)
	{
		LOG(ERROR, " listen: "+string(strerror(errno))+".", "");
		close(UI_parentSocket.m_socketFD);
		UI_parentSocket.m_socketFD = -1;
		return false;
	}

	callbackInitialized = true;
	return true;
}

namespace Nova
{
bool ConnectToNovad()
{
	Lock lock(&novadListenSocket.m_mutex);

	if(!callbackInitialized)
	{
		if(!InitCallbackSocket())
		{
			//No logging needed, InitCallbackSocket logs if it fails
			return false;
		}
	}

	if(IsNovadUp(false))
	{
		return true;
	}


	//Builds the key path
	string key = Config::Inst()->GetPathHome();
	key += "/keys";
	key += NOVAD_LISTEN_FILENAME;

	//Builds the address
	novadAddress.sun_family = AF_UNIX;
	strcpy(novadAddress.sun_path, key.c_str());

	if((novadListenSocket.m_socketFD = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		LOG(ERROR, " socket: "+string(strerror(errno))+".", "");
		return false;
	}

	if(connect(novadListenSocket.m_socketFD, (struct sockaddr *)&novadAddress, sizeof(novadAddress)) == -1)
	{
		LOG(DEBUG, " connect: "+string(strerror(errno))+".", "");
		close(novadListenSocket.m_socketFD);
		novadListenSocket.m_socketFD = -1;
		return false;
	}

	ControlMessage connectRequest(CONTROL_CONNECT_REQUEST, DIRECTION_TO_NOVAD);
	if(!UI_Message::WriteMessage(&connectRequest, novadListenSocket.m_socketFD))
	{
		LOG(ERROR, " Message: "+string(strerror(errno))+".", "");
		close(novadListenSocket.m_socketFD);
		novadListenSocket.m_socketFD = -1;
		return false;
	}

	//Wait for a connection on the callback socket
	int len = sizeof(struct sockaddr_un);
	UI_ListenSocket.m_socketFD = accept(UI_parentSocket.m_socketFD, (struct sockaddr *)&UI_Address, (socklen_t*)&len);
	if (UI_ListenSocket.m_socketFD == -1)
	{
		LOG(ERROR, " accept: "+string(strerror(errno))+".", "");
		close(UI_ListenSocket.m_socketFD);
		UI_ListenSocket.m_socketFD = -1;
		return false;
	}

	UI_Message *reply = UI_Message::ReadMessage(novadListenSocket.m_socketFD, DIRECTION_TO_NOVAD, REPLY_TIMEOUT);
	if (reply->m_messageType == ERROR_MESSAGE && ((ErrorMessage*)reply)->m_errorType == ERROR_TIMEOUT)
	{
		LOG(ERROR, "Timeout error when waiting for message reply", "");
		delete ((ErrorMessage*)reply);
		return false;
	}

	if(reply->m_messageType != CONTROL_MESSAGE)
	{
		delete reply;
		close(novadListenSocket.m_socketFD);
		novadListenSocket.m_socketFD = -1;
		return false;
	}
	ControlMessage *connectionReply = (ControlMessage*)reply;
	if(connectionReply->m_controlType != CONTROL_CONNECT_REPLY)
	{
		delete connectionReply;
		close(novadListenSocket.m_socketFD);
		novadListenSocket.m_socketFD = -1;
		return false;
	}
	bool replySuccess = connectionReply->m_success;
	delete connectionReply;

	return replySuccess;
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
		usleep(timeout_ms * 1000);
		return ConnectToNovad();
	}
}

bool CloseNovadConnection()
{
	Lock lock(&novadListenSocket.m_mutex);

	if((novadListenSocket.m_socketFD == -1) && (UI_ListenSocket.m_socketFD == -1))
	{
		return true;
	}

	bool success = true;
	callbackInitialized = false;

	ControlMessage disconnectNotice(CONTROL_DISCONNECT_NOTICE, DIRECTION_TO_NOVAD);
	if(!UI_Message::WriteMessage(&disconnectNotice, novadListenSocket.m_socketFD))
	{
		success = false;
	}

	UI_Message *reply = UI_Message::ReadMessage(novadListenSocket.m_socketFD, DIRECTION_TO_NOVAD, REPLY_TIMEOUT);
	if (reply->m_messageType == ERROR_MESSAGE && ((ErrorMessage*)reply)->m_errorType == ERROR_TIMEOUT)
	{
		LOG(ERROR, "Timeout error when waiting for message reply", "");
		delete ((ErrorMessage*)reply);
		return false;
	}

	if(reply->m_messageType != CONTROL_MESSAGE)
	{
		delete reply;
		success = false;
	}
	else
	{
		ControlMessage *connectionReply = (ControlMessage*)reply;
		if(connectionReply->m_controlType != CONTROL_DISCONNECT_ACK)
		{
			delete connectionReply;
			success = false;
		}
	}

	if(UI_ListenSocket.m_socketFD != -1 && close(UI_ListenSocket.m_socketFD))
	{
		LOG(ERROR, " close:"+string(strerror(errno))+".", "");
		close(UI_ListenSocket.m_socketFD);
		UI_ListenSocket.m_socketFD = -1;
		success = false;
	}

	if(novadListenSocket.m_socketFD != -1 && close(novadListenSocket.m_socketFD))
	{
		LOG(ERROR, " close:"+string(strerror(errno))+".", "");
		close(novadListenSocket.m_socketFD);
		novadListenSocket.m_socketFD = -1;
		success = false;
	}

	UI_ListenSocket.m_socketFD = -1;
	novadListenSocket.m_socketFD = -1;

	return success;
}
}
