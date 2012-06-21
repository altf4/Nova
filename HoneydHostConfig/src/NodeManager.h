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
	double m_increment;
	double m_count;
	std::string m_portName;
};

struct MacCounter
{
	double m_increment;
	double m_count;
	std::string m_ethVendor;
};

struct ProfileCounter
{
	Nova::NodeProfile m_profile;
	double m_increment;
	double m_count;
	std::vector<struct PortCounter> m_portCounters;
	std::vector<struct MacCounter> m_macCounters;
};

class NodeManager
{

public:

	NodeManager(PersonalityTree *persTree);

	bool SetPersonalityTree(PersonalityTree *persTree);

	void GenerateNodes(unsigned int num_nodes);

	PersonalityTree *m_persTree;

private:

	void GenerateProfileCounters();
	MacCounter GenerateMacCounter(const std::string &vendor, const double dist_val);
	PortCounter GeneratePortCounter(const std::string &portName, const double dist_val);

	void RecursiveGenProfileCounter(PersonalityNode *parent);

	unsigned int m_nodeCount;
	unsigned int m_targetNodeCount;
	unsigned int m_hostCount;
	std::vector<struct ProfileCounter> m_profileCounters;
	std::vector<Node> m_nodes;

	HoneydConfiguration * m_hdconfig;
};

}

#endif /*NODEMANAGER_H_*/
