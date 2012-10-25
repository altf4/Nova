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
	m_vendors.set_empty_key("EMPTY");
	m_vendors.set_deleted_key("DELETED");
	m_count = 0;
	m_avgPortCount = 0;
	m_redundant = false;
	m_parent = parent;

	PersonalityTreeItem *loopParent = parent;
	while(loopParent != NULL)
	{
		if(!loopParent->m_TCP_behavior.empty() && !loopParent->m_UDP_behavior.empty() && !loopParent->m_ICMP_behavior.empty())
		{
			m_TCP_behavior = loopParent->m_TCP_behavior;
			m_UDP_behavior = loopParent->m_UDP_behavior;
			m_ICMP_behavior = loopParent->m_ICMP_behavior;
			break;
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

void PersonalityTreeItem::GenerateDistributions()
{
	uint16_t count = 0;

	m_vendor_dist.clear();
	for(MACVendorMap::iterator it = m_vendors.begin(); it != m_vendors.end(); it++)
	{
		pair<string, double> push_vendor;
		push_vendor.first = it->first;
		push_vendor.second = (100 * (((double)it->second)/((double)m_count)));
		m_vendor_dist.push_back(push_vendor);
	}

	//TODO: remove?
	m_avgPortCount = count / m_count;
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
	for(MACVendorMap::iterator it = m_vendors.begin(); it != m_vendors.end(); it++)
	{
		totalOccurrences += it->second;
	}

	if(totalOccurrences == 0)
	{
		return "";
	}

	//Now we pick a random number between 1 and totalOccurrences
	// Not technically a proper distribution, but we'll live
	int random = (rand() % totalOccurrences) + 1;

	//Second pass, let's see who the winning MAC vendor actually is
	for(MACVendorMap::iterator it = m_vendors.begin(); it != m_vendors.end(); it++)
	{
		//Iteratively remove values from our total, until we get to zero. Then stop.
		random -= it->second;
		if(random <= 0)
		{
			//Winner!
			return it->first;
		}
	}
	return "";
}

}
