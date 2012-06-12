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
	hhconfig->LoadAllTemplates();
	m_profiles = &hhconfig->m_profiles;

	if(persTable != NULL)
	{
		LoadTable(persTable);
	}
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

	//For each child repeat the recursion
	for(uint i = 0; i < node->m_children.size(); i++)
	{
		RecursiveCleanTree(node->m_children[i].second, node);
	}

	// ***** Determine if this node can be cleaned ******
	//If we have only one child, we may be able to remove
	bool compress = (node->m_children.size() == 1);
	profile *prof = hhconfig->GetProfile(node->m_key);

	//If every value for the profile is inherited we can clean
	for(uint i = 0; i < sizeof(prof->inherited)/sizeof(bool) && compress; i++)
	{
		compress &= prof->inherited[i];
	}
	for(uint i = 0; i < prof->ports.size() && compress; i++)
	{
		compress &= prof->ports[i].second;
	}

	//If we can clean a profile
	if(compress)
	{
		//Store a pointer to the personality node to delete.
		PersonalityNode * child = node->m_children[0].second;
		m_to_delete.push_back(node);


		string oldKey = child->m_key;
		child->m_key = node->m_key + " " + child->m_key;

		for(uint16_t i = 0; i < child->m_children.size(); i++)
		{
			hhconfig->m_profiles[child->m_children[i].first].parentProfile = child->m_key;
		}

		//Create new profile for new key
		hhconfig->RenameProfile(oldKey, child->m_key);
		hhconfig->InheritProfile(child->m_key, parent->m_key);

		//Updates pers node keys and list
		for(uint i = 0; i < parent->m_children.size(); i++)
		{
			if(!parent->m_children[i].first.compare(node->m_key))
			{
				pair<std::string, PersonalityNode *> newPair;
				newPair.first = child->m_key;
				newPair.second = child;
				parent->m_children.erase(parent->m_children.begin()+i);
				parent->m_children.insert(parent->m_children.begin()+i, newPair);
			}
		}

		node->m_children.clear();
		hhconfig->DeleteProfile(node->m_key);
	}
}

void PersonalityTree::LoadTable(PersonalityTable *persTable)
{
	Personality_Table *pTable = &persTable->m_personalities;

	for(Personality_Table::iterator it = pTable->begin(); it != pTable->end(); it++)
	{
		InsertPersonality(it->second);
	}
	for(uint i = 0; i < m_root.m_children.size(); i++)
	{
		GenerateProfiles(m_root.m_children[i].second, &m_root, &hhconfig->m_profiles["default"]);
	}
}

void PersonalityTree::GenerateProfiles(PersonalityNode *node, PersonalityNode *parent, profile *parentProfile)
{
	//Create profile object
	node->GenerateDistributions();
	if(m_profiles->find(node->m_key) != m_profiles->end())
	{
		// Probably not the right way of going about this
		uint16_t i = 1;
		stringstream ss;
		string key = node->m_key;
		ss << i;

		for(; m_profiles->find(key) != m_profiles->end() && (i < ~0); i++)
		{
			ss.str("");
			ss << node->m_key << i;
			key = ss.str();
		}
	}
	profile tempProf = node->GenerateProfile(parentProfile);
	if(!node->m_redundant)
	{
		if(!hhconfig->AddProfile(&tempProf))
		{
			cout << "Error adding "<< tempProf.name << endl;
			return;
		}
		else
		{
			for(uint i = 0; i < node->m_children.size(); i++)
			{
				GenerateProfiles(node->m_children[i].second, node, &hhconfig->m_profiles[tempProf.name]);
			}
		}
	}
	else
	{
		// Probably not the right way of going about this
		uint16_t i = 1;
		stringstream ss;
		string key = parentProfile->name + " " + node->m_key;
		ss << i;
		while(!hhconfig->RenameProfile(parentProfile->name, key))
		{
			ss.str("");
			ss << i;
			key = parentProfile->name + " " + node->m_key + ss.str();
		}
		for(uint i = 0; i < node->m_children.size(); i++)
		{
			GenerateProfiles(node->m_children[i].second, node, &hhconfig->m_profiles[key]);
		}
	}
}

void PersonalityTree::InsertPersonality(Personality *pers)
{
	Personality temp = *pers;

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
		delete tablePair;
	}
	tablePair = &parent->m_children[i];

	//Insert or count port occurrences
	for(Port_Table::iterator it = pers->m_ports.begin(); it != pers->m_ports.end(); it++)
	{
		tablePair->second->m_ports[it->first] += it->second;
	}

	//Insert or count MAC vendor occurrences
	for(MAC_Table::iterator it = pers->m_vendors.begin(); it != pers->m_vendors.end(); it++)
	{
		tablePair->second->m_vendors[it->first] += it->second;
	}

	pers->m_personalityClass.pop_back();
	tablePair->second->m_count += pers->m_count;
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
	node->GenerateDistributions();
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

	ret = hhconfig->GetProfileNames();

	for(uint16_t i = 0; i < ret.size(); i++)
	{
		cout << "Profile after Clean: " << ret[i] << " has parent: " << hhconfig->GetProfile(ret[i])->parentProfile << endl;
	}

	hhconfig->SaveAllTemplates();
}

void PersonalityTree::RecursiveToXmlTemplate(PersonalityNode * node, string prefix)
{
	for(uint16_t i = 0; i < node->m_children.size(); i++)
	{
		// XXX .second is showing up as a NULL value, referencing an already deleted PersonalityNode, figure out why
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

void PersonalityTree::CleanInheritance()
{
	for(uint16_t i = 0; i < m_root.m_children.size(); i++)
	{
		RecursiveCleanPortInheritance(m_root.m_children[i].second, &m_root);
		RecursiveCleanMacInheritance(m_root.m_children[i].second, &m_root);
	}
}

void PersonalityTree::RecursiveCleanPortInheritance(PersonalityNode * node, PersonalityNode * parent)
{
	for(uint16_t i = 0; i < node->m_children.size(); i++)
	{
		RecursiveCleanPortInheritance(node->m_children[i].second, node);
	}
	// look at parent and adjust inherited booleans in m_ports
	if(hhconfig->GetProfile(node->m_key)->parentProfile.compare("default"))
	{
		for(uint16_t n = 0; n < node->m_ports_dist.size(); n++)
		{
			for(uint16_t p = 0; p < parent->m_ports_dist.size(); p++)
			{
				if(!node->m_ports_dist[n].first.compare(parent->m_ports_dist[p].first) && (parent->m_ports_dist[p].second == 100))
				{
					for(uint16_t k = 0; k < hhconfig->m_profiles[node->m_key].ports.size(); k++)
					{
						if(!hhconfig->m_profiles[node->m_key].ports[k].first.compare(node->m_ports_dist[n].first))
						{
							hhconfig->m_profiles[node->m_key].ports[k].second = true;
							break;
						}
					}
					break;
				}
			}
		}
	}
}

void PersonalityTree::RecursiveCleanMacInheritance(PersonalityNode * node, PersonalityNode * parent)
{
	for(uint16_t i = 0; i < node->m_children.size(); i++)
	{
		RecursiveCleanMacInheritance(node->m_children[i].second, node);
	}
	// look at parent and adjust inherited booleans in m_ports
	if(hhconfig->GetProfile(node->m_key)->parentProfile.compare("default"))
	{
		//If the parent and child each have only one ethernet vendor and they are the same, the child inherits the parent
		if((parent->m_vendor_dist.size() == 1) && (node->m_vendor_dist.size() == 1)
			&& (!node->m_vendor_dist[0].first.compare(parent->m_vendor_dist[0].first)))
		{
			hhconfig->m_profiles[node->m_key].inherited[ETHERNET] = true;
		}
	}
}

void PersonalityTree::RecursiveAddAllPorts(PersonalityNode * node)
{
	for(uint16_t i = 0; i < node->m_ports_dist.size(); i++)
	{
		port pass;

		pass.portName = node->m_ports_dist[i].first;
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
