/*
 * PersonalityTree.h
 *
 *  Created on: Jun 4, 2012
 *      Author: victim
 */

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
	void InsertPersonality(Personality *pers);

	//Prints each child of the root node in the tree as a string
	void ToString();

private:

	void UpdatePersonality(Personality *pers, PersonalityNode *parent);
	//Empty 'root' node of the tree, this node can be treated as the 'any' case or all personalities.
	PersonalityNode m_root;

	std::vector<PersonalityNode *> m_nodes;

	void RecursiveToString(PersonalityNode * persNode);

};

}

#endif /* PERSONALITYTREE_H_ */
