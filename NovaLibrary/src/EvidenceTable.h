//============================================================================
// Name        : EvidenceTable.h
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
// Description : The EvidenceTable manages a series of EvidenceQueues paired with a suspect's
//				host ordered ipv4 address
//============================================================================/*

#ifndef EVIDENCETABLE_H_
#define EVIDENCETABLE_H_

#include "HashMapStructs.h"
#include "EvidenceQueue.h"

typedef Nova::HashMap<uint64_t, Nova::EvidenceQueue*, std::tr1::hash<uint64_t>, eqkey > EvidenceHashTable;

namespace Nova
{

class EvidenceTable
{

public:

	// Default Constructor for EvidenceTable
	EvidenceTable();

	// Default Deconstructor for EvidenceTable
	~EvidenceTable();

	//Inserts the Evidence into the table at the location specified by the destination address
	void InsertEvidence(Evidence * packet);

	// Returns the first evidence object in a Evidence linked list or NULL if no evidence for any entries
	// After use each Evidence object must be explicitly deallocated
	Evidence * GetEvidence();

private:

	EvidenceQueue m_processingList;
	EvidenceHashTable m_table;
	pthread_mutex_t m_lock;
	pthread_cond_t m_cond;
};

}

#endif /* EVIDENCETABLE_H_ */
