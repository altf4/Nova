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

typedef google::dense_hash_map<in_addr_t, Suspect *, tr1::hash<in_addr_t>, eqaddr > SuspectHashTable;


namespace Nova {

enum SuspectTableTableRet{Invalid = -2, CheckedOut = -1,
	Success = 0, NotCheckedOut = 1, Exists = 2};

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
	// Returns 1 on failure, 0
	// Note: there are no guarantees about the state or existence of the Suspect in the table after this call.
	SuspectTableIterator Find(in_addr_t  key);

	// Adds the Suspect pointed to in 'suspect' into the table using suspect->GetIPAddress() as the key;
	// 		suspect: pointer to the Suspect you wish to add
	// Returns (0) on Success, and (2) if the suspect exists;
	int AddNewSuspect(Suspect * suspect);

	// Copies the suspect pointed to in 'suspect', into the table at location key
	// 		suspect: pointer to the Suspect you wish to copy in
	// Returns (0) on Success, (-1) if the Suspect is checked out by someone else
	// and (1) if the Suspect is not checked out
	// Note:  This function blocks until it can acquire a write lock on the suspect
	// IP address must be the same as key checked out with
	int CheckIn(Suspect * suspect);

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
	int Erase(in_addr_t key);

	// Iterates over the Suspect Table and returns a vector containing each Hostile Suspect's in_addr_t
	// Returns a vector of hostile suspect in_addr_t keys
	vector<in_addr_t> GetHostileSuspectKeys();

	// Iterates over the Suspect Table and returns a vector containing each Benign Suspect's in_addr_t
	// Returns a vector of benign suspect in_addr_t keys
	vector<in_addr_t> GetBenignSuspectKeys();

	// Looks at the hostility of the suspect at key
	// 		key: in_addr_t of the Suspect
	// Returns 0 for Benign, 1 for Hostile, and -2 if the key is invalid
	int GetHostility(in_addr_t key);

	// Get the size of the Suspect Table
	// Returns the size of the Table
	uint Size();


protected:

	// Google Dense Hashmap used for constant time key lookups
	SuspectHashTable m_table;

	// Lock used to maintain concurrency between threads
	pthread_rwlock_t m_lock;

	vector<in_addr_t> m_keys;

	// Checks the validity of the key
	// Returns true if there is a suspect associated with the given key, false otherwise
	bool IsValidKey(in_addr_t key);

};

}

#endif /* SUSPECTTABLE_H_ */
