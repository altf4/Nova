// ============================================================================
// Name        : SuspectTable.h
// Copyright   : DataSoft Corporation 2011-2012
// 	Nova is free software: you can redistribute it and/or modify
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
//   along with Nova.  If not, see <http:// www.gnu.org/licenses/>.
// Description : Wrapper class for the SuspectHashMap object used to maintain a
// 		list of suspects.
// ============================================================================/*

#ifndef SUSPECTTABLE_H_
#define SUSPECTTABLE_H_

#include <arpa/inet.h>
#include <vector>

#include "Suspect.h"

typedef google::dense_hash_map<uint64_t, Nova::Suspect *, std::tr1::hash<uint64_t>, eqkey > SuspectHashTable;

struct SuspectLock
{
	int ref_cnt;
	pthread_mutex_t lock;
	bool deleted;
};

typedef google::dense_hash_map<uint64_t,SuspectLock, std::tr1::hash<uint64_t>, eqkey> SuspectLockTable;

namespace Nova {

enum SuspectTableRet : int32_t
{
	SUSPECT_KEY_INVALID = -2, //The key cannot be associated with (or assigned to) a recognized suspect
	SUSPECT_NOT_CHECKED_OUT = -1, //The suspect isn't checked out by this thread
	SUSPECT_TABLE_CALL_SUCCEESS = 0, //The call succeeded
};

class SuspectTable
{

public:

	// Default Constructor for SuspectTable
	SuspectTable();

	// Default Deconstructor for SuspectTable
	~SuspectTable();

	// Adds the Suspect pointed to in 'suspect' into the table using suspect->GetIPAddress() as the key;
	// 		suspect: pointer to the Suspect you wish to add
	// Returns true on Success, and false if the suspect already exists
	bool AddNewSuspect(Suspect *suspect);

	// Adds the Suspect pointed to in 'suspect' into the table using the source of the packet as the key;
	// 		packet: copy of the packet you whish to create a suspect from
	// Returns true on Success, and false if the suspect already exists
	bool AddNewSuspect(Packet packet);

	//XXX
	bool AddEvidenceToSuspect(in_addr_t key, Packet packet);

	// Copies the suspect pointed to in 'suspect', into the table location associated with key
	// 		suspect: pointer to the Suspect you wish to copy in
	// Returns (0) on Success, (-1) if the Suspect is Checked Out by another thread
	// and (-2) if the Suspect does not exist (Key invalid or suspect was deleted)
	// Note:  This function blocks until it can acquire a write lock on the suspect
	// IP address must be the same as key checked out with
	SuspectTableRet CheckIn(Suspect * suspect);

	//Releases the lock on the suspect at key allowing another thread to check it out
	//		key: IP address of the suspect as a uint value (host byte order)
	// Returns (0) on Success, (-1) if the Suspect is checked out by someone else
	// and (1) if the Suspect is not checked out
	// Note:  This function blocks until it can acquire a write lock on the suspect
	SuspectTableRet CheckIn(in_addr_t key);

	// Copies out a suspect and marks the suspect so that it cannot be written or deleted
	// 		key: IP address of the suspect as a uint value (host byte order)
	// Returns empty Suspect on failure.
	// Note: A 'Checked Out' Suspect cannot be modified until is has been replaced by the suspect 'Checked In'
	// 		However the suspect can continue to be read. It is similar to having a read lock.
	Suspect CheckOut(in_addr_t ipAddr);

	// Lookup and get an Asynchronous copy of the Suspect
	// 		key: IP address of the suspect as a uint value (host byte order)
	// Returns an empty suspect on failure
	// Note: To modify or lock a suspect use CheckOut();
	// Note: This is the same as GetSuspectStatus except it copies the feature set object which can grow very large.
	Suspect GetSuspect(in_addr_t ipAddr);

	// Lookup and get an Asynchronous copy of the Suspect
	// 		key: IP address of the suspect as a uint value (host byte order)
	// Returns an empty suspect on failure
	// Note: To modify or lock a suspect use CheckOut();
	// Note: This is the same as GetSuspect except is does not copy the feature set object which can grow very large.
	Suspect GetSuspectStatus(in_addr_t ipAddr);

	// Erases a suspect from the table if it is not locked
	// 		key: IP address of the suspect as a uint value (host byte order)
	// Returns 0 on success.
	SuspectTableRet Erase(in_addr_t ipAddr);

	// Iterates over the Suspect Table and returns a std::vector containing each Hostile Suspect's uint64_t
	// Returns a std::vector of hostile suspect in_addr_t ipAddrs
	std::vector<uint64_t> GetHostileSuspectKeys();

	// Iterates over the Suspect Table and returns a std::vector containing each Benign Suspect's uint64_t
	// Returns a std::vector of benign suspect in_addr_t ipAddrs
	std::vector<uint64_t> GetBenignSuspectKeys();

	// Get the size of the Suspect Table
	// Returns the size of the Table
	uint Size();

	// Resizes the table to the given size.
	//		size: the number of bins to use
	// Note: Choosing an initial size that covers normal usage can improve performance.
	void Resize(uint size);

	// Clears the Suspect Table of all entries
	// Returns (0) on Success, and (-1) if the table cannot be cleared safely
	// Note: Locks may still persist until all threads unlock or return from blocking on them.
	void EraseAllSuspects();

	// Saves suspectTable to a human readable text file
	void SaveSuspectsToFile(std::string filename);

	void UpdateAllSuspects(); //XXX
	void UpdateSuspect(Suspect *); //XXX

	// Checks the validity of the key - public thread-safe version
	//		key: The in_addr_t of the Suspect
	// Returns true if there is a suspect associated with the given key, false otherwise
	bool IsValidKey(in_addr_t ipAddr);

private:

	uint64_t m_empty_key;

	uint64_t m_deleted_key;

	Suspect m_emptySuspect;

	// Google Dense Hashmap used for constant time key lookups
	SuspectHashTable m_suspectTable;
	SuspectLockTable m_lockTable;

	// Lock used to maintain concurrency between threads
	pthread_rwlock_t m_lock;

	std::vector<uint64_t> m_keys;

	// Checks the validity of the key - private use non-locking version
	//		key: The uint64_t of the Suspect
	// Returns true if there is a suspect associated with the given key, false otherwise
	// *Note: Assumes you have already locked the table
	bool IsValidKey_NoLocking(in_addr_t key);

};

}

#endif /* SUSPECTTABLE_H_ */
