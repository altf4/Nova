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

#ifndef MESSAGEMANAGER_H_
#define MESSAGEMANAGER_H_

#include "messages/UI_Message.h"
#include "MessageQueue.h"

#include <map>
#include "pthread.h"

namespace Nova
{

class MessageManager
{

public:

	MessageManager &Instance();

	Nova::UI_Message *GetMessage(int socketFD);

	void StartSocket(int socketFD);

	//success/fail
	void CloseSocket(int socketFD);

private:

	static MessageManager *m_instance;

	MessageManager();

	pthread_mutex_t m_queuesMutex;
	std::map<int, MessageQueue*> m_queues;
};

}

#endif /* MESSAGEMANAGER_H_ */
