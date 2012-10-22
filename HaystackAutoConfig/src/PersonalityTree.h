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
// Description : Header for the PersonalityTree class; contains the function
//               contracts and declarations for the PersonalityTree cpp file,
//               as well as the member variables for the class.
//============================================================================

#ifndef PERSONALITYTREE_H_
#define PERSONALITYTREE_H_

#include "PersonalityTreeItem.h"
#include "PersonalityTable.h"
#include "HoneydConfiguration/ServiceToScriptMap.h"

namespace Nova
{

class PersonalityTree
{

public:

	// Default constructor that initializes the relevant member variables and
	// calls different functions both within this class and others to generate
	// the actual tree structure.
	//  PersonalityTable *persTable - the personality table containing the personalities
	//                           with which to create the tree structure.
	//  std::vector<Subnet> &subnetsToUse - a vector of subnet objects that were found in
	//                           the main HoneydHostConfig class. Used to add the subnets
	//                           to the HoneydConfiguration object.
	PersonalityTree(PersonalityTable *persTable, std::vector<Subnet> &subnetsToUse);
	~PersonalityTree();

	// Returns a random (leaf) profile, according to the distributions given
	//	returns - A valid PersonalityNode on success, NULL on error
	PersonalityTreeItem *GetRandomProfile();

	HoneydConfiguration *m_hdconfig;

private:
	// LoadTable first iterates over the persTable object and calls InsertPersonality on each
	// Personality* within the HashMap structure. It then calls GenerateProfiles recursively on
	// each subtree of the root node. Note that authentically this method is called within
	// the constructor, and thus persTable is passed down from the constructor's arguements;
	// if you want to generate a different tree structure from a different table, you need to
	// create a new instance of PersonalityTree.
	//  PersonalityTable *persTable - personalityTable to use for populating the ProfileTable
	//                           in the m_hdconfig object's m_profiles HashMap.
	// Returns nothing.
	bool LoadTable(PersonalityTable *persTable);

	// GenerateProfiles will recurse through the tree structure and create NodeProfile objects
	// from each PersonalityNode in the tree. Also performs compression in the profile table;
	// if node's parent has no other children than node, then rename the parentProfile struct
	// to parentProfile's name + " " + the profileName, which is a concatenation of previous
	// compressions.
	//  PersonalityNode *node - the node that GenerateProfiles is currently at within the tree
	//							structure. Used to generate a NodeProfile struct, as well as
	//							calculate distributions.
	//  PersonalityNode *parent - used for context for node. there is a special case for when
	//                          node has siblings that must be taken into account.
	//  NodeProfile *parentProfile - used for renaming
	//  const std::string &profileName - also used for a different renaming case
	// Returns nothing.
	bool GenerateProfiles(PersonalityTreeItem *node, PersonalityTreeItem *parent, NodeProfile *parentProfile, const std::string &profileName);

	// InsertPersonality serves as the starting point for UpdatePersonality.
	//  Personality *pers - the Personality to Insert/Update
	// Returns nothing.
	bool InsertPersonality(Personality *pers);

	// GetHostCount gets the number of hosts in each of the root node's subtrees and
	// adds them into m_root's m_count value.
	// Returns nothing.
	bool CalculateDistributions(PersonalityTreeItem *node);

	// UpdatePersonality will, for each Personality, create an item for the Personality,
	// add the item to that parent's children if not present (and aggregate the data of the
	// pre-existing node and the new data in *pers if it is present) and then continue to the
	// next personality. In this way, there exists one PersonalityTreeItem that represents the data
	// (in aggregate) of one or more Personalities for each Personality in the PersonalityTable.
	//  Personality *pers - Personality object with which to create a new node or whose data
	//                      to add to a pre-existing node with the same personality.
	//  PersonalityTreeItem *parent - the parent node of the current node to be created or
	//                      modified.
	// Returns nothing.
	bool UpdatePersonality(Personality *pers, PersonalityTreeItem *parent);

	// At each node, will add an open version and a script
	// specific version of each found port for the node.
	//  PersonalityNode *node - the node whose ports to look at, and change if a script
	//                      is found in the ServiceToScriptMap that matches the services for
	//                      those ports.
	// Returns nothing.
	bool AddAllPorts(PersonalityTreeItem *node);

	//Empty 'root' node of the tree, this node can be treated as the 'any' case or all personalities.
	PersonalityTreeItem *m_root;

	ProfileTable *m_profiles;

	ServiceToScriptMap m_serviceMap;
};

}

#endif /* PERSONALITYTREE_H_ */
