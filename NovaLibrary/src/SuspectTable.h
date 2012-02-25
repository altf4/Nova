//============================================================================
// Name        : SuspectTable.h
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
//		list of suspects.
//============================================================================/*

#ifndef SUSPECTTABLE_H_
#define SUSPECTTABLE_H_

#include "Suspect.h"
#include <arpa/inet.h>
#include <vector>

typedef google::dense_hash_map<in_addr_t, Suspect *, tr1::hash<in_addr_t>, eqaddr > SuspectHashTable;

namespace Nova {

class SuspectTable
{

public:

	// Default Constructor for SuspectTable
	SuspectTable();

	// Default Deconstructor for SuspectTable
	~SuspectTable();

	Suspect * find(in_addr_t key);

	int insert(in_addr_t key, Suspect * suspect);

	int remove(in_addr_t key);

	vector<Suspect *> getHostileSuspects();

	vector<Suspect *> getBenignSuspects();

	bool isHostile(in_addr_t key);

	bool isBenign(in_addr_t key);

private:

	SuspectHashTable table;

	bool isValidKey(in_addr_t key);

	bool keyInUse(in_addr_t key);

};

}

#endif /* SUSPECTTABLE_H_ */
