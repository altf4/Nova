//============================================================================
// Name        : SuspectTableIterator.cpp
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
// Description : Iterator used for traversing over a Suspect Table
//============================================================================/*

#include <math.h>

#include "SuspectTableIterator.h"

using namespace std;
using namespace Nova;

namespace Nova
{

//***************************************************************************//
// - - - - - - - - - - - - SuspectTable::iterator - - - - - - - - - - - - - -
//***************************************************************************//


//Default iterator constructor
SuspectTableIterator::SuspectTableIterator(SuspectHashTable* table, vector<uint64_t>* keys, pthread_rwlock_t *lock)
: m_table_ref(*table), m_keys_ref(*keys), m_lock_ref(*lock)
{
	m_table_ref = *table;
	m_keys_ref = *keys;
	m_lock_ref = *lock;
	m_index = 0;
}

//Default iterator deconstructor
SuspectTableIterator::~SuspectTableIterator()
{

}

//Gets the Next Suspect in the table and increments the iterator
// Returns a reference to the Suspect
Suspect SuspectTableIterator::Next()
{
	pthread_rwlock_rdlock(&m_lock_ref);
	m_index++;
	if(m_index >= m_keys_ref.size())
	{
		m_index = 0;
	}

	SuspectHashTable::iterator it = m_table_ref.find(m_keys_ref[m_index]);
	if(it != m_table_ref.end())
	{
		pthread_rwlock_unlock(&m_lock_ref);
		Suspect ret = *it->second;
		return ret;
	}
	else
	{
		it = m_table_ref.begin();
		pthread_rwlock_unlock(&m_lock_ref);
		Suspect ret = *it->second;
		return ret;
	}
}

//Gets the Next Suspect in the table, does not increment the iterator
// Returns a reference to the Suspect
Suspect SuspectTableIterator::LookAhead()
{
	pthread_rwlock_rdlock(&m_lock_ref);
	SuspectHashTable::iterator it;
	//Make sure we have a valid index for the key query
	if((m_index+1) >= m_keys_ref.size())
	{
		it = m_table_ref.begin();
	}
	else
	{
		it = m_table_ref.find(m_keys_ref[m_index+1]);
	}

	//Make sure the key returned a suspect
	if(it != m_table_ref.end())
	{
		pthread_rwlock_unlock(&m_lock_ref);
		Suspect ret = *it->second;
		return ret;
	}
	else
	{
		it = m_table_ref.begin();
		pthread_rwlock_unlock(&m_lock_ref);
		Suspect ret = *it->second;
		return ret;
	}
}

//Gets the Previous Suspect in the Table and decrements the iterator
// Returns a reference to the Suspect
Suspect SuspectTableIterator::Previous()
{
	pthread_rwlock_rdlock(&m_lock_ref);
	m_index--;
	if(m_index >= m_keys_ref.size())
	{
		m_index = m_keys_ref.size()-1;
	}

	SuspectHashTable::iterator it = m_table_ref.find(m_keys_ref[m_index]);

	if(it != m_table_ref.end())
	{
		pthread_rwlock_unlock(&m_lock_ref);
		Suspect ret = *it->second;
		return ret;
	}
	else
	{
		it = m_table_ref.begin();
		pthread_rwlock_unlock(&m_lock_ref);
		Suspect ret = *it->second;
		return ret;
	}
}

//Gets the Previous Suspect in the Table, does not decrement the iterator
// Returns a reference to the Suspect
Suspect SuspectTableIterator::LookBack()
{
	pthread_rwlock_rdlock(&m_lock_ref);
	SuspectHashTable::iterator it;
	//Make sure we have a valid index for the key query
	if((m_index-1) >= m_keys_ref.size())
	{
		it = m_table_ref.begin();
		it.operator ++(m_table_ref.size()-1);
	}
	else
	{
		it = m_table_ref.find(m_keys_ref[m_index-1]);
	}

	//Make sure the key returned a suspect
	if(it != m_table_ref.end())
	{
		pthread_rwlock_unlock(&m_lock_ref);
		Suspect ret = *it->second;
		return ret;
	}
	else
	{
		it = m_table_ref.begin();
		it.operator ++(m_table_ref.size()-1);
		pthread_rwlock_unlock(&m_lock_ref);
		Suspect ret = *it->second;
		return ret;
	}
}

//Gets the Current Suspect in the Table
// Returns a reference to the Suspect
Suspect SuspectTableIterator::Current()
{
	pthread_rwlock_rdlock(&m_lock_ref);
	SuspectHashTable::iterator it;
	if((m_index >= 0) && (m_index < m_keys_ref.size()))
	{
		it = m_table_ref.find(m_keys_ref[m_index]);

		if (it != m_table_ref.end())
		{
			Suspect ret = *it->second;
			pthread_rwlock_unlock(&m_lock_ref);
			return ret;
		}
	}
	it = m_table_ref.begin();
	Suspect ret = *it->second;
	pthread_rwlock_unlock(&m_lock_ref);
	return ret;
}

// Gets a reference to the index of the iterator
// Returns a reference to m_index
uint SuspectTableIterator::GetIndex()
{
	return m_index;
}

uint64_t SuspectTableIterator::GetKey()
{
	pthread_rwlock_rdlock(&m_lock_ref);
	uint64_t ret;
	if(m_index < m_keys_ref.size())
	{
		ret = m_keys_ref[m_index];
		if(m_table_ref.find(ret) == m_table_ref.end())
			ret = m_table_ref.begin()->first;
	}
	else
	{
		ret = m_table_ref.begin()->first;
	}
	memcpy(&m_keys_ref.at(m_index), &ret, 4);
	pthread_rwlock_unlock(&m_lock_ref);
	return ret;
}

//Increments the iterator by 1
// Returns 'this'
SuspectTableIterator SuspectTableIterator::operator++()
{
	m_index++;
	return *this;
}
//Increments the iterator by int 'rhs'
// Returns 'this'
SuspectTableIterator SuspectTableIterator::operator+=(int rhs)
{
	m_index += rhs;
	return *this;
}

//Decrements the iterator by 1
// Returns 'this'
SuspectTableIterator SuspectTableIterator::operator--()
{
	m_index--;
	return *this;
}
//Decrements the iterator by int 'rhs'
// Returns 'this'
SuspectTableIterator SuspectTableIterator::operator-=(int rhs)
{
	m_index-= rhs;
	return *this;
}

//Checks if the iterator 'rhs' is equal to 'this'
bool SuspectTableIterator::operator!=(SuspectTableIterator rhs)
{
	return !(this == &rhs);
}

//Checks if the iterator 'rhs' is not equal to 'this'
bool SuspectTableIterator::operator==(SuspectTableIterator rhs)
{
	return (this == &rhs);
}

}
