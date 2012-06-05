/*
 * PersonalityNode.h
 *
 *  Created on: Jun 4, 2012
 *      Author: victim
 */

#ifndef PERSONALITYNODE_H_
#define PERSONALITYNODE_H_

#include "AutoConfigHashMaps.h"

namespace Nova
{

class PersonalityNode
{

public:

	//Default constructor
	PersonalityNode(std::string key = "");

	//Deconstructor
	~PersonalityNode();

	std::string m_key;
	std::vector<std::pair<std::string, PersonalityNode *> > m_children;

	//HashMap of MACs; Key is Vendor, Value is number of times the MAC vendor is seen for hosts of this personality type
	MAC_Table m_vendors;

	//HashMap of ports; Key is port (format: <NUM>_<PROTOCOL>), Value is a uint16_t count
	Port_Table m_ports;

};

}

#endif /* PERSONALITYNODE_H_ */
