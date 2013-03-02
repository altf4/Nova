//============================================================================
// Name        : MessageQueueBimap.h
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

#ifndef MESSAGEQUEUEBIMAP_H_
#define MESSAGEQUEUEBIMAP_H_

#include "MessageQueue.h"
#include "../Lock.h"

#include <map>
#include "pthread.h"

namespace Nova
{

enum PushSuccess
{
	PUSH_FAIL,
	PUSH_SUCCESS_NEW,
	PUSH_SUCCESS_OLD
};

class MessageQueueBimap
{

public:

	MessageQueueBimap();
	~MessageQueueBimap();

	//Functions for pushing and popping messages off the Message queue
	Message *PopMessage(int timeout, uint32_t ourSerial);
	enum PushSuccess PushMessage(Message *message, uint32_t &outSerial);

	uint32_t GetTheirSerialNum(uint32_t ourSerial);

	//Shuts down all the MessageQueues in the bimap
	void Shutdown();

	//Makes a new MessageQueue at the given serial number
	//	ourSerial - The "our" serial number to index by
	//	theirSerial - Optional parameter that will also index by the "their" serial number
	//	NOTE: You can call the latter AddQueue() a second time to add the new "their" serial number to the MessageQueue
	//	NOTE: Safely does nothing in the event that a MessageQueue already exists at the given serial number
	void AddQueue(uint32_t ourSerial);
	void AddQueue(uint32_t ourSerial, uint32_t theirSerial);

	//Removes and deletes the MessageQueue at the given "our" serial number
	//	ourSerial - The "our" serial number of the MessageQueue to delete
	//	returns - true on successful deletion, false on error or if the MessageQueue did not exist.
	//NOTE: Safely does nothing if no MessageQueue exists at ourSerial (but returns false)
	bool RemoveQueue(uint32_t ourSerial);

	std::vector<uint32_t> GetUsedSerials();

	//Returns a new "our" serial number to use for a conversation. This is guaranteed to not currently be in use and not be 0
	//	This increments m_forwardSerialNumber
	//	NOTE: You must have the lock on the socket before using this. (Used inside UseSocket)
	uint32_t GetNextOurSerialNum();


private:

	//Returns the MessageQueue for the given "our" serial number, NULL if it doesn't exist
	MessageQueue *GetByOurSerial(uint32_t ourSerial);

	//Returns the MessageQueue for the given "their" serial number, NULL if it doesn't exist
	//	NOTE: "Their" serial numbers are not always known, and therefore you should always prefer to call
	//		GetByOurSerial() if you know the "our" serial number. Only call this function if you really
	//		don't know what the "our" serial number that you want is.
	MessageQueue *GetByTheirSerial(uint32_t theirSerial);

	Lock ReadLockQueue(uint32_t ourSerial);

	Lock WriteLockQueue(uint32_t ourSerial);

	//Message queues, uniquely identified by serial numbers
	std::map<uint32_t, MessageQueue*> m_ourQueues;
	std::map<uint32_t, MessageQueue*> m_theirQueues;
	pthread_mutex_t m_queuesMutex;

	std::map<uint32_t, pthread_rwlock_t *> m_queuelocks;
	pthread_mutex_t m_locksMutex;

	//The next serial number that will be given to a conversation
	uint32_t m_nextSerial;
	pthread_mutex_t m_nextSerialMutex;

};

}

#endif /* MESSAGEQUEUEBIMAP_H_ */
