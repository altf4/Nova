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
#include <boost/algorithm/string.hpp>

using namespace std;

namespace Nova
{

//. ************ Public Methods ************ .//

// Constructor that assigns the m_persTree and m_hdconfig variables
// to values within the PersonalityTree* that's being passed. It then immediately
// begins generating ProfileCounters.
//		honeydConfig: The HoneydConfiguration to use in the NodeManager
// Note: This function constructs a PersonalityTree from the HoneydConfiguration object passed
NodeManager::NodeManager(HoneydConfiguration *honeydConfig)
{
	if(honeydConfig != NULL)
	{
		m_hdconfig = honeydConfig;
		if(m_hdconfig->LoadAllTemplates())
		{
			GenerateProfileCounters(m_hdconfig->GetProfile("default"));
		}
	}
}

// GenerateNodes does exactly what it sounds like -- creates nodes. Using the
// ProfileCounters in the m_profileCounters variable, it will generate up to
// num_nodes nodes of varying profiles, macs and ports, depending on the calculated
// per-personality distributions.
//  unsigned int num_nodes - ceiling on the amount of nodes to make
// Returns nothing.
void NodeManager::GenerateNodes(unsigned int num_nodes)
{
	vector<Node> nodesToAdd;
	nodesToAdd.clear();

	for(unsigned int i = 0; i < num_nodes;)
	{
		for(unsigned int j = 0; j < m_profileCounters.size(); j++)
		{
			//If we're skipping this profile
			if(m_profileCounters[j].m_count < 0)
			{
				//Update Counter
				m_profileCounters[j].m_count += m_profileCounters[j].m_increment;
			}
			//If we're using this profile
			else
			{
				//Update Counter
				m_profileCounters[j].m_count -= (1 - m_profileCounters[j].m_increment);

				Node curNode;
				//Determine which mac vendor to use
				for(unsigned int k = 0; k < m_profileCounters[j].m_macCounters.size(); k++)
				{
					MacCounter *curCounter = &m_profileCounters[j].m_macCounters[k];
					//If we're skipping this vendor
					if(curCounter->m_count < 0)
					{
						curCounter->m_count += curCounter->m_increment;
					}
					//If we're using this vendor
					else
					{
						curCounter->m_count -= (1 - curCounter->m_increment);
						//Generate unique MAC address and set profile name
						curNode.m_IP = "DHCP";
						curNode.m_MAC = m_hdconfig->GenerateUniqueMACAddress(curCounter->m_ethVendor);
						curNode.m_pfile = m_profileCounters[j].m_profile.m_name;
						curNode.m_name = curNode.m_IP + " - " + curNode.m_MAC;
						m_hdconfig->m_profiles[m_profileCounters[j].m_profile.m_name].m_nodeKeys.push_back(curNode.m_name);

						//Update counters for remaining macs
						for(unsigned int l = k + 1; l < m_profileCounters[j].m_macCounters.size(); l++)
						{
							m_profileCounters[j].m_macCounters[l].m_count += m_profileCounters[j].m_macCounters[l].m_increment;
						}

						break;
					}
				}
				std::vector<PortCounter *> portCounters;
				for(unsigned int index = 0; index < m_profileCounters[j].m_portCounters.size(); index++)
				{
					portCounters.push_back(&m_profileCounters[j].m_portCounters[index]);
				}
				//Determine whether or not to use a port
				unsigned int num_ports = 0, avg_ports = m_profileCounters[j].m_numAvgPorts;

				curNode.m_ports.clear();

				while(!portCounters.empty() && (num_ports < avg_ports))
				{
					unsigned int randIndex = time(NULL) % portCounters.size();
					PortCounter *curCounter = portCounters[randIndex];
					portCounters.erase(portCounters.begin() + randIndex);
					NodeProfile* p = m_hdconfig->GetProfile(curNode.m_pfile);

					//If we're closing the port
					if(curCounter->m_count < 0)
					{
						curCounter->m_count += curCounter->m_increment;

						vector<string> tokens;

						boost::split(tokens, curCounter->m_portName, boost::is_any_of("_"));

						string addPort = tokens[0] + "_" + tokens[1] + "_";

						Port newPort;
						newPort.m_portNum = tokens[0];
						newPort.m_type = tokens[1];

						// ***** TCP ****
						if(!tokens[1].compare("TCP"))
						{
							//Default to reset for TCP unless we have an explicit block for the default action
							if(p->m_tcpAction.compare("block"))
							{
								addPort += "reset";
								newPort.m_behavior = "reset";
								curNode.m_ports.push_back(addPort);
								curNode.m_isPortInherited.push_back(false);
								newPort.m_portName = addPort;

								m_hdconfig->AddPort(newPort);
								continue;
							}
						}
						//UDP or tcp with default to block
						addPort += "block";
						newPort.m_behavior = "block";

						curNode.m_ports.push_back(addPort);
						curNode.m_isPortInherited.push_back(false);
						newPort.m_portName = addPort;

						m_hdconfig->AddPort(newPort);
						continue;
					}

					//If we use the port behavior from the parent profile
					curCounter->m_count -= (1 - curCounter->m_increment);
					curNode.m_ports.push_back(curCounter->m_portName);
					curNode.m_isPortInherited.push_back(true);
					num_ports++;
				}
				while(!portCounters.empty())
				{
					unsigned int randIndex = time(NULL) % portCounters.size();
					PortCounter *curCounter = portCounters[randIndex];
					portCounters.erase(portCounters.begin() + randIndex);
					NodeProfile* p = m_hdconfig->GetProfile(curNode.m_pfile);

					curCounter->m_count += curCounter->m_increment;

					vector<string> tokens;

					boost::split(tokens, curCounter->m_portName, boost::is_any_of("_"));

					string addPort = tokens[0] + "_" + tokens[1] + "_";

					Port newPort;
					newPort.m_portNum = tokens[0];
					newPort.m_type = tokens[1];

					// ***** TCP ****
					if(!tokens[1].compare("TCP"))
					{
						//Default to reset for TCP unless we have an explicit block for the default action
						if(p->m_tcpAction.compare("block"))
						{
							addPort += "reset";
							newPort.m_behavior = "reset";
							newPort.m_portName = addPort;
							m_hdconfig->AddPort(newPort);

							curNode.m_ports.push_back(addPort);
							curNode.m_isPortInherited.push_back(false);
							continue;
						}
					}
					//UDP or tcp with default to block
					addPort += "block";
					newPort.m_behavior = "block";
					newPort.m_portName = addPort;
					m_hdconfig->AddPort(newPort);

					curNode.m_ports.push_back(addPort);
					curNode.m_isPortInherited.push_back(false);
				}
				// Only progress the outermost for loop if we've completely generated a
				// Node from the profile counter; this way, we get a number of nodes equal
				// to num_nodes
				nodesToAdd.push_back(curNode);
				i++;
				//increment the remaining counters since they were 'skipped'
				for(j = (j+1); j < m_profileCounters.size(); j++)
				{
					m_profileCounters[j].m_count += m_profileCounters[j].m_increment;
				}
			}
		}
	}
	m_hdconfig->SaveAllTemplates();
	while(!nodesToAdd.empty())
	{
		m_hdconfig->AddNewNode(nodesToAdd.back());
		nodesToAdd.pop_back();
	}
}

//. ************ Private Methods ************ .//

// GenerateProfileCounters serves as the starting point for RecursiveGenProfileCounter when loading
//		a Honeyd Configuration rather than an nmap scan, used mainly by the UI's for user configuration
// 	NodeProfile *rootProfile: This usually corresponds to the 'Default' NodeProfile and is the top of
//		the profile tree
void NodeManager::GenerateProfileCounters(NodeProfile *rootProfile)
{
	m_hostCount = 0;
	RecursiveGenProfileCounter(rootProfile);
}

// RecursiveGenProfileCounter recurses through the m_persTree member variable and generates
//		randomized nodes from the profiles at the different nodes of the Personality Tree.
//		Creates, populates and pushes a ProfileCounter struct, complete with Mac- and PortCounters
//		for each node in the tree into the m_profileCounters member variable.
//  NodeProfile *profile - pointer to a NodeProfile in the tree structure;
//                         the profile's information is read and generates a ProfileCounter
// Returns nothing.
void NodeManager::RecursiveGenProfileCounter(NodeProfile *profile)
{
	if(m_hdconfig->GetProfile(profile->m_name) == NULL)
	{
		LOG(ERROR, "Couldn't retrieve expected NodeProfile: " + profile->m_name, "");
		return;
	}
	else if((profile->m_generated) && (NumberOfChildren(profile->m_name) == 0))
	{
		struct ProfileCounter pCounter;
		pCounter.m_profile = *m_hdconfig->GetProfile(profile->m_name);
		pCounter.m_increment = profile->m_distribution;

		// XXX I think totalPorts is always going to be 0 here. I stepped through
		// this after seeing that all the profiles that were being created had no ports,
		// and saw that m_numAvgPorts was always 0. I think this arises from the fact that
		// the nodeKeys vector is always going to be empty at this point in execution due to
		// the fact that no nodes have been created yet.

		// The loop below is a good solution for pre-existing profiles, but for a first run of
		// the tool it creates profiles with all blocked ports.
		if(profile->m_nodeKeys.size() > 0)
		{
			int totalPorts = 0;

			for(uint i = 0; i < profile->m_nodeKeys.size(); i++)
			{
				totalPorts += m_hdconfig->GetNode(profile->m_nodeKeys[i])->m_ports.size();
			}

			pCounter.m_numAvgPorts = (uint)((double)totalPorts)/((double)profile->m_nodeKeys.size());
		}
		else
		{
			// I think this is an acceptable sequence for a first run, the only problem being that
			// m_ports has no distribution value (m_ports[i].second.second == 0)
			uint testCounter = 0;
			uint fallBackAmount = 0;

			for(uint i = 0; i < profile->m_ports.size(); i++)
			{
				if(profile->m_ports[i].second.second == 100)
				{
					testCounter++;
				}
				else
				{
					fallBackAmount++;
				}
			}

			if(testCounter == 0)
			{
				pCounter.m_numAvgPorts = ((fallBackAmount / 2) + 1);
			}
			else
			{
				pCounter.m_numAvgPorts = testCounter;
			}
		}

		pCounter.m_count = 0;

		for(unsigned int i = 0; i < profile->m_ethernetVendors.size(); i++)
		{
			pCounter.m_macCounters.push_back(GenerateMacCounter(profile->m_ethernetVendors[i].first, profile->m_ethernetVendors[i].second));
		}
		for(unsigned int i = 0; i < profile->m_ports.size(); i++)
		{
			pCounter.m_portCounters.push_back(GeneratePortCounter(profile->m_ports[i].first, profile->m_ports[i].second.second));
		}
		m_profileCounters.push_back(pCounter);
	}
	for(ProfileTable::iterator it = m_hdconfig->m_profiles.begin(); it != m_hdconfig->m_profiles.end(); it++)
	{
		if(!it->second.m_parentProfile.compare(profile->m_name) && (it->second.m_generated))
		{
			RecursiveGenProfileCounter(&m_hdconfig->m_profiles[it->first]);
		}
	}
}

// GenerateMacCounter takes in a vendor string and its corresponding distribution value
// 		and populates a MacCounter struct with these values. Used in RecursiveGenProfileCounter
//		to add MacCounters to athe ProfileCounter's m_macCounters vector.
//  const std::string &vendor - const reference to a string containing a MAC Vendor
//  const double dist_val - a const double that contains the calculated distribution value
//                          in the current node's m_vendor_dist vector.
// Returns a MacCounter struct.
MacCounter NodeManager::GenerateMacCounter(const string &vendor, const double dist_val)
{
	struct MacCounter ret;
	ret.m_ethVendor = vendor;
	ret.m_increment = dist_val/100;
	ret.m_count = 0;
	return ret;
}

// GeneratePortCounter takes in a port name string and its corresponding distribution value
//		and populates a PortCounter struct with these values. Used in RecursiveGenProfileCounter
//		to add PortCounters to athe ProfileCounter's m_portCounters vector.
//  const std::string &portName - const reference to a string containing a port name of the form
//                          NUM_PROTOCOL_(open || SCRIPTNAME)
//  const double dist_val - a const double that contains the calculated distribution value
//                          in the current node's m_ports_dist vector.
// Returns a PortCounter struct.
PortCounter NodeManager::GeneratePortCounter(const string &portName, const double dist_val)
{
	struct PortCounter ret;
	ret.m_portName = portName;
	ret.m_increment = dist_val/100;
	ret.m_count = 0;
	return ret;
}

uint NodeManager::NumberOfChildren(const string &profileName)
{
	uint ret = 0;

	for(ProfileTable::iterator it = m_hdconfig->m_profiles.begin(); it != m_hdconfig->m_profiles.end(); it++)
	{
		if(!it->second.m_parentProfile.compare(profileName))
		{
			ret++;
		}
	}

	return ret;
}

}
