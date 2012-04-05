//============================================================================
// Name        : MessageManager.h
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
// Description : Manages all incoming messages on sockets
//============================================================================

#include "MessageManager.h"
#include "../Lock.h"

namespace Nova
{

MessageManager *MessageManager::m_instance = NULL;

MessageManager::MessageManager()
{
	pthread_mutex_init(&m_queuesMutex, NULL);
}

MessageManager &MessageManager::Instance()
{
	if (m_instance == NULL)
	{
		m_instance = new MessageManager();
	}
	return *m_instance;
}

UI_Message *MessageManager::GetMessage(int socketFD)
{
	Lock lock(&m_queuesMutex);

	if(m_queues.count(socketFD) > 0)
	{
		m_queues[socketFD]->PopMessage();
	}
}

//success/fail
void MessageManager::StartSocket(int socketFD)
{
	Lock lock(&m_queuesMutex);

	if(m_queues.count(socketFD) == 0)
	{
		//TODO XXX OMG fix this later. CALLBACKS!!!
		pthread_t todo;
		m_queues[socketFD] = new MessageQueue(socketFD, todo);
	}
}

//success/fail
void MessageManager::CloseSocket(int socketFD)
{
	Lock lock(&m_queuesMutex);

	if(m_queues.count(socketFD) > 0)
	{
		delete m_queues[socketFD];
		m_queues.erase(socketFD);
	}
}

}


