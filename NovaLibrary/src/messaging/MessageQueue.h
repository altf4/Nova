//============================================================================
// Name        : MessageQueue.h
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
// Description : An item in the MessageManager's table. Contains a pair of queues
//	of received messages on a particular socket
//============================================================================

#ifndef MESSAGEQUEUE_H_
#define MESSAGEQUEUE_H_

#include "messages/Message.h"

#include "pthread.h"
#include <queue>

namespace Nova
{

class MessageQueue
{

public:

	MessageQueue(uint32_t ourSerial);
	~MessageQueue();

	//Functions for pushing and popping messages off the Message queue
	Message *PopMessage(int timeout);
	bool PushMessage(Message *message);

	uint32_t GetTheirSerialNum();
	uint32_t GetOurSerialNum();

	//Shuts down MessageQueue, also wakes up any reading threads
	void Shutdown();

private:

	void SetTheirSerialNum(uint32_t serial);

	std::queue<Message*> m_queue;
	pthread_mutex_t m_queueMutex;

	pthread_cond_t m_popWakeupCondition;

	uint32_t m_theirSerialNum;
	pthread_mutex_t m_theirSerialNumMutex;

	uint32_t m_ourSerialNum;
	pthread_mutex_t m_ourSerialNumMutex;

	bool m_isShutdown;

};

}

#endif /* MESSAGEQUEUE_H_ */
