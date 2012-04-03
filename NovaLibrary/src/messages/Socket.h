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
// Description : A synchronized socket class
//============================================================================

#ifndef SOCKET_H_
#define SOCKET_H_

#include "pthread.h"

#include "../Lock.h"

namespace Nova
{

class Socket
{
public:

	Socket(int socketFD)
	{
		m_socketFD = socketFD;
		pthread_mutex_init(&m_mutex, NULL);
	}

	int m_socketFD;

private:

	pthread_mutex_t m_mutex;
};

}


#endif /* SOCKET_H_ */
