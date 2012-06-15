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

#include "PersonalityTree.h"

#ifndef NODEMANAGER_H_
#define NODEMANAGER_H_

namespace Nova
{

struct PortCounter
{
	double m_maxCount;
	double m_minCount;
	port m_port;
};

struct MacCounter
{
	double m_maxCount;
	double m_minCount;
	std::string m_ethVendor;
};

struct ProfileCounter
{
	Nova::profile m_profile;
	double m_maxCount;
	double m_minCount;
	std::vector<struct PortCounter> m_portCounters;
	std::vector<struct MacCounter> m_macCounters;
};

class NodeManager
{

public:

	NodeManager(PersonalityTree * persTree);
	~NodeManager();

private:

	void GenerateProfileCounters();
	void GenerateMacCounters();
	void GeneratePortCounters();
	void GenerateNodesFromProfile(profile *prof, int numNodes);

	PersonalityTree *m_persTree;
	int m_nodeCount;
	int m_targetNodeCount;
	std::vector<struct ProfileCounter> m_profileCounters;
	std::vector<Node> m_nodes;
};

}

#endif /*NODEMANAGER_H_*/
