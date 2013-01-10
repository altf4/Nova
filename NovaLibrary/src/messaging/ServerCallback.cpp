//============================================================================
// Name        : ServerCallback.cpp
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
// Description : Abstract parent class for user defined messaging callback thread
//		The user must define this function in a child class of their own
//============================================================================

#include "ServerCallback.h"
#include "MessageManager.h"
#include "../Logger.h"

namespace Nova
{

void ServerCallback::StartServerCallbackThread(int socketFD, struct bufferevent *bufferevent)
{
	MessageManager::Instance().DeleteEndpoint(socketFD);
	MessageManager::Instance().StartSocket(socketFD, bufferevent);

	struct ServerCallbackArg *arg = new ServerCallbackArg();
	arg->m_socketFD = socketFD;
	arg->m_callback = this;
	int err = pthread_create(&m_callbackThread, NULL, StaticThreadHelper, arg);

	if(err != 0)
	{
		LOG(WARNING, "Internal error: Thread failed to launch. Error code: " + err, "");
	}
	else
	{
		pthread_detach(m_callbackThread);
	}
}

void *ServerCallback::StaticThreadHelper(void *ptr)
{
	struct ServerCallbackArg *arg = reinterpret_cast<struct ServerCallbackArg *>(ptr);
	arg->m_callback->CallbackThread(arg->m_socketFD);
	delete arg;
	return NULL;
}

}
