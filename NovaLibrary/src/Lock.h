//============================================================================
// Name        : Lock.h
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
// Description : Simple wrapper class for easy use of pthread mutex locking
//============================================================================

#ifndef LOCK_H_
#define LOCK_H_

#include "pthread.h"

namespace Nova
{

class Lock
{

public:
	Lock(pthread_mutex_t *lock)
	{
		m_lock = lock;
		pthread_mutex_lock(m_lock);
	}

	~Lock()
	{
		pthread_mutex_unlock(m_lock);
	}

private:
	pthread_mutex_t *m_lock;

};

}


#endif /* LOCK_H_ */
