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

#include <sys/un.h>
#include <sys/socket.h>
#include <string>
#include "stdlib.h"
#include "syslog.h"
#include <cerrno>

using namespace std;

//Socket communication variables
int UI_ListenSocket = 0, novadListenSocket = 0;
struct sockaddr_un UI_Address, novadAddress;

bool Nova::ConnectToNovad()
{
	//Builds the key path
	string homePath = Config::Inst()->getPathHome();
	string key = homePath;
	key += "/key/";
	key += NOVAD_LISTEN_FILENAME;

	//Builds the address
	UI_Address.sun_family = AF_UNIX;
	strcpy(UI_Address.sun_path, key.c_str());
	unlink(UI_Address.sun_path);

	//Builds the key path
	key = homePath;
	key += "/key/";
	key += NOVAD_LISTEN_FILENAME;

	//Builds the address
	novadAddress.sun_family = AF_UNIX;
	strcpy(novadAddress.sun_path, key.c_str());

	if((UI_ListenSocket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d socket: %s", __FILE__, __LINE__, strerror(errno));
		close(UI_ListenSocket);
		return false;
	}

	int len = strlen(UI_Address.sun_path) + sizeof(UI_Address.sun_family);

	if(bind(UI_ListenSocket,(struct sockaddr *)&UI_Address,len) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d bind: %s", __FILE__, __LINE__, strerror(errno));
		close(UI_ListenSocket);
		return false;
	}

	if(listen(UI_ListenSocket, SOCKET_QUEUE_SIZE) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d listen: %s", __FILE__, __LINE__, strerror(errno));
		close(UI_ListenSocket);
		return false;
	}


	if((novadListenSocket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d socket: %s", __FILE__, __LINE__, strerror(errno));
		close(UI_ListenSocket);
		return false;
	}

	if(connect(novadListenSocket, (struct sockaddr *)&novadAddress, sizeof(novadAddress)) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d connect: %s", __FILE__, __LINE__, strerror(errno));
		close(UI_ListenSocket);
		close(novadListenSocket);
		return false;
	}

	return true;
}

bool Nova::CloseNovadConnection()
{
	if(close(UI_ListenSocket))
	{
		syslog(SYSL_ERR, "File: %s Line: %d close: %s", __FILE__, __LINE__, strerror(errno));
		close(UI_ListenSocket);
		return false;
	}

	if(close(novadListenSocket))
	{
		syslog(SYSL_ERR, "File: %s Line: %d close: %s", __FILE__, __LINE__, strerror(errno));
		close(novadListenSocket);
		return false;
	}
	return true;
}
