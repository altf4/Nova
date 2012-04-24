//============================================================================
// Name        : Socket.h
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
// Description : A synchronized socket class. Not much more than a pairing of a file descriptor and mutex
//============================================================================

#ifndef SOCKET_H_
#define SOCKET_H_

#include "pthread.h"
#include "unistd.h"
#include <sys/socket.h>

namespace Nova
{

class Socket
{
public:

	Socket()
	{
		m_socketFD = -1;
		m_mutex = new pthread_mutex_t;

		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

		pthread_mutex_init(m_mutex, &attr);
	}
	~Socket()
	{
		shutdown(m_socketFD, SHUT_RDWR);
		m_socketFD = -1;
		pthread_mutex_destroy(m_mutex);
		delete m_mutex;
	}

	int m_socketFD;
	pthread_mutex_t *m_mutex;
};

}


#endif /* SOCKET_H_ */
