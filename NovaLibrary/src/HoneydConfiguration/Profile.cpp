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

#include "Profile.h"
#include "HoneydConfiguration.h"

#include <sstream>

using namespace std;

namespace Nova
{

//Default constructor
Profile::Profile(Profile *parent, string key)
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
	m_isGenerated = false;	//Manually set this to true in Autoconfig
	m_uptimeMin = ~0;	//TODO: What if we don't see any uptimes? We need to have a sane default
	m_uptimeMax = 0;	//TODO: Let;'s have a more reasonable "maximum" here. We don't want to be setting random uptimes this high
}

//Destructor
Profile::~Profile()
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

string Profile::ToString(const std::string &portSetName, const std::string &nodeName)
{
	stringstream out;


	if(!nodeName.compare("default"))
	{
		out << "create " << nodeName << '\n';
	}
	else
	{
		out << "clone " << nodeName << " default\n";
	}

	//If the portset name is empty, just use the first port set in the list
	if(portSetName.empty())
	{
		if(!m_portSets.empty())
		{
			out << "set " << nodeName << " default tcp action " << PortBehaviorToString(m_portSets[0]->m_defaultTCPBehavior) << '\n';
			out << "set " << nodeName << " default udp action " << PortBehaviorToString(m_portSets[0]->m_defaultUDPBehavior) << '\n';
			out << "set " << nodeName << " default icmp action " << PortBehaviorToString(m_portSets[0]->m_defaultICMPBehavior) << '\n';
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
				out << m_portSets[i]->ToString(nodeName);
			}
		}
	}


	if((m_personality != "") && (m_personality != "NULL"))
	{
		out << "set " << nodeName << " personality \"" << m_personality << '"' << '\n';
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
		out << "set " << nodeName << " ethernet \"" << vendor << '"' << '\n';
	}

	if(m_dropRate.compare(""))
	{
		out << "set " << nodeName << " droprate in " << m_dropRate << '\n';
	}

	return out.str();
}

std::string Profile::GetRandomVendor()
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

PortSet *Profile::GetRandomPortSet()
{
	if(m_portSets.empty())
	{
		return NULL;
	}

	//Now we pick a random number between 1 and totalOccurrences
	// Not technically a proper distribution, but we'll live
	int random = rand() % m_portSets.size();

	return m_portSets[random];
}

double Profile::GetVendorDistribution(std::string vendorName)
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

void Profile::RecalculateChildDistributions()
{
	for(uint i = 0; i < m_children.size(); i++)
	{
		m_children[i]->m_distribution = (((double)m_children[i]->m_count) / (double)m_count) * 100;
	}
}

}
