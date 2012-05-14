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
#include "messaging/MessageManager.h"
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
int IPCSocketFD = -1;

namespace Nova
{

void InitializeUI()
{
	MessageManager::Initialize(DIRECTION_TO_NOVAD);
}

bool ConnectToNovad()
{
	if(IsNovadUp(false))
	{
		return true;
	}

	Lock lock = MessageManager::Instance().UseSocket(IPCSocketFD);

	//Builds the key path
	string key = Config::Inst()->GetPathHome();
	key += "/keys";
	key += NOVAD_LISTEN_FILENAME;

	//Builds the address
	struct sockaddr_un novadAddress;
	novadAddress.sun_family = AF_UNIX;
	strcpy(novadAddress.sun_path, key.c_str());

	if((IPCSocketFD = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		LOG(ERROR, " socket: "+string(strerror(errno))+".", "");
		return false;
	}

	if(connect(IPCSocketFD, (struct sockaddr *)&novadAddress, sizeof(novadAddress)) == -1)
	{
		LOG(DEBUG, " connect: "+string(strerror(errno))+".", "");
		close(IPCSocketFD);
		IPCSocketFD = -1;
		return false;
	}

	MessageManager::Instance().StartSocket(IPCSocketFD);

	ControlMessage connectRequest(CONTROL_CONNECT_REQUEST, DIRECTION_TO_NOVAD);
	if(!Message::WriteMessage(&connectRequest, IPCSocketFD))
	{
		LOG(ERROR, " Message: "+string(strerror(errno))+".", "");
		close(IPCSocketFD);
		IPCSocketFD = -1;
		return false;
	}

	Message *reply = Message::ReadMessage(IPCSocketFD, DIRECTION_TO_NOVAD, REPLY_TIMEOUT);
	if (reply->m_messageType == ERROR_MESSAGE && ((ErrorMessage*)reply)->m_errorType == ERROR_TIMEOUT)
	{
		LOG(ERROR, "Timeout error when waiting for message reply", "");
		delete ((ErrorMessage*)reply);
		return false;
	}

	if(reply->m_messageType != CONTROL_MESSAGE)
	{
		delete reply;
		close(IPCSocketFD);
		IPCSocketFD = -1;
		return false;
	}
	ControlMessage *connectionReply = (ControlMessage*)reply;
	if(connectionReply->m_controlType != CONTROL_CONNECT_REPLY)
	{
		delete connectionReply;
		close(IPCSocketFD);
		IPCSocketFD = -1;
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
	Lock lock = MessageManager::Instance().UseSocket(IPCSocketFD);

	if(IPCSocketFD == -1)
	{
		return true;
	}

	bool success = true;

	ControlMessage disconnectNotice(CONTROL_DISCONNECT_NOTICE, DIRECTION_TO_NOVAD);
	if(!Message::WriteMessage(&disconnectNotice, IPCSocketFD))
	{
		success = false;
	}

	Message *reply = Message::ReadMessage(IPCSocketFD, DIRECTION_TO_NOVAD, REPLY_TIMEOUT);
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

	MessageManager::Instance().CloseSocket(IPCSocketFD);
	IPCSocketFD = -1;
	return success;
}
}
