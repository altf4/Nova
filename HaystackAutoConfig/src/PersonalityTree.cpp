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
// Description : Source file for the PersonalityTree object. Contains a tree
//               representation of the relationships between different personalities.
//============================================================================

#include "Config.h"
#include "PersonalityTree.h"
#include <boost/algorithm/string.hpp>
#include "Logger.h"

using namespace std;

namespace Nova
{

PersonalityTree::PersonalityTree(PersonalityTable *persTable, vector<Subnet>& subnetsToUse)
{
	m_hdconfig = new HoneydConfiguration();

	m_root = PersonalityNode("default");
	m_root.m_count = persTable->m_num_of_hosts;
	m_root.m_distribution = 100;
	m_hdconfig->LoadAllTemplates();

	m_hdconfig->AddGroup("HaystackAutoConfig");
	Config::Inst()->SetGroup("HaystackAutoConfig");
	Config::Inst()->SaveUserConfig();
	m_hdconfig->SaveAllTemplates();
	m_hdconfig->LoadAllTemplates();

	for(NodeTable::iterator it = m_hdconfig->m_nodes.begin(); it != m_hdconfig->m_nodes.end(); it++)
	{
		if(it->first.compare("Doppelganger"))
		{
			m_hdconfig->DeleteNode(it->first);
		}
	}

	for(ProfileTable::iterator it = m_hdconfig->m_profiles.begin(); it != m_hdconfig->m_profiles.end(); it++)
	{
		if(it->second.m_generated)
		{
			m_hdconfig->DeleteProfile(it->first);
		}
	}


	m_hdconfig->SaveAllTemplates();
	m_hdconfig->LoadAllTemplates();
	m_profiles = &m_hdconfig->m_profiles;
	m_serviceMap = ServiceToScriptMap(&m_hdconfig->GetScriptTable());

	if(persTable != NULL)
	{
		LoadTable(persTable);
	}
}

PersonalityTree::~PersonalityTree()
{
	delete m_hdconfig;
}

PersonalityNode *PersonalityTree::GetRootNode()
{
	return &m_root;
}
HoneydConfiguration *PersonalityTree::GetHDConfig()
{
	return m_hdconfig;
}

bool PersonalityTree::LoadTable(PersonalityTable *persTable)
{
	if(persTable == NULL)
	{
		LOG(ERROR, "Unable to load NULL PersonalityTable!", "");
		return false;
	}

	Personality_Table *pTable = &persTable->m_personalities;

	for(Personality_Table::iterator it = pTable->begin(); it != pTable->end(); it++)
	{
		InsertPersonality(it->second);
	}
	CalculateDistributions();
	for(uint i = 0; i < m_root.m_children.size(); i++)
	{
		if(!GenerateProfiles(m_root.m_children[i].second, &m_root, &m_hdconfig->m_profiles["default"], m_root.m_children[i].first))
		{
			LOG(ERROR, "Unable to generate profiles for children of the root node!", "");
			return false;
		}
	}
	return true;
}

bool PersonalityTree::GenerateProfiles(PersonalityNode *node, PersonalityNode *parent, NodeProfile *parentProfile, const string &profileName)
{
	if(node == NULL || parent == NULL || parentProfile == NULL)
	{
		return false;
	}

	//Create profile object
	node->GenerateDistributions();
	NodeProfile tempProf = node->GenerateProfile(*parentProfile);
	tempProf.m_name = profileName;
	tempProf.m_distribution = node->m_distribution;
	stringstream ss;
	ss << tempProf.m_name << " distrib: " << tempProf.m_distribution << " count: " << node->m_count;
	LOG(DEBUG, ss.str(), "");
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

	if(!node->m_redundant || (parent == &m_root) || (parent->m_children.size() != 1))
	{
		if(!m_hdconfig->AddProfile(&tempProf))
		{
			LOG(ERROR, "Error adding " + tempProf.m_name, "");
			return false;
		}
		else
		{
			for(uint i = 0; i < node->m_children.size(); i++)
			{
				if(!GenerateProfiles(node->m_children[i].second, node, &m_hdconfig->m_profiles[tempProf.m_name], node->m_children[i].first))
				{
					LOG(ERROR, "Unable to generate profiles for children of node '" + node->m_key + "'.", "");
					return false;
				}
			}
		}
	}
	else
	{
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
			if(!GenerateProfiles(node->m_children[i].second, node, &m_hdconfig->m_profiles[key], node->m_children[i].first))
			{
				LOG(ERROR, "Unable to generate profiles for children of node '" + node->m_key + "'.", "");
				return false;
			}
		}
	}
	return true;
}

bool PersonalityTree::InsertPersonality(Personality *pers)
{
	if(pers == NULL)
	{
		LOG(ERROR, "Unable to insert NULL personality!", "");
		return false;
	}
	Personality temp = *pers;
	return UpdatePersonality(&temp, &m_root);
}

bool PersonalityTree::UpdatePersonality(Personality *targetPersonality, PersonalityNode *parentPersonalityNode)
{
	if(targetPersonality == NULL)
	{
		LOG(ERROR, "Unable to update a NULL personality!", "");
		return false;
	}
	else if(parentPersonalityNode == NULL)
	{
		LOG(ERROR, "Unable to update personality with a NULL PersonalityNode parent", "");
		return false;
	}

	string curOSClass = targetPersonality->m_personalityClass.back();

	uint i = 0;
	for(; i < parentPersonalityNode->m_children.size(); i++)
	{
		if(!curOSClass.compare(parentPersonalityNode->m_children[i].first))
		{
			break;
		}
	}

	pair<string, PersonalityNode *> *childPersonalityPair = NULL;

	//If node not found
	if(i == parentPersonalityNode->m_children.size())
	{
		childPersonalityPair = new pair<string, PersonalityNode *>();
		childPersonalityPair->first = curOSClass;
		childPersonalityPair->second = new PersonalityNode(curOSClass);
		childPersonalityPair->second->m_distribution = targetPersonality->m_distribution;
		childPersonalityPair->second->m_count = targetPersonality->m_count;
		childPersonalityPair->second->m_osclass = targetPersonality->m_osclass;
		parentPersonalityNode->m_children.push_back(*childPersonalityPair);
		delete childPersonalityPair;
	}
	else
	{
		parentPersonalityNode->m_children[i].second->m_count += targetPersonality->m_count;
	}

	childPersonalityPair = &parentPersonalityNode->m_children[i];
	//Insert or count port occurrences
	for(PortServiceMap::iterator it = targetPersonality->m_ports.begin(); it != targetPersonality->m_ports.end(); it++)
	{
		childPersonalityPair->second->m_ports[it->first].first += it->second.first;
		childPersonalityPair->second->m_ports[it->first].second = it->second.second;
	}

	//Insert or count MAC vendor occurrences
	for(MACVendorMap::iterator it = targetPersonality->m_vendors.begin(); it != targetPersonality->m_vendors.end(); it++)
	{
		childPersonalityPair->second->m_vendors[it->first] += it->second;
	}

	targetPersonality->m_personalityClass.pop_back();
	if(!targetPersonality->m_personalityClass.empty())
	{
		if(!UpdatePersonality(targetPersonality, childPersonalityPair->second))
		{
			return false;
		}
	}
	return true;
}

string PersonalityTree::ToString()
{
	stringstream ss;
	for(uint i = 0; i < m_root.m_children.size(); i++)
	{
		ss << RecursiveToString(*m_root.m_children[i].second);
	}
	return ss.str();
}

string PersonalityTree::RecursiveToString(PersonalityNode &persNode)
{
	stringstream ss;
	ss << persNode.ToString();
	for(uint i = 0; i < persNode.m_children.size(); i++)
	{
		ss << RecursiveToString(*persNode.m_children[i].second);
	}
	return ss.str();
}

string PersonalityTree::ToDebugString()
{
	stringstream ss;
	for(ProfileTable::iterator it = m_profiles->begin(); it != m_profiles->end(); it++)
	{
		ss << '\n';
		ss << "Profile: " << it->second.m_name << '\n';
		ss << "Parent: " << it->second.m_parentProfile << '\n';

		if(!it->second.m_personality.empty())
		{
			ss << "Personality: " << it->second.m_personality << '\n';
		}
		for(uint i = 0; i < it->second.m_ethernetVendors.size(); i++)
		{
			ss << "MAC Vendor:  " << it->second.m_ethernetVendors[i].first << " - " << it->second.m_ethernetVendors[i].second	<< "% \n";
		}
		if(!it->second.m_ports.empty())
		{
			ss << "Ports for this scope (<NUM>_<PROTOCOL>, inherited):" << '\n';
			for(uint16_t i = 0; i < it->second.m_ports.size(); i++)
			{
				cout << "\t" << it->second.m_ports[i].first << ", " << it->second.m_ports[i].second.first << '\n';
			}
		}
		ss << '\n';
	}
	return ss.str();
}

bool PersonalityTree::ToXmlTemplate()
{
	return m_hdconfig->SaveAllTemplates();
}

bool PersonalityTree::AddAllPorts()
{
	for(uint16_t i = 0; i < m_root.m_children.size(); i++)
	{
		if(!RecursiveAddAllPorts(m_root.m_children[i].second))
		{
			return false;
		}
	}
	return m_hdconfig->SaveAllTemplates();
}

bool PersonalityTree::RecursiveAddAllPorts(PersonalityNode *node)
{
	if(node == NULL)
	{
		LOG(ERROR, "NULL PersonalityNode passed as paremeter!", "");
		return false;
	}

	for(unsigned int i = 0; i < node->m_children.size(); i++)
	{
		if(!RecursiveAddAllPorts(node->m_children[i].second))
		{
			return false;
		}
	}

	for(unsigned int  i = 0; i < node->m_ports_dist.size(); i++)
	{
		Port scriptedPort;
		Port openPort;

		vector<string> portTokens;

		boost::split(portTokens, node->m_ports_dist[i].first, boost::is_any_of("_"));

		openPort.m_portName = portTokens[0] + "_" + portTokens[1] + "_reset";
		openPort.m_portNum = portTokens[0];
		openPort.m_type = portTokens[1];
		openPort.m_behavior = "reset";
		m_hdconfig->AddPort(openPort);

		scriptedPort.m_portNum = portTokens[0];
		scriptedPort.m_type = portTokens[1];
		scriptedPort.m_portName = scriptedPort.m_portNum + "_" + scriptedPort.m_type + "_open";
		scriptedPort.m_behavior = "open";
		m_hdconfig->AddPort(scriptedPort);

		scriptedPort.m_portName = portTokens[0] + "_" + portTokens[1];
		scriptedPort.m_behavior = "script";

		vector<pair<string, uint> > potentialMatches;

		for(PortServiceMap::iterator it = node->m_ports.begin(); it != node->m_ports.end(); it++)
		{
			if(!m_serviceMap.GetScripts(it->second.second).empty() && !(it->first + "_open").compare(node->m_ports_dist[i].first))
			{
				scriptedPort.m_service = it->second.second;

				vector<string> nodeOSClasses;
				vector<string> scriptOSClasses;

				vector<Script> potentialScripts = m_serviceMap.GetScripts(it->second.second);
				for(uint j = 0; j < potentialScripts.size(); j++)
				{
					uint depthOfMatch = 0;

					boost::split(nodeOSClasses, node->m_osclass, boost::is_any_of("|"));
					boost::split(scriptOSClasses, potentialScripts[j].m_osclass, boost::is_any_of("|"));

					for(uint k = 0; k < nodeOSClasses.size(); k++)
					{
						nodeOSClasses[k] = boost::trim_left_copy(nodeOSClasses[k]);
						nodeOSClasses[k] = boost::trim_right_copy(nodeOSClasses[k]);
					}
					for(uint k = 0; k < scriptOSClasses.size(); k++)
					{
						scriptOSClasses[k] = boost::trim_left_copy(scriptOSClasses[k]);
						scriptOSClasses[k] = boost::trim_right_copy(scriptOSClasses[k]);
					}

					if(nodeOSClasses.size() < scriptOSClasses.size())
					{
						for(int k = (int)nodeOSClasses.size() - 1; k >= 0; k--)
						{
							if(!nodeOSClasses[k].compare(scriptOSClasses[k]))
							{
								depthOfMatch++;
							}
						}

						pair<string, uint> potentialMatch;
						potentialMatch.first = potentialScripts[j].m_name;
						potentialMatch.second = depthOfMatch;
						potentialMatches.push_back(potentialMatch);
					}
					else
					{
						for(uint k = 0; k < scriptOSClasses.size(); k++)
						{
							if(!nodeOSClasses[nodeOSClasses.size() - 1 - k].compare(scriptOSClasses[k]))
							{
								depthOfMatch++;
							}
						}

						pair<string, uint> potentialMatch;
						potentialMatch.first = potentialScripts[j].m_name;
						potentialMatch.second = depthOfMatch;
						potentialMatches.push_back(potentialMatch);
					}
				}

				string closestMatch = potentialMatches[0].first;
				uint highestMatchDepth = potentialMatches[0].second;

				for(uint k = 1; k < potentialMatches.size(); k++)
				{
					if(potentialMatches[k].second > highestMatchDepth)
					{
						highestMatchDepth = potentialMatches[k].second;
						closestMatch = potentialMatches[k].first;
					}
				}

				scriptedPort.m_scriptName = closestMatch;
				scriptedPort.m_portName = scriptedPort.m_portNum + "_" + scriptedPort.m_type + "_" + scriptedPort.m_scriptName;

				vector<string> nodeOSClassesCopy = nodeOSClasses;
				string profileName = "";

				if(!node->m_key.compare(nodeOSClassesCopy[0]))
				{
					for(uint k = 0; k < m_hdconfig->GetProfile(node->m_key)->m_ports.size(); k++)
					{
						if(!(it->first + "_open").compare(m_hdconfig->GetProfile(node->m_key)->m_ports[k].first))
						{
							m_hdconfig->GetProfile(node->m_key)->m_ports[k].first = scriptedPort.m_portName;
							node->m_ports_dist[i].first = scriptedPort.m_portName;
						}
					}

					m_hdconfig->UpdateProfile(node->m_key);
				}
				else
				{
					while(nodeOSClassesCopy.size())
					{
						profileName.append(nodeOSClassesCopy.back());
						//Here we've matched the profile corresponding to the compressed osclass tokens in name
						if(m_hdconfig->GetProfile(profileName) != NULL)
						{
							//modify profile's ports and stuff here
							for(uint k = 0; k < m_hdconfig->GetProfile(profileName)->m_ports.size(); k++)
							{
								if(!(it->first + "_open").compare(m_hdconfig->GetProfile(profileName)->m_ports[k].first))
								{
									m_hdconfig->GetProfile(profileName)->m_ports[k].first = scriptedPort.m_portName;
									node->m_ports_dist[i].first = scriptedPort.m_portName;
								}
							}
							m_hdconfig->UpdateProfile(profileName);
							profileName = "";
						}
						else
						{
							profileName.append(" ");
						}
						nodeOSClassesCopy.pop_back();
					}
				}
				m_hdconfig->AddPort(scriptedPort);
				continue;
			}
		}
	}
	return true;
}

void PersonalityTree::RecursivePrintTree(PersonalityNode *node)
{
	if(node == NULL)
	{
		return;
	}

	LOG(DEBUG, node->m_key, "");

	for(uint i = 0; i < node->m_children.size(); i++)
	{
		RecursivePrintTree(node->m_children[i].second);
	}
}

bool PersonalityTree::CalculateDistributions()
{
	return RecursiveCalculateDistribution(&m_root);
}

bool PersonalityTree::RecursiveCalculateDistribution(PersonalityNode *targetNode)
{
	if(targetNode == NULL)
	{
		LOG(ERROR, "Unable to calculate the distribution for a NULL node!", "");
		return false;
	}
	for(uint i = 0; i < targetNode->m_children.size(); i++)
	{
		if((targetNode->m_children[i].second->m_count > 0) && (targetNode->m_count > 0))
		{
			targetNode->m_children[i].second->m_distribution = (((double)targetNode->m_children[i].second->m_count)/((double)targetNode->m_count))*100;
		}
		else
		{
			targetNode->m_children[i].second->m_distribution = 0;
		}
		if(!RecursiveCalculateDistribution(targetNode->m_children[i].second))
		{
			return false;
		}
	}
	return true;
}
}
