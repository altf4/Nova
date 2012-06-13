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
	m_ports.set_empty_key("EMPTY");
	m_vendors.set_empty_key("EMPTY");
	m_ports.set_deleted_key("DELETED");
	m_vendors.set_deleted_key("DELETED");
	m_count = 0;
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
	ss << endl << m_key << " has " << m_count << " hosts in it's scope." << endl << endl;
	ss << "MAC Address Vendors: <Vendor>, <Number of occurrences>" << endl;

	for(MAC_Table::iterator it = m_vendors.begin(); it != m_vendors.end(); it++)
	{
		ss << it->first << ", " << it->second << endl;
	}

	ss << endl << "Ports : <Number>_<Protocol>, <Number of occurrences>" << endl;

	for(Port_Table::iterator it = m_ports.begin(); it != m_ports.end(); it++)
	{
		ss << it->first << ", " << it->second << endl;
	}

	return ss.str();
}

void PersonalityNode::GenerateDistributions()
{

	m_vendor_dist.clear();
	for(MAC_Table::iterator it = m_vendors.begin(); it != m_vendors.end(); it++)
	{
		pair<string, double> push_vendor;
		push_vendor.first = it->first;
		push_vendor.second = (100 * (((double)it->second)/((double)m_count)));
		m_vendor_dist.push_back(push_vendor);
	}

	for(Port_Table::iterator it = m_ports.begin(); it != m_ports.end(); it++)
	{
		pair<string, double> push_ports;
		push_ports.first = it->first + "_open";
		push_ports.second = (100 * (((double)it->second)/((double)m_count)));
		m_ports_dist.push_back(push_ports);
	}
}

profile PersonalityNode::GenerateProfile(profile* parentProfile)
{
	profile push;

	push.name = m_key;
	push.parentProfile = parentProfile->name;

	m_redundant = true;

	for(uint i = 0; i < (sizeof(push.inherited)/sizeof(bool)); i++)
	{
		push.inherited[i] = true;
	}

	if(m_children.size() == 0)
	{
		push.personality = m_key;
		push.inherited[PERSONALITY] = false;
		m_redundant = false;
	}

	if((m_vendor_dist.size() == 1) && m_vendor_dist[0].first.compare(parentProfile->ethernet))
	{
		push.ethernet = m_vendor_dist[0].first;
		push.inherited[ETHERNET] = false;
		m_redundant = false;
	}
	// Go through every element of the ports distribution
	// vector and create a pair for the ports vector in the
	// profile struct for every 100% known port.
	for(uint16_t i = 0; i < m_ports_dist.size(); i++)
	{
		if(m_ports_dist[i].second == 100)
		{
			pair<string, bool> push_port;
			push_port.first = m_ports_dist[i].first;
			push_port.second = false;
			for(uint16_t i = 0; i < parentProfile->ports.size(); i++)
			{
				if(!parentProfile->ports[i].first.compare(push_port.first))
				{
					push_port.second = true;
				}
			}
			push.ports.push_back(push_port);
			if(!push_port.second)
			{
				m_redundant = false;
			}
		}
	}

	return push;
}

}
