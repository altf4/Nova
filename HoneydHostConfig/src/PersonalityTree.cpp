//============================================================================
// Name        : PersonalityTree.cpp
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
// Description : TODO
//============================================================================

#include "PersonalityTree.h"

using namespace std;

namespace Nova
{

PersonalityTree::PersonalityTree(PersonalityTable *persTable)
{
	hhconfig = new HoneydConfiguration();

	m_root = PersonalityNode("default");
	if(persTable != NULL)
	{
		LoadTable(persTable);
	}
	hhconfig->LoadAllTemplates();
	m_profiles = &hhconfig->m_profiles;
}

PersonalityTree::~PersonalityTree()
{
}

void PersonalityTree::CleanTree()
{
	for(uint16_t i = 0; i < m_root.m_children.size(); i++)
	{
		RecursiveCleanTree(m_root.m_children[i].second, &m_root);
	}

	// need to clear m_to_delete
	for(uint i = 0; i < m_to_delete.size(); i++)
	{
		hhconfig->DeleteProfile(m_to_delete[i]->m_key);
		delete m_to_delete[i];
	}
}

void PersonalityTree::RecursiveCleanTree(PersonalityNode * node, PersonalityNode * parent)
{
	//If we have no children, this is a leaf node that won't be cleaned, terminate recursion and return.
	if(node->m_children.size() == 0)
	{
		return;
	}

	// ***** Determine if this node can be cleaned ******
	//If we have only one child, we may be able to remove
	bool compress = (node->m_children.size() == 1);
	profile *prof = hhconfig->GetProfile(node->m_key);

	//If every value for the profile is inherited we can clean
	for(uint i = 0; i < sizeof(p->inherited)/sizeof(bool) && compress; i++)
	{
		compress &= prof->inherited[i];
	}
	for(uint i = 0; i < prof->ports.size() && compress; i++)
	{
		compress &= prof->ports[i].second;
	}

	//For each child repeat the recursion
	for(uint i = 0; i < node->m_children.size(); i++)
	{
		RecursiveCleanTree(node->m_children[i].second, node);
	}

	//If we can clean a profile
	if(compress)
	{
		//Store a pointer to the personality node to delete.
		PersonalityNode * child = node->m_children[0].second;
		m_to_delete.push_back(child);

		//Updates pers node keys and list
		for(uint i = 0; i < parent->m_children.size(); i++)
		{
			if(!parent->m_children[i].first.compare(node->m_key))
			{
				parent->m_children[i].first = node->m_key + " " + child->m_key;
			}
		}
		string oldKey = node->m_key;
		node->m_key = node->m_key + " " + child->m_key;

		//Moves children in the pers nodes/tree
		node->m_children = child->m_children;
		child->m_children.clear();

		//Create new profile for new key
		profile * p = new profile(*hhconfig->GetProfile(child->m_key));
		p->name = node->m_key;
		hhconfig->AddProfile(p);

		//Update profile inheritance
		for(uint i = 0; i < node->m_children.size(); i++)
		{
			hhconfig->InheritProfile(node->m_children[i].first, p->name);
		}

		hhconfig->DeleteProfile(oldKey);
		hhconfig->DeleteProfile(child->m_key);
	}
}

void PersonalityTree::LoadTable(PersonalityTable *persTable)
{
	Personality_Table *pTable = &persTable->m_personalities;

	for(Personality_Table::iterator it = pTable->begin(); it != pTable->end(); it++)
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
		parent->m_children.push_back(*tablePair);
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

	tablePair->second->m_count += pers->m_count;

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

void PersonalityTree::GenerateProfileTable()
{
	for(uint i = 0; i < m_root.m_children.size(); i++)
	{
		RecursiveGenerateProfileTable(m_root.m_children[i].second, m_root.m_key);
	}
}

void PersonalityTree::RecursiveGenerateProfileTable(PersonalityNode * node, string parent)
{
	if(m_profiles->find(node->m_key) == m_profiles->end())
	{
		profile tempProf = node->GenerateProfile(parent);

		if(!hhconfig->AddProfile(&tempProf))
		{
			cout << tempProf.name << endl;
		}
	}
	else
	{
		// Probably not the right way of going about this
		uint16_t i = 1;
		stringstream ss;
		ss << i;

		for(; m_profiles->find(node->m_key + ss.str()) != m_profiles->end() && (i < ~0); i++)
		{
			ss.str("");
			ss << i;
		}

		node->m_key = node->m_key + ss.str();

		profile tempProf = node->GenerateProfile(parent);

		if(!hhconfig->AddProfile(&tempProf))
		{
			cout << tempProf.name << endl;
		}
	}
	for(uint16_t i = 0; i < node->m_children.size(); i++)
	{
		RecursiveGenerateProfileTable(node->m_children[i].second, node->m_key);
	}
}

void PersonalityTree::GenerateDistributions()
{
	for(uint16_t i = 0; i < m_root.m_children.size(); i++)
	{
		RecursiveGenerateDistributions(m_root.m_children[i].second);
	}
}

void PersonalityTree::RecursiveGenerateDistributions(PersonalityNode * node)
{
	cout << node->GenerateDistribution() << endl;
	for(uint16_t i = 0; i < node->m_children.size(); i++)
	{
		RecursiveGenerateDistributions(node->m_children[i].second);
	}
}

void PersonalityTree::DebugPrintProfileTable()
{
	for(ProfileTable::iterator it = m_profiles->begin(); it != m_profiles->end(); it++)
	{
		cout << endl;
		cout << "Profile: " << it->second.name << endl;
		cout << "Parent: " << it->second.parentProfile << endl;

		if(!it->second.personality.empty())
		{
			cout << "Personality: " << it->second.personality << endl;
		}
		if(!it->second.ethernet.empty())
		{
			cout << "MAC vendor: " << it->second.ethernet << endl;
		}
		if(!it->second.ports.empty())
		{
			cout << "Ports for this scope (<NUM>_<PROTOCOL>, inherited):" << endl;
			for(uint16_t i = 0; i < it->second.ports.size(); i++)
			{
				cout << "\t" << it->second.ports[i].first << ", " << it->second.ports[i].second << endl;
			}
		}
		cout << endl;
	}
}

void PersonalityTree::ToXmlTemplate()
{

	vector<string> ret = hhconfig->GetProfileNames();

	hhconfig->m_nodes.clear();
	hhconfig->m_subnets.clear();

	AddAllPorts();

	CleanTree();

	for(uint16_t i = 0; i < m_root.m_children.size(); i++)
	{
		RecursiveToXmlTemplate(m_root.m_children[i].second, m_root.m_key);
	}

	ret = hhconfig->GetProfileNames();

	for(uint16_t i = 0; i < ret.size(); i++)
	{
		cout << "Profile after Clean: " << ret[i] << endl;
	}

	hhconfig->SaveAllTemplates();
}

void PersonalityTree::RecursiveToXmlTemplate(PersonalityNode * node, string prefix)
{
	for(uint16_t i = 0; i < node->m_children.size(); i++)
	{
		RecursiveToXmlTemplate(node->m_children[i].second, node->m_key);
	}
}

void PersonalityTree::AddAllPorts()
{
	hhconfig->m_ports.clear();

	for(uint16_t i = 0; i < m_root.m_children.size(); i++)
	{
		RecursiveAddAllPorts(m_root.m_children[i].second);
	}
}

void PersonalityTree::RecursiveAddAllPorts(PersonalityNode * node)
{
	for(uint16_t i = 0; i < node->m_ports_dist.size(); i++)
	{
		port pass;

		pass.portName = node->m_ports_dist[i].first + "_open";
		pass.portNum = node->m_ports_dist[i].first.substr(0, node->m_ports_dist[i].first.find("_"));
		pass.type = node->m_ports_dist[i].first.substr(node->m_ports_dist[i].first.find("_") + 1, node->m_ports_dist[i].first.find("_", node->m_ports_dist[i].first.find("_")) + 1);
		pass.behavior = "open";

		hhconfig->AddPort(pass);
	}
	for(uint16_t i = 0; i < node->m_children.size(); i++)
	{
		RecursiveAddAllPorts(node->m_children[i].second);
	}
}

void PersonalityTree::RecursivePrintTree(PersonalityNode * node)
{
	cout << node->m_key << endl;

	for(uint i = 0; i < node->m_children.size(); i++)
	{
		RecursivePrintTree(node->m_children[i].second);
	}
}

}
