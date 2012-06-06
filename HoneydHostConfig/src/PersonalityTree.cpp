/*
 * PersonalityTree.cpp
 *
 *  Created on: Jun 4, 2012
 *      Author: victim
 */

#include "PersonalityTree.h"

using namespace std;

namespace Nova
{

PersonalityTree::PersonalityTree(PersonalityTable *persTable)
{
	m_root = PersonalityNode("root");
	if(persTable != NULL)
	{
		LoadTable(persTable);
	}
}

PersonalityTree::~PersonalityTree()
{
  m_root.~PersonalityNode();
}

void PersonalityTree::LoadTable(PersonalityTable *persTable)
{
	Personality_Table &pTable = persTable->m_personalities;
	for(Personality_Table::iterator it = pTable.begin(); it != pTable.end(); it++)
	{
		InsertPersonality(it->second);
	}
}

void PersonalityTree::InsertPersonality(Personality *pers)
{
	Personality temp = * pers;
	UpdatePersonality(&temp, &m_root);
}

void PersonalityTree::UpdatePersonality(Personality *pers, PersonalityNode *parent)
{
	string cur = pers->m_personalityClass.back();

	uint i = 0;
	for(; i < parent->m_children.size(); i++)
	{
		if(!cur.compare(parent->m_children[i].first))
		{
			break;
		}
	}

	pair<string, PersonalityNode *> *tablePair = NULL;

	//If node not found
	if(i == parent->m_children.size())
	{
		tablePair = new pair<string, PersonalityNode *>();
		tablePair->first = cur;
		tablePair->second = new PersonalityNode(cur);
	}
	else
	{
		tablePair = &parent->m_children[i];
	}

	//Insert or count port occurrences
	for(Port_Table::iterator it = pers->m_ports.begin(); it != pers->m_ports.end(); it++)
	{
		//If we found a corresponding entry
		if(tablePair->second->m_ports.find(it->first) != tablePair->second->m_ports.end())
		{
			tablePair->second->m_ports[it->first] += it->second;
		}
		else
		{
			tablePair->second->m_ports[it->first] = it->second;
		}
	}

	//Insert or count MAC vendor occurrences
	for(MAC_Table::iterator it = pers->m_vendors.begin(); it != pers->m_vendors.end(); it++)
	{
		//If we found a corresponding entry
		if(tablePair->second->m_vendors.find(it->first) != tablePair->second->m_vendors.end())
		{
			tablePair->second->m_vendors[it->first] += it->second;
		}
		else
		{
			tablePair->second->m_vendors[it->first] = it->second;
		}
	}
	pers->m_personalityClass.pop_back();

	//Recursively descend until all nodes updated
	if(!pers->m_personalityClass.empty())
	{
		UpdatePersonality(pers, tablePair->second);
	}
}

void PersonalityTree::ToString()
{
	for(uint i = 0; i < m_root.m_children.size(); i++)
	{
		RecursiveToString(m_root.m_children[i].second);
	}
}

void PersonalityTree::RecursiveToString(PersonalityNode * persNode)
{
	cout << persNode->ToString() << endl;
	for(uint i = 0; i < persNode->m_children.size(); i++)
	{
		RecursiveToString(persNode->m_children[i].second);
	}

}



}
