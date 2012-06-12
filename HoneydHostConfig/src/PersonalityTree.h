//============================================================================
// Name        : PersonalityTree.h
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
// Description : TODO
//============================================================================

#ifndef PERSONALITYTREE_H_
#define PERSONALITYTREE_H_

#include "PersonalityNode.h"
#include "PersonalityTable.h"

namespace Nova
{

class PersonalityTree
{

public:

	PersonalityTree(PersonalityTable *persTable = NULL);
	~PersonalityTree();

	void LoadTable(PersonalityTable *persTable);

	void GenerateProfiles(PersonalityNode *node, PersonalityNode *parent, profile *parentProfile);

	void InsertPersonality(Personality *pers);

	//Prints each child of the root node in the tree as a string
	void ToString();

	void GenerateDistributions();

	void ToXmlTemplate();

	//Dummy function def -> implement to produce fuzzy output from populated table
	void GenerateFuzzyOutput();

	// Dummy function def ->
	// Generate a haystack that matches only what is seen and to near exact ratios, essentially duplicating the network n times until it's full.
	void GenerateExactOutput();

	void DebugPrintProfileTable();

	void AddAllPorts();

private:

	void UpdatePersonality(Personality *pers, PersonalityNode *parent);
	//Empty 'root' node of the tree, this node can be treated as the 'any' case or all personalities.
	PersonalityNode m_root;

	std::vector<PersonalityNode *> m_nodes;

	std::vector<PersonalityNode *> m_to_delete;

	ProfileTable * m_profiles;

	HoneydConfiguration * hhconfig;

	void RecursiveToString(PersonalityNode * persNode);
	void RecursiveGenerateDistributions(PersonalityNode * node);
	void RecursiveAddAllPorts(PersonalityNode * node);
	void RecursivePrintTree(PersonalityNode * node);
};

}

#endif /* PERSONALITYTREE_H_ */
