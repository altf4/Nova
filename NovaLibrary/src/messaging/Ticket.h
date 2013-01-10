//============================================================================
// Name        : Ticket.h
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
// Description : Manages unique IDs for a user of the MessageManager. Represents
//		a "ticket" as one might receive from the DMV. It holds your spot in line
//		and lets the system let you know when a new message has been received for you.
//============================================================================

#ifndef TICKET_H_
#define TICKET_H_

#include "MessageEndpoint.h"

#include "pthread.h"

namespace Nova
{

//Forward declaration to resolve mutual inclusion with MessageEndpoint
class MessageEndpoint;

class Ticket
{

public:

	Ticket();

	//NOTE: pthread_rwlock_t * MUST be read-locked when passing in
	Ticket(uint32_t, uint32_t, bool, bool, int, pthread_rwlock_t *,  MessageEndpoint *);

	~Ticket();

	uint32_t m_ourSerialNum;
	uint32_t m_theirSerialNum;
	bool m_isCallback;
	bool m_hasInit;
	int m_socketFD;
	pthread_rwlock_t *m_endpointLock;
	MessageEndpoint *m_endpoint;

};

}


#endif /* TICKET_H_ */
