/*
 * PersonalityNode.cpp
 *
 *  Created on: Jun 4, 2012
 *      Author: victim
 */

#include "PersonalityNode.h"

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

}
