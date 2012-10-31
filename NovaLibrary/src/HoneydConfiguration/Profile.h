//============================================================================
// Name        : Profile.h
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
// Description : Represents a single item in the PersonalityTree. A linked list
//		tree structure representing the hierarchy of profiles discovered
//============================================================================

#ifndef PROFILE_H_
#define PROFILE_H_

#include "AutoConfigHashMaps.h"
#include "PortSet.h"

namespace Nova
{

class Profile
{

public:

	Profile(Profile *parent, std::string key = "");

	~Profile();

	//Returns a string suitable for inserting into the honeyd configuration file
	std::string ToString(const std::string &portSetName = "");

	//Returns a random vendor from the internal list of MAC vendors, according to the given probabilities
	// If a profile has no defined ethernet vendors, look to the parent profile
	//	returns an empty string "" on error
	std::string GetRandomVendor();

	//Returns a random PortSet from the internal list
	//	returns - NULL if no port sets present
	PortSet *GetRandomPortSet();

	double GetVendorDistribution(std::string vendorName);

	// Number of hosts that have this personality
	uint32_t m_count;
	double m_distribution;

	// Name for the node
	std::string m_key;

	// Is this something we can compress?
	bool m_redundant;

	//Is this an autoconfig generated node? (or manually created)
	bool m_isGenerated;

	// Vector of the child nodes to this node
	std::vector<Profile*> m_children;

	//Parent PersonalityTreeItem
	Profile *m_parent;

	//Upper and lower bound for set uptime
	uint m_uptimeMin;
	uint m_uptimeMax;

	// String representing the osclass. Used for matching ports to
	// scripts from the script table.
	std::string m_osclass;

	std::string m_dropRate;

	//HashMap of MACs; Key is Vendor, Value is number of times the MAC vendor is seen for hosts of this personality type
	std::vector<std::pair<std::string, double> > m_vendors;

	//A collection of PortSets, representing each group of ports found
	std::vector<PortSet *> m_portSets;

private:

	// The average number of ports that this personality has
	uint16_t m_avgPortCount;

};

}

#endif /* PROFILE_H_ */
