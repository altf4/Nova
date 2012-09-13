//============================================================================
// Name        : Ticket.h
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
// Description : Manages unique IDs for a user of the MessageManager. Represents
//		a "ticket" as one might receive from the DMV. It holds your spot in line
//		and lets the system let you know when a new message has been received for you.
//============================================================================

#include "MessageManager.h"
#include "Ticket.h"

namespace Nova
{

Ticket::Ticket()
{
	m_ourSerialNum = 0;
	m_theirSerialNum = 0;
	m_isCallback = true;
	m_hasInit = false;
	m_rwMQlock = NULL;
	m_socketFD = -1;
}

Ticket::Ticket(uint32_t ourSerial, uint32_t theirSerial, bool isCallback, bool hasInit, pthread_rwlock_t *rwLock, int socketFD)
{
	m_ourSerialNum = ourSerial;
	m_theirSerialNum = theirSerial;
	m_isCallback = isCallback;
	m_hasInit = hasInit;
	m_rwMQlock = rwLock;
	m_socketFD = socketFD;
}

Ticket::~Ticket()
{
	if(m_rwMQlock != NULL)
	{
		pthread_rwlock_unlock(m_rwMQlock);
	}
	//TODO: Delete the MessageQueue this ticket used
}


}

