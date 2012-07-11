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
// Description : The NodeManager class takes all of the calculations for
//               distributions of the PersonalityNodes found so far and
//               generates randomized nodes.
//============================================================================


#ifndef NODEMANAGER_H_
#define NODEMANAGER_H_

#include "PersonalityTree.h"

namespace Nova
{
// The following structs are matched to a specific port, mac vendor or profile.
// Each counter keeps track of whether the value they are bound to has been
// used recently or not, and if not, if it should be used again at the time that
// node generation reaches the counter.

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
	uint m_numAvgPorts;
	std::vector<struct PortCounter> m_portCounters;
	std::vector<struct MacCounter> m_macCounters;
};

class NodeManager
{

public:

	// Constructor that assigns the m_persTree and m_hdconfig variables
	// to values within the PersonalityTree* that's being passed. It then immediately
	// begins generating ProfileCounters.
	//		persTree: The Personality Tree to use in the NodeManager
	// Note: This function loads the same HoneydConfiguration nova would get if loaded from the current templates.
	NodeManager(PersonalityTree *persTree);

	// Constructor that assigns the m_persTree and m_hdconfig variables
	// to values within the PersonalityTree* that's being passed. It then immediately
	// begins generating ProfileCounters.
	//		honeydConfig: The HoneydConfiguration to use in the NodeManager
	// Note: This function constructs a PersonalityTree from the HoneydConfiguration object passed
	NodeManager(HoneydConfiguration *honeydConfig);

	// SetPersonalityTree is used to change the target PersonalityTree from the
	// current m_persTree to the PersonalityTree* argument. If the pointer passed
	// is null, returns false; else, perform the same actions as the constructor and
	// return true. Can also be used in the case that you wish to instantiate the object
	// without explicitly declaring a PersonalityTree; if the constructor receives a NULL
	// pointer as an argument, it won't do anything, so you have to set the tree yourself
	// using this method.
	//  PersonalityTree *persTree - PersonalityTree pointer to shift m_persTree to.
	// Returns a bool indicating success or failure.
	bool SetPersonalityTree(PersonalityTree *persTree);

	// GenerateNodes does exactly what it sounds like -- creates nodes. Using the
	// ProfileCounters in the m_profileCounters variable, it will generate up to
	// num_nodes nodes of varying profiles, macs and ports, depending on the calculated
	// per-personality distributions.
	//  unsigned int num_nodes - ceiling on the amount of nodes to make
	// Returns nothing.
	void GenerateNodes(unsigned int num_nodes);

	PersonalityTree *m_persTree;

private:

	// GenerateProfileCounters serves as the starting point for RecursiveGenProfileCounter.
	// Returns nothing, takes no arguments.
	void GenerateProfileCounters();

	// GenerateMacCounter takes in a vendor string and its corresponding distribution value
	// and populates a MacCounter struct with these values. Used in RecursiveGenProfileCounter
	// to add MacCounters to athe ProfileCounter's m_macCounters vector.
	//  const std::string &vendor - const reference to a string containing a MAC Vendor
	//  const double dist_val - a const double that contains the calculated distribution value
	//                          in the current node's m_vendor_dist vector.
	// Returns a MacCounter struct.
	MacCounter GenerateMacCounter(const std::string &vendor, const double dist_val);

	// GeneratePortCounter takes in a port name string and its corresponding distribution value
	// and populates a PortCounter struct with these values. Used in RecursiveGenProfileCounter
	// to add PortCounters to athe ProfileCounter's m_portCounters vector.
	//  const std::string &portName - const reference to a string containing a port name of the form
	//                          NUM_PROTOCOL_(open || SCRIPTNAME)
	//  const double dist_val - a const double that contains the calculated distribution value
	//                          in the current node's m_ports_dist vector.
	// Returns a PortCounter struct.
	PortCounter GeneratePortCounter(const std::string &portName, const double dist_val);

	// RecursiveGenProfileCounter recurses through the m_persTree member variable and generates
	// randomized nodes from the profiles at the different nodes of the Personality Tree.
	// Creates, populates and pushes a ProfileCounter struct, complete with Mac- and PortCounters
	// for each node in the tree into the m_profileCounters member variable.
	//  const PersonalityNode &parent - const reference to a PersonalityNode of the tree;
	//                                  the node's information is read and placed into a complete
	//                                  ProfileCounter struct.
	// Returns nothing.
	void RecursiveGenProfileCounter(const PersonalityNode &parent);

	unsigned int m_nodeCount;
	unsigned int m_targetNodeCount;
	unsigned int m_hostCount;
	std::vector<struct ProfileCounter> m_profileCounters;
	std::vector<Node> m_nodes;

	HoneydConfiguration * m_hdconfig;
};

}

#endif /*NODEMANAGER_H_*/
