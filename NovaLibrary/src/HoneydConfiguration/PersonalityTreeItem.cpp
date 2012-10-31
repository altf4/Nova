//============================================================================
// Name        : PersonalityTreeItem.cpp
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

#include "PersonalityTreeItem.h"
#include "HoneydConfiguration.h"

#include <sstream>

using namespace std;

namespace Nova
{

//Default constructor
PersonalityTreeItem::PersonalityTreeItem(PersonalityTreeItem *parent, string key)
{
	// Creates an empty personality node; set the
	// empty and deleted keys for tables inside the
	// node class so that the only exception we will
	// get is accessing the empty key.
	m_children.clear();
	m_key = key;
	m_osclass = "";
	m_count = 0;
	m_avgPortCount = 0;
	m_redundant = false;
	m_parent = parent;

	PersonalityTreeItem *loopParent = parent;
	while(loopParent != NULL)
	{
		//XXX: just takes the first portset. but is that right? How to inherit properly?
		if(loopParent->m_portSets.size() > 0)
		{
			PortSet *portset = new PortSet();
			portset->m_defaultICMPBehavior = loopParent->m_portSets[0]->m_defaultICMPBehavior;
			portset->m_defaultTCPBehavior = loopParent->m_portSets[0]->m_defaultTCPBehavior;
			portset->m_defaultUDPBehavior = loopParent->m_portSets[0]->m_defaultUDPBehavior;
			m_portSets.push_back(portset);
		}
		loopParent = loopParent->m_parent;
	}
}

//Destructor
PersonalityTreeItem::~PersonalityTreeItem()
{
	for(uint i = 0; i < m_children.size(); i++)
	{
		if(m_children[i] != NULL)
		{
			delete m_children[i];
		}
	}

	for(uint i = 0; i < m_portSets.size(); i++)
	{
		//TODO: Double delete?
		//delete m_portSets[i];
	}
}

string PersonalityTreeItem::ToString(const std::string &portSetName)
{
	stringstream out;

	//XXX This is just a temporary band-aid on a larger wound, we cannot allow whitespaces in profile names.
	string profName = HoneydConfiguration::SanitizeProfileName(m_key);
	string parentProfName = "";
	if(m_parent != NULL)
	{
		string parentProfName = HoneydConfiguration::SanitizeProfileName(m_parent->m_key);
	}

	if(!profName.compare("default") || !profName.compare(""))
	{
		out << "create " << profName << '\n';
	}
	else
	{
		out << "clone " << profName << " default\n";
	}

	//If the portset name is empty, just use the first port set in the list
	if(portSetName.empty())
	{
		if(!m_portSets.empty())
		{
			out << "set " << profName << " default tcp action " << PortBehaviorToString(m_portSets[0]->m_defaultTCPBehavior) << '\n';
			out << "set " << profName << " default udp action " << PortBehaviorToString(m_portSets[0]->m_defaultUDPBehavior) << '\n';
			out << "set " << profName << " default icmp action " << PortBehaviorToString(m_portSets[0]->m_defaultICMPBehavior) << '\n';
		}
	}
	else
	{
		//Ports
		for(uint i = 0; i < m_portSets.size(); i++)
		{
			//If we get a match for the port set
			if(!portSetName.compare(m_portSets[i]->m_name))
			{
				cout << m_portSets[i]->ToString(profName);
			}
		}
	}


	if(m_key.compare("") && m_key.compare("NULL"))
	{
		out << "set " << profName << " personality \"" << m_osclass << '"' << '\n';
	}

	//TODO: This always uses the vendor with the highest likelihood. Should get changed?
	string vendor = "";
	double maxDist = 0;
	for(uint i = 0; i < m_vendors.size(); i++)
	{
		if(m_vendors[i].second > maxDist)
		{
			maxDist = m_vendors[i].second;
			vendor = m_vendors[i].first;
		}
	}
	if(vendor.compare(""))
	{
		out << "set " << profName << " ethernet \"" << vendor << '"' << '\n';
	}

	if(m_dropRate.compare(""))
	{
		out << "set " << profName << " droprate in " << m_dropRate << '\n';
	}

	out << "\n\n";
	return out.str();
}

std::string PersonalityTreeItem::GetRandomVendor()
{
	if(m_vendors.empty())
	{
		if(m_parent == NULL)
		{
			return "";
		}
		else
		{
			return m_parent->GetRandomVendor();
		}
	}

	//First pass, let's find out what the total is for all occurrences
	uint totalOccurrences = 0;
	for(uint i = 0; i < m_vendors.size(); i++)
	{
		totalOccurrences += m_vendors[i].second;
	}

	if(totalOccurrences == 0)
	{
		return "";
	}

	//Now we pick a random number between 1 and totalOccurrences
	// Not technically a proper distribution, but we'll live
	int random = (rand() % totalOccurrences) + 1;

	//Second pass, let's see who the winning MAC vendor actually is
	for(uint i = 0; i < m_vendors.size(); i++)
	{
		//Iteratively remove values from our total, until we get to zero. Then stop.
		random -= m_vendors[i].second;
		if(random <= 0)
		{
			//Winner!
			return m_vendors[i].first;
		}
	}
	return "";
}

double PersonalityTreeItem::GetVendorDistribution(std::string vendorName)
{
	for(uint i = 0; i < m_vendors.size(); i++)
	{
		if(!m_vendors[i].first.compare(vendorName))
		{
			return m_vendors[i].second;
		}
	}
	return 0;
}

}
