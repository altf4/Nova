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
#include "../Logger.h"

#include <math.h>
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
	}
	else
	{
		m_hdconfig = new HoneydConfiguration();
	}
	if(!m_hdconfig->LoadAllTemplates())
	{
		LOG(ERROR, "Unable to load Honeyd Configuration templates!", "");
	}
}

//This function adds or subtracts nodes from the parent profile and it's children until the desired number is matched
bool NodeManager::SetNumNodesOnProfileTree(NodeProfile *rootProfile, int num_nodes)
{
	m_profileCounters.clear();
	if(rootProfile == NULL)
	{
		LOG(ERROR, "Passed pointer to NodeProfile parameter is NULL!", "");
		return false;
	}
	if(!GenerateProfileCounters(rootProfile))
	{
		LOG(ERROR, "Unable to generate profile counters.", "");
		return false;
	}
	vector<string> profList;
	profList.push_back(rootProfile->m_name);
	int totalNodes = rootProfile->m_nodeKeys.size();
	for(uint i = 0; i < profList.size(); i++)
	{
		for(ProfileTable::iterator it = m_hdconfig->m_profiles.begin(); it != m_hdconfig->m_profiles.end(); it++)
		{
			if(!it->second.m_parentProfile.compare(profList[i]) && it->second.m_generated)
			{
				profList.push_back(it->first);
				totalNodes += it->second.m_nodeKeys.size();
			}
		}
	}

	//Delete Nodes Case
	if(totalNodes > num_nodes)
	{
		if(!RemoveNodes(totalNodes - num_nodes))
		{
			LOG(ERROR, "Unable to remove nodes.", "");
			return false;
		}
	}
	//Add Nodes Case
	else if(totalNodes < num_nodes)
	{
		if(!GenerateNodes(num_nodes - totalNodes))
		{
			LOG(ERROR, "Unable to generate nodes.", "");
			return false;
		}
	}
	//Redistribute Ports and MAC vendors if needed
	if(!AdjustNodesToTargetDistributions())
	{
		LOG(ERROR, "Unable to auto adjust nodes to target distributions.", "");
		return false;
	}
	return true;
}

//This function adds or subtracts nodes directly from the target profile until the desired number is matched
bool NodeManager::SetNumNodesOnProfile(NodeProfile *targetProfile, int num_nodes)
{
	int totalNodes = targetProfile->m_nodeKeys.size();
	struct ProfileCounter pCounter;
	pCounter.m_profile = *m_hdconfig->GetProfile(targetProfile->m_name);
	pCounter.m_increment = targetProfile->m_distribution;

	double avgPorts = 0;
	for(uint i = 0; i < targetProfile->m_ports.size(); i++)
	{
		avgPorts += (targetProfile->m_ports[i].second.second/100);
	}
	pCounter.m_numAvgPorts = (uint)::floor(avgPorts + 0.5);
	pCounter.m_count = 0;

	for(unsigned int i = 0; i < targetProfile->m_ethernetVendors.size(); i++)
	{
		pCounter.m_macCounters.push_back(GenerateMacCounter(targetProfile->m_ethernetVendors[i].first, targetProfile->m_ethernetVendors[i].second));
	}
	for(unsigned int i = 0; i < targetProfile->m_ports.size(); i++)
	{
		pCounter.m_portCounters.push_back(GeneratePortCounter(targetProfile->m_ports[i].first, targetProfile->m_ports[i].second.second));
	}
	m_profileCounters.push_back(pCounter);

	//Delete Nodes Case
	if(totalNodes > num_nodes)
	{
		if(!RemoveNodes(totalNodes - num_nodes))
		{
			LOG(ERROR, "Unable to remove nodes.", "");
			return false;
		}
	}
	//Add Nodes Case
	else if(totalNodes < num_nodes)
	{
		if(!GenerateNodes(num_nodes - totalNodes))
		{
			LOG(ERROR, "Unable to generate nodes.", "");
			return false;
		}
	}
	//Redistribute Ports and MAC vendors if needed
	if(!AdjustNodesToTargetDistributions())
	{
		LOG(ERROR, "Unable to auto adjust nodes to target distributions.", "");
		return false;
	}
	return true;
}

//. ************ Private Methods ************ .//

// GenerateNodes does exactly what it sounds like -- creates nodes. Using the
// ProfileCounters in the m_profileCounters variable, it will generate up to
// num_nodes nodes of varying profiles, macs and ports, depending on the calculated
// per-personality distributions.
//  unsigned int num_nodes - ceiling on the amount of nodes to make
// Returns nothing.
bool NodeManager::GenerateNodes(int num_nodes)
{
	vector<Node> nodesToAdd;
	nodesToAdd.clear();
	if(!GetCurrentCount())
	{
		LOG(ERROR, "Unable to get the current status of the Honeyd configuration.", "");
		return false;
	}
	for(int i = 0; i < num_nodes;)
	{
		for(unsigned int j = 0; (j < m_profileCounters.size()) && (i < num_nodes); j++)
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
				curNode.m_IP = "DHCP";
				curNode.m_name = curNode.m_IP + " - " + curNode.m_MAC;
				curNode.m_pfile = m_profileCounters[j].m_profile.m_name;

				//Determine which mac vendor to use
				for(unsigned int k = 0; k < m_profileCounters[j].m_macCounters.size(); k++)
				{
					MacCounter *curCounter = &m_profileCounters[j].m_macCounters[k];
					if(curCounter == NULL)
					{
						LOG(ERROR, "Unable to retrieve expected MacCounter.", "");
						return false;
					}
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
						curNode.m_MAC = m_hdconfig->GenerateUniqueMACAddress(curCounter->m_ethVendor);
						curNode.m_name = curNode.m_IP + " - " + curNode.m_MAC;
						curNode.m_pfile = m_profileCounters[j].m_profile.m_name;
						curNode.m_enabled = true;
						curNode.m_interface = Config::Inst()->GetInterface(0);
						curNode.m_sub = curNode.m_interface;
						curNode.m_realIP = 0;

						//Update counters for remaining macs
						for(unsigned int l = k + 1; l < m_profileCounters[j].m_macCounters.size(); l++)
						{
							m_profileCounters[j].m_macCounters[l].m_count += m_profileCounters[j].m_macCounters[l].m_increment;
						}
						break;
					}
				}

				if (curNode.m_MAC == "")
				{
					LOG(ERROR, "TODO: Fix this bug. Skipping generation of node with no m_MAC", "");
					continue;
				}

				std::vector<PortCounter *> portCounters;
				for(unsigned int index = 0; index < m_profileCounters[j].m_portCounters.size(); index++)
				{
					portCounters.push_back(&m_profileCounters[j].m_portCounters[index]);
				}
				//Determine whether or not to use a port
				unsigned int avg_ports = m_profileCounters[j].m_numAvgPorts;
				int num_ports = 0;

				curNode.m_ports.clear();
				int maxPorts  = (int)(avg_ports*(log10(avg_ports)));
				while(!portCounters.empty() && (num_ports < maxPorts))
				{
					unsigned int randIndex = time(NULL) % portCounters.size();
					PortCounter *curCounter = portCounters[randIndex];
					if(curCounter == NULL)
					{
						LOG(ERROR, "Unable to retrieve expected PortCounter.", "");
						return false;
					}

					portCounters.erase(portCounters.begin() + randIndex);
					NodeProfile* p = m_hdconfig->GetProfile(curNode.m_pfile);
					if(p == NULL)
					{
						LOG(ERROR, "Unable to find expected NodeProfile '" + curNode.m_pfile + "'.", "");
					}

					//If we're closing the port
					if(curCounter->m_count < 0)
					{
						curCounter->m_count += curCounter->m_increment;

						vector<string> tokens;

						boost::split(tokens, curCounter->m_portName, boost::is_any_of("_"));
						if (tokens.size() < 2)
						{
							LOG(ERROR, "Unable to parse port name " + curCounter->m_portName, "");
						}

						string addPort = tokens[0] + "_" + tokens[1] + "_";

						Port newPort;
						newPort.m_portNum = tokens[0];
						newPort.m_type = tokens[1];

						// ***** TCP ****
						if(!newPort.m_type.compare("TCP"))
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
					if(curCounter == NULL)
					{
						LOG(ERROR, "Unable to retrieve expected PortCounter.", "");
						return false;
					}
					portCounters.erase(portCounters.begin() + randIndex);
					NodeProfile* p = m_hdconfig->GetProfile(curNode.m_pfile);
					if(p == NULL)
					{
						LOG(ERROR, "Unable to retrieve expected NodeProfile '" + curNode.m_pfile + "'.", "");
						return false;
					}

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
				for(unsigned int k = (j+1); k < m_profileCounters.size(); k++)
				{
					m_profileCounters[k].m_count += m_profileCounters[k].m_increment;
				}
			}
		}
	}
	if(!m_hdconfig->SaveAllTemplates())
	{
		LOG(ERROR, "Unable to save Honeyd configuration templates.", "");
		return false;
	}
	while(!nodesToAdd.empty())
	{
		m_hdconfig->AddPreGeneratedNode(nodesToAdd.back());
		nodesToAdd.pop_back();
	}
	if(!m_hdconfig->SaveAllTemplates())
	{
		LOG(ERROR, "Unable to save Honeyd configuration templates.", "");
		return false;
	}
	return true;
}

bool NodeManager::RemoveNodes(int num_nodes)
{
	vector<Node> nodesToDelete;
	for(int i = 0; i < num_nodes; i++)
	{
		if(m_profileCounters.empty())
		{
			LOG(ERROR, "Unable to remove nodes because there are no profile counters in the NodeManager object.", "");
			return false;
		}

		unsigned int indexOfCounter = 0;
		double lowestCount = m_profileCounters[0].m_count;

		//Select profile to remove from
		for(unsigned int j = 1; j < m_profileCounters.size(); j++)
		{
			if(m_profileCounters[j].m_count < lowestCount)
			{
				lowestCount = m_profileCounters[j].m_count;
				indexOfCounter = j;
			}
		}
		ProfileCounter *pCounter = &m_profileCounters[indexOfCounter];
		if(pCounter == NULL)
		{
			LOG(ERROR, "Unable to retrieve expected ProfileCounter.", "");
			return false;
		}

		//If we have no macCounters
		if(pCounter->m_macCounters.empty())
		{
			for(NodeTable::iterator it = m_hdconfig->m_nodes.begin(); it != m_hdconfig->m_nodes.end(); it++)
			{
				//If this node uses the profile
				if(!it->second.m_pfile.compare(pCounter->m_profile.m_name))
				{
					nodesToDelete.push_back(it->second);
					pCounter->m_count += (1- pCounter->m_increment);
					break;
				}
			}

			//Update remaining profile counters
			for(unsigned int j = 0; j < m_profileCounters.size(); j++)
			{
				//If this isn't the profile we removed from
				if(m_profileCounters[j].m_profile.m_name.compare(pCounter->m_profile.m_name))
				{
					m_profileCounters[j].m_count -= m_profileCounters[j].m_increment;
				}
			}

			Node *lastNode = &nodesToDelete.back();
			if(lastNode == NULL)
			{
				LOG(ERROR, "Unable to retrieve expected Node for deletion.", "");
				return false;
			}

			//Track ports
			for(unsigned int j = 0; j < lastNode->m_ports.size(); j++)
			{
				Port curPort = m_hdconfig->GetPort(lastNode->m_ports[j]);
				if(!curPort.m_portName.compare(""))
				{
					LOG(ERROR, "Unable to find expected Port " + lastNode->m_ports[j] + ".", "");
					return false;
				}
				for(unsigned int k = 0; k < pCounter->m_portCounters.size(); k++)
				{
					Port comparePort = m_hdconfig->GetPort(pCounter->m_portCounters[k].m_portName);
					if(!comparePort.m_portName.compare(""))
					{
						LOG(ERROR, "Unable to find expected Port " + pCounter->m_portCounters[k].m_portName + ".", "");
						return false;
					}
					//If both the port number and protocol are the same
					if((!comparePort.m_portNum.compare(curPort.m_portNum))
					&& (!comparePort.m_type.compare(curPort.m_type)))
					{
						//If the port is the same
						if(!comparePort.m_portName.compare(curPort.m_portName))
						{
							pCounter->m_portCounters[k].m_count += (1 - pCounter->m_portCounters[k].m_increment);
						}
						else
						{
							pCounter->m_portCounters[k].m_count -= pCounter->m_portCounters[k].m_increment;
						}
					}
				}
			}
			continue;
		}
		//If we have some macCounters
		else
		{
			lowestCount = pCounter->m_macCounters[0].m_count;
			indexOfCounter = 0;
			//Select MAC to remove
			for(unsigned int j = 1; j < pCounter->m_macCounters.size(); j++)
			{
				//If we don't use this ethernet vendor
				if(lowestCount < pCounter->m_macCounters[j].m_count)
				{
					lowestCount = pCounter->m_macCounters[j].m_count;
					indexOfCounter = j;
				}
			}
			for(NodeTable::iterator it = m_hdconfig->m_nodes.begin(); it != m_hdconfig->m_nodes.end(); it++)
			{
				//If this node uses the profile
				if(!it->second.m_pfile.compare(pCounter->m_profile.m_name))
				{
					//If we match the mac vendor with the node
					uint macPrefix = m_hdconfig->m_macAddresses.AtoMACPrefix(it->second.m_MAC);
					string macVendor = m_hdconfig->m_macAddresses.LookupVendor(macPrefix);
					if(!macVendor.compare(pCounter->m_macCounters[indexOfCounter].m_ethVendor))
					{
						nodesToDelete.push_back(it->second);
						pCounter->m_count += (1 - pCounter->m_increment);
						pCounter->m_macCounters[indexOfCounter].m_count += (1 - pCounter->m_macCounters[indexOfCounter].m_increment);
						break;
					}
					else if(!macVendor.compare(""))
					{
						LOG(ERROR, "Unable to lookup mac vendor for ethernet address " + it->second.m_MAC, "");
						return false;
					}
				}
			}

			//Update remaining profile counters
			for(unsigned int j = 0; j < m_profileCounters.size(); j++)
			{
				//If this isn't the profile we removed from
				if(m_profileCounters[j].m_profile.m_name.compare(pCounter->m_profile.m_name))
				{
					m_profileCounters[j].m_count -= m_profileCounters[j].m_increment;
				}
			}

			//Update remaining mac counters
			for(unsigned int j = 0; j < pCounter->m_macCounters.size(); j++)
			{
				if(j != indexOfCounter)
				{
					pCounter->m_macCounters[j].m_count -= pCounter->m_macCounters[j].m_increment;
				}
			}

			Node *lastNode = &nodesToDelete.back();
			if(lastNode == NULL)
			{
				LOG(ERROR, "Unable to retrieve expected Node for deletion.", "");
				continue;
			}
			//Track ports
			for(unsigned int j = 0; j < lastNode->m_ports.size(); j++)
			{
				Port curPort = m_hdconfig->GetPort(lastNode->m_ports[j]);
				if(!curPort.m_portName.compare(""))
				{
					LOG(ERROR, "Unable to find expected Port " + lastNode->m_ports[j] + ".", "");
					continue;
				}
				for(unsigned int k = 0; k < pCounter->m_portCounters.size(); k++)
				{
					Port comparePort = m_hdconfig->GetPort(pCounter->m_portCounters[k].m_portName);
					if(!comparePort.m_portName.compare(""))
					{
						LOG(ERROR, "Unable to find expected Port " + pCounter->m_portCounters[k].m_portName + ".", "");
						continue;
					}
					//If both the port number and protocol are the same
					if((!comparePort.m_portNum.compare(curPort.m_portNum))
					&& (!comparePort.m_type.compare(curPort.m_type)))
					{
						//If the port is the same
						if(!comparePort.m_portName.compare(curPort.m_portName))
						{
							pCounter->m_portCounters[k].m_count += (1 - pCounter->m_portCounters[k].m_increment);
						}
						else
						{
							pCounter->m_portCounters[k].m_count -= pCounter->m_portCounters[k].m_increment;
						}
					}
				}
			}
			continue;
		}
	}
	return true;
}

bool NodeManager::GetCurrentCount()
{
	for(NodeTable::iterator it = m_hdconfig->m_nodes.begin(); it != m_hdconfig->m_nodes.end(); it++)
	{
		for(unsigned int j = 0; j < m_profileCounters.size(); j++)
		{
			//If the node doesn't use this counter, increase the count, decrement the one it uses
			if(it->second.m_pfile.compare(m_profileCounters[j].m_profile.m_name))
			{
				m_profileCounters[j].m_count += m_profileCounters[j].m_increment;
			}
			else
			{
				m_profileCounters[j].m_count -= (1 - m_profileCounters[j].m_increment);
				uint macRawPrefix = m_hdconfig->m_macAddresses.AtoMACPrefix(it->second.m_MAC);
				string ethVendor = m_hdconfig->m_macAddresses.LookupVendor(macRawPrefix);
				if(!ethVendor.compare(""))
				{
					LOG(ERROR, "Unable to lookup mac vendor for ethernet address " + it->second.m_MAC, "");
					return false;
				}
				//Determine which mac vendor to usecurCounter
				for(unsigned int k = 0; k < m_profileCounters[j].m_macCounters.size(); k++)
				{
					//If we don't use this ethernet vendor
					if(m_profileCounters[j].m_macCounters[k].m_ethVendor.compare(ethVendor))
					{
						m_profileCounters[j].m_macCounters[k].m_count -= (1 - m_profileCounters[j].m_macCounters[k].m_increment);
					}
					else
					{
						m_profileCounters[j].m_macCounters[k].m_count += m_profileCounters[j].m_macCounters[k].m_increment;
					}
				}
				for(unsigned int k = 0; k< m_profileCounters[j].m_portCounters.size(); k++)
				{
					bool portFound = false;
					for(unsigned int m = 0; m < it->second.m_ports.size(); m++)
					{
						//If this port exists for the node
						if(!it->second.m_ports[m].compare(m_profileCounters[j].m_portCounters[k].m_portName))
						{
							m_profileCounters[j].m_portCounters[k].m_count -= (1 - m_profileCounters[j].m_portCounters[k].m_increment);
							portFound = true;
							break;
						}
					}
					if(!portFound)
					{
						m_profileCounters[j].m_portCounters[k].m_count += m_profileCounters[j].m_portCounters[k].m_increment;
					}
				}
			}
		}
	}
	return true;
}

bool NodeManager::AdjustNodesToTargetDistributions()
{
	if(!GenerateProfileCounters(&m_hdconfig->m_profiles["default"]))
	{
		LOG(ERROR, "Unable to generate profile counters using the 'default' profile.", "");
		return false;
	}
	if(!GetCurrentCount())
	{
		LOG(ERROR, "Unable to get the current status of the Honeyd Configuration.", "");
		return false;
	}

	int targetNodeCount = 0;
	for(unsigned int i = 0; i < m_profileCounters.size(); i++)
	{
		targetNodeCount += m_profileCounters[i].m_profile.m_nodeKeys.size();
	}
	if(!AdjustProfiles(targetNodeCount))
	{
		LOG(ERROR, "Unable to adjust Honeyd node profiles.", "");
		return false;
	}


	//Shift any macs or ports that are needed
	for(unsigned int i = 0; i < m_profileCounters.size(); i++)
	{
		ProfileCounter *curCounter = &m_profileCounters[i];
		if(curCounter == NULL)
		{
			LOG(ERROR, "Unable to retrieve valid profile counter.", "");
			return false;
		}
		if(!AdjustMACVendors(curCounter))
		{
			LOG(ERROR, "Unable to adjust Honeyd node MAC Vendors.", "");
			return false;
		}
		if(!AdjustPortsOnNodes(curCounter))
		{
			LOG(ERROR, "Unable to adjust Honeyd node MAC Vendors.", "");
			return false;
		}
	}
	return true;
}

bool NodeManager::AdjustProfiles(int targetNodeCount)
{
	vector<ProfileCounter *> underPopulatedProfiles;
	vector<ProfileCounter *> overPopulatedProfiles;

	for(unsigned int i = 0; i < m_profileCounters.size(); i++)
	{
		//If this profile more nodes than expected
		if(m_profileCounters[i].m_count < (1 - m_profileCounters[i].m_increment))
		{
			if(!m_profileCounters[i].m_profile.m_nodeKeys.size())
			{
				continue;
			}
			unsigned int j = 0;
			for(; j < overPopulatedProfiles.size(); j++)
			{
				if(overPopulatedProfiles[j]->m_count > m_profileCounters[i].m_count)
				{
					break;
				}
			}
			overPopulatedProfiles.insert(overPopulatedProfiles.begin() + j, &m_profileCounters[i]);
		}
		//If this profile fewer nodes than expected
		else if(m_profileCounters[i].m_count > m_profileCounters[i].m_increment)
		{
			double expectedNodes = m_profileCounters[i].m_increment * targetNodeCount;
			if(expectedNodes < 1)
			{
				continue;
			}
			unsigned int j = 0;
			for(; j < underPopulatedProfiles.size(); j++)
			{
				if(underPopulatedProfiles[j]->m_count < m_profileCounters[i].m_count)
				{
					break;
				}
			}
			underPopulatedProfiles.insert(underPopulatedProfiles.begin() + j, &m_profileCounters[i]);
		}
	}

	// *** Move Nodes
	//If we have potential candidates to transfer nodes to and from
	while(!underPopulatedProfiles.empty() && !overPopulatedProfiles.empty())
	{
		ProfileCounter *lowCounter = underPopulatedProfiles.back();
		ProfileCounter *highCounter = overPopulatedProfiles.back();
		underPopulatedProfiles.pop_back();
		overPopulatedProfiles.pop_back();

		//Grab a pointer to the real profile for modification
		NodeProfile *lowProf = &m_hdconfig->m_profiles[lowCounter->m_profile.m_name];
		if(lowProf == NULL)
		{
			LOG(ERROR, "Unable to retrieve expected profile '" +lowCounter->m_profile.m_name + "'.", "");
			return false;
		}
		NodeProfile *highProf = &m_hdconfig->m_profiles[highCounter->m_profile.m_name];
		if(highProf == NULL)
		{
			LOG(ERROR, "Unable to retrieve expected profile '" + highCounter->m_profile.m_name + "'.", "");
			return false;
		}
		//If we have at least one eth vendor
		if(!highCounter->m_macCounters.empty() && !lowCounter->m_macCounters.empty())
		{
			//Get most populated MAC on over populated prof
			int indexOfHighVendor = 0;
			for(unsigned int i = 1; i < highCounter->m_macCounters.size(); i++)
			{
				if(highCounter->m_macCounters[i].m_count < highCounter->m_macCounters[indexOfHighVendor].m_count)
				{
					indexOfHighVendor = i;
				}
			}

			//Get least populated MAC on under populated prof
			int indexOfLowVendor = 0;
			for(unsigned int i = 1; i < lowCounter->m_macCounters.size(); i++)
			{
				if(lowCounter->m_macCounters[i].m_count > lowCounter->m_macCounters[indexOfLowVendor].m_count)
				{
					indexOfLowVendor = i;
				}
			}

			//Get the vendor strings
			string lowVendor = lowCounter->m_macCounters[indexOfLowVendor].m_ethVendor;
			string highVendor = highCounter->m_macCounters[indexOfHighVendor].m_ethVendor;

			//Find the node to remove
			Node *curNode = NULL;
			for(unsigned int i = 0; i < highProf->m_nodeKeys.size(); i++)
			{
				curNode = &m_hdconfig->m_nodes[highProf->m_nodeKeys[i]];
				if(curNode == NULL)
				{
					LOG(ERROR, "Unable to retrieve expected node in node profile '" + highProf->m_nodeKeys[i] + "'.", "");
					return false;
				}

				//Lookup and try to match the mac vendor
				uint macPrefix = m_hdconfig->m_macAddresses.AtoMACPrefix(curNode->m_MAC);
				string nodeVendor = m_hdconfig->m_macAddresses.LookupVendor(macPrefix);

				if(!highVendor.compare(nodeVendor))
				{
					//Change the profile
					curNode->m_pfile = lowProf->m_name;
					highCounter->m_count++;
					lowCounter->m_count--;

					//Generate a new MAC
					curNode->m_MAC = m_hdconfig->m_macAddresses.GenerateRandomMAC(lowVendor);

					//Adjust the counters for the high mac vendor
					highCounter->m_macCounters[indexOfHighVendor].m_count += (1 - highCounter->m_macCounters[indexOfHighVendor].m_increment);
					for(unsigned int j = 0; j < highCounter->m_macCounters.size(); j++)
					{
						//If not the selected mac for the high profile
						if(highCounter->m_macCounters[j].m_ethVendor.compare(highCounter->m_macCounters[indexOfHighVendor].m_ethVendor))
						{
							highCounter->m_macCounters[j].m_count -= (1 - highCounter->m_macCounters[j].m_increment);
						}
					}
					//Adjust the counters for the low mac vendor
					lowCounter->m_macCounters[indexOfLowVendor].m_count -= (1 - lowCounter->m_macCounters[indexOfLowVendor].m_increment);
					for(unsigned int j = 0; j < lowCounter->m_macCounters.size(); j++)
					{
						//If not the selected mac for the low profile
						if(lowCounter->m_macCounters[j].m_ethVendor.compare(lowCounter->m_macCounters[indexOfLowVendor].m_ethVendor))
						{
							lowCounter->m_macCounters[j].m_count += lowCounter->m_macCounters[j].m_increment;
						}
					}

					//Recalculate the ports
					for(unsigned int j = 0; j < lowCounter->m_portCounters.size(); j++)
					{
						Port lowPort = m_hdconfig->GetPort(lowCounter->m_portCounters[j].m_portName);
						if(!lowPort.m_portName.compare(""))
						{
							LOG(ERROR, "Unable to retrieve expected port '" + lowCounter->m_portCounters[j].m_portName + "'.", "");
							return false;
						}
						for(unsigned int k = 0; k < curNode->m_ports.size(); k++)
						{
							Port curPort = m_hdconfig->GetPort(curNode->m_ports[k]);
							if(!curPort.m_portName.compare(""))
							{
								LOG(ERROR, "Unable to retrieve expected port '" + curNode->m_ports[k] + "'.", "");
								return false;
							}

							//If the current node port is the same protocol and number as the counter
							if((!curPort.m_type.compare(lowPort.m_type)) && (!curPort.m_portNum.compare(lowPort.m_portNum)))
							{
								//If the node used this port
								if(!curPort.m_portName.compare(curNode->m_ports[k]))
								{
									lowCounter->m_portCounters[j].m_count += (1 - lowCounter->m_portCounters[j].m_increment);
								}
								//Else if the node closed this port
								else
								{
									lowCounter->m_portCounters[j].m_count -= lowCounter->m_portCounters[j].m_increment;
								}
							}
						}
					}
					curNode->m_ports.clear();
					curNode->m_isPortInherited.clear();
					for(unsigned int j = 0; j < highCounter->m_portCounters.size(); j++)
					{
						Port curPort = m_hdconfig->GetPort(highCounter->m_portCounters[j].m_portName);
						if(!curPort.m_portName.compare(""))
						{
							LOG(ERROR, "Unable to retrieve expected port '" + highCounter->m_portCounters[j].m_portName + "'.", "");
							return false;
						}
						//If we're adding the port
						if(highCounter->m_portCounters[j].m_count > 0)
						{
							highCounter->m_portCounters[j].m_count -= (1 - highCounter->m_portCounters[j].m_increment);
							curNode->m_ports.push_back(curPort.m_portName);
							curNode->m_isPortInherited.push_back(true);
						}
						else
						{
							highCounter->m_portCounters[j].m_count += highCounter->m_portCounters[j].m_increment;
							string portString = curPort.m_portNum + "_" + curPort.m_type + "_";
							string portAction = "block";

							NodeProfile *curNodeProf = &m_hdconfig->m_profiles[curNode->m_pfile];
							if((!curPort.m_type.compare("TCP")) && (!curNodeProf->m_tcpAction.compare("reset")))
							{
								portAction = "reset";
							}
							if(!m_hdconfig->GetPort(portString+portAction).m_portName.compare(""))
							{
								Port newPort = curPort;
								curPort.m_portName = portString+portAction;
								curPort.m_behavior = portAction;
								m_hdconfig->AddPort(newPort);
							}
							curNode->m_ports.push_back(portString+portAction);
							curNode->m_isPortInherited.push_back(false);
						}
					}
				}
			}
		}
		//If for some reason we don't have an eth vendor
		else if(!lowCounter->m_macCounters.empty())
		{
			LOG(ERROR, "Unable to manage the MAC addresses for generated nodes on the '" + highProf->m_name + "' profile!", "");
			return false;
		}
		else
		{
			LOG(ERROR, "Unable to manage the MAC addresses for generated nodes on the '" + lowProf->m_name + "' profile!", "");
			return false;
		}

		//Is still over allocated
		if(lowCounter->m_count > (1 - lowCounter->m_increment))
		{
			double expectedNodes = lowCounter->m_increment * targetNodeCount;
			if(expectedNodes >= 1)
			{
				unsigned int j = 0;
				for(; j < underPopulatedProfiles.size(); j++)
				{
					if(underPopulatedProfiles[j]->m_count < lowCounter->m_count)
					{
						break;
					}
				}
				underPopulatedProfiles.insert(underPopulatedProfiles.begin() + j, lowCounter);
			}
		}
		//If the profile is still under allocated
		if(highCounter->m_count < (-highCounter->m_increment))
		{
			if(!highCounter->m_profile.m_nodeKeys.size())
			{
				unsigned int j = 0;
				for(; j < overPopulatedProfiles.size(); j++)
				{
					if(overPopulatedProfiles[j]->m_count < highCounter->m_count)
					{
						break;
					}
				}
				overPopulatedProfiles.insert(overPopulatedProfiles.begin() + j, highCounter);
			}
		}
	}
	return true;
}

bool NodeManager::AdjustMACVendors(ProfileCounter *targetProfile)
{
	vector<MacCounter*> underPopulatedVendors;
	vector<MacCounter*> overPopulatedVendors;

	for(unsigned int j = 0; j < targetProfile->m_macCounters.size(); j++)
	{
		//If the vendor is under allocated
		if(targetProfile->m_macCounters[j].m_count > (1-targetProfile->m_macCounters[j].m_increment))
		{
			underPopulatedVendors.push_back(&targetProfile->m_macCounters[j]);
		}
		//If the vendor is over allocated
		else if(targetProfile->m_macCounters[j].m_count < (-targetProfile->m_macCounters[j].m_increment))
		{
			overPopulatedVendors.push_back(&targetProfile->m_macCounters[j]);
		}
	}

	//If we have vendors that need adjustment on the profile
	while(!underPopulatedVendors.empty() && !overPopulatedVendors.empty())
	{
		//Get the first pairing
		MacCounter *lowVCounter = underPopulatedVendors.back();
		MacCounter *highVCounter = overPopulatedVendors.back();
		underPopulatedVendors.pop_back();
		overPopulatedVendors.pop_back();

		//Get the profile and search for a node using the overpopulated vendor
		NodeProfile * curProfile = &m_hdconfig->m_profiles[targetProfile->m_profile.m_name];
		if(curProfile == NULL)
		{
			LOG(ERROR, "Unable to retrieve expected profile '" + targetProfile->m_profile.m_name + "'.", "");
			return false;
		}
		if(curProfile->m_nodeKeys.empty())
		{
			break;
		}
		for(unsigned int j = 0; j < curProfile->m_nodeKeys.size(); j++)
		{
			Node *curNode = &m_hdconfig->m_nodes[curProfile->m_nodeKeys[j]];
			if(curNode == NULL)
			{
				LOG(ERROR, "Unable to retrieve expected node in target profile '" + targetProfile->m_profile.m_name + "'.", "");
				return false;
			}
			string vendor = m_hdconfig->m_macAddresses.LookupVendor(m_hdconfig->m_macAddresses.AtoMACPrefix(curNode->m_MAC));

			if(!vendor.compare(highVCounter->m_ethVendor))
			{
				//Remove the overpopulated counter values
				highVCounter->m_count++;
				lowVCounter->m_count--;
				curNode->m_MAC = m_hdconfig->m_macAddresses.GenerateRandomMAC(lowVCounter->m_ethVendor);
			}
		}
		//If the vendor is still under allocated
		if(lowVCounter->m_count < (-lowVCounter->m_increment))
		{
			underPopulatedVendors.push_back(lowVCounter);
		}
		//If the vendor is still over allocated
		else if(highVCounter->m_count > (1 - highVCounter->m_increment))
		{
			overPopulatedVendors.push_back(highVCounter);
		}
	}
	return true;
}

bool NodeManager::AdjustPortsOnNodes(ProfileCounter *targetProfile)
{
	vector<PortCounter*> underPopulatedPorts;
	vector<PortCounter*> overPopulatedPorts;

	//Check for over or under allocated
	for(unsigned int j = 0; j < targetProfile->m_portCounters.size(); j++)
	{
		//If the port is under allocated
		if(targetProfile->m_portCounters[j].m_count > (1 - targetProfile->m_portCounters[j].m_increment))
		{
			underPopulatedPorts.push_back(&targetProfile->m_portCounters[j]);
		}
		//If the port is over allocated
		else if(targetProfile->m_portCounters[j].m_count < (-targetProfile->m_portCounters[j].m_increment))
		{
			overPopulatedPorts.push_back(&targetProfile->m_portCounters[j]);
		}
	}

	//Get the avg number of ports per node to prevent adding to many ports to a single node
	double avg_ports = 0;
	for(unsigned int j = 0; j < targetProfile->m_profile.m_ports.size(); j++)
	{
		avg_ports += targetProfile->m_profile.m_ports[j].second.second;
	}

	//If we have ports that need adjustment on the profile
	while(!underPopulatedPorts.empty())
	{
		PortCounter *lowPCounter = underPopulatedPorts.back();
		underPopulatedPorts.pop_back();

		Port lowPort = m_hdconfig->GetPort(lowPCounter->m_portName);
		NodeProfile * curProfile = &m_hdconfig->m_profiles[targetProfile->m_profile.m_name];
		if(curProfile->m_nodeKeys.empty())
		{
			break;
		}
		bool foundMatch = false;
		for(unsigned int j = 0; j < curProfile->m_nodeKeys.size(); j++)
		{
			Node *curNode = &m_hdconfig->m_nodes[curProfile->m_nodeKeys[j]];
			for(unsigned int k = 0; k < curNode->m_ports.size(); k++)
			{
				Port nodePort = m_hdconfig->GetPort(curNode->m_ports[k]);

				//If this port is a match
				if(!curNode->m_ports[k].compare(lowPort.m_portName))
				{
					string behaviorStr = "reset";
					curNode->m_ports[k] = nodePort.m_portNum + "_" + nodePort.m_type + "_" + behaviorStr;
					curNode->m_isPortInherited[k] = false;
					// -= 1 - m_increment for port addition, -= m_increment for removing a blocked port
					lowPCounter->m_count--;
					foundMatch = true;
				}
			}
		}
		//If the vendor is still under allocated
		if((lowPCounter->m_count < (-lowPCounter->m_increment)) && foundMatch)
		{
			underPopulatedPorts.push_back(lowPCounter);
		}
	}
	while(!overPopulatedPorts.empty())
	{
		//Get the first pairing
		PortCounter *highPCounter = overPopulatedPorts.back();
		overPopulatedPorts.pop_back();

		Port highPort = m_hdconfig->GetPort(highPCounter->m_portName);

		NodeProfile * curProfile = &m_hdconfig->m_profiles[targetProfile->m_profile.m_name];
		if(curProfile->m_nodeKeys.empty())
		{
			break;
		}
		bool foundMatch = false;
		for(unsigned int j = 0; j < curProfile->m_nodeKeys.size(); j++)
		{
			Node *curNode = &m_hdconfig->m_nodes[curProfile->m_nodeKeys[j]];
			for(unsigned int k = 0; k < curNode->m_ports.size(); k++)
			{
				Port nodePort = m_hdconfig->GetPort(curNode->m_ports[k]);

				//If this port is a match
				if(!curNode->m_ports[k].compare(highPort.m_portName))
				{
					string behaviorStr = "block";
					if((!nodePort.m_type.compare("TCP")) && (!curProfile->m_tcpAction.compare("reset")))
					{
						behaviorStr = "reset";
					}
					curNode->m_ports[k] = nodePort.m_portNum + "_" + nodePort.m_type + "_" + behaviorStr;
					curNode->m_isPortInherited[k] = false;
					// += m_increment for blocking port , += (1-m_increment) for removing a used port
					highPCounter->m_count++;
					foundMatch = true;
				}
			}
		}
		//If the vendor is still over allocated
		if((highPCounter->m_count > (1 - highPCounter->m_increment)) && foundMatch)
		{
			overPopulatedPorts.push_back(highPCounter);
		}
	}
	return true;
}

// GenerateProfileCounters serves as the starting point for RecursiveGenProfileCounter when loading
//		a Honeyd Configuration rather than an nmap scan, used mainly by the UI's for user configuration
// 	NodeProfile *rootProfile: This usually corresponds to the 'Default' NodeProfile and is the top of
//		the profile tree
bool NodeManager::GenerateProfileCounters(NodeProfile *rootProfile)
{
	m_profileCounters.clear();
	return RecursiveGenProfileCounter(rootProfile);
}

// RecursiveGenProfileCounter recurses through the m_persTree member variable and generates
//		randomized nodes from the profiles at the different nodes of the Personality Tree.
//		Creates, populates and pushes a ProfileCounter struct, complete with Mac- and PortCounters
//		for each node in the tree into the m_profileCounters member variable.
//  NodeProfile *profile - pointer to a NodeProfile in the tree structure;
//                         the profile's information is read and generates a ProfileCounter
// Returns nothing.
bool NodeManager::RecursiveGenProfileCounter(NodeProfile *profile)
{
	if(m_hdconfig->GetProfile(profile->m_name) == NULL)
	{
		LOG(ERROR, "Couldn't retrieve expected NodeProfile: " + profile->m_name, "");
		return false;
	}
	else if((profile->m_generated) && (NumberOfChildren(profile->m_name) == 0) && !profile->m_ethernetVendors.empty())
	{
		struct ProfileCounter pCounter;
		pCounter.m_profile = *m_hdconfig->GetProfile(profile->m_name);
		pCounter.m_increment = profile->m_distribution/100;
		string tempStr = profile->m_parentProfile;
		NodeProfile *parentProf = m_hdconfig->GetProfile(tempStr);
		while(parentProf != NULL)
		{
			pCounter.m_increment = pCounter.m_increment*(parentProf->m_distribution/100);
			tempStr = parentProf->m_parentProfile;
			parentProf = m_hdconfig->GetProfile(tempStr);
		}

		double avgPorts = 0;
		for(uint i = 0; i < profile->m_ports.size(); i++)
		{
			avgPorts += (profile->m_ports[i].second.second/100);
		}
		pCounter.m_numAvgPorts = (uint)::floor(avgPorts + 0.5);
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
			if(!RecursiveGenProfileCounter(&m_hdconfig->m_profiles[it->first]))
			{
				return false;
			}
		}
	}
	return true;
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
