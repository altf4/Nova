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

#include "ClassificationEngine.h"
#include "SuspectTable.h"
#include "HashMap.h"
#include "Config.h"
#include "Logger.h"
#include "Novad.h"


#include <fstream>
#include <sstream>

#define NOERROR 0

using namespace std;
using namespace Nova;

extern ClassificationEngine* engine;

namespace Nova
{

//***************************************************************************//
// - - - - - - - - - - - - - - SuspectTable - - - - - - - - - - - - - - - - -
//***************************************************************************//

// Default Constructor for SuspectTable
SuspectTable::SuspectTable()
{
	m_keys.clear();
	pthread_rwlockattr_t tempAttr;
	pthread_rwlockattr_init(&tempAttr);
	pthread_rwlockattr_setkind_np(&tempAttr,PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
	pthread_rwlock_init(&m_lock, &tempAttr);

	uint64_t initKey = 0;
	initKey--;
	m_suspectTable.set_empty_key(initKey);
	m_lockTable.set_empty_key(initKey);
	m_empty_key = initKey;
	initKey--;
	m_suspectTable.set_deleted_key(initKey);
	m_lockTable.set_deleted_key(initKey);
	m_deleted_key = initKey;

	m_emptySuspect.SetClassification(EMPTY_SUSPECT_CLASSIFICATION);
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
	if(!IsValidKey_NonBlocking(key))
	{
		//If there is already a SuspectLock this Suspect is listed for deletion but it's lock still exists
		if(m_lockTable.keyExists(key))
		{
			//Wait for the suspect lock, this call releases the table while blocking.
			LockSuspect(key);
			//Reverse the deletion flag
			m_lockTable[key].deleted = false;
			UnlockSuspect(key);
		}
		//Else we need to init a SuspectLock
		{
			pthread_mutexattr_t tempAttr;
			pthread_mutexattr_init(&tempAttr);
			pthread_mutexattr_settype(&tempAttr, PTHREAD_MUTEX_ERRORCHECK);
			//Destroy before init just to be safe
			pthread_mutex_destroy(&m_lockTable[key].lock);
			pthread_mutex_init(&m_lockTable[key].lock, &tempAttr);
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
	//Else this suspect already exists, we cannot add it again->
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
	if(!IsValidKey_NonBlocking(key))
	{
		//If there is already a SuspectLock this Suspect is listed for deletion but it's lock still exists
		if(m_lockTable.keyExists(key))
		{
			//Wait for the suspect lock, this call releases the table while blocking.
			LockSuspect(key);
			//Reverse the deletion flag
			m_lockTable[key].deleted = false;
			UnlockSuspect(key);
		}
		//Else we need to init a SuspectLock
		{
			pthread_mutexattr_t tempAttr;
			pthread_mutexattr_init(&tempAttr);
			pthread_mutexattr_settype(&tempAttr, PTHREAD_MUTEX_ERRORCHECK);
			//Destroy before init just to be safe
			pthread_mutex_destroy(&m_lockTable[key].lock);
			pthread_mutex_init(&m_lockTable[key].lock, &tempAttr);
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
	//Else this suspect already exists, we cannot add it again->
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
	if(IsValidKey_NonBlocking(key))
	{
		//Wait for the suspect lock, this call releases the table while blocking.
		LockSuspect(key);
		if(!IsValidKey_NonBlocking(key))
		{
			UnlockSuspect(key);
			pthread_rwlock_unlock(&m_lock);
			return false;
		}
		m_suspectTable[key]->AddEvidence(packet);
		UnlockSuspect(key);
		pthread_rwlock_unlock(&m_lock);
		return true;
	}
	pthread_rwlock_unlock(&m_lock);
	return false;
}
// Updates a suspects evidence and calculates the FeatureSet
//		key: IP address of the suspect as a uint value (host byte order)
// Returns (true) if the call succeeds, (false) if the suspect doesn't exist or doesn't need updating
bool SuspectTable::UpdateSuspect(in_addr_t key)
{
	pthread_rwlock_wrlock(&m_lock);
	if(IsValidKey_NonBlocking(key))
	{
		Suspect* suspect = m_suspectTable[key];
		if(suspect->GetNeedsClassificationUpdate())
		{
			suspect->UpdateEvidence();
			suspect->CalculateFeatures();
			if(!Config::Inst()->GetIsTraining())
			{
				engine->Classify(suspect);
			}
			pthread_rwlock_unlock(&m_lock);
			return true;
		}
	}
	pthread_rwlock_unlock(&m_lock);
	return false;
}

// Updates every suspects evidence and calculates the FeatureSet
void SuspectTable::UpdateAllSuspects()
{
	pthread_rwlock_wrlock(&m_lock);
	for(uint i = 0; i < m_keys.size(); i++)
	{
		if(IsValidKey_NonBlocking(m_keys[i]))
		{
			Suspect* suspect = m_suspectTable[m_keys[i]];
			if(suspect->GetNeedsClassificationUpdate())
			{
				suspect->UpdateEvidence();
				suspect->CalculateFeatures();
				if(!Config::Inst()->GetIsTraining())
				{
					engine->Classify(suspect);
				}
			}
		}
	}
	pthread_rwlock_unlock(&m_lock);
}

// Copies the suspect pointed to in 'suspect', into the table at location key
//		suspect: the Suspect you wish to check in
// Returns (0) on Success, (-1) if the Suspect isn't Checked Out by this thread
// and (-2) if the Suspect does not exist (Key invalid or suspect was deleted)
// Note:  This function blocks until it can acquire a write lock on the suspect
SuspectTableRet SuspectTable::CheckIn(Suspect * suspect)
{
	pthread_rwlock_wrlock(&m_lock);
	in_addr_t key = suspect->GetIpAddress();
	//If the key has a valid suspect
	if(IsValidKey_NonBlocking(key))
	{
		//Attempt to unlock the Suspect, this will tell us if we are allowed to check in this suspect
		switch(pthread_mutex_unlock(&m_lockTable[key].lock))
		{
			//Suspect is not CheckedOut by this thread (called CheckIn without CheckOut)
			case EPERM:
			{
				pthread_rwlock_unlock(&m_lock);
				return SUSPECT_NOT_CHECKED_OUT;
			}
			//If we are allowed to check in the suspect
			case NOERROR:
			{
				m_lockTable[key].ref_cnt--;
				*m_suspectTable[key] = *suspect;
				pthread_rwlock_unlock(&m_lock);
				return SUSPECT_TABLE_CALL_SUCCESS;
			}
			//Any other case we return KEY_INVALID
			default:
			{
				pthread_rwlock_unlock(&m_lock);
				return SUSPECT_KEY_INVALID;
			}
		}
	}
	//Attempt to unlock the Suspect anyway, harmless if the suspect doesn't exist and
	// necessary if it has been deleted while the suspect was checked out->
	// this call will decrease the ref count, unlock the mutex and delete it if needed
	UnlockSuspect(key);
	pthread_rwlock_unlock(&m_lock);
	return SUSPECT_KEY_INVALID;
}

//Releases the lock on the suspect at key allowing another thread to check it out
//		key: IP address of the suspect as a uint value (host byte order)
// Returns (0) on Success, (-1) if the Suspect is checked out by someone else
// and (1) if the Suspect is not checked out
// Note:  This function blocks until it can acquire a write lock on the suspect
SuspectTableRet SuspectTable::CheckIn(in_addr_t key)
{
	pthread_rwlock_wrlock(&m_lock);
	//If the key has a valid suspect
	if(IsValidKey_NonBlocking(key))
	{
		//Attempt to unlock the Suspect, this will tell us if we are allowed to check in this suspect
		switch(pthread_mutex_unlock(&m_lockTable[key].lock))
		{
			//Suspect is not CheckedOut by this thread (called CheckIn without CheckOut)
			case EPERM:
			{
				pthread_rwlock_unlock(&m_lock);
				return SUSPECT_NOT_CHECKED_OUT;
			}
			//If we are allowed to check in the suspect
			case NOERROR:
			{
				m_lockTable[key].ref_cnt--;
				pthread_rwlock_unlock(&m_lock);
				return SUSPECT_TABLE_CALL_SUCCESS;
			}
			//Any other case we return KEY_INVALID
			default:
			{
				pthread_rwlock_unlock(&m_lock);
				return SUSPECT_KEY_INVALID;
			}
		}
	}
	//Attempt to unlock the Suspect anyway, harmless if the suspect doesn't exist and
	// necessary if it has been deleted while the suspect was checked out->
	// this call will decrease the ref count, unlock the mutex and delete it if needed
	UnlockSuspect(key);
	pthread_rwlock_unlock(&m_lock);
	return SUSPECT_KEY_INVALID;
}

//Copies out a suspect and marks the suspect so that it cannot be written or deleted
//		key: IP address of the suspect as a uint value (host byte order)
// Returns a copy of the Suspect associated with 'key', it returns an empty suspect if that key is Invalid.
// Note: This function read locks the suspect until CheckIn(&suspect) or suspect->UnsetOwner() is called.
Suspect SuspectTable::CheckOut(in_addr_t key)
{
	pthread_rwlock_wrlock(&m_lock);
	if(IsValidKey_NonBlocking(key))
	{
		LockSuspect(key);
		if(!IsValidKey_NonBlocking(key))
		{
			UnlockSuspect(key);
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
// 		key: IP address of the suspect as a uint value (host byte order)
// Returns an empty suspect on failure
// Note: To modify or lock a suspect use CheckOut();
// Note: This is the same as GetSuspectStatus except it copies the feature set object which can grow very large.
Suspect SuspectTable::GetSuspect(in_addr_t key)
{
	//Read lock the table, Suspects can only change in the table while it's write locked.
	pthread_rwlock_rdlock(&m_lock);
	if(IsValidKey_NonBlocking(key))
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

// Get a lightweight copy of the suspect using 'key' used only to query basic bools, IP address, and classification.
// 		key: IP address of the suspect as a uint value (host byte order)
// Returns: if the call does not succeed returns an empty/invalid suspect (Classification == -1)
// 		otherwise it returns a shallow copy of the suspect.
// Note: The suspect returned has an empty feature set and evidence vector
Suspect SuspectTable::GetSuspectStatus(in_addr_t key)
{
	pthread_rwlock_rdlock(&m_lock);
	Suspect ret = m_emptySuspect;
	if(IsValidKey_NonBlocking(key))
	{
		Suspect *temp = m_suspectTable[key];
		ret.SetInAddr(temp->GetInAddr());
		ret.SetClassification(temp->GetClassification());
		ret.SetNeedsClassificationUpdate(temp->GetNeedsClassificationUpdate());
		ret.SetIsHostile(temp->GetIsHostile());
		ret.SetIsLive(temp->GetIsLive());
		ret.SetFlaggedByAlarm(temp->GetFlaggedByAlarm());
		pthread_rwlock_unlock(&m_lock);
		return ret;
	}
	pthread_rwlock_unlock(&m_lock);
	return ret;
}

//Erases a suspect from the table if it is not locked
// 		key: IP address of the suspect as a uint value (host byte order)
// Returns (true) on success, (false) if the suspect does not exist (key is invalid)
bool SuspectTable::Erase(in_addr_t key)
{
	pthread_rwlock_wrlock(&m_lock);
	if(IsValidKey_NonBlocking(key))
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
		CleanSuspectLock(key);
		pthread_rwlock_unlock(&m_lock);
		return true;
	}
	pthread_rwlock_unlock(&m_lock);
	return false;
}

// Clears the Suspect Table of all entries
// Note: Locks may still persist until all threads unlock or return from blocking on them.
void SuspectTable::EraseAllSuspects()
{
	pthread_rwlock_wrlock(&m_lock);
	m_keys.clear();
	for(SuspectLockTable::iterator it = m_lockTable.begin(); it != m_lockTable.end(); it++)
	{
		m_lockTable[it->first].deleted = true;
		CleanSuspectLock(it->first);
	}
	for(SuspectHashTable::iterator it = m_suspectTable.begin(); it != m_suspectTable.end(); it++)
	{
		delete m_suspectTable[it->first];
	}
	m_suspectTable.clear();
	pthread_rwlock_unlock(&m_lock);
}

// This function returns a vector of suspects keys the caller can iterate over to access the table.
// Returns a std::vector of every suspect currently in the table
vector<uint64_t> SuspectTable::GetAllKeys()
{
	vector<uint64_t> ret;
	ret.clear();
	pthread_rwlock_rdlock(&m_lock);
	for(uint i = 0; i < m_keys.size(); i++)
	{
		if(IsValidKey_NonBlocking(m_keys[i]))
		{
			ret.push_back(m_keys[i]);
		}
	}
	pthread_rwlock_unlock(&m_lock);
	return ret;
}

// This function returns a vector of suspects keys the caller can iterate over to access the table.
// Returns a std::vector containing all hostile suspect keys
vector<uint64_t> SuspectTable::GetKeys_of_HostileSuspects()
{
	vector<uint64_t> ret;
	ret.clear();
	pthread_rwlock_rdlock(&m_lock);
	for(uint i = 0; i < m_keys.size(); i++)
	{
		if(IsValidKey_NonBlocking(m_keys[i]))
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

// This function returns a vector of suspects keys the caller can iterate over to access the table.
// Returns a std::vector containing all benign suspect keys
vector<uint64_t> SuspectTable::GetKeys_of_BenignSuspects()
{
	vector<uint64_t> ret;
	ret.clear();
	pthread_rwlock_rdlock(&m_lock);
	for(uint i = 0; i < m_keys.size(); i++)
	{
		if(IsValidKey_NonBlocking(m_keys[i]))
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

// This function returns a vector of suspects keys the caller can iterate over to access the table.
// Returns a std::vector containing the keys of all suspects that need a classification update.
vector<uint64_t> SuspectTable::GetKeys_of_ModifiedSuspects()
{
	vector<uint64_t> ret;
	ret.clear();
	pthread_rwlock_rdlock(&m_lock);
	for(uint i = 0; i < m_keys.size(); i++)
	{
		if(IsValidKey_NonBlocking(m_keys[i]))
		{
			if(m_suspectTable[m_keys[i]]->GetNeedsClassificationUpdate())
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

void SuspectTable::SaveSuspectsToFile(string filename)
{
	pthread_rwlock_rdlock(&m_lock);
	LOG(NOTICE, "Saving Suspects...", "Saving Suspects in a text format to file "+filename);
	ofstream out(filename.c_str());
	if(!out.is_open())
	{
		LOG(ERROR,"Unable to open file requested file.",
			"Unable to open or create file located at "+filename+".");
		pthread_rwlock_unlock(&m_lock);
		return;
	}
	for(uint i = 0; i < m_keys.size(); i++)
	{
		if(IsValidKey_NonBlocking(m_keys[i]))
		{
			out << m_suspectTable[m_keys[i]]->ToString() << endl;
		}
	}
	out.close();
	pthread_rwlock_unlock(&m_lock);
}

//Iterates over the table, serializing each suspect and dumping the raw data to out
//		out: ofstream you wish to write the contents to
//		timestamp: the timestamp you wish to mark the table's contents with
//Note: Information can be retrieved using ReadContents
uint32_t SuspectTable::DumpContents(ofstream *out, time_t saveTime)
{
	pthread_rwlock_rdlock(&m_lock);
	//Get save time
	if(!out->is_open())
	{
		LOG(WARNING, "Unable to open CE state file.", "");
	}
	if(saveTime == 0)
	{
		saveTime = time(NULL);
	}

	uint32_t ret = 0, dataSize = 0;
	u_char tableBuffer[MAX_MSG_SIZE];
	for(uint i = 0; i < m_keys.size(); i++)
	{
		if(IsValidKey_NonBlocking(m_keys[i]))
		{
			Suspect * suspect = m_suspectTable[m_keys[i]];
			if(!suspect->GetFeatureSet(MAIN_FEATURES).m_packetCount
				&& !suspect->GetFeatureSet(UNSENT_FEATURES).m_packetCount)
			{
				continue;
			}
			dataSize += suspect->GetSerializeLength(ALL_FEATURE_DATA);
		}
	}
	if(dataSize)
	{
		out->write((char*)&saveTime, sizeof saveTime);
		ret += sizeof saveTime;
		out->write((char*)&dataSize, sizeof dataSize);
		ret += sizeof dataSize;
		for(uint i = 0; i < m_keys.size(); i++)
		{
			if(IsValidKey_NonBlocking(m_keys[i]))
			{
				Suspect * suspect = m_suspectTable[m_keys[i]];
				if(!suspect->GetFeatureSet(MAIN_FEATURES).m_packetCount
					&& !suspect->GetFeatureSet(UNSENT_FEATURES).m_packetCount)
				{
					continue;
				}
				dataSize = suspect->Serialize(tableBuffer, ALL_FEATURE_DATA);
				out->write((char*) tableBuffer, dataSize);
				ret += dataSize;
			}
		}
	}
	pthread_rwlock_unlock(&m_lock);
	return ret;
}

uint32_t SuspectTable::ReadContents(ifstream *in, time_t expirationTime)
{
	pthread_rwlock_wrlock(&m_lock);
	uint32_t ret = 0;
	if(!in->is_open())
	{
		LOG(WARNING, "Unable to open CE state file.", "");
	}
	uint32_t cur = in->tellg();
	in->seekg(0,ifstream::end);
	uint32_t lengthLeft = (uint32_t)in->tellg() - cur;

	in->seekg(cur, ifstream::beg);

	time_t saveTime = 0;
	uint32_t dataSize = 0;

	while(in->is_open() && !in->eof() && lengthLeft)
	{
		// Bytes left, but not enough to make a header (timestamp + size)?
		if(lengthLeft < (sizeof expirationTime + sizeof dataSize))
		{
			LOG(ERROR, "The state file may be corruput", "");
			pthread_rwlock_unlock(&m_lock);
			return 0;
		}

		in->read((char*) &saveTime, sizeof saveTime);
		lengthLeft -= sizeof saveTime;
		in->read((char*) &dataSize, sizeof dataSize);
		lengthLeft -= sizeof dataSize;

		// Not as many bytes left as the size of the entry?
		if(lengthLeft < dataSize)
		{
			LOG(ERROR, "The CE state file may be corruput", "");
			pthread_rwlock_unlock(&m_lock);
			return 0;
		}
		if(saveTime < expirationTime)
		{
			stringstream ss;
			ss << "Throwing out old CE state at time: " << saveTime << ".";
			LOG(DEBUG,"Throwing out old CE state.", ss.str());
			in->seekg(dataSize, ifstream::cur);
			pthread_rwlock_unlock(&m_lock);
			return (sizeof(expirationTime) + sizeof(dataSize) + dataSize);
		}

		u_char tableBuffer[dataSize];
		in->read((char*) tableBuffer, dataSize);
		lengthLeft -= dataSize;

		// Read each suspect
		uint32_t offset = 0;
		while(offset < dataSize)
		{
			Suspect* newSuspect = new Suspect();

			try {
				offset += newSuspect->Deserialize(tableBuffer+ offset, ALL_FEATURE_DATA);
			} catch (Nova::hashMapException& e) {
				LOG(ERROR, "The state file may be corrupt, a hash table invalid key exception was caught during deserialization", "");
				pthread_rwlock_unlock(&m_lock);
				return 0;
			}

			in_addr_t key = newSuspect->GetIpAddress();
			// If our suspect has no evidence, throw it out
			if(!newSuspect->GetFeatureSet(MAIN_FEATURES).m_packetCount
					&& !newSuspect->GetFeatureSet(UNSENT_FEATURES).m_packetCount)
			{
				LOG(WARNING,"Discarding invalid suspect.",
					"A suspect containing no evidence was detected and discarded");
				delete newSuspect;
				continue;
			}

			if(IsValidKey_NonBlocking(key))
			{
				LockSuspect(key);
				newSuspect->UpdateFeatureData(INCLUDE);
				FeatureSet fs = newSuspect->GetFeatureSet(MAIN_FEATURES);
				m_suspectTable[key]->AddFeatureSet(&fs, MAIN_FEATURES);
				m_suspectTable[key]->SetNeedsClassificationUpdate(true);
				UnlockSuspect(key);
				delete newSuspect;
				continue;
			}
			else
			{
				//If there is already a SuspectLock this Suspect is listed for deletion but it's lock still exists
				if(m_lockTable.keyExists(key))
				{
					//Wait for the suspect lock, this call releases the table while blocking.
					LockSuspect(key);
					//Reverse the deletion flag
					m_lockTable[key].deleted = false;
					newSuspect->SetNeedsClassificationUpdate(true);
					//Allocate the Suspect and copy the contents
					m_suspectTable[key] = newSuspect;
					//Store the key
					m_keys.push_back(key);
					UnlockSuspect(key);
				}
				else //Else we need to init a SuspectLock
				{
					pthread_mutexattr_t tempAttr;
					pthread_mutexattr_init(&tempAttr);
					pthread_mutexattr_settype(&tempAttr, PTHREAD_MUTEX_ERRORCHECK);
					//Destroy before init just to be safe
					pthread_mutex_destroy(&m_lockTable[key].lock);
					pthread_mutex_init(&m_lockTable[key].lock, &tempAttr);
					m_lockTable[key].deleted = false;
					m_lockTable[key].ref_cnt = 0;
					newSuspect->SetNeedsClassificationUpdate(true);
					newSuspect->UpdateFeatureData(INCLUDE);
					//Allocate the Suspect and copy the contents
					m_suspectTable[key] = newSuspect;
					//Store the key
					m_keys.push_back(key);
				}
			}
		}
	}
	ret = (uint32_t)in->tellg() - cur;
	pthread_rwlock_unlock(&m_lock);
	return ret;
}


// Checks the validity of the key - public thread-safe version
//		key: IP address of the suspect as a uint value (host byte order)
// Returns true if there is a suspect associated with the given key, false otherwise
bool SuspectTable::IsValidKey(in_addr_t key)
{
	pthread_rwlock_rdlock(&m_lock);
	//If we find a SuspectLock the suspect exists or is scheduled to be deleted
	bool ret = IsValidKey_NonBlocking(key);
	pthread_rwlock_unlock(&m_lock);
	return ret;
}

bool SuspectTable::IsEmptySuspect(Suspect * suspect)
{
	return (suspect->GetClassification() == EMPTY_SUSPECT_CLASSIFICATION);
}
// Checks the validity of the key - private use non-locking version
//		key: IP address of the suspect as a uint value (host byte order)
// Returns true if there is a suspect associated with the given key, false otherwise
// *Note: Assumes you have already locked the table
bool SuspectTable::IsValidKey_NonBlocking(in_addr_t key)
{
	//If we find a SuspectLock the suspect exists or is scheduled to be deleted
	if(m_lockTable.keyExists(key))
	{
		return !m_lockTable[key].deleted;
	}
	return false;
}
//Used internally by threads to lock on a suspect
// 		key: IP address of the suspect as a uint value (host byte order)
// Returns (true) after locking the suspect and (false) if the suspect couldn't be found
// Note: A suspect's lock won't be deleted until there are no more threads trying to lock it
// Imporant: table MUST be write locked before calling this.
bool SuspectTable::LockSuspect(in_addr_t key)
{
	//If the suspect has a lock
	if(m_lockTable.keyExists(key))
	{
		m_lockTable[key].ref_cnt++;
		pthread_rwlock_unlock(&m_lock);
		pthread_mutex_lock(&m_lockTable[key].lock);
		pthread_rwlock_wrlock(&m_lock);
		return true;
	}
	return false;
}

//Used internally by threads when done accesses a suspect
// 		key: IP address of the suspect as a uint value (host byte order)
// Returns (true) if the Lock could be unlocked and still exists and
// (false) if the Suspect has been deleted or could not be unlocked.
// Note: automatically deletes the lock if the suspect has been deleted and the ref count is 0
bool SuspectTable::UnlockSuspect(in_addr_t key)
{
	//If the suspect has a lock
	if(m_lockTable.keyExists(key))
	{
		m_lockTable[key].ref_cnt--;
		//If unlock fails
		if(pthread_mutex_unlock(&m_lockTable[key].lock))
		{
			return false;
		}
		return !CleanSuspectLock(key);
	}
	return false;
}

//Used internally, Calls to this function check if ref_cnt is 0 and deleted == true
// if so then we remove the Suspect lock, this is done to prevent destroying a lock a thread is blocking on
// 		key: IP address of the suspect as a uint value (host byte order)
// Returns (true) if the Lock doesn't exist or it was successfully removed
// false if threads are blocking on it or the Suspect has not been erased
bool  SuspectTable::CleanSuspectLock(in_addr_t key)
{
	//If the suspect has a lock
	if(m_lockTable.keyExists(key))
	{
		if((m_lockTable[key].ref_cnt <= 0) && m_lockTable[key].deleted)
		{
			m_lockTable[key].ref_cnt = 0;
			pthread_mutex_destroy(&m_lockTable[key].lock);
			m_lockTable.erase(key);
			return true;
		}
		return false;
	}
	return true;
}

}
