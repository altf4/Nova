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


#ifndef NODEMANAGER_H_
#define NODEMANAGER_H_

#include "PersonalityTree.h"

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

	NodeManager(PersonalityTree *persTree);
	~NodeManager();

	bool SetPersonalityTree(PersonalityTree *persTree);

	PersonalityTree *m_persTree;

private:

	void GenerateProfileCounters();
	MacCounter GenerateMacCounter(std::string vendor, double dist_val);
	PortCounter GeneratePortCounter(std::string portName, double dist_val);

	void RecursiveGenProfileCounter(PersonalityNode *parent);
	std::vector<Node> GenerateNodesFromProfile(profile *prof, int numNodes);

	unsigned int m_nodeCount;
	unsigned int m_targetNodeCount;
	unsigned int m_hostCount;
	std::vector<struct ProfileCounter> m_profileCounters;
	std::vector<Node> m_nodes;

	HoneydConfiguration * m_hdconfig;
};

}

#endif /*NODEMANAGER_H_*/
