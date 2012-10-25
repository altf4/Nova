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

#include "../Config.h"
#include "PersonalityTree.h"
#include "../Logger.h"
#include "HoneydConfiguration.h"

#include <boost/algorithm/string.hpp>

using namespace std;

namespace Nova
{

PersonalityTree::PersonalityTree(ScannedHostTable *persTable, vector<Subnet>& subnetsToUse)
{
	m_root = new PersonalityTreeItem(NULL, "default");
	m_root->m_count = persTable->m_num_of_hosts;
	m_root->m_distribution = 100;
	m_root->m_TCP_behavior = "reset";
	m_root->m_UDP_behavior = "reset";
	m_root->m_ICMP_behavior = "open";

	HoneydConfiguration::Inst()->LoadAllTemplates();

	HoneydConfiguration::Inst()->AddGroup("HaystackAutoConfig");
	Config::Inst()->SetGroup("HaystackAutoConfig");
	Config::Inst()->SaveUserConfig();
	HoneydConfiguration::Inst()->SaveAllTemplates();
	HoneydConfiguration::Inst()->LoadAllTemplates();

	vector<string> nodesToDelete;
	for(NodeTable::iterator it = HoneydConfiguration::Inst()->m_nodes.begin(); it != HoneydConfiguration::Inst()->m_nodes.end(); it++)
	{
		if(it->first.compare("Doppelganger"))
		{
			nodesToDelete.push_back(it->first);
		}
	}
	for (vector<string>::iterator it = nodesToDelete.begin(); it != nodesToDelete.end(); it++)
	{
		HoneydConfiguration::Inst()->DeleteNode(*it);
	}

	HoneydConfiguration::Inst()->SaveAllTemplates();
	HoneydConfiguration::Inst()->LoadAllTemplates();
	m_serviceMap = ServiceToScriptMap(&HoneydConfiguration::Inst()->GetScriptTable());

	if(persTable != NULL)
	{
		LoadTable(persTable);
	}

	//AddAllPorts(m_root);
}

PersonalityTree::~PersonalityTree()
{
	delete m_root;
}

PersonalityTreeItem *PersonalityTree::GetRandomProfile()
{
	//Start with the root
	PersonalityTreeItem *personality = m_root;

	//Keep going until you get to a leaf node
	while(!personality->m_children.empty())
	{
		//Random double between 0 and 100
		double random = ((double)rand() / (double)RAND_MAX) * 100;

		double runningTotal = 0;
		bool found = false;
		//For each child, pick one
		for(uint i = 0; i < personality->m_children.size(); i++)
		{
			runningTotal += personality->m_children[i]->m_distribution;
			if(random < runningTotal)
			{
				//Winner
				personality = personality->m_children[i];
				found = true;
				break;
			}
		}
		if(!found)
		{
			//If we've gotten here, then something strange happened, like children distributions not adding to 100
			//Just pick the last child, to err on the side of caution. (maybe they summed to 99.98, and we rolled 99.99)
			personality = personality->m_children.back();
		}
	}

	return personality;
}

bool PersonalityTree::LoadTable(ScannedHostTable *persTable)
{
	if(persTable == NULL)
	{
		LOG(ERROR, "Unable to load NULL PersonalityTable!", "");
		return false;
	}

	ScannedHost_Table *pTable = &persTable->m_personalities;

	for(ScannedHost_Table::iterator it = pTable->begin(); it != pTable->end(); it++)
	{
		InsertHost(it->second, m_root);
	}
	CalculateDistributions(m_root);

	return true;
}

bool PersonalityTree::InsertHost(ScannedHost *targetHost, PersonalityTreeItem *parentItem)
{
	if(targetHost == NULL)
	{
		LOG(ERROR, "Unable to update a NULL personality!", "");
		return false;
	}
	else if(parentItem == NULL)
	{
		LOG(ERROR, "Unable to update personality with a NULL PersonalityNode parent", "");
		return false;
	}

	string curOSClass = targetHost->m_personalityClass.back();

	//Find
	uint i = 0;
	for(; i < parentItem->m_children.size(); i++)
	{
		if(!curOSClass.compare(parentItem->m_children[i]->m_key))
		{
			break;
		}
	}

	PersonalityTreeItem *childPersonality = NULL;

	//If node not found
	if(i == parentItem->m_children.size())
	{
		childPersonality = new PersonalityTreeItem(parentItem, curOSClass);
		childPersonality->m_distribution = targetHost->m_distribution;
		childPersonality->m_count = targetHost->m_count;
		childPersonality->m_osclass = targetHost->m_osclass;
		parentItem->m_children.push_back(childPersonality);
	}
	else
	{
		parentItem->m_children[i]->m_count += targetHost->m_count;
	}

	childPersonality = parentItem->m_children[i];

	//Add every PortSet from the target host into the childPersonality
	for(uint j = 0; j < targetHost->m_portSets.size(); j++)
	{
		childPersonality->m_portSets.push_back(targetHost->m_portSets[i]);
	}

	//Insert or count MAC vendor occurrences
	for(MACVendorMap::iterator it = targetHost->m_vendors.begin(); it != targetHost->m_vendors.end(); it++)
	{
		childPersonality->m_vendors[it->first] += it->second;
	}

	targetHost->m_personalityClass.pop_back();
	if(!targetHost->m_personalityClass.empty())
	{
		if(!InsertHost(targetHost, childPersonality))
		{
			return false;
		}
	}
	return true;
}

bool PersonalityTree::AddAllPorts(PersonalityTreeItem *personalityItem)
{
//	if(personalityItem == NULL)
//	{
//		LOG(ERROR, "NULL PersonalityNode passed as parameter!", "");
//		return false;
//	}
//
//	//Depth first search through the tree
//	for(unsigned int i = 0; i < personalityItem->m_children.size(); i++)
//	{
//		if(!AddAllPorts(personalityItem->m_children[i]))
//		{
//			return false;
//		}
//	}
//
//	for(unsigned int  i = 0; i < personalityItem->m_ports_dist.size(); i++)
//	{
//		PortStruct scriptedPort;
//		PortStruct openPort;
//
//		vector<string> portTokens;
//
//		boost::split(portTokens, personalityItem->m_ports_dist[i].first, boost::is_any_of("_"));
//
//		openPort.m_portName = portTokens[0] + "_" + portTokens[1] + "_reset";
//		openPort.m_portNum = portTokens[0];
//		openPort.m_type = portTokens[1];
//		openPort.m_behavior = "reset";
//		m_hdconfig->AddPort(openPort);
//
//		scriptedPort.m_portNum = portTokens[0];
//		scriptedPort.m_type = portTokens[1];
//		scriptedPort.m_portName = scriptedPort.m_portNum + "_" + scriptedPort.m_type + "_open";
//		scriptedPort.m_behavior = "open";
//		m_hdconfig->AddPort(scriptedPort);
//
//		scriptedPort.m_portName = portTokens[0] + "_" + portTokens[1];
//		scriptedPort.m_behavior = "script";
//
//		vector<pair<string, uint> > potentialMatches;
//
//		for(PortServiceMap::iterator it = personalityItem->m_portSets.begin(); it != personalityItem->m_portSets.end(); it++)
//		{
//			if(!m_serviceMap.GetScripts(it->second.second).empty() && !(it->first + "_open").compare(personalityItem->m_ports_dist[i].first))
//			{
//				scriptedPort.m_service = it->second.second;
//
//				vector<string> nodeOSClasses;
//				vector<string> scriptOSClasses;
//
//				vector<Script> potentialScripts = m_serviceMap.GetScripts(it->second.second);
//				for(uint j = 0; j < potentialScripts.size(); j++)
//				{
//					uint depthOfMatch = 0;
//
//					boost::split(nodeOSClasses, personalityItem->m_osclass, boost::is_any_of("|"));
//					boost::split(scriptOSClasses, potentialScripts[j].m_osclass, boost::is_any_of("|"));
//
//					for(uint k = 0; k < nodeOSClasses.size(); k++)
//					{
//						nodeOSClasses[k] = boost::trim_left_copy(nodeOSClasses[k]);
//						nodeOSClasses[k] = boost::trim_right_copy(nodeOSClasses[k]);
//					}
//					for(uint k = 0; k < scriptOSClasses.size(); k++)
//					{
//						scriptOSClasses[k] = boost::trim_left_copy(scriptOSClasses[k]);
//						scriptOSClasses[k] = boost::trim_right_copy(scriptOSClasses[k]);
//					}
//
//					if(nodeOSClasses.size() < scriptOSClasses.size())
//					{
//						for(int k = (int)nodeOSClasses.size() - 1; k >= 0; k--)
//						{
//							if(!nodeOSClasses[k].compare(scriptOSClasses[k]))
//							{
//								depthOfMatch++;
//							}
//						}
//
//						pair<string, uint> potentialMatch;
//						potentialMatch.first = potentialScripts[j].m_name;
//						potentialMatch.second = depthOfMatch;
//						potentialMatches.push_back(potentialMatch);
//					}
//					else
//					{
//						for(uint k = 0; k < scriptOSClasses.size(); k++)
//						{
//							if(!nodeOSClasses[nodeOSClasses.size() - 1 - k].compare(scriptOSClasses[k]))
//							{
//								depthOfMatch++;
//							}
//						}
//
//						pair<string, uint> potentialMatch;
//						potentialMatch.first = potentialScripts[j].m_name;
//						potentialMatch.second = depthOfMatch;
//						potentialMatches.push_back(potentialMatch);
//					}
//				}
//
//				string closestMatch = potentialMatches[0].first;
//				uint highestMatchDepth = potentialMatches[0].second;
//
//				for(uint k = 1; k < potentialMatches.size(); k++)
//				{
//					if(potentialMatches[k].second > highestMatchDepth)
//					{
//						highestMatchDepth = potentialMatches[k].second;
//						closestMatch = potentialMatches[k].first;
//					}
//				}
//
//				scriptedPort.m_scriptName = closestMatch;
//				scriptedPort.m_portName = scriptedPort.m_portNum + "_" + scriptedPort.m_type + "_" + scriptedPort.m_scriptName;
//
//				vector<string> nodeOSClassesCopy = nodeOSClasses;
//				string profileName = "";
//
//				if(!personalityItem->m_key.compare(nodeOSClassesCopy[0]))
//				{
//					for(uint k = 0; k < m_hdconfig->GetProfile(personalityItem->m_key)->m_ports.size(); k++)
//					{
//						if(!(it->first + "_open").compare(m_hdconfig->GetProfile(personalityItem->m_key)->m_ports[k].first))
//						{
//							m_hdconfig->GetProfile(personalityItem->m_key)->m_ports[k].first = scriptedPort.m_portName;
//							personalityItem->m_ports_dist[i].first = scriptedPort.m_portName;
//						}
//					}
//
//					m_hdconfig->UpdateProfile(personalityItem->m_key);
//				}
//				else
//				{
//					while(nodeOSClassesCopy.size())
//					{
//						profileName.append(nodeOSClassesCopy.back());
//						//Here we've matched the profile corresponding to the compressed osclass tokens in name
//						if(m_hdconfig->GetProfile(profileName) != NULL)
//						{
//							//modify profile's ports and stuff here
//							for(uint k = 0; k < m_hdconfig->GetProfile(profileName)->m_ports.size(); k++)
//							{
//								if(!(it->first + "_open").compare(m_hdconfig->GetProfile(profileName)->m_ports[k].first))
//								{
//									m_hdconfig->GetProfile(profileName)->m_ports[k].first = scriptedPort.m_portName;
//									personalityItem->m_ports_dist[i].first = scriptedPort.m_portName;
//								}
//							}
//							m_hdconfig->UpdateProfile(profileName);
//							profileName = "";
//						}
//						else
//						{
//							profileName.append(" ");
//						}
//						nodeOSClassesCopy.pop_back();
//					}
//				}
//				m_hdconfig->AddPort(scriptedPort);
//				continue;
//			}
//		}
//	}
//	return m_hdconfig->SaveAllTemplates();
	return false;
}

bool PersonalityTree::CalculateDistributions(PersonalityTreeItem *targetNode)
{
	if(targetNode == NULL)
	{
		LOG(ERROR, "Unable to calculate the distribution for a NULL node!", "");
		return false;
	}
	for(uint i = 0; i < targetNode->m_children.size(); i++)
	{
		if((targetNode->m_children[i]->m_count > 0) && (targetNode->m_count > 0))
		{
			targetNode->m_children[i]->m_distribution =
					(((double)targetNode->m_children[i]->m_count)/((double)targetNode->m_count))*100;
		}
		else
		{
			targetNode->m_children[i]->m_distribution = 0;
		}
		if(!CalculateDistributions(targetNode->m_children[i]))
		{
			return false;
		}
	}
	return true;
}
}
