/*
 * PersonalityNode.cpp
 *
 *  Created on: Jun 4, 2012
 *      Author: victim
 */

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
	m_count = 1;
}

//Deconstructor
PersonalityNode::~PersonalityNode()
{
	for(unsigned int i = 0; i < m_children.size(); i++)
	{
		if(m_children[i].second != NULL)
		{
			m_children[i].second->~PersonalityNode();
			m_children[i].second = NULL;
		}
	}
	m_children.clear();
}

string PersonalityNode::ToString()
{
	stringstream ss;
	ss << m_key << " has " << m_count << " hosts in it's scope." << endl << endl;
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

}
