//============================================================================
// Name        : MessageEndpoint.h
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
// Description : Represents all the plumbing needed to hold several concurrent
//		conversations with another remote endpoint over a socket.
//============================================================================

#include "../Logger.h"
#include "MessageManager.h"
#include "MessageEndpoint.h"
#include "../Lock.h"
#include "messages/ErrorMessage.h"
#include "Ticket.h"

#include <sys/socket.h>
#include "unistd.h"
#include "string.h"
#include "stdio.h"
#include <sys/time.h>
#include "errno.h"

namespace Nova
{

MessageEndpoint::MessageEndpoint(int socketFD)
{
	pthread_mutex_init(&m_isShutdownMutex, NULL);
	pthread_mutex_init(&m_theirUsedSerialsMutex, NULL);
	pthread_mutex_init(&m_callbackRegisterMutex, NULL);
	pthread_mutex_init(&m_availableCBsMutex, NULL);
	pthread_mutex_init(&m_nextSerialMutex, NULL);

	pthread_cond_init(&m_callbackWakeupCondition, NULL);


	m_nextSerial = 0;

	m_consecutiveTimeouts = 0;

	m_isShutDown = false;
	m_socketFD = socketFD;

	//We will later do a pthread_join, so don't detach here
	pthread_create(&m_producerThread, NULL, StaticThreadHelper, this);
}

//Destructor should only be called by the callback thread, and also only while
//	the protocol lock in MessageManager is held. This is done to avoid
//	race conditions in deleting the object.
MessageEndpoint::~MessageEndpoint()
{
	//Wait for the producer thread to finish,
	// We can't have his object destroyed out from underneath him
	pthread_join(m_producerThread, NULL);

	pthread_mutex_destroy(&m_isShutdownMutex);
	pthread_mutex_destroy(&m_theirUsedSerialsMutex);
	pthread_mutex_destroy(&m_callbackRegisterMutex);
	pthread_mutex_destroy(&m_availableCBsMutex);
	pthread_mutex_destroy(&m_nextSerialMutex);

	pthread_cond_destroy(&m_callbackWakeupCondition);
}

//blocking call
Message *MessageEndpoint::PopMessage(Ticket ticket, int timeout)
{
	Message *ret;

	MessageQueue *queue = m_queues.GetByOurSerial(ticket.m_ourSerialNum);
	if(queue == NULL)
	{
		//Tried to read from a queue, but it didn't exist. We must have closed it, and then tried reading.
		//	This is a protocol error
		return new ErrorMessage(ERROR_PROTOCOL_MISTAKE);
	}

	ret = queue->PopMessage(timeout);

	if(ret->m_messageType == ERROR_MESSAGE)
	{
		ErrorMessage *error = (ErrorMessage*)ret;
		if(error->m_errorType == ERROR_TIMEOUT)
		{
			m_consecutiveTimeouts++;
			//TODO: deal with timeouts
		}
	}

	return ret;
}

uint32_t MessageEndpoint::StartConversation()
{
	uint32_t nextSerial = GetNextOurSerialNum();
	//Technically, there's a race condition here if we happen to go all the way around
	//	the 32 bit integer space in serial numbers before this next command is executed.
	//	Suffice it to say that I think it's safe as it is.
	m_queues.AddQueue(nextSerial);

	return nextSerial;
}

void *MessageEndpoint::StaticThreadHelper(void *ptr)
{
	return reinterpret_cast<MessageEndpoint*>(ptr)->ProducerThread();
}

bool MessageEndpoint::PushMessage(Message *message)
{
	if(message == NULL)
	{
		return false;
	}

	//The other endpoint must always provide a valid serial number, ignore if they don't
	if(message->m_ourSerialNumber == 0)
	{
		message->DeleteContents();
		delete message;
		return false;
	}

	//If this is a new conversation from the endpoint (new callback conversation)
	if(message->m_theirSerialNumber == 0)
	{
		//Look up to see if there's a MessageQueue for this serial
		MessageQueue *queue = m_queues.GetByTheirSerial(message->m_ourSerialNumber);

		//If this is a brand new callback message
		if(queue == NULL)
		{
			uint32_t ourSerial = GetNextOurSerialNum();
			m_queues.AddQueue(ourSerial, message->m_ourSerialNumber);

			queue = m_queues.GetByOurSerial(ourSerial);
			//It's possible to have a race condition that would delete this MessageQueue before we could access it
			if(queue == NULL)
			{
				message->DeleteContents();
				delete message;
				return false;
			}
		}
		queue->PushMessage(message);
		return true;
	}

	//If we got here, the message should be for an existing conversation
	//	(IE: Both serials should be valid)

	MessageQueue *queue = m_queues.GetByOurSerial(message->m_theirSerialNumber);
	if(queue == NULL)
	{
		//If there wasn't a MessageQueue for this serial number, then this message must be late or in error
		message->DeleteContents();
		delete message;
		return false;
	}
	queue->PushMessage(message);
	return true;
}

bool MessageEndpoint::RegisterCallback(Ticket &outTicket)
{
	//Only one thread in this function at a time
	Lock lock(&m_callbackRegisterMutex);
	{
		uint32_t nextSerial;
		{
			//Protection for the m_callbackDoWakeup bool
			Lock condLock(&m_availableCBsMutex);
			while(m_availableCBs.empty())
			{
				pthread_cond_wait(&m_callbackWakeupCondition, &m_availableCBsMutex);
			}

			//This is the first message of the protocol. This message contains the serial number we will be expecting later
			nextSerial = m_availableCBs.front();
			m_availableCBs.pop();
		}

		outTicket.m_isCallback = true;
		outTicket.m_hasInit = true;
		outTicket.m_ourSerialNum = nextSerial;
		outTicket.m_socketFD = m_socketFD;

		MessageQueue *queue = m_queues.GetByOurSerial(nextSerial);
		if(queue == NULL)
		{
			outTicket.m_theirSerialNum = 0;
		}
		outTicket.m_theirSerialNum = queue->GetTheirSerialNum();
	}

	Lock shutdownLock(&m_isShutdownMutex);
	return !m_isShutDown;
}

uint32_t MessageEndpoint::GetNextOurSerialNum()
{
	Lock lock(&m_nextSerialMutex);
	m_nextSerial++;

	//Must not be in use nor zero (we might overflow back to used serials)
	while( (m_queues.GetByOurSerial(m_nextSerial) != NULL) || (m_nextSerial == 0))
	{
		m_nextSerial++;
	}

	return m_nextSerial;
}

void *MessageEndpoint::ProducerThread()
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
			bytesRead = read(m_socketFD, buff + totalBytesRead, sizeof(length) - totalBytesRead);
			if(bytesRead <= 0)
			{
				if (bytesRead == 0)
				{
					LOG(DEBUG, "Socket was closed by peer", "");
				}
				else
				{
					LOG(DEBUG, "Socket was closed due to error: " + std::string(strerror(errno)), "");
				}

				//The socket died on us!
				{
					Lock shutdownLock(&m_isShutdownMutex);
					//Mark the queue as closed, put an error message on the queue, and quit reading
					m_isShutDown = true;
				}
				//Push an ERROR_SOCKET_CLOSED message into both queues. So that everyone knows we're closed
				PushMessage(new ErrorMessage(ERROR_SOCKET_CLOSED));
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
		if(length == 0)
		{
			PushMessage(new ErrorMessage(ERROR_MALFORMED_MESSAGE));
			continue;
		}

		length -= sizeof(length);
		char *buffer = (char*)malloc(length);

		if(buffer == NULL)
		{
			// This should never happen. If it does, probably because length is an absurd value (or we're out of memory)
			PushMessage(new ErrorMessage(ERROR_MALFORMED_MESSAGE));
			free(buffer);
			continue;
		}

		// Read in the actual message
		totalBytesRead = 0;
		bytesRead = 0;
		while(totalBytesRead < length)
		{
			bytesRead = read(m_socketFD, buffer + totalBytesRead, length - totalBytesRead);

			if(bytesRead <= 0)
			{
				if (bytesRead == 0)
				{
					LOG(DEBUG, "Socket was closed by peer", "");
				}
				else
				{
					LOG(DEBUG, "Socket was closed due to error: " + std::string(strerror(errno)), "");
				}

				//The socket died on us!
				{
					Lock shutdownLock(&m_isShutdownMutex);
					//Mark the queue as closed, put an error message on the queue, and quit reading
					m_isShutDown = true;
				}
				//Push an ERROR_SOCKET_CLOSED message into both queues. So that everyone knows we're closed
				PushMessage(new ErrorMessage(ERROR_SOCKET_CLOSED));
				PushMessage(new ErrorMessage(ERROR_SOCKET_CLOSED));
				free(buffer);
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
			free(buffer);
			continue;
		}

		PushMessage(Message::Deserialize(buffer, length));
		free(buffer);
		continue;
	}

	return NULL;
}

}
