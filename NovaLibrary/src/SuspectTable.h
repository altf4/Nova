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
#include "SuspectTableIterator.h"

typedef google::dense_hash_map<uint64_t, Suspect *, tr1::hash<uint64_t>, eqkey > SuspectHashTable;

namespace Nova {

enum SuspectTableRet : int32_t
{
	KEY_INVALID = -2, //The key cannot be associated with a recognized suspect
	SUSPECT_CHECKED_OUT = -1, //If the suspect is checked out by another thread,
	SUCCESS = 0, //The call succeeded
	SUSPECT_NOT_CHECKED_OUT = 1, //The suspect isn't checked out, only returned by CheckIn()
	SUSPECT_EXISTS = 2 //If the suspect already exists, only returned by AddNewSuspect()
};

class SuspectTable
{

public:

	// Default Constructor for SuspectTable
	SuspectTable();

	// Default Deconstructor for SuspectTable
	~SuspectTable();

	// Get a SuspectTableIterator that points to the beginning of the table
	// Returns the SuspectTableIterator
	SuspectTableIterator Begin();

	// Get a SuspectTableIterator that points to the last element in the table;
	// Returns the SuspectTableIterator
	SuspectTableIterator End();

	// Get a SuspectTableIterator that points to the element at key;
	// 		key: in_addr_t of the Suspect
	// Returns a SuspectTableIterator, if the suspect cannot be found the iterator's index is equal to the table's size
	// Note: there are no guarantees about the state or existence of the Suspect in the table after this call.
	SuspectTableIterator Find(in_addr_t  key);

	// Adds the Suspect pointed to in 'suspect' into the table using suspect->GetIPAddress() as the key;
	// 		suspect: pointer to the Suspect you wish to add
	// Returns (0) on Success, and (2) if the suspect exists;
	SuspectTableRet AddNewSuspect(Suspect * suspect);

	// Adds the Suspect pointed to in 'suspect' into the table using the source of the packet as the key;
	// 		packet: copy of the packet you whish to create a suspect from
	// Returns (0) on Success, and (2) if the suspect exists;
	SuspectTableRet AddNewSuspect(Packet packet);

	// Copies the suspect pointed to in 'suspect', into the table at location key
	// 		suspect: pointer to the Suspect you wish to copy in
	// Returns (0) on Success, (-1) if the Suspect is checked out by someone else
	// and (1) if the Suspect is not checked out
	// Note:  This function blocks until it can acquire a write lock on the suspect
	// IP address must be the same as key checked out with
	SuspectTableRet CheckIn(Suspect * suspect);

	// Copies out a suspect and marks the suspect so that it cannot be written or deleted
	// 		key: in_addr_t of the Suspect
	// Returns empty Suspect on failure.
	// Note: A 'Checked Out' Suspect cannot be modified until is has been replaced by the suspect 'Checked In'
	// 		However the suspect can continue to be read. It is similar to having a read lock.
	Suspect CheckOut(in_addr_t key);

	// Lookup and get an Asynchronous copy of the Suspect
	// 		key: in_addr_t of the Suspect
	// Returns an empty suspect on failure
	// Note: there are no guarantees about the state or existence of the Suspect in the table after this call.
	Suspect Peek(in_addr_t  key);

	// Erases a suspect from the table if it is not locked
	// 		key: in_addr_t of the Suspect
	// Returns 0 on success.
	SuspectTableRet Erase(in_addr_t key);

	// Iterates over the Suspect Table and returns a vector containing each Hostile Suspect's in_addr_t
	// Returns a vector of hostile suspect in_addr_t keys
	vector<in_addr_t> GetHostileSuspectKeys();

	// Iterates over the Suspect Table and returns a vector containing each Benign Suspect's in_addr_t
	// Returns a vector of benign suspect in_addr_t keys
	vector<in_addr_t> GetBenignSuspectKeys();

	// Looks at the hostility of the suspect at key
	// 		key: in_addr_t of the Suspect
	// Returns (0) for Benign, (1) for Hostile, and (-2) if the key is invalid
	SuspectTableRet GetHostility(in_addr_t key);

	// Get the size of the Suspect Table
	// Returns the size of the Table
	uint Size();

	// Resizes the table to the given size.
	//		size: the number of bins to use
	// Note: Choosing an initial size that covers normal usage can improve performance.
	void Resize(uint size);

	// Clears the Suspect Table of all entries
	//		blockUntilDone: if this value is set to true the function will block until the table can be safely cleared
	// Returns (0) on Success, and (-1) if the table cannot be cleared safely
	SuspectTableRet Clear(bool blockUntilDone = true);

	// Checks the validity of the key
	//		key: The in_addr_t of the Suspect
	// Returns true if there is a suspect associated with the given key, false otherwise
	bool IsValidKey(in_addr_t key);

	// Write locks the suspect
	// Note: This function will block until the lock is acquired
	void Wrlock();

	// Write locks the suspect
	// Returns (true) if the lock was acquired, (false) if the table is locked by someone else
	bool TryWrlock();

	// Read locks the suspect
	// Note: This function will block until the lock is acquired
	void Rdlock();

	// Read locks the suspect
	// Returns (true) if the lock was acquired, (false) if the table is locked by someone else
	bool TryRdlock();

	// Unlocks the suspect
	// Returns: (true) If the table was already or successfully unlocked, (false) if the table is locked by someone else
	bool Unlock();

	//Returns a reference to a suspect,
	Suspect& operator[](in_addr_t key);

	//Returns a reference to a suspect,
	Suspect& operator[](uint64_t realKey);

	Suspect m_emptySuspect;

//protected:

	// Google Dense Hashmap used for constant time key lookups
	SuspectHashTable m_table;

	// Lock used to maintain concurrency between threads
	pthread_rwlock_t m_lock;

	vector<uint64_t> m_keys;



private:

	// Returns true if the current thread has a lock on the Table
	bool IsOwner();

	uint64_t m_empty_key;

	uint64_t m_deleted_key;

	vector<pthread_t> m_owners;

};

}

#endif /* SUSPECTTABLE_H_ */
