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
	pthread_rwlock_wrlock(&m_lock);
	//Deletes the suspects pointed to by the table
	for(SuspectHashTable::iterator it = m_table.begin(); it != m_table.end(); it++)
	{
		delete it->second;
	}
	pthread_rwlock_destroy(&m_lock);
}

//Adds the Suspect pointed to in 'suspect' into the table using suspect->GetIPAddress() as the key;
//		suspect: pointer to the Suspect you wish to add
// Returns (0) on Success, (2) if the suspect exists, and (-2) if the key is invalid;
SuspectTableRet SuspectTable::AddNewSuspect(Suspect * suspect)
{
	in_addr_t key = suspect->GetIpAddress();
	if(IsValidKey(key))
	{
		return SUSPECT_EXISTS;
	}
	else if((key == m_table.empty_key()) || (key == m_table.deleted_key()))
	{
		return KEY_INVALID;
	}
	else
	{
		pthread_rwlock_wrlock(&m_lock);
		m_table[key] = suspect;
		m_keys.push_back(key);
		pthread_rwlock_unlock(&m_lock);
		return SUCCESS;
	}
}

// Adds the Suspect pointed to in 'suspect' into the table using the source of the packet as the key;
// 		packet: copy of the packet you whish to create a suspect from
// Returns (0) on Success, and (2) if the suspect exists;
SuspectTableRet SuspectTable::AddNewSuspect(Packet packet)
{
	Suspect * s = new Suspect(packet);
	in_addr_t key = s->GetIpAddress();
	if(IsValidKey(key))
	{
		delete s;
		return SUSPECT_EXISTS;
	}
	else if((key == m_table.empty_key()) || (key == m_table.deleted_key()))
	{
		delete s;
		return KEY_INVALID;
	}
	else
	{
		pthread_rwlock_wrlock(&m_lock);
		m_table[key] = s;
		m_keys.push_back(key);
		pthread_rwlock_unlock(&m_lock);
		return SUCCESS;
	}
}

// Copies the suspect pointed to in 'suspect', into the table at location key
//		key: uint64_t of the Suspect
//		suspect: pointer to the Suspect you wish to copy in
// Returns (0) on Success, (-1) if the Suspect is checked out by someone else,
// and (-2) if the Suspect does not exist
// Note:  This function blocks until it can acquire a write lock on the suspect
SuspectTableRet SuspectTable::CheckIn(Suspect * suspect)
{
	Suspect suspectCopy = *suspect;

	in_addr_t key = suspectCopy.GetIpAddress();
	if(IsValidKey(key))
	{
		SuspectHashTable::iterator it = m_table.find(key);
		if(it != m_table.end())
		{
			pthread_rwlock_wrlock(&m_lock);
			ANNpoint aNN =  annAllocPt(Config::Inst()->GetEnabledFeatureCount());
			aNN = suspectCopy.GetAnnPoint();
			annDeallocPt(aNN);
			m_table[key]->SetClassification(suspectCopy.GetClassification());
			for(uint i = 0; i < DIM; i++)
			{
				m_table[key]->SetFeatureAccuracy((featureIndex)i, suspectCopy.GetFeatureAccuracy((featureIndex)i));
			}
			//Copy relevant values instead of the entire suspect so as not to upset the suspect state and lock.
			FeatureSet fs = suspectCopy.GetFeatureSet();
			m_table[key]->SetFeatureSet(&fs);

			m_table[key]->SetHostileNeighbors(suspectCopy.GetHostileNeighbors());
			m_table[key]->SetInAddr(suspectCopy.GetInAddr());

			m_table[key]->SetFlaggedByAlarm(suspectCopy.GetFlaggedByAlarm());
			m_table[key]->SetIsHostile(suspectCopy.GetIsHostile());
			m_table[key]->SetIsLive(suspectCopy.GetIsLive());
			m_table[key]->SetNeedsClassificationUpdate(suspectCopy.GetNeedsClassificationUpdate());
			fs = suspectCopy.GetUnsentFeatureSet();
			m_table[key]->SetUnsentFeatureSet(&fs);
			m_table[key]->ClearEvidence();
			vector<Packet> temp = suspectCopy.GetEvidence();
			for(uint i = 0; i < temp.size(); i++)
			{
				m_table[key]->AddEvidence(temp[i]);
			}
			pthread_rwlock_unlock(&m_lock);
			return SUCCESS;
		}
		//Else if the owner is not this thread
		else
		{
			pthread_rwlock_unlock(&m_lock);
			return SUSPECT_CHECKED_OUT;
		}
	}
	return KEY_INVALID;
}

//Copies out a suspect and marks the suspect so that it cannot be written or deleted
//		key: uint64_t of the Suspect
// Returns a copy of the Suspect associated with 'key', it returns an empty suspect if that key is Invalid.
// Note: This function read locks the suspect until CheckIn(&suspect) or suspect->UnsetOwner() is called.
// Note: If you wish to block until the suspect is available use suspect->SetOwner();
Suspect SuspectTable::CheckOut(in_addr_t key)
{
	if(IsValidKey(key))
	{
		pthread_rwlock_wrlock(&m_lock);
		SuspectHashTable::iterator it = m_table.find(key);
		if(it != m_table.end())
		{
			Suspect ret = *it->second;
			pthread_rwlock_unlock(&m_lock);
			return ret;
		}
	}
	else
	{
		pthread_rwlock_wrlock(&m_lock);
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
	if(IsValidKey(key))
	{
		pthread_rwlock_wrlock(&m_lock);
		SuspectHashTable::iterator it = m_table.find(key);
		if(it != m_table.end())
		{
			Suspect ret = *it->second;
			pthread_rwlock_unlock(&m_lock);
			return ret;
		}
	}
	else
	{
		pthread_rwlock_wrlock(&m_lock);
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
	if(!IsValidKey(key))
	{
		return KEY_INVALID;
	}
	else
	{
		pthread_rwlock_wrlock(&m_lock);
		SuspectHashTable::iterator it = m_table.find(key);
		if(it != m_table.end())
		{
			Suspect * suspectPtr = m_table[key];
			m_table.erase(key);
			delete suspectPtr;
			for(vector<uint64_t>::iterator vit = m_keys.begin(); vit != m_keys.end(); vit++)
			{
				if(*vit == key)
				{
					m_keys.erase(vit);
					break;
				}
			}
			pthread_rwlock_unlock(&m_lock);
			return SUCCESS;
		}
		pthread_rwlock_unlock(&m_lock);
	}
	//Shouldn't get here, IsValidKey should cover this case, this is here only to prevent warnings or incase of error
	return KEY_INVALID;
}

//Iterates over the Suspect Table and returns a vector containing each Hostile Suspect's uint64_t
// Returns a vector of hostile suspect in_addr_t temps
vector<uint64_t> SuspectTable::GetHostileSuspectKeys()
{
	vector<uint64_t> ret;
	ret.clear();
	pthread_rwlock_wrlock(&m_lock);
	for(SuspectHashTable::iterator it = m_table.begin(); it != m_table.end(); it++)
	{
		if(it->second->GetIsHostile())
		{
			ret.push_back(it->first);
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
	pthread_rwlock_wrlock(&m_lock);
	for(SuspectHashTable::iterator it = m_table.begin(); it != m_table.end(); it++)
	{
		if(!it->second->GetIsHostile())
		{
			ret.push_back(it->first);
		}
	}
	pthread_rwlock_unlock(&m_lock);
	return ret;
}

//Looks at the hostility of the suspect at key
//		key: uint64_t of the Suspect
// Returns 0 for Benign, 1 for Hostile, and -2 if the key is invalid
SuspectTableRet SuspectTable::GetHostility(in_addr_t key)
{
	if(!IsValidKey(key))
	{
		return KEY_INVALID;
	}
	pthread_rwlock_rdlock(&m_lock);
	if(m_table[key]->GetIsHostile())
	{
		pthread_rwlock_unlock(&m_lock);
		return IS_HOSTILE;
	}
	pthread_rwlock_unlock(&m_lock);
	return IS_BENIGN;
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

// Resizes the table to the given size.
//		size: the number of bins to use
// Note: Choosing an initial size that covers normal usage can improve performance.
void SuspectTable::Resize(uint size)
{
	pthread_rwlock_wrlock(&m_lock);
	m_table.resize(size);
	pthread_rwlock_unlock(&m_lock);
}

SuspectTableRet SuspectTable::Clear(bool blockUntilDone)
{
/*	pthread_rwlock_wrlock(&m_lock);
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
			pthread_rwlock_unlock(&m_lock);
			return SUSPECT_CHECKED_OUT;
		}
	}
	for(uint i = 0; i < lockedKeys.size(); i++)
	{
		m_table[m_keys[i]]->~Suspect();
	}
	m_table.clear();
	m_keys.clear();
	pthread_rwlock_unlock(&m_lock);*/
	return SUCCESS;
}

bool SuspectTable::IsValidKey(in_addr_t key)
{
	pthread_rwlock_rdlock(&m_lock);
	SuspectHashTable::iterator it = m_table.find(key);
	SuspectHashTable::iterator end =  m_table.end();
	pthread_rwlock_unlock(&m_lock);
	return (it != end);
}

void SuspectTable::SaveSuspectsToFile(string filename)
{
	pthread_rwlock_wrlock(&m_lock);
	LOG(NOTICE, "Saving Suspects...", "Saving Suspects in a text format to file "+filename);
	ofstream out(filename.c_str());
	if(!out.is_open())
	{
		LOG(ERROR,"Unable to open file requested file.",
			"Unable to open or create file located at "+filename+".");
		return;
	}
	for(SuspectHashTable::iterator it = m_table.begin(); it != m_table.end(); it++)
	{
		out << it->second->ToString() << endl;
	}
	out.close();
	pthread_rwlock_unlock(&m_lock);
}
}

