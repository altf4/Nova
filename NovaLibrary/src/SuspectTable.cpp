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
// 		list of suspects.
//============================================================================/*

#include "SuspectTable.h"

using namespace std;
using namespace Nova;
namespace Nova
{

//***************************************************************************//
// - - - - - - - - - - - - - - SuspectTable - - - - - - - - - - - - - - - - -
//***************************************************************************//

// Default Constructor for SuspectTable
SuspectTable::SuspectTable()
{
	m_keys.clear();
	m_owners.clear();
	pthread_rwlock_init(&m_lock, NULL);
	uint64_t initKey = 0;
	initKey--;
	m_table.set_empty_key(initKey);
	m_empty_key = initKey;
	initKey--;
	m_table.set_deleted_key(initKey);
	m_deleted_key = initKey;
}

// Default Deconstructor for SuspectTable
SuspectTable::~SuspectTable()
{
	Wrlock();
	//Deletes the suspects pointed to by the table
	for(SuspectHashTable::iterator it = m_table.begin(); it != m_table.end(); it++)
	{
		delete it->second;
	}
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

// Get a SuspectTableIterator that points to the element at key;
// 		key: in_addr_t of the Suspect
// Returns a SuspectTableIterator or an empty iterator with an index equal to Size() if the suspect cannot be found
// Note: there are no guarantees about the state or existence of the Suspect in the table after this call.
SuspectTableIterator SuspectTable::Find(in_addr_t  key)
{
	Rdlock();
	uint64_t realKey = key;
	if(IsValidKey(key))
	{
		SuspectTableIterator it = SuspectTableIterator(&m_table, &m_keys);
		for(uint i = 0; i < m_keys.size(); i++)
		{
			if(realKey == m_keys[i])
			{
				it += i;
				break;
			}
		}
		Unlock();
		return it;
	}
	else
	{
		SuspectTableIterator it = SuspectTableIterator(&m_table, &m_keys);
		it += m_table.size();
		Unlock();
		return it;
	}
}

//Adds the Suspect pointed to in 'suspect' into the table using suspect->GetIPAddress() as the key;
//		suspect: pointer to the Suspect you wish to add
// Returns (0) on Success, (2) if the suspect exists, and (-2) if the key is invalid;
SuspectTableRet SuspectTable::AddNewSuspect(Suspect * suspect)
{
	Rdlock();
	in_addr_t key = suspect->GetIpAddress();
	uint64_t realKey = key;
	if(IsValidKey(key))
	{
		Unlock();
		return SUSPECT_EXISTS;
	}
	else if((realKey == m_table.empty_key()) || (realKey == m_table.deleted_key()))
	{
		Unlock();
		return KEY_INVALID;
	}
	else
	{
		Unlock();
		Wrlock();
		Suspect * s = new Suspect();
		*s = *suspect;
		m_table[realKey] = s;
		m_keys.push_back(realKey);
		Unlock();
		return SUCCESS;
	}
}

// Adds the Suspect pointed to in 'suspect' into the table using the source of the packet as the key;
// 		packet: copy of the packet you whish to create a suspect from
// Returns (0) on Success, and (2) if the suspect exists;
SuspectTableRet SuspectTable::AddNewSuspect(Packet packet)
{
	Rdlock();
	Suspect * s = new Suspect(packet);
	in_addr_t key = s->GetIpAddress();
	uint64_t realKey = key;
	if(IsValidKey(key))
	{
		Unlock();
		delete s;
		return SUSPECT_EXISTS;
	}
	else if((realKey == m_table.empty_key()) || (realKey == m_table.deleted_key()))
	{
		Unlock();
		delete s;
		return KEY_INVALID;
	}
	else
	{
		Unlock();
		Wrlock();
		m_table[realKey] = s;
		m_keys.push_back(realKey);
		Unlock();
		return SUCCESS;
	}
}

// Copies the suspect pointed to in 'suspect', into the table at location key
//		key: in_addr_t of the Suspect
//		suspect: pointer to the Suspect you wish to copy in
// Returns (0) on Success, (-1) if the Suspect is checked out by someone else,
// (-2) if the Suspect does not exist and (1) if the Suspect is not checked out
// Note:  This function blocks until it can acquire a write lock on the suspect
SuspectTableRet SuspectTable::CheckIn(Suspect * suspect)
{
	Rdlock();
	in_addr_t key = suspect->GetIpAddress();
	uint64_t realKey = key;
	if(IsValidKey(key))
	{
		for(uint i = 0; i < m_keys.size(); i++)
		{
			if(m_keys[i] == realKey)
			{
				Unlock();
				Wrlock();
				switch(m_table[realKey]->ResetOwner())
				{
					//If this suspect was never checked out
					case 1:
						Unlock();
						return SUSPECT_NOT_CHECKED_OUT;
					//If the suspect is checked out but not by this thread
					case -1:
						Unlock();
						return SUSPECT_CHECKED_OUT;
					//If the suspect was checked out by this thread
					case 0:
						*m_table[realKey] = *suspect;
						Unlock();
						return SUCCESS;
				}
			}
		}
	}
	return KEY_INVALID;
}

//Copies out a suspect and marks the suspect so that it cannot be written or deleted
//		key: in_addr_t of the Suspect
// Returns a copy of the Suspect associated with 'key', it returns an empty suspect if that key is Invalid.
// Note: This function read locks the suspect until CheckIn(&suspect) or suspect->UnsetOwner() is called.
// Note: If you wish to block until the suspect is available use suspect->SetOwner();
Suspect SuspectTable::CheckOut(in_addr_t key)
{
	Rdlock();
	if(IsValidKey(key))
	{
		uint64_t realKey = key;
		for(uint i = 0; i < m_keys.size(); i++)
		{
			if(m_keys[i] == realKey)
			{
				if(!m_table[realKey]->HasOwner() || pthread_equal(m_table[realKey]->GetOwner(), pthread_self()))
				{
					m_table[realKey]->SetOwner();
					Suspect ret = *m_table[realKey];
					FeatureSet * fs = new FeatureSet();
					*fs  = *ret.GetFeatureSet().m_unsentData;
					ret.SetFeatureSet(fs);
					Unlock();
					return ret;
				}
				else
				{
					Unlock();
					return m_emptySuspect;
				}
			}
		}
	}
	Unlock();
	return m_emptySuspect;
}

//Lookup and get an Asynchronous copy of the Suspect
//		key: in_addr_t of the Suspect
// Returns an empty suspect on failure
// Note: there are no guarantees about the state or existence of the Suspect in the table after this call.
Suspect SuspectTable::Peek(in_addr_t  key)
{
	Rdlock();
	if(IsValidKey(key))
	{
		uint64_t realKey = key;
		return *m_table[realKey];
	}
	Unlock();
	return m_emptySuspect;
}

//Erases a suspect from the table if it is not locked
//		key: in_addr_t of the Suspect
// Returns (0) on success, (-2) if the suspect does not exist, (-1) if the suspect is checked out
SuspectTableRet SuspectTable::Erase(in_addr_t key)
{
	Rdlock();
	uint64_t realKey = key;
	if(!IsValidKey(key))
	{
		Unlock();
		return KEY_INVALID;
	}

	for(uint i = 0; i < m_keys.size(); i++)
	{
		if(m_keys[i] == realKey)
		{
			if(!m_table[realKey]->HasOwner())
			{
				Unlock();
				Wrlock();
				//Assert the key is still valid, if false it means it was already erased
				if(IsValidKey(key))
				{
					m_table[realKey]->~Suspect();
					m_table.erase(realKey);
					for(uint i = 0; i < m_keys.size(); i++)
					{
						if(m_keys[i] == realKey)
						{
							m_keys.erase(m_keys.begin()+i);
							break;
						}
					}
				}
				Unlock();
				return SUCCESS;
			}
			else
			{
				Unlock();
				return SUSPECT_CHECKED_OUT;
			}
		}
	}
	Unlock();
	//Shouldn't hit here, IsValidKey should on this case, this is here only to prevent warnings or incase of error
	return KEY_INVALID;
}

//Iterates over the Suspect Table and returns a vector containing each Hostile Suspect's in_addr_t
// Returns a vector of hostile suspect in_addr_t keys
vector<in_addr_t> SuspectTable::GetHostileSuspectKeys()
{
	vector<in_addr_t> ret;
	ret.clear();
	Rdlock();
	for(SuspectHashTable::iterator it = m_table.begin(); it != m_table.end(); it++)
		if(it->second->GetIsHostile())
			ret.push_back(it->first);
	Unlock();
	return ret;
}

//Iterates over the Suspect Table and returns a vector containing each Benign Suspect's in_addr_t
// Returns a vector of benign suspect in_addr_t keys
vector<in_addr_t> SuspectTable::GetBenignSuspectKeys()
{
	vector<in_addr_t> ret;
	ret.clear();
	Rdlock();
	for(SuspectHashTable::iterator it = m_table.begin(); it != m_table.end(); it++)
		if(!it->second->GetIsHostile())
			ret.push_back(it->first);
	Unlock();
	return ret;
}

//Looks at the hostility of the suspect at key
//		key: in_addr_t of the Suspect
// Returns 0 for Benign, 1 for Hostile, and -1 if the key is invalid
SuspectTableRet SuspectTable::GetHostility(in_addr_t key)
{
	Rdlock();
	uint64_t realKey = key;
	if(!IsValidKey(key))
	{
		Unlock();
		return SUSPECT_CHECKED_OUT;
	}
	else if(m_table[realKey]->GetIsHostile())
	{
		Unlock();
		return SUSPECT_NOT_CHECKED_OUT;
	}
	Unlock();
	return SUCCESS;
}

//Get the size of the Suspect Table
// Returns the size of the Table
uint SuspectTable::Size()
{
	Rdlock();
	uint ret = m_table.size();
	Unlock();
	return ret;
}

// Resizes the table to the given size.
//		size: the number of bins to use
// Note: Choosing an initial size that covers normal usage can improve performance.
void SuspectTable::Resize(uint size)
{
	Wrlock();
	m_table.resize(size);
	Unlock();
}

SuspectTableRet SuspectTable::Clear(bool blockUntilDone)
{
	Wrlock();
	vector<uint64_t> lockedKeys;
	for(uint i = 0; i < m_keys.size(); i++)
	{
		if(blockUntilDone || !m_table[m_keys[i]]->HasOwner()
				|| pthread_equal(m_table[m_keys[i]]->GetOwner(), pthread_self()))
		{
			m_table[m_keys[i]]->SetOwner();
			lockedKeys.push_back(m_keys[i]);
			m_keys.erase(m_keys.begin()+i);
		}
		else
		{
			for(uint i = 0; i < lockedKeys.size(); i++)
			{
				m_keys.push_back(lockedKeys[i]);
				m_table[m_keys[i]]->ResetOwner();
			}
			Unlock();
			return SUSPECT_CHECKED_OUT;
		}
	}
	for(uint i = 0; i < lockedKeys.size(); i++)
	{
		m_table[m_keys[i]]->~Suspect();
	}
	m_table.clear();
	m_keys.clear();
	Unlock();
	return SUCCESS;
}

bool SuspectTable::IsValidKey(in_addr_t key)
{
	Rdlock();
	uint64_t realKey = key;
	SuspectHashTable::iterator it = m_table.find(realKey);
	Unlock();
	return (it != m_table.end());
}

// Write locks the suspect
// Note: This function will block until the lock is acquired
void SuspectTable::Wrlock()
{
	//Checks if the thread already has a lock
	if(IsOwner())
	{
		//If the table's only owner is this thread we assert a write lock
		if(m_owners.size() == 1)
		{
			pthread_rwlock_unlock(&m_lock);
			pthread_rwlock_wrlock(&m_lock);
			return;
		}
		//If this thread is one of many owners then we release our lock
		Unlock();
	}
	pthread_rwlock_wrlock(&m_lock);
	m_owners.push_back(pthread_self());
}

// Write locks the suspect
// Returns (true) if the lock was acquired, (false) if the table is locked by someone else
// Note: May block if the table is already write locked by this thread.
bool SuspectTable::TryWrlock()
{
	//Checks if the thread already has a lock
	if(IsOwner())
	{
		//If the table's only owner is this thread we assert a write lock
		if(m_owners.size() == 1)
		{
			pthread_rwlock_unlock(&m_lock);
			pthread_rwlock_wrlock(&m_lock);
			return true;
		}
		//If this thread is one of many owners then we release our lock
		Unlock();
	}
	//If the current thread doesn't have a lock we attempt a read lock
	if(pthread_rwlock_trywrlock(&m_lock))
	{
		//If we get a lock we store the thread as an owner
		m_owners.push_back(pthread_self());
		return true;
	}
	//If we fail to get a lock
	return false;
}

// Read locks the suspect
// Note: This function will block until the lock is acquired
void SuspectTable::Rdlock()
{
	//Checks if the thread already has a lock
	if(IsOwner())
	{
		//If the table's only owner is this thread it may be write locked, so we assert a read lock
		if(m_owners.size() == 1)
		{
			pthread_rwlock_unlock(&m_lock);
			pthread_rwlock_rdlock(&m_lock);
		}
		//If this thread is one of many owners then it must have a read lock already so we can return true;
		return;
	}
	pthread_rwlock_rdlock(&m_lock);
	pthread_t tid = pthread_self();
	m_owners.push_back(tid);
}

// Read locks the suspect
// Returns (true) if the lock was or already is acquired, (false) if the table is locked by someone else
// Note: May block if the table is already read locked by this thread.
bool SuspectTable::TryRdlock()
{
	//Checks if the thread already has a lock
	if(IsOwner())
	{
		//If the table's only owner is this thread it may be write locked, so we assert a read lock
		if(m_owners.size() == 1)
		{
			pthread_rwlock_unlock(&m_lock);
			pthread_rwlock_rdlock(&m_lock);
		}
		//If this thread is one of many owners then it must have a read lock already so we can return true;
		return true;
	}

	//If the current thread doesn't have a lock we attempt a read lock
	if(pthread_rwlock_tryrdlock(&m_lock))
	{
		//If we get a new read lock we store the thread as an owner
		m_owners.push_back(pthread_self());
		return true;
	}
	//If we fail to get a read lock then it's currently write locked by another thread
	else
	{
		return false;
	}
}

// Unlocks the suspect
// Returns: (true) If the table was already or successfully unlocked, (false) if the table is locked by someone else
bool SuspectTable::Unlock()
{
	//Checks if the thread already has a lock
	for(uint i = 0; i < m_owners.size(); i++)
	{
		if(pthread_equal(m_owners[i], pthread_self()))
		{
			m_owners.erase(m_owners.begin()+i);
			pthread_rwlock_unlock(&m_lock);
			return true;
		}
	}
	return false;
}

Suspect& SuspectTable::operator[](in_addr_t key)
{
	uint64_t realKey = key;
	if(IsValidKey(key))
	{
		return *m_table[realKey];
	}
	else
	{
		return m_emptySuspect;
	}
}

Suspect& SuspectTable::operator[](uint64_t realKey)
{
	in_addr_t key = realKey;
	if(IsValidKey(key))
		return *m_table[realKey];
	else
	{
		return m_emptySuspect;
	}
}

// Returns true if the current thread has a lock on the Table
bool SuspectTable::IsOwner()
{
	//Checks if the thread already has a lock
	for(uint i = 0; i < m_owners.size(); i++)
	{
		if(pthread_equal(m_owners[i], pthread_self()))
		{
			return true;
		}
	}
	return false;
}
}
