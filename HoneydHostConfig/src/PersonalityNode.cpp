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
	m_children.clear();
	m_key = key;
	m_ports.set_empty_key("EMPTY");
	m_vendors.set_empty_key("EMPTY");
	m_ports.set_deleted_key("DELETED");
	m_vendors.set_deleted_key("DELETED");
	m_count = 0;
}

//Deconstructor
PersonalityNode::~PersonalityNode()
{
	vector<PersonalityNode *> delPtrs;
	for(unsigned int i = 0; i < m_children.size(); i++)
	{
		if(m_children[i].second != NULL)
		{
			delPtrs.push_back(m_children[i].second);
		}
	}
	for(unsigned int i = 0; i < delPtrs.size(); i++)
	{
		delete delPtrs[i];
	}
}

string PersonalityNode::ToString()
{
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

string PersonalityNode::GenerateDistribution()
{
	// Need a port count in PersonalityNode, or sum on the fly. I skew towards the former
	stringstream ss;
	ss << endl << "Distributions for " << m_key << ":" << endl;
	for(MAC_Table::iterator it = m_vendors.begin(); it != m_vendors.end(); it++)
	{
		ss << "\t" << it->first << " constitutes " << (100 * (((double)it->second)/((double)m_count))) << "% of MACs for this scope." << endl;
	}
	for(Port_Table::iterator it = m_ports.begin(); it != m_ports.end(); it++)
	{
		ss << "\t" << it->first << " constitutes " << (100 * (((double)it->second)/((double)m_count))) << "% of ports for this scope." << endl;
	}

	return ss.str();
}

}
