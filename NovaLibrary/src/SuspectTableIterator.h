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

typedef google::dense_hash_map<in_addr_t, Suspect *, tr1::hash<in_addr_t>, eqaddr > SuspectHashTable;

namespace Nova {

class SuspectTableIterator
{

public:
	// Default iterator constructor
	SuspectTableIterator(SuspectHashTable * table = NULL, vector<in_addr_t> * keys = NULL);

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

	SuspectHashTable * m_table_ref;

	vector<in_addr_t> * m_keys_ref;

};
}

#endif /* SUSPECTTABLEITERATOR_H_ */
