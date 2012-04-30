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
#include "../Logger.h"
#include "messages/ErrorMessage.h"

#include <sys/socket.h>

//TODO: Get rid of this after debugging
#include <iostream>
using namespace std;

namespace Nova
{

MessageManager *MessageManager::m_instance = NULL;

MessageManager::MessageManager(enum ProtocolDirection direction)
{
	pthread_mutex_init(&m_queuesLock, NULL);
	pthread_mutex_init(&m_socketsLock, NULL);
	m_forwardDirection = direction;
}

void MessageManager::Initialize(enum ProtocolDirection direction)
{
	if(m_instance == NULL)
	{
		m_instance = new MessageManager(direction);
	}
}

MessageManager &MessageManager::Instance()
{
	if (m_instance == NULL)
	{
		LOG(ERROR, "Critical error in Message Manager", "Critical error in MessageManager: You must first initialize it with a direction"
				"before calling Instance()");
	}
	return *MessageManager::m_instance;
}

UI_Message *MessageManager::GetMessage(int socketFD, enum ProtocolDirection direction)
{
	UI_Message *retMessage;
	{
		Lock lock = LockQueue(socketFD);
		if(m_queues.count(socketFD) > 0)
		{
			retMessage = m_queues[socketFD]->PopMessage(direction);
		}
		else
		{
			return new ErrorMessage(ERROR_SOCKET_CLOSED);
		}
	}

	if(retMessage->m_messageType == ERROR_MESSAGE)
	{
		ErrorMessage *errorMessage = (ErrorMessage*)retMessage;
		if(errorMessage->m_errorType == ERROR_SOCKET_CLOSED)
		{
			{
				Lock lock = LockQueue(socketFD);
				delete m_queues[socketFD];
				m_queues.erase(socketFD);
			}

			cout << "DELETED SOCKET: " << socketFD << endl;

		}
	}

	return retMessage;
}

void MessageManager::StartSocket(int socketFD)
{
	cout << "Number of MessageQueues open = " << m_queues.size() << endl;

	//Initialize the socket lock if it doesn't yet exist
	{
		Lock socketLock(&m_socketsLock);
		if(m_socketLocks.count(socketFD) == 0)
		{
			//If there is no lock object here yet, initialize it
			m_socketLocks[socketFD] = new pthread_mutex_t;
			pthread_mutex_init(m_socketLocks[socketFD], NULL);
		}
	}

	//Initialize the MessageQueue if it doesn't yet exist
	{
		Lock lock = LockQueue(socketFD);
		if(m_queues.count(socketFD) == 0)
		{
			m_queues[socketFD] = new MessageQueue(socketFD, m_forwardDirection);
		}
	}
}

//Looks up the mutex in the m_queueLocks list
Lock MessageManager::LockQueue(int socketFD)
{
	pthread_mutex_t *qMutex;
	//Initialize the queue lock if it doesn't yet exist
	{
		Lock queueLock(&m_queuesLock);
		if(m_queueLocks.count(socketFD) == 0)
		{
			//If there is no lock object here yet, initialize it
			m_queueLocks[socketFD] = new pthread_mutex_t;
			pthread_mutex_init(m_queueLocks[socketFD], NULL);
		}
		qMutex = m_queueLocks[socketFD];
	}

	return Lock(qMutex);
}

Lock MessageManager::UseSocket(int socketFD)
{

	StartSocket(socketFD);

	Lock lock(&m_socketsLock);
	return Lock(m_socketLocks[socketFD]);
}

void MessageManager::CloseSocket(int socketFD)
{
	Lock lock(&m_queuesLock);

	if(m_queues.count(socketFD) > 0)
	{
		shutdown(socketFD, SHUT_RDWR);
		close(socketFD);
		cout << "CLOSED IT!: << " << socketFD << endl;
	}
}

void MessageManager::RegisterCallback(int socketFD)
{
	bool foundIt = false;
	MessageQueue *queue;
	{
		Lock lock(&m_queuesLock);

		if(m_queues.count(socketFD) > 0)
		{
			foundIt = true;
			queue = m_queues[socketFD];
		}
	}

	if(foundIt)
	{
		queue->RegisterCallback();
	}

}

}
