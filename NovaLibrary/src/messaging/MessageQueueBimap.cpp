//============================================================================
// Name        : MessageQueueBimap.cpp
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
// Description : A map with two keys (one mandatory, one optional) that
//		maintain pointers to MessageQueues.
//============================================================================

#include "MessageQueueBimap.h"
#include "messages/ErrorMessage.h"
#include "../Lock.h"
#include "../Logger.h"

namespace Nova
{

MessageQueueBimap::MessageQueueBimap()
{
	pthread_mutex_init(&m_queuesMutex, NULL);
	pthread_mutex_init(&m_nextSerialMutex, NULL);
	pthread_mutex_init(&m_locksMutex, NULL);

	m_nextSerial = 0;
}

MessageQueueBimap::~MessageQueueBimap()
{
	//Delete any straggling messages in the "our" map
	//We DON'T need to also delete anything from the "their" map since that one is optional. Therefore, its contents
	//	is a subset of the "our" map. We only need to delete what's here.
	for(std::map<uint32_t, MessageQueue*>::iterator iterator = m_ourQueues.begin(); iterator != m_ourQueues.end(); iterator++)
	{
		if(iterator->second != NULL)
		{
			delete iterator->second;
		}
	}
	m_ourQueues.clear();
	m_theirQueues.clear();

	for(std::map<uint32_t, pthread_rwlock_t *>::iterator iterator = m_queuelocks.begin(); iterator != m_queuelocks.end(); iterator++)
	{
		delete iterator->second;
	}
	m_queuelocks.clear();

	pthread_mutex_destroy(&m_queuesMutex);
	pthread_mutex_destroy(&m_nextSerialMutex);
	pthread_mutex_destroy(&m_locksMutex);
}

Message *MessageQueueBimap::PopMessage(int timeout, uint32_t ourSerial)
{
	Lock lock = ReadLockQueue(ourSerial);
	MessageQueue *queue = GetByOurSerial(ourSerial);
	if(queue == NULL)
	{
		return new ErrorMessage(ERROR_PROTOCOL_MISTAKE);
	}
	return queue->PopMessage(timeout);
}

enum PushSuccess MessageQueueBimap::PushMessage(Message *message, uint32_t &outSerial)
{
	bool isNewCallback = false;
	uint32_t newOurSerial = 0;

	Lock lock = ReadLockQueue(message->m_theirSerialNumber);
	//Try to find the message's Queue by looking it up by their serial number
	MessageQueue *queue = GetByTheirSerial(message->m_ourSerialNumber);

	//It's a brand new callback message IFF ourSerial is 0 AND there's no queue for it
	if((message->m_theirSerialNumber == 0) && (queue == NULL))
	{
		isNewCallback = true;
		newOurSerial = GetNextOurSerialNum();
		outSerial = newOurSerial;
		AddQueue(newOurSerial, message->m_ourSerialNumber);
		queue = GetByOurSerial(newOurSerial);
	}
	else if(queue == NULL)
	{
		queue = GetByOurSerial(message->m_theirSerialNumber);
		if(queue == NULL)
		{
			//If we still don't have a queue for this message, then it's an error
			//	Maybe the queue got deleted before the message came in.
			return PUSH_FAIL;
		}
	}

	//Do the actual message push
	if(!queue->PushMessage(message))
	{
		return PUSH_FAIL;
	}
	else
	{
		if(isNewCallback)
		{
			return PUSH_SUCCESS_NEW;
		}
		else
		{
			return PUSH_SUCCESS_OLD;
		}
	}
}

uint32_t MessageQueueBimap::GetTheirSerialNum(uint32_t ourSerial)
{
	Lock lock = ReadLockQueue(ourSerial);
	MessageQueue *queue = GetByOurSerial(ourSerial);
	if(queue == NULL)
	{
		return 0;
	}
	return queue->GetTheirSerialNum();
}

//TODO UNUSED?!
uint32_t MessageQueueBimap::GetOurSerialNum(uint32_t theirSerial)
{
	//TODO: Locking
	MessageQueue *queue = GetByOurSerial(theirSerial);
	if(queue == NULL)
	{
		return 0;
	}
	return queue->GetOurSerialNum();
}

//Shuts down all the MessageQueues in the bimap
void MessageQueueBimap::Shutdown()
{
//	Lock lock(&m_queuesMutex);
//
//	std::map<uint32_t, MessageQueue*>::iterator it;
//	for(it = m_ourQueues.begin(); it != m_ourQueues.end(); ++it)
//	{
//		Lock lock = ReadLockQueue(it->first);
//		it->second->Shutdown();
//	}
	vector<uint32_t> serials = GetUsedSerials();
	for(uint i = 0; i < serials.size(); i++)
	{
		Lock lock = ReadLockQueue(serials[i]);
		MessageQueue *queue = GetByOurSerial(serials[i]);
		if(queue == NULL)
		{
			continue;
		}
		queue->Shutdown();
	}
}

//NOTE: Unsynchronized functions. Make sure to lock the queue as necessary before using
MessageQueue *MessageQueueBimap::GetByOurSerial(uint32_t ourSerial)
{
	MessageQueue *queue = NULL;

	Lock lock(&m_queuesMutex);
	if(m_ourQueues.count(ourSerial) > 0)
	{
		queue = m_ourQueues[ourSerial];
	}

	return queue;
}

//NOTE: Unsynchronized functions. Make sure to lock the queue as necessary before using
MessageQueue *MessageQueueBimap::GetByTheirSerial(uint32_t theirSerial)
{
	MessageQueue *queue = NULL;

	Lock lock(&m_queuesMutex);
	if(m_theirQueues.count(theirSerial) > 0)
	{
		queue = m_theirQueues[theirSerial];
	}

	return queue;
}

Lock MessageQueueBimap::ReadLockQueue(uint32_t ourSerial)
{
	Lock lock(&m_locksMutex);

	if(m_queuelocks.count(ourSerial) == 0)
	{
		m_queuelocks[ourSerial] = new pthread_rwlock_t;
		pthread_rwlock_init(m_queuelocks[ourSerial], NULL);
	}
	return Lock(m_queuelocks[ourSerial], READ_LOCK);
}

Lock MessageQueueBimap::WriteLockQueue(uint32_t ourSerial)
{
	Lock lock(&m_locksMutex);

	if(m_queuelocks.count(ourSerial) == 0)
	{
		m_queuelocks[ourSerial] = new pthread_rwlock_t;
	}
	return Lock(m_queuelocks[ourSerial], WRITE_LOCK);
}

void MessageQueueBimap::AddQueue(uint32_t ourSerial)
{
	Lock qlock = ReadLockQueue(ourSerial);
	Lock lock(&m_queuesMutex);

	if(m_ourQueues.count(ourSerial) > 0)
	{
		//We've already got one
		return;
	}

	m_ourQueues[ourSerial] = new MessageQueue(ourSerial);
}

void MessageQueueBimap::AddQueue(uint32_t ourSerial, uint32_t theirSerial)
{
	Lock qlock = ReadLockQueue(ourSerial);
	//First, let's make sure a MessageQueue exists for the "our" serial
	Lock lock(&m_queuesMutex);

	MessageQueue *queue;
	if(m_ourQueues.count(ourSerial) > 0)
	{
		//We've already got one
		queue = m_ourQueues[ourSerial];
	}
	else
	{
		queue = new MessageQueue(ourSerial);
		m_ourQueues[ourSerial] = queue;
	}

	//Then let's hook up the "their" serial number to the same MessageQueue
	//If we've already got one...
	if(m_theirQueues.count(theirSerial) > 0)
	{
		//If the existing queue isn't the same as the new one!
		if(queue != m_theirQueues[theirSerial])
		{
			LOG(ERROR, "Mismatch in MessageQueueBimap AddQueue", "Tried to add a new MessageQueue, "
					"but there was already one here. This shouldn't happen");
			delete m_theirQueues[theirSerial];
			m_theirQueues[theirSerial] = queue;
			return;
		}
	}
	else
	{
		m_theirQueues[theirSerial] = queue;
		return;
	}
}

bool MessageQueueBimap::RemoveQueue(uint32_t ourSerial)
{
	Lock qlock = WriteLockQueue(ourSerial);
	Lock lock(&m_queuesMutex);
	if(m_ourQueues.count(ourSerial) == 0)
	{
		return false;
	}

	//Get the serial number out of the MessageQueue, then delete it
	uint32_t theirSerial = m_ourQueues[ourSerial]->GetTheirSerialNum();
	delete m_ourQueues[ourSerial];
	m_ourQueues.erase(ourSerial);

	if(theirSerial == 0)
	{
		//Zero means "no serial number"
		//	So there's no need to do anything more here
		return true;
	}

	if(m_theirQueues.count(theirSerial) == 0)
	{
		return true;
	}

	m_theirQueues.erase(theirSerial);

	return true;
}

//TODO: UNUSED?
std::vector<uint32_t> MessageQueueBimap::GetUsedSerials()
{
	Lock lock(&m_queuesMutex);
	std::vector<uint32_t> serials;
	std::map<uint32_t, MessageQueue*>::iterator it;

	for(it = m_ourQueues.begin(); it != m_ourQueues.end(); ++it)
	{
		serials.push_back(it->first);
	}

	return serials;
}

uint32_t MessageQueueBimap::GetNextOurSerialNum()
{
	Lock lock(&m_nextSerialMutex);
	m_nextSerial++;

	//Must not be in use nor zero (we might overflow back to used serials)
	while((GetByOurSerial(m_nextSerial) != NULL) || (m_nextSerial == 0))
	{
		m_nextSerial++;
	}

	return m_nextSerial;
}

}
