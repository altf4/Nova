//============================================================================
// Name        : MessageQueue.cpp
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
// Description : An item in the MessageManager's table. Contains a queue of received
//	messages on a particular socket
//============================================================================

#include "MessageQueue.h"
#include "../Lock.h"
#include "messages/ErrorMessage.h"

#include <sys/socket.h>
#include <errno.h>
#include "unistd.h"
#include "string.h"

namespace Nova
{

MessageQueue::MessageQueue(int socketFD, pthread_t callbackHandler)
{
	pthread_mutex_init(&m_queueMutex, NULL);
	pthread_cond_init(&m_wakeupCondition, NULL);

	m_socket = socketFD;
	m_callbackThread = callbackHandler;

	pthread_create(&m_producerThread, NULL, StaticThreadHelper, this);
}

MessageQueue::~MessageQueue()
{
	shutdown(m_socket, SHUT_RDWR);
	pthread_join(m_producerThread, NULL);
	pthread_mutex_destroy(&m_queueMutex);
	pthread_cond_destroy(&m_wakeupCondition);
}


//blocking call
UI_Message *MessageQueue::PopMessage()
{
	Lock lock(&m_queueMutex);

	//While loop to protect against spurious wakeups
	while(m_messages.empty())
	{
		pthread_cond_wait(&m_wakeupCondition, &m_queueMutex);
	}

	UI_Message* retMessage = m_messages.front();
	m_messages.pop();
	return retMessage;
}

void *MessageQueue::StaticThreadHelper(void *ptr)
{
	return reinterpret_cast<MessageQueue*>(ptr)->ProducerThread();
}

void MessageQueue::PushMessage(UI_Message *message)
{
	Lock lock(&m_queueMutex);

	//Wake up a reader who may be blocked, waiting for a message
	if(m_messages.empty())
	{
		pthread_cond_signal(&m_wakeupCondition);
	}

	m_messages.push(message);
}

void *MessageQueue::ProducerThread()
{
	while(true)
	{
		uint32_t length = 0;
		char buff[sizeof(length)];
		uint totalBytesRead = 0;
		int bytesRead = 0;

		// Read in the message length
		while( totalBytesRead < sizeof(length))
		{
			bytesRead = read(m_socket, buff + totalBytesRead, sizeof(length) - totalBytesRead);

			if( bytesRead < 0 )
			{
				//The socket died on us!
				//Put an error message on the queue, and quit reading
				PushMessage(new ErrorMessage(ERROR_SOCKET_CLOSED));
				return NULL;
			}
			else
			{
				totalBytesRead += bytesRead;
			}
		}

		// Make sure the length appears valid
		// TODO: Assign some arbitrary max message size to avoid filling up memory by accident
		memcpy(&length, buff, sizeof(length));
		if (length == 0)
		{
			PushMessage(new ErrorMessage(ERROR_MALFORMED_MESSAGE));
			continue;
		}

		char *buffer = (char*)malloc(length);

		if (buffer == NULL)
		{
			// This should never happen. If it does, probably because length is an absurd value (or we're out of memory)
			PushMessage(new ErrorMessage(ERROR_MALFORMED_MESSAGE));
			continue;
		}

		// Read in the actual message
		totalBytesRead = 0;
		bytesRead = 0;
		while(totalBytesRead < length)
		{
			bytesRead = read(m_socket, buffer + totalBytesRead, length - totalBytesRead);

			if( bytesRead < 0 )
			{
				//The socket died on us!
				//Put an error message on the queue, and quit reading
				PushMessage(new ErrorMessage(ERROR_SOCKET_CLOSED));
				return NULL;
			}
			else
			{
				totalBytesRead += bytesRead;
			}
		}


		if(length < MESSAGE_MIN_SIZE)
		{
			PushMessage(new ErrorMessage(ERROR_MALFORMED_MESSAGE));
			continue;
		}

		PushMessage(UI_Message::Deserialize(buffer, length));
		continue;
	}

	return NULL;
}

}
