/*
 * PersonalityNode.h
 *
 *  Created on: Jun 4, 2012
 *      Author: victim
 */

#ifndef PERSONALITYNODE_H_
#define PERSONALITYNODE_H_

namespace Nova
{

class PersonalityNode
{

public:

	//Default constructor
	PersonalityNode();

	//Deconstructor
	~PersonalityNode();

	std::string m_key;
	std::vector<std::pair<std::string, PersonalityNode *>> m_children;

};

}

#endif /* PERSONALITYNODE_H_ */
