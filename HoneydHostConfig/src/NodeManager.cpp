//============================================================================
// Name        :
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
// Description :
//============================================================================

#include "NodeManager.h"
#include "Logger.h"

using namespace std;

namespace Nova
{

NodeManager::NodeManager(PersonalityTree *persTree)
{
	if(persTree != NULL)
	{
		m_persTree = persTree;
		m_hdconfig = m_persTree->GetHDConfig();
	}
}

bool NodeManager::SetPersonalityTree(PersonalityTree *persTree)
{
	if(persTree == NULL)
	{
		return false;
	}
	m_persTree = persTree;
	m_hdconfig = m_persTree->GetHDConfig();
	return true;
}

void NodeManager::GenerateProfileCounters()
{
	PersonalityNode *rootNode = m_persTree->GetRootNode();
	m_hostCount = rootNode->m_count;
	RecursiveGenProfileCounter(rootNode);
}

void NodeManager::RecursiveGenProfileCounter(PersonalityNode *parent)
{
	if(parent->m_children.empty())
	{
		struct ProfileCounter pCounter;

		if(m_hdconfig->GetProfile(parent->m_key) == NULL)
		{
			LOG(ERROR, "Couldn't retrieve expected profile: " + parent->m_key, "");
			return;
		}
		pCounter.m_profile = *m_hdconfig->GetProfile(parent->m_key);
		pCounter.m_maxCount = parent->m_count/m_hostCount;
		pCounter.m_minCount = 100 - pCounter.m_maxCount;
		for(unsigned int i = 0; i < parent->m_vendor_dist.size(); i++)
		{
			pCounter.m_macCounters.push_back(GenerateMacCounter(parent->m_vendor_dist[i].first, parent->m_vendor_dist[i].second));
		}
		for(unsigned int i = 0; i < parent->m_ports_dist.size(); i++)
		{
			pCounter.m_portCounters.push_back(GeneratePortCounter(parent->m_vendor_dist[i].first, parent->m_vendor_dist[i].second));
		}
	}
	for(uint i = 0; i < parent->m_children.size(); i++)
	{
		RecursiveGenProfileCounter(parent->m_children[i].second);
	}
}

MacCounter NodeManager::GenerateMacCounter(string vendor, double dist_val)
{
	struct MacCounter ret;
	return ret;
}

PortCounter NodeManager::GeneratePortCounter(string portName, double dist_val)
{
	struct PortCounter ret;
	return ret;
}

vector<Node> NodeManager::GenerateNodesFromProfile(profile *prof, int numNodes)
{
	return vector<Node> {};
}
}
