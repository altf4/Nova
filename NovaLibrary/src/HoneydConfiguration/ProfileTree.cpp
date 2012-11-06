//============================================================================
// Name        : ProfileTree.cpp
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
// Description : Contains a tree of profiles in their respective hierarchy,
//		along with useful helper functions
//============================================================================

#include "../Config.h"
#include "ProfileTree.h"
#include "../Logger.h"
#include "HoneydConfiguration.h"

#include <boost/algorithm/string.hpp>

using namespace std;

namespace Nova
{

ProfileTree::ProfileTree()
{
	m_root = new Profile(NULL, "default");
	m_root->m_count = 1;
	m_root->m_distribution = 100;
	m_root->m_vendors.push_back(pair<string, double>("Dell", 1));

	PortSet *portset = new PortSet("default");
	portset->m_defaultICMPBehavior = PORT_OPEN;
	portset->m_defaultTCPBehavior = PORT_CLOSED;
	portset->m_defaultUDPBehavior = PORT_CLOSED;
	m_root->m_portSets.push_back(portset);
}

ProfileTree::~ProfileTree()
{
	delete m_root;
}

Profile *ProfileTree::GetRandomProfile()
{
	//Start with the root
	Profile *personality = m_root;

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

//TODO: Search could be improved by a heuristic, such as by using string similarity in chosing which children to pursue
Profile *GetProfile_helper(Profile *item, const std::string &name)
{
	if(item == NULL)
	{
		return NULL;
	}

	if(!item->m_key.compare(name))
	{
		return item;
	}

	//Depth first traversal of the tree
	for(uint i = 0; i < item->m_children.size(); i++)
	{
		Profile *foundItem = GetProfile_helper(item->m_children[i], name);
		if(foundItem != NULL)
		{
			return foundItem;
		}
	}

	return NULL;
}

Profile *ProfileTree::GetProfile(const std::string &name)
{
	return GetProfile_helper(m_root, name);
}

bool ProfileTree::LoadTable(ScannedHostTable *persTable)
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

	return true;
}

bool ProfileTree::InsertHost(ScannedHost *targetHost, Profile *parentItem)
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

	Profile *childProfile = NULL;

	//If node not found
	if(i == parentItem->m_children.size())
	{
		childProfile = new Profile(parentItem, curOSClass);
		childProfile->m_osclass = targetHost->m_osclass;
		childProfile->m_personality = targetHost->m_personality;
		parentItem->m_children.push_back(childProfile);
	}
	else
	{
		childProfile = parentItem->m_children[i];
	}

	//Increment not only this profile, but all of its ancestors
	Profile *loopProfile = childProfile;
	while(loopProfile != NULL)
	{
		loopProfile->m_count += targetHost->m_count;
		loopProfile->RecalculateChildDistributions();
		loopProfile = loopProfile->m_parent;
	}

	//Set the uptimes
	if(targetHost->m_uptime > childProfile->m_uptimeMax)
	{
		childProfile->m_uptimeMax = targetHost->m_uptime;
	}
	if(targetHost->m_uptime < childProfile->m_uptimeMin)
	{
		childProfile->m_uptimeMin = targetHost->m_uptime;
	}

	//TODO: Is this what we really want? Maybe we should just have the portset at the leaf node?
	//Add every PortSet from the target host into the childPersonality
//	for(uint j = 0; j < targetHost->m_portSets.size(); j++)
//	{
		childProfile->m_portSets = targetHost->m_portSets;
//	}

	//Insert or count MAC vendor occurrences
	for(MACVendorMap::iterator it = targetHost->m_vendors.begin(); it != targetHost->m_vendors.end(); it++)
	{
		for(uint j = 0; j < childProfile->m_vendors.size(); j++)
		{
			if(!childProfile->m_vendors[j].first.compare(it->first))
			{
				childProfile->m_vendors[j].second += it->second;
			}
		}
	}

	targetHost->m_personalityClass.pop_back();
	if(!targetHost->m_personalityClass.empty())
	{
		if(!InsertHost(targetHost, childProfile))
		{
			return false;
		}
	}
	return true;
}

}
