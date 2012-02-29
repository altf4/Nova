//============================================================================
// Name        : SuspectTable.cpp
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
// Description : Wrapper class for the SuspectHashMap object used to maintain a
//		list of suspects.
//============================================================================/*

#include "SuspectTable.h"
#include "Suspect.h"

using namespace std;
using namespace Nova;
namespace Nova
{

//***************************************************************************//
// - - - - - - - - - - - - SuspectTable::iterator - - - - - - - - - - - - - -
//***************************************************************************//


//Default iterator constructor
SuspectTableIterator::SuspectTableIterator(SuspectHashTable * table, vector<in_addr_t> * keys)
{
	m_index = 0;
	m_table_ref = table;
	m_keys_ref = keys;
}

//Default iterator deconstructor
SuspectTableIterator::~SuspectTableIterator()
{

}

//Gets the Next Suspect in the table and increments the iterator
// Returns a copy of the Suspect
Suspect SuspectTableIterator::Next()
{
	m_index++;
	if(m_index >= m_table_ref->size())
		m_index = 0;
	SuspectHashTable::iterator it = m_table_ref->find(m_keys_ref->at(m_index));
	return *it->second;
}

//Gets the Next Suspect in the table, does not increment the iterator
// Returns a copy of the Suspect
Suspect SuspectTableIterator::LookAhead()
{
	SuspectHashTable::iterator it;
	if((m_index+1) == m_table_ref->size())
		it = m_table_ref->find(m_keys_ref->front());
	else
		it = m_table_ref->find(m_keys_ref->at(m_index+1));
	return *it->second;
}

//Gets the Previous Suspect in the Table and decrements the iterator
// Returns a copy of the Suspect
Suspect SuspectTableIterator::Previous()
{
	SuspectHashTable::iterator it;
	m_index--;
	if(m_index < 0)
		it = m_table_ref->find(m_keys_ref->size()-1);
	return *it->second;

}

//Gets the Previous Suspect in the Table, does not decrement the iterator
// Returns a copy of the Suspect
Suspect SuspectTableIterator::LookBack()
{
	SuspectHashTable::iterator it;
	if(m_index > 0)
		it = m_table_ref->find(m_keys_ref->at(m_index -1));
	else
		it = m_table_ref->find(m_keys_ref->back());
	return *it->second;

}

//Gets the Current Suspect in the Table
// Returns a copy of the Suspect
Suspect SuspectTableIterator::Current()
{
	SuspectHashTable::iterator it;
	it = m_table_ref->find(m_keys_ref->at(m_index));
	return *it->second;
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

//***************************************************************************//
// - - - - - - - - - - - - - - SuspectTable - - - - - - - - - - - - - - - - -
//***************************************************************************//

// Default Constructor for SuspectTable
SuspectTable::SuspectTable()
{
	m_keys.clear();
	pthread_rwlock_init(&m_lock, NULL);
	m_table.set_empty_key(1);
	m_table.set_deleted_key(5);
}

// Default Deconstructor for SuspectTable
SuspectTable::~SuspectTable()
{
	pthread_rwlock_wrlock(&m_lock);
	//Deletes the suspects pointed to by the table
	for(SuspectHashTable::iterator it = m_table.begin(); it != m_table.end(); it++)
	{
		delete it->second;
	}
	pthread_rwlock_unlock(&m_lock);
	pthread_rwlock_destroy(&m_lock);
}

// Get a SuspectTableIterator that points to the beginning of the table
// Returns the SuspectTableIterator
SuspectTableIterator SuspectTable::Begin()
{
	SuspectTableIterator it = SuspectTableIterator(&m_table, &m_keys);
	return it;
}

// Get a SuspectTableIterator that points to the last element in the table;
// Returns the SuspectTableIterator
SuspectTableIterator SuspectTable::End()
{
	SuspectTableIterator it = SuspectTableIterator(&m_table, &m_keys);
	it += Size() -1;
	return it;
}

//Adds the Suspect pointed to in 'suspect' into the table using suspect->GetIPAddress() as the key;
//		suspect: pointer to the Suspect you wish to add
// Returns (0) on Success, (2) if the suspect exists, and (-2) if the key is invalid;
int SuspectTable::AddNewSuspect(Suspect * suspect)
{
	pthread_rwlock_rdlock(&m_lock);
	in_addr_t key = suspect->GetIpAddress();
	if(IsValidKey(key))
	{
		pthread_rwlock_unlock(&m_lock);
		return 2;
	}
	else if((key == m_table.empty_key()) || (key == m_table.deleted_key()))
	{
		pthread_rwlock_unlock(&m_lock);
		return -2;
	}
	else
	{
		pthread_rwlock_unlock(&m_lock);
		pthread_rwlock_wrlock(&m_lock);
		Suspect * s = new Suspect();
		*s = *suspect;
		m_table[key] = s;
		pthread_rwlock_unlock(&m_lock);
		return 0;
	}
}

// Copies the suspect pointed to in 'suspect', into the table at location key
//		key: in_addr_t of the Suspect
//		suspect: pointer to the Suspect you wish to copy in
// Returns (0) on Success, (-1) if the Suspect is checked out by someone else,
// (-2) if the Suspect does not exist and (1) if the Suspect is not checked out
// Note:  This function blocks until it can acquire a write lock on the suspect
int SuspectTable::CheckIn(Suspect * suspect)
{
	pthread_rwlock_rdlock(&m_lock);
	in_addr_t key = suspect->GetIpAddress();
	if(IsValidKey(key))
	{
		for(uint i = 0; i < m_keys.size(); i++)
		{
			if(m_keys[i] == key)
			{
				//If this suspect was never checked out
				if(!m_table[key]->HasOwner())
				{
					pthread_rwlock_unlock(&m_lock);
					return NotCheckedOut;
				}

				//If the suspect was checked out by this thread
				if(pthread_equal(m_table[key]->GetOwner(), pthread_self()))
				{
					pthread_rwlock_unlock(&m_lock);
					pthread_rwlock_wrlock(&m_lock);
					m_table[key]->UnsetOwner();
					*m_table[key] = *suspect;
					pthread_rwlock_unlock(&m_lock);
					return Success;
				}
				//If the suspect is checked out but not by this thread
				else
				{
					pthread_rwlock_unlock(&m_lock);
					return CheckedOut;
				}
			}
		}
	}
	return Invalid;
}

//Copies out a suspect and marks the suspect so that it cannot be written or deleted
//		key: in_addr_t of the Suspect
// Returns empty Suspect on failure.
// Note: A 'Checked Out' Suspect cannot be modified until is has been replaced by the suspect 'Checked In'
//		However the suspect can continue to be read. It is similar to having a read lock.
Suspect SuspectTable::CheckOut(in_addr_t key)
{
	pthread_rwlock_rdlock(&m_lock);
	if(IsValidKey(key))
	{
		for(uint i = 0; i < m_keys.size(); i++)
		{
			if(m_keys[i] == key)
			{
				if(!m_table[key]->HasOwner())
				{
					pthread_rwlock_unlock(&m_lock);
					pthread_rwlock_wrlock(&m_lock);
					Suspect ret = *m_table[key];
					m_table[key]->SetOwner(pthread_self());
					pthread_rwlock_unlock(&m_lock);
					return ret;
				}
				else
				{
					pthread_rwlock_unlock(&m_lock);
					return Suspect();
				}
			}
		}
	}
	pthread_rwlock_unlock(&m_lock);
	return Suspect();
}

//Lookup and get an Asynchronous copy of the Suspect
//		key: in_addr_t of the Suspect
// Returns an empty suspect on failure
// Note: there are no guarantees about the state or existence of the Suspect in the table after this call.
Suspect SuspectTable::Peek(in_addr_t  key)
{
	pthread_rwlock_rdlock(&m_lock);
	if(IsValidKey(key))
	{
		Suspect ret = *m_table[key];
		pthread_rwlock_unlock(&m_lock);
		return ret;
	}
	pthread_rwlock_unlock(&m_lock);
	return Suspect();
}

//Erases a suspect from the table if it is not locked
//		key: in_addr_t of the Suspect
// Returns (0) on success, (-2) if the suspect does not exist, (-1) if the suspect is checked out
int SuspectTable::Erase(in_addr_t key)
{
	pthread_rwlock_rdlock(&m_lock);
	if(!IsValidKey(key))
	{
		pthread_rwlock_unlock(&m_lock);
		return Invalid;
	}

	for(uint i = 0; i < m_keys.size(); i++)
	{
		if(m_keys[i] == key)
		{
			if(!m_table[key]->HasOwner())
			{
				pthread_rwlock_unlock(&m_lock);
				pthread_rwlock_wrlock(&m_lock);
				//Assert the key is still valid, if it happened to be
				if(IsValidKey(key))
					m_table.erase(key);
				pthread_rwlock_unlock(&m_lock);
				return Success;
			}
			else
			{
				pthread_rwlock_unlock(&m_lock);
				return CheckedOut;
			}
		}
	}
	pthread_rwlock_unlock(&m_lock);
	//Shouldn't hit here, IsValidKey should on this case, this is here only to prevent warnings or incase of error
	return Invalid;
}

//Iterates over the Suspect Table and returns a vector containing each Hostile Suspect's in_addr_t
// Returns a vector of hostile suspect in_addr_t keys
vector<in_addr_t> SuspectTable::GetHostileSuspectKeys()
{
	vector<in_addr_t> ret;
	ret.clear();
	pthread_rwlock_rdlock(&m_lock);
	for(SuspectHashTable::iterator it = m_table.begin(); it != m_table.end(); it++)
		if(it->second->GetIsHostile())
			ret.push_back(it->first);
	pthread_rwlock_unlock(&m_lock);
	return ret;
}

//Iterates over the Suspect Table and returns a vector containing each Benign Suspect's in_addr_t
// Returns a vector of benign suspect in_addr_t keys
vector<in_addr_t> SuspectTable::GetBenignSuspectKeys()
{
	vector<in_addr_t> ret;
	ret.clear();
	pthread_rwlock_rdlock(&m_lock);
	for(SuspectHashTable::iterator it = m_table.begin(); it != m_table.end(); it++)
		if(!it->second->GetIsHostile())
			ret.push_back(it->first);
	pthread_rwlock_unlock(&m_lock);
	return ret;
}

//Looks at the hostility of the suspect at key
//		key: in_addr_t of the Suspect
// Returns 0 for Benign, 1 for Hostile, and -1 if the key is invalid
int SuspectTable::GetHostility(in_addr_t key)
{
	pthread_rwlock_rdlock(&m_lock);
	if(!IsValidKey(key))
	{
		pthread_rwlock_unlock(&m_lock);
		return CheckedOut;
	}
	else if(m_table[key]->GetIsHostile())
	{
		pthread_rwlock_unlock(&m_lock);
		return NotCheckedOut;
	}
	pthread_rwlock_unlock(&m_lock);
	return Success;
}

//Get the size of the Suspect Table
// Returns the size of the Table
uint SuspectTable::Size()
{
	pthread_rwlock_rdlock(&m_lock);
	uint ret = m_table.size();
	pthread_rwlock_unlock(&m_lock);
	return ret;
}

bool SuspectTable::IsValidKey(in_addr_t key)
{
	pthread_rwlock_rdlock(&m_lock);
	SuspectHashTable::iterator it = m_table.find(key);
	pthread_rwlock_unlock(&m_lock);

	return (it != m_table.end());
}

}
