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

#include "PersonalityNode.h"

#include <sstream>

using namespace std;

namespace Nova
{

//Default constructor
PersonalityNode::PersonalityNode(string key)
{
	// Creates an empty personality node; set the
	// empty and deleted keys for tables inside the
	// node class so that the only exception we will
	// get is accessing the empty key.
	m_children.clear();
	m_key = key;
	m_osclass = "";
	m_ports.set_empty_key("EMPTY");
	m_vendors.set_empty_key("EMPTY");
	m_ports.set_deleted_key("DELETED");
	m_vendors.set_deleted_key("DELETED");
	m_count = 0;
	m_avgPortCount = 0;
	m_redundant = false;
}

//Deconstructor
PersonalityNode::~PersonalityNode()
{
	for(unsigned int i = 0; i < m_children.size(); i++)
	{
		if(m_children[i].second != NULL)
		{
			delete m_children[i].second;
		}
	}
	m_children.clear();
}

string PersonalityNode::ToString()
{
	// Simple ToString for the personality node for debugging purposes.

	stringstream ss;
	ss << '\n' << m_key << " has " << m_count << " hosts in it's scope." << '\n' << '\n';
	ss << "MAC Address Vendors: <Vendor>, <Number of occurrences>" << '\n';

	for(MAC_Table::iterator it = m_vendors.begin(); it != m_vendors.end(); it++)
	{
		ss << it->first << ", " << it->second << '\n';
	}

	ss << '\n' << "Ports : <Number>_<Protocol>, <Number of occurrences>" << '\n';

	for(PortsTable::iterator it = m_ports.begin(); it != m_ports.end(); it++)
	{
		ss << it->first << ", " << it->second.first << '\n';
	}

	return ss.str();
}

void PersonalityNode::GenerateDistributions()
{
	uint16_t count = 0;

	m_vendor_dist.clear();
	for(MAC_Table::iterator it = m_vendors.begin(); it != m_vendors.end(); it++)
	{
		pair<string, double> push_vendor;
		push_vendor.first = it->first;
		push_vendor.second = (100 * (((double)it->second)/((double)m_count)));
		m_vendor_dist.push_back(push_vendor);
	}

	for(PortsTable::iterator it = m_ports.begin(); it != m_ports.end(); it++)
	{
		count += it->second.first;
		pair<string, double> push_ports;
		push_ports.first = it->first + "_open";
		push_ports.second = (100 * (((double)it->second.first)/((double)m_count)));
		m_ports_dist.push_back(push_ports);
	}

	m_avgPortCount = count / m_count;
}

NodeProfile PersonalityNode::GenerateProfile(const NodeProfile &parentProfile)
{
	NodeProfile profileToReturn;

	profileToReturn.m_name = m_key;
	profileToReturn.m_parentProfile = parentProfile.m_name;
	profileToReturn.m_generated = true;

	m_redundant = true;

	if(!m_key.compare("default") || !parentProfile.m_name.compare("default"))
	{
		m_redundant = false;
	}

	for(uint i = 0; i < (sizeof(profileToReturn.m_inherited)/sizeof(bool)); i++)
	{
		profileToReturn.m_inherited[i] = true;
	}

	if(m_children.size() == 0)
	{
		profileToReturn.m_personality = m_key;
		profileToReturn.m_inherited[PERSONALITY] = false;
		m_redundant = false;
	}

	if((m_vendor_dist.size() == 1) && m_vendor_dist[0].first.compare(parentProfile.m_ethernet))
	{
		profileToReturn.m_ethernet = m_vendor_dist[0].first;
		profileToReturn.m_inherited[ETHERNET] = false;
		m_redundant = false;
	}
	// Go through every element of the ports distribution
	// vector and create a pair for the ports vector in the
	// profile struct for every 100% known port.
	for(uint16_t i = 0; i < m_ports_dist.size(); i++)
	{
		if(m_ports_dist[i].second == 100 || m_children.size() == 0)
		{
			pair<string, pair<bool, double> > portToAdd;
			portToAdd.first = m_ports_dist[i].first;
			portToAdd.second.first = false;
			portToAdd.second.second = m_ports_dist[i].second;
			for(uint16_t i = 0; i < parentProfile.m_ports.size(); i++)
			{
				if(!parentProfile.m_ports[i].first.compare(portToAdd.first))
				{
					portToAdd.second.first = true;
				}
			}
			profileToReturn.m_ports.push_back(portToAdd);
			if(!portToAdd.second.first)
			{
				m_redundant = false;
			}
		}
	}

	return profileToReturn;
}

}
