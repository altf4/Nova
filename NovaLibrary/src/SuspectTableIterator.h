// ============================================================================
// Name        : SuspectTableIterator.h
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
// Description : Iterator used for traversing over a Suspect Table
// ============================================================================/*

#ifndef SUSPECTTABLEITERATOR_H_
#define SUSPECTTABLEITERATOR_H_

#include <vector>
#include <arpa/inet.h>

#include "Suspect.h"

typedef google::dense_hash_map<uint64_t, Nova::Suspect *, std::tr1::hash<uint64_t>, eqkey > SuspectHashTable;

namespace Nova {

class SuspectTableIterator
{

public:
	// Default iterator constructor
	SuspectTableIterator(SuspectHashTable *table, std::vector<uint64_t> *keys, pthread_rwlock_t *lock);

	// Default iterator deconstructor
	~SuspectTableIterator();

	// Gets the Next Suspect in the table and increments the iterator
	// Returns a copy of the Suspect
	Suspect Next();

	// Gets the Next Suspect in the table, does not increment the iterator
	// Returns a copy of the Suspect
	Suspect LookAhead();

	// Gets the Previous Suspect in the Table and decrements the iterator
	// Returns a copy of the Suspect
	Suspect Previous();

	// Gets the Previous Suspect in the Table, does not decrement the iterator
	// Returns a copy of the Suspect
	Suspect LookBack();

	// Gets the Current Suspect in the Table
	// Returns a copy of the Suspect
	Suspect Current();

	// Gets a reference to the index of the iterator
	// Returns a reference to m_index
	uint GetIndex();

	// Returns the in_addr_t associated with the current iterator's position
	uint64_t GetKey();

	// Increments the iterator by 1
	// Returns 'this'
	SuspectTableIterator operator++();
	// Increments the iterator by int 'rhs'
	// Returns 'this'
	SuspectTableIterator operator+=(int rhs);

	// Decrements the iterator by 1
	// Returns 'this'
	SuspectTableIterator operator--();
	// Decrements the iterator by int 'rhs'
	// Returns 'this'
	SuspectTableIterator operator-=(int rhs);

	// Checks if the iterator 'rhs' is equal to 'this'
	bool operator!=(SuspectTableIterator rhs);

	// Checks if the iterator 'rhs' is not equal to 'this'
	bool operator==(SuspectTableIterator rhs);

private:

	uint m_index;

	SuspectHashTable& m_table_ref;

	std::vector<uint64_t>& m_keys_ref;

	pthread_rwlock_t& m_lock_ref;

};
}

#endif /* SUSPECTTABLEITERATOR_H_ */
