//============================================================================
// Name        : ServerCallback.cpp
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
// Description : Abstract parent class for user defined messaging callback thread
//		The user must define this function in a child class of their own
//============================================================================

#include "ServerCallback.h"
#include "MessageManager.h"

namespace Nova
{


void ServerCallback::StartServerCallbackThread(int socketFD)
{
	MessageManager::Instance().DeleteEndpoint(socketFD);
	MessageManager::Instance().StartSocket(socketFD);
	m_socketFD = socketFD;
	pthread_create(&m_callbackThread, NULL, StaticThreadHelper, this);
}

void *ServerCallback::StaticThreadHelper(void *ptr)
{
	reinterpret_cast<ServerCallback*>(ptr)->CallbackThread(reinterpret_cast<ServerCallback*>(ptr)->m_socketFD);
	return NULL;
}

}
