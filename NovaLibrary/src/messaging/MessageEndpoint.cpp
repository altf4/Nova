//============================================================================
// Name        : MessageEndpoint.cpp
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
#include <sys/time.h>
#include "errno.h"

#define CONSECUTIVETIMEOUTTHRESHOLD 5

namespace Nova
{

MessageEndpoint::MessageEndpoint(int socketFD, struct bufferevent *bufferevent)
{
	pthread_mutex_init(&m_isShutdownMutex, NULL);
	pthread_mutex_init(&m_theirUsedSerialsMutex, NULL);
	pthread_mutex_init(&m_callbackRegisterMutex, NULL);
	pthread_mutex_init(&m_availableCBsMutex, NULL);
	pthread_mutex_init(&m_buffereventMutex, NULL);

	pthread_cond_init(&m_callbackWakeupCondition, NULL);

	m_consecutiveTimeouts = 0;

	m_isShutDown = false;
	m_socketFD = socketFD;
	m_bufferevent = bufferevent;
}

//Destructor should only be called by the callback thread, and also only while
//	the protocol lock in MessageManager is held. This is done to avoid
//	race conditions in deleting the object.
MessageEndpoint::~MessageEndpoint()
{
	pthread_mutex_destroy(&m_isShutdownMutex);
	pthread_mutex_destroy(&m_theirUsedSerialsMutex);
	pthread_mutex_destroy(&m_callbackRegisterMutex);
	pthread_mutex_destroy(&m_availableCBsMutex);
	pthread_mutex_destroy(&m_buffereventMutex);

	pthread_cond_destroy(&m_callbackWakeupCondition);
}

//blocking call
Message *MessageEndpoint::PopMessage(Ticket &ticket, int timeout)
{
	//TODO: This is not quite right. We should keep returning valid messages as long as
	//we have them. We should ask the MessageQueue for a new message (maybe peek?)
	//and only return a shutdown message if one doesn't exist there.
	//If we're shut down, then return a shutdown message


	{
		Lock shutdownLock(&m_isShutdownMutex);
		if(m_isShutDown)
		{
				Lock shutdownLock(&m_isShutdownMutex);
				return new ErrorMessage(ERROR_SOCKET_CLOSED);
		}
	}

	Message *ret = m_queues.PopMessage(timeout, ticket.m_ourSerialNum);

	if(ret->m_messageType == ERROR_MESSAGE)
	{
		if(((ErrorMessage*)ret)->m_errorType == ERROR_TIMEOUT)
		{
			m_consecutiveTimeouts++;
			if(m_consecutiveTimeouts > CONSECUTIVETIMEOUTTHRESHOLD)
			{
				delete ret;
				return new ErrorMessage(ERROR_SOCKET_CLOSED);
			}
		}
	}
	return ret;
}

uint32_t MessageEndpoint::StartConversation()
{
	uint32_t nextSerial = m_queues.GetNextOurSerialNum();
	//Technically, there's a race condition here if we happen to go all the way around
	//	the 32 bit integer space in serial numbers before this next command is executed.
	//	Suffice it to say that I think it's safe as it is.
	m_queues.AddQueue(nextSerial);

	return nextSerial;
}

bool MessageEndpoint::PushMessage(Message *message)
{
	if(message == NULL)
	{
		return false;
	}

	{
		Lock shutdownLock(&m_isShutdownMutex);
		if(m_isShutDown)
		{
			message->DeleteContents();
			delete message;
			return false;
		}
	}

	//The other endpoint must always provide a valid "our" serial number, ignore if they don't
	if(message->m_ourSerialNumber == 0)
	{
		message->DeleteContents();
		delete message;
		return false;
	}

	uint32_t newSerial = 0;
	enum PushSuccess pushRet = m_queues.PushMessage(message, newSerial);
	if(pushRet == PUSH_FAIL)
	{
		return false;
	}
	else if(pushRet == PUSH_SUCCESS_NEW)
	{
		Lock lock(&m_availableCBsMutex);
		m_availableCBs.push(newSerial);
		pthread_cond_signal(&m_callbackWakeupCondition);
		return true;
	}
	else if(pushRet == PUSH_SUCCESS_OLD)
	{
		return true;
	}

	//Technically possible, this function would throw a warning without this case
	return false;
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

			while(m_availableCBs.empty() && !m_isShutDown)
			{
				pthread_cond_wait(&m_callbackWakeupCondition, &m_availableCBsMutex);
			}

			if(m_isShutDown)
			{
				return false;
			}
			//This is the first message of the protocol. This message contains the serial number we will be expecting later
			nextSerial = m_availableCBs.front();
			m_availableCBs.pop();
		}

		outTicket.m_isCallback = true;
		outTicket.m_hasInit = true;
		outTicket.m_ourSerialNum = nextSerial;
		outTicket.m_socketFD = m_socketFD;
		outTicket.m_endpoint = this;

		outTicket.m_theirSerialNum = m_queues.GetTheirSerialNum(nextSerial);
	}

	return true;
}

void MessageEndpoint::Shutdown()
{
	//Shut... down... EVERYTHING
	m_queues.Shutdown();

	Lock condLock(&m_availableCBsMutex);

	{
		Lock shutdownLock(&m_isShutdownMutex);
		m_isShutDown = true;
	}

	pthread_cond_signal(&m_callbackWakeupCondition);
}

bool MessageEndpoint::RemoveMessageQueue(uint32_t ourSerial)
{
	return m_queues.RemoveQueue(ourSerial);
}

Lock MessageEndpoint::LockBufferevent(struct bufferevent **bufferevent)
{
	*bufferevent = m_bufferevent;
	return Lock(&m_buffereventMutex);
}

}
