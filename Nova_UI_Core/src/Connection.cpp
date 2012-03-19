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

#include "messages/ControlMessage.h"
#include "StatusQueries.h"
#include "Connection.h"
#include "NovaUtil.h"
#include "Logger.h"
#include "Config.h"

#include <string>
#include <cerrno>
#include <sys/un.h>
#include <sys/socket.h>
#include <boost/format.hpp>

//Socket communication variables
int UI_parentSocket = -1, UI_ListenSocket = -1, novadListenSocket = -1;
struct sockaddr_un UI_Address, novadAddress;

bool callbackInitialized = false;

using namespace std;
using namespace Nova;
using boost::format;

//Initializes the Callback socket. IE: The socket the UI listens on
//	NOTE: This is run internally and not meant to be executed by the user
//	returns - true if socket successfully initialized, false on error (such as another UI already listening)
bool InitCallbackSocket()
{
	//Builds the key path
	string homePath = Nova::Config::Inst()->GetPathHome();
	string key = homePath;
	key += "/keys";
	key += UI_LISTEN_FILENAME;

	//Builds the address
	UI_Address.sun_family = AF_UNIX;
	strcpy(UI_Address.sun_path, key.c_str());
	unlink(UI_Address.sun_path);

	if((UI_parentSocket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		LOG(ERROR, (format("File %1% at line %2%: socket: %3%")%__FILE__%__LINE__%strerror(errno)).str());
		close(UI_parentSocket);
		return false;
	}
	socklen_t len = sizeof(UI_Address);

	if(::bind(UI_parentSocket,(struct sockaddr *)&UI_Address, len) == -1)
	{
		LOG(ERROR, (format("File %1% at line %2%: bind: %3%")%__FILE__%__LINE__%strerror(errno)).str());
		close(UI_parentSocket);
		return false;
	}

	if(listen(UI_parentSocket, SOCKET_QUEUE_SIZE) == -1)
	{
		LOG(ERROR, (format("File %1% at line %2%: listen: %3%")%__FILE__%__LINE__%strerror(errno)).str());
		close(UI_parentSocket);
		return false;
	}

	callbackInitialized = true;
	return true;
}

bool Nova::ConnectToNovad()
{
	if(!callbackInitialized)
	{
		if(!InitCallbackSocket())
		{
			LOG(ERROR, (format("File %1% at line %2%: InitCallbackSocket failed.")%__FILE__%__LINE__).str());
			return false;
		}
	}

	if(IsUp())
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

	if((novadListenSocket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		LOG(ERROR, (format("File %1% at line %2%: socket: %3%")%__FILE__%__LINE__%strerror(errno)).str());
		return false;
	}

	if(connect(novadListenSocket, (struct sockaddr *)&novadAddress, sizeof(novadAddress)) == -1)
	{
		LOG(ERROR, (format("File %1% at line %2%: connect: %3%")%__FILE__%__LINE__%strerror(errno)).str());
		close(novadListenSocket);
		return false;
	}

	ControlMessage connectRequest;
	connectRequest.m_controlType = CONTROL_CONNECT_REQUEST;
	if(!UI_Message::WriteMessage(&connectRequest, novadListenSocket))
	{
		LOG(ERROR, (format("File %1% at line %2%: Message: %3%")%__FILE__%__LINE__%strerror(errno)).str());
		close(novadListenSocket);
		return false;
	}

	//Wait for a connection on the callback socket
	int len = sizeof(struct sockaddr_un);
	UI_ListenSocket = accept(UI_parentSocket, (struct sockaddr *)&UI_Address, (socklen_t*)&len);
	if (UI_ListenSocket == -1)
	{
		LOG(ERROR, (format("File %1% at line %2%: accept: %3%")%__FILE__%__LINE__%strerror(errno)).str());
		close(UI_ListenSocket);
		return false;
	}

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
		delete connectionReply;
		close(novadListenSocket);
		return false;
	}
	bool replySuccess = connectionReply->m_success;
	delete connectionReply;

	return replySuccess;
}


bool Nova::TryWaitConenctToNovad(int timeout_ms)
{
	if(!callbackInitialized)
	{
		if(!InitCallbackSocket())
		{
			LOG(ERROR, (format("File %1% at line %2%: InitCallbackSocket Failed")%__FILE__%__LINE__).str());
			return false;
		}
	}

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

bool Nova::CloseNovadConnection()
{
	bool success = true;
	callbackInitialized = false;

	ControlMessage disconnectNotice;
	disconnectNotice.m_controlType = CONTROL_DISCONNECT_NOTICE;
	if(!UI_Message::WriteMessage(&disconnectNotice, novadListenSocket))
	{
		success = false;
	}

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
				delete connectionReply;
				success = false;
			}
		}

	}


	if(UI_ListenSocket != -1 && close(UI_ListenSocket))
	{
		LOG(ERROR, (format("File %1% at line %2%: close: %3%")%__FILE__%__LINE__%strerror(errno)).str());
		close(UI_ListenSocket);
		success = false;
	}

	if(novadListenSocket != -1 && close(novadListenSocket))
	{
		LOG(ERROR, (format("File %1% at line %2%: close: %3%")%__FILE__%__LINE__%strerror(errno)).str());
		close(novadListenSocket);
		success = false;
	}

	UI_ListenSocket = -1;
	novadListenSocket = -1;

	return success;
}
