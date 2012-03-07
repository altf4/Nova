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

#include "Connection.h"
#include "NovaUtil.h"
#include "Config.h"
#include "messages/ControlMessage.h"

#include <sys/un.h>
#include <sys/socket.h>
#include <string>
#include "stdlib.h"
#include "syslog.h"
#include <cerrno>

using namespace std;

//Socket communication variables
int UI_parentSocket = 0, UI_ListenSocket = 0, novadListenSocket = 0;
struct sockaddr_un UI_Address, novadAddress;

bool Nova::InitCallbackSocket()
{
	//Builds the key path
	string homePath = Config::Inst()->getPathHome();
	string key = homePath;
	key += "/keys";
	key += UI_LISTEN_FILENAME;

	//Builds the address
	UI_Address.sun_family = AF_UNIX;
	strcpy(UI_Address.sun_path, key.c_str());
	unlink(UI_Address.sun_path);

	if((UI_parentSocket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d socket: %s", __FILE__, __LINE__, strerror(errno));
		close(UI_parentSocket);
		return false;
	}

	int len = strlen(UI_Address.sun_path) + sizeof(UI_Address.sun_family);

	if(bind(UI_parentSocket,(struct sockaddr *)&UI_Address,len) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d bind: %s", __FILE__, __LINE__, strerror(errno));
		close(UI_parentSocket);
		return false;
	}

	if(listen(UI_parentSocket, SOCKET_QUEUE_SIZE) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d listen: %s", __FILE__, __LINE__, strerror(errno));
		close(UI_parentSocket);
		return false;
	}

	return true;
}

bool Nova::ConnectToNovad()
{
	//Builds the key path
	string key = Config::Inst()->getPathHome();
	key += "/keys";
	key += NOVAD_LISTEN_FILENAME;

	//Builds the address
	novadAddress.sun_family = AF_UNIX;
	strcpy(novadAddress.sun_path, key.c_str());

	if((novadListenSocket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d socket: %s", __FILE__, __LINE__, strerror(errno));
		return false;
	}

	if(connect(novadListenSocket, (struct sockaddr *)&novadAddress, sizeof(novadAddress)) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d connect: %s", __FILE__, __LINE__, strerror(errno));
		close(novadListenSocket);
		return false;
	}

	ControlMessage *connectRequest = new ControlMessage();
	connectRequest->m_controlType = CONTROL_CONNECT_REQUEST;
	if(!UI_Message::WriteMessage(connectRequest, novadListenSocket))
	{
		delete connectRequest;
		syslog(SYSL_ERR, "File: %s Line: %d Message: %s", __FILE__, __LINE__, strerror(errno));
		close(novadListenSocket);
		return false;
	}
	delete connectRequest;

	UI_Message *reply = UI_Message::ReadMessage(novadListenSocket);
	if(reply == NULL)
	{
		close(novadListenSocket);
		return false;
	}
	if(reply->m_messageType != CONTROL_MESSAGE)
	{
		delete reply;
		close(novadListenSocket);
		return false;
	}
	ControlMessage *connectionReply = (ControlMessage*)reply;
	if(connectionReply->m_controlType != CONTROL_CONNECT_REPLY)
	{
		close(novadListenSocket);
		return false;
	}
	bool replySuccess = connectionReply->m_success;
	delete connectionReply;

	return replySuccess;
}

bool Nova::CloseNovadConnection()
{
	bool success = true;

	ControlMessage *disconnectNotice = new ControlMessage();
	disconnectNotice->m_controlType = CONTROL_DISCONNECT_NOTICE;
	if(!UI_Message::WriteMessage(disconnectNotice, novadListenSocket))
	{
		success = false;
	}
	delete disconnectNotice;

	UI_Message *reply = UI_Message::ReadMessage(novadListenSocket);
	if(reply == NULL)
	{
		success = false;
	}
	else
	{
		if(reply->m_messageType != CONTROL_MESSAGE)
		{
			delete reply;
			success = false;
		}
		else
		{
			ControlMessage *connectionReply = (ControlMessage*)reply;
			if(connectionReply->m_controlType != CONTROL_CONNECT_REPLY)
			{
				success = false;
			}
		}

	}

	if(close(UI_parentSocket))
	{
		syslog(SYSL_ERR, "File: %s Line: %d close: %s", __FILE__, __LINE__, strerror(errno));
		close(UI_parentSocket);
		success = false;
	}

	if(close(UI_ListenSocket))
	{
		syslog(SYSL_ERR, "File: %s Line: %d close: %s", __FILE__, __LINE__, strerror(errno));
		close(UI_ListenSocket);
		success = false;
	}

	if(close(novadListenSocket))
	{
		syslog(SYSL_ERR, "File: %s Line: %d close: %s", __FILE__, __LINE__, strerror(errno));
		close(novadListenSocket);
		success = false;
	}

	return success;
}
