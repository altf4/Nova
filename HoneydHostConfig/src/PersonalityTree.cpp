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
#include <boost/algorithm/string.hpp>
#include "NodeRandomizer.h"

using namespace std;

namespace Nova
{

PersonalityTree::PersonalityTree(PersonalityTable *persTable)
{
	m_hdconfig = new HoneydConfiguration();

	m_root = PersonalityNode("default");
	m_hdconfig->LoadAllTemplates();
	m_profiles = &m_hdconfig->m_profiles;
	m_scripts = ScriptTable(m_hdconfig->GetScriptTable());

	m_scripts.PrintScriptsTable();

	if(persTable != NULL)
	{
		LoadTable(persTable);
	}
}

PersonalityTree::~PersonalityTree()
{
}

HoneydConfiguration * PersonalityTree::GetHHConfig()
{
	return m_hdconfig;
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
		GenerateProfiles(m_root.m_children[i].second, &m_root, &m_hdconfig->m_profiles["default"], m_root.m_children[i].first);
	}
}

void PersonalityTree::GenerateProfiles(PersonalityNode *node, PersonalityNode *parent, profile *parentProfile, string profileName)
{
	//Create profile object
	node->GenerateDistributions();
	profile tempProf = node->GenerateProfile(parentProfile);
	if(m_profiles->find(tempProf.name) != m_profiles->end())
	{
		// Probably not the right way of going about this
		uint16_t i = 1;
		stringstream ss;
		string key = profileName;
		ss << i;

		for(; m_profiles->find(key) != m_profiles->end() && (i < ~0); i++)
		{
			ss.str("");
			ss << profileName << i;
			key = ss.str();
		}
		tempProf.name = key;
	}
	if(!node->m_redundant)
	{
		if(!m_hdconfig->AddProfile(&tempProf))
		{
			cout << "Error adding "<< tempProf.name << endl;
			return;
		}
		else
		{
			for(uint i = 0; i < node->m_children.size(); i++)
			{
				GenerateProfiles(node->m_children[i].second, node, &m_hdconfig->m_profiles[tempProf.name], node->m_children[i].first);
			}
		}
	}
	else if(parent->m_children.size() == 1)
	{
		// Probably not the right way of going about this
		uint16_t i = 1;
		stringstream ss;
		string key = parentProfile->name + " " + profileName;
		ss << i;
		while(!m_hdconfig->RenameProfile(parentProfile->name, key))
		{
			ss.str("");
			ss << i;
			key = parentProfile->name + " " + profileName + ss.str();
		}
		for(uint i = 0; i < node->m_children.size(); i++)
		{
			GenerateProfiles(node->m_children[i].second, node, &m_hdconfig->m_profiles[key], node->m_children[i].first);
		}
	}
	else
	{
		string keyPrefix = profileName + " ";
		for(uint i = 0; i < node->m_children.size(); i++)
		{
			GenerateProfiles(node->m_children[i].second, node, parentProfile, keyPrefix + node->m_children[i].first);
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
	for(PortsTable::iterator it = pers->m_ports.begin(); it != pers->m_ports.end(); it++)
	{
		tablePair->second->m_ports[it->first].first += it->second.first;
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

	vector<string> ret = m_hdconfig->GetProfileNames();

	AddAllPorts();

	ret = m_hdconfig->GetProfileNames();

	for(uint16_t i = 0; i < ret.size(); i++)
	{
		cout << "Profile after Clean: " << ret[i] << " has parent: " << m_hdconfig->GetProfile(ret[i])->parentProfile << endl;
	}

	m_hdconfig->SaveAllTemplates();
}

void PersonalityTree::AddAllPorts()
{
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

		vector<string> tokens;

		boost::split(tokens, node->m_ports_dist[i].first, boost::is_any_of("_"));

		pass.portName = node->m_ports_dist[i].first;
		pass.portNum = tokens[0];
		pass.type = tokens[1];
		pass.behavior = tokens[2];

		m_hdconfig->AddPort(pass);
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

bool PersonalityTree::AddSubnet(subnet * add)
{
	return m_hdconfig->AddSubnet(add);
}

}
