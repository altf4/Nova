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

using namespace std;

namespace Nova
{

PersonalityTree::PersonalityTree(PersonalityTable *persTable)
{
	m_hdconfig = new HoneydConfiguration();

	m_root = PersonalityNode("default");
	m_hdconfig->LoadAllTemplates();
	m_hdconfig->m_subnets.clear();
	m_profiles = &m_hdconfig->m_profiles;
	m_scripts = ScriptTable(m_hdconfig->GetScriptTable());

	m_scripts.PrintScriptsTable();

	if(persTable != NULL)
	{
		LoadTable(persTable);
	}
	GetHostCount();

	cout << m_root.m_count << endl;
}

PersonalityTree::~PersonalityTree()
{
}

PersonalityNode *PersonalityTree::GetRootNode()
{
	return &m_root;
}
HoneydConfiguration *PersonalityTree::GetHDConfig()
{
	return m_hdconfig;
}

void PersonalityTree::LoadTable(PersonalityTable *persTable)
{
	if(persTable == NULL)
	{
		return;
	}

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

void PersonalityTree::GenerateProfiles(PersonalityNode *node, PersonalityNode *parent, NodeProfile *parentProfile, string profileName)
{
	if(node == NULL || parent == NULL || parentProfile == NULL)
	{
		return;
	}

	//Create profile object
	node->GenerateDistributions();
	NodeProfile tempProf = node->GenerateProfile(parentProfile);
	if(m_profiles->find(tempProf.m_name) != m_profiles->end())
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
		tempProf.m_name = key;
	}

	if(!node->m_redundant)
	{
		if(!m_hdconfig->AddProfile(&tempProf))
		{
			cout << "Error adding "<< tempProf.m_name << endl;
			return;
		}
		else
		{
			for(uint i = 0; i < node->m_children.size(); i++)
			{
				GenerateProfiles(node->m_children[i].second, node, &m_hdconfig->m_profiles[tempProf.m_name], node->m_children[i].first);
			}
		}
	}
	else if(parent->m_children.size() == 1)
	{
		// Probably not the right way of going about this
		uint16_t i = 1;
		stringstream ss;
		string key = parentProfile->m_name + " " + profileName;
		ss << i;
		while(!m_hdconfig->RenameProfile(parentProfile->m_name, key))
		{
			ss.str("");
			ss << i;
			key = parentProfile->m_name + " " + profileName + ss.str();
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
	if(pers == NULL)
	{
		return;
	}

	Personality temp = *pers;

	UpdatePersonality(&temp, &m_root);
}

void PersonalityTree::UpdatePersonality(Personality *pers, PersonalityNode *parent)
{
	if(pers == NULL || parent == NULL)
	{
		return;
	}

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

	tablePair->second->m_osclass = pers->m_osclass;

	//Insert or count port occurrences
	for(PortsTable::iterator it = pers->m_ports.begin(); it != pers->m_ports.end(); it++)
	{
		tablePair->second->m_ports[it->first].first += it->second.first;
		tablePair->second->m_ports[it->first].second = it->second.second;
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

void PersonalityTree::RecursiveToString(PersonalityNode *persNode)
{
	if(persNode == NULL)
	{
		return;
	}

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

void PersonalityTree::RecursiveGenerateDistributions(PersonalityNode *node)
{
	if(node == NULL)
	{
		return;
	}

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
		cout << "Profile: " << it->second.m_name << endl;
		cout << "Parent: " << it->second.m_parentProfile << endl;

		if(!it->second.m_personality.empty())
		{
			cout << "Personality: " << it->second.m_personality << endl;
		}
		if(!it->second.m_ethernet.empty())
		{
			cout << "MAC vendor: " << it->second.m_ethernet << endl;
		}
		if(!it->second.m_ports.empty())
		{
			cout << "Ports for this scope (<NUM>_<PROTOCOL>, inherited):" << endl;
			for(uint16_t i = 0; i < it->second.m_ports.size(); i++)
			{
				cout << "\t" << it->second.m_ports[i].first << ", " << it->second.m_ports[i].second << endl;
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
		cout << "Profile after Clean: " << ret[i] << " has parent: " << m_hdconfig->GetProfile(ret[i])->m_parentProfile << endl;
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

void PersonalityTree::RecursiveAddAllPorts(PersonalityNode *node)
{
	if(node == NULL)
	{
		return;
	}

	for(uint16_t i = 0; i < node->m_children.size(); i++)
	{
		RecursiveAddAllPorts(node->m_children[i].second);
	}

	for(uint16_t i = 0; i < node->m_ports_dist.size(); i++)
	{
		Port pass;
		Port passOpenVersion;

		vector<string> tokens;

		boost::split(tokens, node->m_ports_dist[i].first, boost::is_any_of("_"));

		passOpenVersion.m_portName = tokens[0] + "_" + tokens[1] + "_open";
		passOpenVersion.m_portNum = tokens[0];
		passOpenVersion.m_type = tokens[1];
		passOpenVersion.m_behavior = tokens[2];
		m_hdconfig->AddPort(passOpenVersion);

		pass.m_portName = tokens[0] + "_" + tokens[1];
		pass.m_portNum = tokens[0];
		pass.m_type = tokens[1];

		bool endIter = false;

		vector<pair<string, uint> > find_closest_match;

		for(PortsTable::iterator it = node->m_ports.begin(); it != node->m_ports.end() && !endIter; it++)
		{
			if(m_scripts.GetScriptsTable().keyExists(it->second.second) && !(it->first + "_open").compare(node->m_ports_dist[i].first))
			{
				pass.m_behavior = "script";
				pass.m_service = it->second.second;
				vector<string> tokens_osclass;
				vector<string> tokens_script_osclass;

				for(uint j = 0; j < m_scripts.GetScriptsTable()[it->second.second].size(); j++)
				{
					uint count = 0;

					boost::split(tokens_osclass, node->m_osclass, boost::is_any_of("|"));
					boost::split(tokens_script_osclass, m_scripts.GetScriptsTable()[it->second.second][j].first, boost::is_any_of("|"));

					for(uint k = 0; k < tokens_osclass.size(); k++)
					{
						tokens_osclass[k] = boost::trim_left_copy(tokens_osclass[k]);
						tokens_osclass[k] = boost::trim_right_copy(tokens_osclass[k]);
						//cout << tokens_osclass[k] << endl;
					}
					for(uint k = 0; k < tokens_script_osclass.size(); k++)
					{
						tokens_script_osclass[k] = boost::trim_left_copy(tokens_script_osclass[k]);
						tokens_script_osclass[k] = boost::trim_right_copy(tokens_script_osclass[k]);
						//cout << tokens_script_osclass[k] << endl;
					}

					if(tokens_osclass.size() < tokens_script_osclass.size())
					{
						for(uint k = tokens_osclass.size() - 1; k >= 0; k++)
						{
							if(!tokens_osclass[k].compare(tokens_script_osclass[k]))
							{
								count++;
							}
						}

						pair<string, uint> push;
						push.first = m_scripts.GetScriptsTable()[it->second.second][j].second;
						push.second = count;
						find_closest_match.push_back(push);
					}
					else
					{
						for(uint k = 0; k < tokens_script_osclass.size(); k++)
						{
							if(!tokens_osclass[tokens_osclass.size() - 1 - k].compare(tokens_script_osclass[k]))
							{
								count++;
							}
						}

						pair<string, uint> push;
						push.first = m_scripts.GetScriptsTable()[it->second.second][j].second;
						push.second = count;
						find_closest_match.push_back(push);
					}
				}

				string closestMatch = find_closest_match[0].first;
				uint max = find_closest_match[0].second;

				for(uint k = 1; k < find_closest_match.size(); k++)
				{
					if(find_closest_match[k].second > max)
					{
						max = find_closest_match[k].second;
						closestMatch = find_closest_match[k].first;
					}
				}

				pass.m_scriptName = closestMatch;
				pass.m_portName += "_" + pass.m_scriptName;

				vector<string> osclass_copy = tokens_osclass;
				string name = "";

				if(!node->m_key.compare(osclass_copy[0]))
				{
					for(uint k = 0; k < m_hdconfig->GetProfile(node->m_key)->m_ports.size(); k++)
					{
						if(!(it->first + "_open").compare(m_hdconfig->GetProfile(node->m_key)->m_ports[k].first))
						{
							m_hdconfig->GetProfile(node->m_key)->m_ports[k].first = pass.m_portName;
						}
					}

					m_hdconfig->UpdateProfile(node->m_key);
				}
				else
				{
					while(osclass_copy.size())
					{
						name.append(osclass_copy.back());
						//Here we've matched the profile corresponding to the compressed osclass tokens in name
						if(m_hdconfig->GetProfile(name) != NULL)
						{
							//modify profile's ports and stuff here
							for(uint k = 0; k < m_hdconfig->GetProfile(name)->m_ports.size(); k++)
							{
								if(!(it->first + "_open").compare(m_hdconfig->GetProfile(name)->m_ports[k].first))
								{
									m_hdconfig->GetProfile(name)->m_ports[k].first = pass.m_portName;
								}
							}
							m_hdconfig->UpdateProfile(name);
							name = "";
						}
						else
						{
							name.append(" ");
						}
						osclass_copy.pop_back();
					}
				}
				endIter = true;
			}
		}
		if(!endIter)
		{
			pass.m_portName += "_" + tokens[2];
			pass.m_behavior = tokens[2];
		}

		m_hdconfig->AddPort(pass);
	}
}

void PersonalityTree::RecursivePrintTree(PersonalityNode *node)
{
	if(node == NULL)
	{
		return;
	}

	cout << node->m_key << endl;

	for(uint i = 0; i < node->m_children.size(); i++)
	{
		RecursivePrintTree(node->m_children[i].second);
	}
}

bool PersonalityTree::AddSubnet(Subnet *add)
{
	return m_hdconfig->AddSubnet(add);
}

void PersonalityTree::GetHostCount()
{
	for(uint i = 0; i < m_root.m_children.size(); i++)
	{
		m_root.m_count += m_root.m_children[i].second->m_count;
	}
}

}
