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

#include "HoneydConfiguration/HoneydConfiguration.h"
#include "HoneydConfiguration/AutoConfigHashMaps.h"

namespace Nova {

class PersonalityNode {

public:

	PersonalityNode(std::string key = "");

	~PersonalityNode();

	// Number of hosts that have this personality
	uint32_t m_count;
	double m_distribution;

	// Node for the node
	std::string m_key;

	// Is this something we can compress? If m_redundant == true, then yes
	bool m_redundant;

	// Vector of the child nodes to this node
	std::vector<PersonalityNode*> m_children;

	// Distribution vectors containing the percent occurrences of
	// each Port.
	std::vector<std::pair<std::string, double> > m_ports_dist;

	// String representing the osclass. Used for matching ports to
	// scripts from the script table.
	std::string m_osclass;

	//HashMap of MACs; Key is Vendor, Value is number of times the MAC vendor is seen for hosts of this personality type
	MACVendorMap m_vendors;

	//HashMap of ports; Key is port (format: <NUM>_<PROTOCOL>), Value is a uint16_t count
	PortServiceMap m_ports;

	void GenerateDistributions();

	NodeProfile GenerateProfile(const NodeProfile &parentProfile);

	//Returns a random vendor from the internal list of MAC vendors, according to the given probabilities
	//	returns an empty string "" on error
	std::string GetRandomVendor();

private:

	// The average number of ports that this personality has
	uint16_t m_avgPortCount;

	// Distribution vectors containing the percent occurrences of
	// each MAC Vendor
	std::vector<std::pair<std::string, double> > m_vendor_dist;
};

}

#endif /* PERSONALITYNODE_H_ */
