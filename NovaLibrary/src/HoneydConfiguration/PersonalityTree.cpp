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

	HoneydConfiguration::Inst()->ReadAllTemplatesXML();

	HoneydConfiguration::Inst()->AddGroup("HaystackAutoConfig");
	Config::Inst()->SetGroup("HaystackAutoConfig");
	Config::Inst()->SaveUserConfig();

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

string PersonalityTree::ToString()
{
	return ToString_helper(m_root);
}

string PersonalityTree::ToString_helper(PersonalityTreeItem *item)
{
	string runningTotal;

	if(item == NULL)
	{
		return "";
	}

	if(item->m_children.size() == 0)
	{
		runningTotal = item->ToString();
	}

	//Depth first recurse down
	for(uint i = 0; i < item->m_children.size(); i++)
	{
		runningTotal += ToString_helper(item->m_children[i]);
	}

	return runningTotal;
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
		for(uint i = 0; i < childPersonality->m_vendors.size(); i++)
		{
			if(!childPersonality->m_vendors[i].first.compare(it->first))
			{
				childPersonality->m_vendors[i].second += it->second;
			}
		}
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
