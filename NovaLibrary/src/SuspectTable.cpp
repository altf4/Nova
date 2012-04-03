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
#include "Config.h"
#include "Logger.h"

#include <fstream>
#include <sstream>

#define NOERROR 0

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
	pthread_rwlockattr_t * tempAttr = NULL;
	pthread_rwlockattr_init(tempAttr);
	pthread_rwlockattr_setkind_np(tempAttr,PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
	pthread_rwlock_init(&m_lock, tempAttr);
	//After initializing the lock it is safe to delete the attr
	delete tempAttr;

	uint64_t initKey = 0;
	initKey--;
	m_suspectTable.set_empty_key(initKey);
	m_empty_key = initKey;
	initKey--;
	m_suspectTable.set_deleted_key(initKey);
	m_deleted_key = initKey;
}

// Default Deconstructor for SuspectTable
SuspectTable::~SuspectTable()
{
	//Deletes the suspects pointed to by the table
	for(SuspectHashTable::iterator it = m_suspectTable.begin(); it != m_suspectTable.end(); it++)
	{
		delete it->second;
	}
	for(SuspectLockTable::iterator it = m_lockTable.begin(); it != m_lockTable.end(); it++)
	{
		pthread_mutex_destroy(&it->second.lock);
	}
	pthread_rwlock_destroy(&m_lock);
}

//Adds the Suspect pointed to in 'suspect' into the table using suspect->GetIPAddress() as the key;
//		suspect: pointer to the Suspect you wish to add
// Returns true on Success, and false if the suspect already exists
bool SuspectTable::AddNewSuspect(Suspect *suspect)
{
	//Write lock the table and check key validity
	pthread_rwlock_wrlock(&m_lock);
	Suspect * suspectCopy = new Suspect(*suspect);
	in_addr_t key = suspectCopy->GetIpAddress();
	//If we return false then there is no suspect at this ip address yet
	if(!IsValidKey_NoLocking(key))
	{
		//If there is already a SuspectLock this Suspect is listed for deletion but it's lock still exists
		if(m_lockTable.find(key) != m_lockTable.end())
		{
			//Increase the ref count to prevent deletion
			m_lockTable[key].ref_cnt++;
			//Unlock the table and wait for the suspect lock
			pthread_rwlock_unlock(&m_lock);
			pthread_mutex_lock(&m_lockTable[key].lock);
			//After getting the suspect lock, lock the table (lets CheckIn finish)
			pthread_rwlock_wrlock(&m_lock);
			//Reverse the deletion flag
			m_lockTable[key].deleted = false;
			m_lockTable[key].ref_cnt--;
			pthread_mutex_unlock(&m_lockTable[key].lock);
		}
		//Else we need to init a SuspectLock
		{
			pthread_mutexattr_t * tempAttr = NULL;
			pthread_mutexattr_init(tempAttr);
			pthread_mutexattr_settype(tempAttr, PTHREAD_MUTEX_ERRORCHECK);
			//Destroy before init just to be safe
			pthread_mutex_destroy(&m_lockTable[key].lock);
			pthread_mutex_init(&m_lockTable[key].lock, tempAttr);
			//Safe to delete the attr after init.
			delete tempAttr;
			m_lockTable[key].deleted = false;
			m_lockTable[key].ref_cnt = 0;
		}
		//Allocate the Suspect and copy the contents
		m_suspectTable[key] = suspectCopy;
		//Store the key
		m_keys.push_back(key);
		pthread_rwlock_unlock(&m_lock);
		return true;
	}
	//Else this suspect already exists, we cannot add it again.
	pthread_rwlock_unlock(&m_lock);
	return false;
}

// Adds the Suspect pointed to in 'suspect' into the table using the source of the packet as the key;
// 		packet: copy of the packet you whish to create a suspect from
// Returns true on Success, and false if the suspect already exists
bool SuspectTable::AddNewSuspect(Packet packet)
{
	pthread_rwlock_wrlock(&m_lock);
	Suspect * suspect = new Suspect(packet);
	in_addr_t key = suspect->GetIpAddress();
	//If we return false then there is no suspect at this ip address yet
	if(!IsValidKey_NoLocking(key))
	{
		//If there is already a SuspectLock this Suspect is listed for deletion but it's lock still exists
		if(m_lockTable.find(key) != m_lockTable.end())
		{
			//Increase the ref count to prevent deletion
			m_lockTable[key].ref_cnt++;
			//Unlock the table and wait for the suspect lock
			pthread_rwlock_unlock(&m_lock);
			pthread_mutex_lock(&m_lockTable[key].lock);
			//After getting the suspect lock, lock the table (lets CheckIn finish)
			pthread_rwlock_wrlock(&m_lock);
			//Reverse the deletion flag
			m_lockTable[key].deleted = false;
			m_lockTable[key].ref_cnt--;
			pthread_mutex_unlock(&m_lockTable[key].lock);
		}
		//Else we need to init a SuspectLock
		{
			pthread_mutexattr_t * tempAttr = NULL;
			pthread_mutexattr_init(tempAttr);
			pthread_mutexattr_settype(tempAttr, PTHREAD_MUTEX_ERRORCHECK);
			//Destroy before init just to be safe
			pthread_mutex_destroy(&m_lockTable[key].lock);
			pthread_mutex_init(&m_lockTable[key].lock, tempAttr);
			//Safe to delete the attr after init.
			delete tempAttr;
			m_lockTable[key].deleted = false;
			m_lockTable[key].ref_cnt = 0;
		}
		//Allocate the Suspect and copy the contents
		m_suspectTable[key] = suspect;
		//Store the key
		m_keys.push_back(key);
		pthread_rwlock_unlock(&m_lock);
		return true;
	}
	//Else this suspect already exists, we cannot add it again.
	pthread_rwlock_unlock(&m_lock);
	return false;
}

// If the table contains a suspect associated with 'key', then it adds 'packet' to it's evidence
//		key: IP address of the suspect as a uint value (host byte order)
//		packet: packet struct to be added into the suspect's list of evidence.
// Returns true if the call succeeds, false if the suspect could not be located
// Note: this is faster than Checking out a suspect adding the evidence and checking it in but is equivalent
bool SuspectTable::AddEvidenceToSuspect(in_addr_t key, Packet packet)
{
	pthread_rwlock_wrlock(&m_lock);
	if(IsValidKey_NoLocking(key))
	{
		m_lockTable[key].ref_cnt++;
		pthread_rwlock_unlock(&m_lock);
		pthread_mutex_lock(&m_lockTable[key].lock);
		pthread_rwlock_wrlock(&m_lock);
		if(!IsValidKey_NoLocking(key))
		{
			pthread_mutex_unlock(&m_lockTable[key].lock);
			//If the count is 0 and the suspect is to be deleted, delete it.
			if(!m_lockTable[key].ref_cnt && m_lockTable[key].deleted)
			{
				pthread_mutex_destroy(&m_lockTable[key].lock);
				m_lockTable.erase(key);
			}
			pthread_rwlock_unlock(&m_lock);
			return false;
		}
		m_lockTable[key].ref_cnt--;
		m_suspectTable[key]->AddEvidence(packet);
		pthread_mutex_unlock(&m_lockTable[key].lock);
		pthread_rwlock_unlock(&m_lock);
		return true;
	}
	pthread_rwlock_unlock(&m_lock);
	return false;
}

// Copies the suspect pointed to in 'suspect', into the table at location key
//		suspect: the Suspect you wish to check in
// Returns (0) on Success,(1) If the Suspect was not Checked Out, (-1) if the Suspect is Checked Out by another thread
// and (-2) if the Suspect does not exist (Key invalid or suspect was deleted)
// Note:  This function blocks until it can acquire a write lock on the suspect
SuspectTableRet SuspectTable::CheckIn(Suspect * suspect)
{
	pthread_rwlock_wrlock(&m_lock);
	in_addr_t key = suspect->GetIpAddress();

	//If the key has a valid suspect
	if(IsValidKey_NoLocking(key))
	{
		//Attempt to unlock the Suspect, this will tell us if we are allowed to check in this suspect
		switch(pthread_mutex_unlock(&m_lockTable[key].lock))
		{
			//Suspect is not CheckedOut by this thread (called CheckIn without CheckOut)
			case EPERM:
			{
				return SUSPECT_NOT_CHECKED_OUT;
			}
			//If we are allowed to check in the suspect
			case NOERROR:
			{
				m_lockTable[key].ref_cnt--;
				*m_suspectTable[key] = *suspect;
			}
			//Any other case we return KEY_INVALID
			default:
			{
				break;
			}
		}
	}
	//Check if there is a SuspectLock for this Suspect
	else if(m_lockTable.find(key) != m_lockTable.end())
	{
		//If we get here the Suspect is scheduled to be deleted
		//Decrement the ref counter
		if(m_lockTable[key].ref_cnt > 0)
		{
			m_lockTable[key].ref_cnt--;
		}
		//If ref_cnt is <= 0 here something is wrong
		else //XXX Reconsider this case
		{
			m_lockTable[key].ref_cnt = 0;
		}
		//If the count is 0 and the suspect is to be deleted, delete it.
		if(!m_lockTable[key].ref_cnt && m_lockTable[key].deleted)
		{
			pthread_mutex_destroy(&m_lockTable[key].lock);
			m_lockTable.erase(key);
		}
	}
	pthread_rwlock_unlock(&m_lock);
	return SUSPECT_KEY_INVALID;
}

//Copies out a suspect and marks the suspect so that it cannot be written or deleted
//		key: uint64_t of the Suspect
// Returns a copy of the Suspect associated with 'key', it returns an empty suspect if that key is Invalid.
// Note: This function read locks the suspect until CheckIn(&suspect) or suspect->UnsetOwner() is called.
Suspect SuspectTable::CheckOut(in_addr_t key)
{
	pthread_rwlock_wrlock(&m_lock);
	if(IsValidKey_NoLocking(key))
	{
		m_lockTable[key].ref_cnt++;
		pthread_rwlock_unlock(&m_lock);
		pthread_mutex_lock(&m_lockTable[key].lock);
		pthread_rwlock_wrlock(&m_lock);
		if(!IsValidKey_NoLocking(key))
		{
			m_lockTable[key].ref_cnt--;
			Suspect ret = m_emptySuspect;
			pthread_rwlock_unlock(&m_lock);
			return ret;
		}
		Suspect ret = *m_suspectTable[key];
		pthread_rwlock_unlock(&m_lock);
		return ret;
	}
	else
	{
		Suspect ret = m_emptySuspect;
		pthread_rwlock_unlock(&m_lock);
		return ret;
	}
}

// Lookup and get an Asynchronous copy of the Suspect
// 		key: uint64_t of the Suspect
// Returns an empty suspect on failure
// Note: To modify or lock a suspect use CheckOut();
// Note: This is the same as GetSuspectStatus except it copies the feature set object which can grow very large.
Suspect SuspectTable::GetSuspect(in_addr_t key)
{
	//Read lock the table, Suspects can only change in the table while it's write locked.
	pthread_rwlock_rdlock(&m_lock);
	if(IsValidKey_NoLocking(key))
	{
		//Create a copy of the suspect
		Suspect ret = *m_suspectTable[key];
		pthread_rwlock_unlock(&m_lock);
		return ret;
	}
	else
	{
		Suspect ret = m_emptySuspect;
		pthread_rwlock_unlock(&m_lock);
		return ret;
	}
}

//Erases a suspect from the table if it is not locked
//		key: uint64_t of the Suspect
// Returns (0) on success, (-2) if the suspect does not exist (key is invalid)
SuspectTableRet SuspectTable::Erase(in_addr_t key)
{
	pthread_rwlock_wrlock(&m_lock);
	if(IsValidKey_NoLocking(key))
	{
		m_lockTable[key].deleted = true;
		for(vector<uint64_t>::iterator vit = m_keys.begin(); vit != m_keys.end(); vit++)
		{
			if(*vit == key)
			{
				m_keys.erase(vit);
				break;
			}
		}
		SuspectHashTable::iterator it = m_suspectTable.find(key);
		if(it != m_suspectTable.end())
		{
			Suspect * suspectPtr = m_suspectTable[key];
			m_suspectTable.erase(key);
			delete suspectPtr;
		}
		if(m_lockTable[key].ref_cnt <= 0)
		{
			pthread_mutex_destroy(&m_lockTable[key].lock);
			m_lockTable.erase(key);
		}
		pthread_rwlock_unlock(&m_lock);
		return SUSPECT_TABLE_CALL_SUCCEESS;
	}
	return SUSPECT_KEY_INVALID;
}

//Iterates over the Suspect Table and returns a vector containing each Hostile Suspect's uint64_t
// Returns a vector of hostile suspect in_addr_t temps
vector<uint64_t> SuspectTable::GetHostileSuspectKeys()
{
	vector<uint64_t> ret;
	ret.clear();
	pthread_rwlock_rdlock(&m_lock);
	for(uint i = 0; i < m_keys.size(); i++)
	{
		if(IsValidKey_NoLocking(m_keys[i]))
		{
			if(m_suspectTable[m_keys[i]]->GetIsHostile())
			{
				ret.push_back(m_keys[i]);
			}
		}
	}
	pthread_rwlock_unlock(&m_lock);
	return ret;
}

//Iterates over the Suspect Table and returns a vector containing each Benign Suspect's uint64_t
// Returns a vector of benign suspect in_addr_t temps
vector<uint64_t> SuspectTable::GetBenignSuspectKeys()
{
	vector<uint64_t> ret;
	ret.clear();
	pthread_rwlock_rdlock(&m_lock);
	for(uint i = 0; i < m_keys.size(); i++)
	{
		if(IsValidKey_NoLocking(m_keys[i]))
		{
			if(!m_suspectTable[m_keys[i]]->GetIsHostile())
			{
				ret.push_back(m_keys[i]);
			}
		}
	}
	pthread_rwlock_unlock(&m_lock);
	return ret;
}

//Get the size of the Suspect Table
// Returns the size of the Table
uint SuspectTable::Size()
{
	pthread_rwlock_rdlock(&m_lock);
	uint ret = m_suspectTable.size();
	pthread_rwlock_unlock(&m_lock);
	return ret;
}

// Resizes the table to the given size.
//		size: the number of bins to use
// Note: Choosing an initial size that covers normal usage can improve performance.
void SuspectTable::Resize(uint size)
{
	pthread_rwlock_wrlock(&m_lock);
	m_suspectTable.resize(size);
	pthread_rwlock_unlock(&m_lock);
}

void SuspectTable::EraseAllSuspects()
{
	pthread_rwlock_wrlock(&m_lock);
	m_keys.clear();
	for(SuspectLockTable::iterator it = m_lockTable.begin(); it != m_lockTable.end(); it++)
	{
		//If another thread is still referencing(blocking on or using) the Suspect lock, flag for deletion
		if(it->second.ref_cnt)
		{
			m_lockTable[it->first].deleted = true;
		}
		//If the suspect lock is safe to delete.
		else
		{
			pthread_mutex_destroy(&m_lockTable[it->first].lock);
			m_lockTable.erase(it->first);
		}
	}
	for(SuspectHashTable::iterator it = m_suspectTable.begin(); it != m_suspectTable.end(); it++)
	{
		delete m_suspectTable[it->first];
	}
	m_suspectTable.clear();
	pthread_rwlock_unlock(&m_lock);
}

void SuspectTable::SaveSuspectsToFile(string filename)
{
	pthread_rwlock_rdlock(&m_lock);
	LOG(NOTICE, "Saving Suspects...", "Saving Suspects in a text format to file "+filename);
	ofstream out(filename.c_str());
	if(!out.is_open())
	{
		LOG(ERROR,"Unable to open file requested file.",
			"Unable to open or create file located at "+filename+".");
		return;
	}
	for(uint i = 0; i < m_keys.size(); i++)
	{
		if(IsValidKey_NoLocking(m_keys[i]))
		{
			out << m_suspectTable[m_keys[i]]->ToString() << endl;
		}
	}
	out.close();
	pthread_rwlock_unlock(&m_lock);
}

void SuspectTable::UpdateAllSuspects() //XXX
{

}

void SuspectTable::UpdateSuspect(Suspect *) //XXX
{

}

// Checks the validity of the key - public thread-safe version
//		key: The in_addr_t of the Suspect
// Returns true if there is a suspect associated with the given key, false otherwise
bool SuspectTable::IsValidKey(in_addr_t key)
{
	pthread_rwlock_rdlock(&m_lock);
	//If we find a SuspectLock the suspect exists or is scheduled to be deleted
	bool ret = IsValidKey_NoLocking(key);
	pthread_rwlock_unlock(&m_lock);
	return ret;
}

// Checks the validity of the key - private use non-locking version
//		key: The uint64_t of the Suspect
// Returns true if there is a suspect associated with the given key, false otherwise
// *Note: Assumes you have already locked the table
bool SuspectTable::IsValidKey_NoLocking(in_addr_t key)
{
	//If we find a SuspectLock the suspect exists or is scheduled to be deleted
	if(m_lockTable.find(key) != m_lockTable.end())
	{
		return !m_lockTable[key].deleted;
	}
	return false;
}
}
