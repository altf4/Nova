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
// Description : An item in the MessageManager's table. Contains a pair of queues
//	of received messages on a particular socket
//============================================================================

#ifndef MESSAGEQUEUE_H_
#define MESSAGEQUEUE_H_

#include "messages/UI_Message.h"

#include "pthread.h"
#include <queue>

namespace Nova
{

class MessageQueue
{
public:

	//Contructor for MessageQueue
	//	socketFD - The socket file descriptor the queue will listen on
	//	direction - The protocol direction that is considered forward
	MessageQueue(int socketFD, enum ProtocolDirection forwardDirection);

	//Destructor should only be called by the callback thread, and also only while
	//	the protocol lock in MessageManager is held. This is done to avoid
	//	race conditions in deleting the object.
	~MessageQueue();

	//Pop off a message from the specified direction
	//	ProtocolDirection - Which direction is PROTOCOL to which this message belongs initiated
	//	timeout - How long (in seconds) to wait for the message before giving up
	// Returns - A pointer to a valid UI_Message object. Never NULL. Caller is responsible for life cycle of this message
	//		On error, this function returns an ErrorMessage with the details of the error
	//		IE: Returns ErrorMessage of type ERROR_TIMEOUT if timeout has been exceeded
	//	NOTE: You must have the lock on the socket by calling UseSocket() prior to calling this
	//		(Or bad things will happen)
	//	NOTE: Blocking function
	//	NOTE: Will automatically call CloseSocket() for you if the message returned happens to be an ERROR_MESSAGE
	//		of type ERROR_SOCKET_CLOSED. So there is no need to call it again yourself
	//	NOTE: Due to physical constraints, this function may block for longer than timeout. Don't rely on it being very precise.
	UI_Message *PopMessage(enum ProtocolDirection direction, int timeout);

	//Blocks until a callback message has been received
	//	returns true if a callback message is ready and waiting for us
	//			false if message queue has been closed and needs to be deleted
	bool RegisterCallback();

private:

	//Producer thread helper. Used so that the producer thread can be a member of the MessageQueue class
	//	pthreads normally are static
	static void *StaticThreadHelper(void *ptr);

	//Pushes a new message onto the appropriate message queue
	//	message - The UI_Message to push
	//	NOTE: tThe direction of the message is read directly from the message itself
	void PushMessage(UI_Message *message);

	//Thread which continually loops, doing read() calls on the underlying socket and pushing messages read onto the queues
	//	Thread quits as soon as read fails (returns <= 0). This can be made to happen through a CloseSocket() call from MessageManager
	void *ProducerThread();

	std::queue<UI_Message*> m_forwardQueue;
	std::queue<UI_Message*> m_callbackQueue;
	bool m_isShutDown;							//TODO: Synchronize this. Needs to have a mutex around access to it

	enum ProtocolDirection m_forwardDirection;

	int m_socketFD;

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
