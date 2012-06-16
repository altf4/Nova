//============================================================================
// Name        :
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
// Description :
//============================================================================

#ifndef PERSONALITYNODE_H_
#define PERSONALITYNODE_H_

#include "HoneydConfiguration.h"
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

	uint32_t m_count;
	std::string m_key;
	bool m_redundant;

	std::vector<std::pair<std::string, PersonalityNode *> > m_children;

	std::vector<std::pair<std::string, double> > m_vendor_dist;
	std::vector<std::pair<std::string, double> > m_ports_dist;

	//HashMap of MACs; Key is Vendor, Value is number of times the MAC vendor is seen for hosts of this personality type
	MAC_Table m_vendors;

	//HashMap of ports; Key is port (format: <NUM>_<PROTOCOL>), Value is a uint16_t count
	PortsTable m_ports;

	//returns a string representation of node, does not print anything about the children
	std::string ToString();

	void GenerateDistributions();

	NodeProfile GenerateProfile(NodeProfile *parentProfile);
};

}

#endif /* PERSONALITYNODE_H_ */
