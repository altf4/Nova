//============================================================================
// Name        : SuspectTableIterator.cpp
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
// Description : Iterator used for traversing over a Suspect Table
//============================================================================/*

#include "SuspectTableIterator.h"

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

}
