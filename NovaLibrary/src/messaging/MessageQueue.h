//============================================================================
// Name        : MessageQueue.h
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

#ifndef MESSAGEQUEUE_H_
#define MESSAGEQUEUE_H_

#include "messages/UI_Message.h"
#include "Socket.h"

#include "pthread.h"
#include <queue>

namespace Nova
{

class MessageQueue
{
public:

	MessageQueue(Socket &socket);

	//Will block and wait until no threads are waiting on its queue
	//	To prevent a thread from waking up in a destroyed object
	~MessageQueue();

	//blocking call
	UI_Message *PopMessage();

	//Blocks until a callback message has been received
	void RegisterCallback();

private:

	//Producer thread, adds to queue
	static void *StaticThreadHelper(void *ptr);

	void PushMessage(UI_Message *message);

	void *ProducerThread();

	std::queue<UI_Message*> m_forwardQueue;
	std::queue<UI_Message*> m_callbackQueue;
	bool isShutDown;

	Socket &m_socket;

	pthread_cond_t m_readWakeupCondition;
	pthread_cond_t m_callbackWakeupCondition;	//Condition for sleeping for a callback
	bool m_callbackDoWakeup;					//Bool also necessary for waking up callback thread
	pthread_t m_producerThread;					//Thread looping read calls on the underlying socket

	pthread_mutex_t m_forwardQueueMutex;		//Protects access to the forward message queue
	pthread_mutex_t m_callbackQueueMutex;		//Protects access to the callback message queue
	pthread_mutex_t m_popMutex;					//Separate mutex for the pop function. Only one can be popping
	pthread_mutex_t m_callbackRegisterMutex;	//Allows only one function to be waiting for callback
	pthread_mutex_t m_callbackCondMutex;		//Protects access to m_callbackDoWakeup
};

}

#endif /* MESSAGEQUEUE_H_ */
