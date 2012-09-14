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
#include <unistd.h>

namespace Nova
{

MessageManager *MessageManager::m_instance = NULL;

MessageManager::MessageManager()
{
	pthread_mutex_init(&m_endpointsMutex, NULL);
	pthread_mutex_init(&m_deleteEndpointMutex, NULL);
}

MessageManager &MessageManager::Instance()
{
	if(m_instance == NULL)
	{
		m_instance = new MessageManager();
	}
	return *MessageManager::m_instance;
}

Message *MessageManager::ReadMessage(Ticket &ticket, int timeout)
{
	//Read lock the Endpoint (so it can't get deleted from us while we're using it)
	MessageEndpointLock endpointLock = GetEndpoint(ticket.m_socketFD);

	if(endpointLock.m_endpoint == NULL)
	{
		return new ErrorMessage(ERROR_SOCKET_CLOSED);
	}

	Message *message = endpointLock.m_endpoint->PopMessage(ticket, timeout);
	if(message->m_messageType == ERROR_MESSAGE)
	{
		ErrorMessage *errorMessage = (ErrorMessage*)message;
		if(errorMessage->m_errorType == ERROR_SOCKET_CLOSED)
		{
			CloseSocket(ticket.m_socketFD);
		}
	}

	return message;
}

bool MessageManager::WriteMessage(const Ticket &ticket, Message *message)
{
	if(ticket.m_socketFD == -1)
	{
		return false;
	}

	message->m_ourSerialNumber = ticket.m_ourSerialNum;
	message->m_theirSerialNumber = ticket.m_theirSerialNum;

	uint32_t length;
	char *buffer = message->Serialize(&length);

	// Total bytes of a write() call that need to be sent
	uint32_t bytesSoFar;

	// Return value of the write() call, actual bytes sent
	int32_t bytesWritten;

	// Send the message
	bytesSoFar = 0;
	while(bytesSoFar < length)
	{
		bytesWritten = write(ticket.m_socketFD, buffer, length - bytesSoFar);
		if(bytesWritten < 0)
		{
			free(buffer);
			return false;
		}
		else
		{
			bytesSoFar += bytesWritten;
		}
	}

	free(buffer);
	return true;
}

void MessageManager::StartSocket(int socketFD)
{
	//Initialize the MessageEndpoint if it doesn't yet exist
	Lock lock(&m_endpointsMutex);
	if(m_endpoints.count(socketFD) == 0)
	{
		pthread_rwlock_t *rwLock = new pthread_rwlock_t;
		pthread_rwlock_init(rwLock, NULL);

		m_endpoints[socketFD] = std::pair<MessageEndpoint*, pthread_rwlock_t*>( new MessageEndpoint(socketFD), rwLock);
	}
	else
	{
		if(m_endpoints[socketFD].first == NULL)
		{
			m_endpoints[socketFD].first = new MessageEndpoint(socketFD);
		}
	}
}

Ticket MessageManager::StartConversation(int socketFD)
{
	Lock lock(&m_endpointsMutex);

	//If the endpoint doesn't exist, then it was closed. Just exit with failure
	if(m_endpoints.count(socketFD) == 0)
	{
		return Ticket();
	}

	if(m_endpoints[socketFD].first == NULL)
	{
		return Ticket();
	}

	return Ticket(m_endpoints[socketFD].first->StartConversation(), 0, false, false, socketFD);
}

void MessageManager::DeleteEndpoint(int socketFD)
{
	//Ensure that only one thread can be deleting an Endpoint at a time
	Lock functionLock(&m_deleteEndpointMutex);

	//Deleting the message endpoint requires that nobody else is using it!
	Lock lock(&m_endpointsMutex);
	if(m_endpoints.count(socketFD) > 0)
	{
		Lock lock(m_endpoints[socketFD].second, WRITE_LOCK);

		//We can't hold the endpointsMutex while doing a delete here, or else we get a deadlock race condition.
		//	The destructor does a join on a thread that can grab the mutex.
		//	So we need to release this lock just for the duration of the delete.
		//	Note that this is safe because no changes are made to the m_endpoints variable itself. Just the data
		//	pointed to by a variable in it. Furthermore, we hold the Endpoint's write lock, so we're allowed.
		pthread_mutex_unlock(&m_endpointsMutex);
		delete m_endpoints[socketFD].first;
		pthread_mutex_lock(&m_endpointsMutex);

		//This command, however does make a change to the m_endpoints map, so we need to have a lock on the mutex
		m_endpoints[socketFD].first = NULL;
	}
}

void MessageManager::CloseSocket(int socketFD)
{
	if(shutdown(socketFD, SHUT_RDWR) == -1)
	{
		LOG(DEBUG, "Failed to shut down socket", "");
	}

	if(close(socketFD) == -1)
	{
		LOG(DEBUG, "Failed to close socket", "");
	}
}

bool MessageManager::RegisterCallback(int socketFD, Ticket &outTicket)
{
	MessageEndpointLock endpointLock = GetEndpoint(socketFD);
	if(endpointLock.m_endpoint != NULL)
	{
		return endpointLock.m_endpoint->RegisterCallback(outTicket);
	}

	return false;
}

std::vector <int>MessageManager::GetSocketList()
{
	Lock lock(&m_endpointsMutex);
	std::vector<int> sockets;

	std::map<int, std::pair<MessageEndpoint*, pthread_rwlock_t*>>::iterator it;
	for(it = m_endpoints.begin(); it != m_endpoints.end(); ++it)
	{
		//If the MessageEndpoint is NULL, don't count it
		if(it->second.first != NULL)
		{
			sockets.push_back(it->first);
		}
	}

	return sockets;
}

MessageEndpointLock MessageManager::GetEndpoint(int socketFD)
{
	pthread_rwlock_t *endpointLock;

	//get the rw lock for the endpoint
	{
		Lock lock(&m_endpointsMutex);

		if(m_endpoints.count(socketFD) > 0)
		{
			endpointLock = m_endpoints[socketFD].second;
		}
		else
		{
			return MessageEndpointLock();
		}
	}

	pthread_rwlock_rdlock(endpointLock);
	return MessageEndpointLock( m_endpoints[socketFD].first, endpointLock);
}

}
