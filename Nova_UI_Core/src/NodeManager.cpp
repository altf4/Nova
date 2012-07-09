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


NodeManager::NodeManager(PersonalityTree *persTree)
{
	if(persTree != NULL)
	{
		m_persTree = persTree;
		m_hdconfig = m_persTree->GetHDConfig();
		m_persTree->AddAllPorts();
		GenerateProfileCounters();
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
	GenerateProfileCounters();
	return true;
}

void NodeManager::GenerateNodes(unsigned int num_nodes)
{
	vector<Node> nodesToAdd;
	nodesToAdd.clear();

	for(unsigned int i = 0; i < num_nodes;)
	{
		for(unsigned int j = 0; j < m_profileCounters.size() && i < num_nodes; j++)
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
							m_profileCounters[j].m_macCounters[l].m_count -= m_profileCounters[j].m_macCounters[l].m_increment;
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
void NodeManager::GenerateProfileCounters()
{
	PersonalityNode *rootNode = m_persTree->GetRootNode();
	m_hostCount = rootNode->m_count;
	RecursiveGenProfileCounter(*rootNode);
}

void NodeManager::RecursiveGenProfileCounter(const PersonalityNode &parent)
{
	if(parent.m_children.empty())
	{
		struct ProfileCounter pCounter;

		if(m_hdconfig->GetProfile(parent.m_key) == NULL)
		{
			LOG(ERROR, "Couldn't retrieve expected NodeProfile: " + parent.m_key, "");
			return;
		}
		pCounter.m_profile = *m_hdconfig->GetProfile(parent.m_key);
		pCounter.m_increment = ((double)parent.m_count / (double)m_hostCount);
		pCounter.m_numAvgPorts = parent.m_avgPortCount;
		pCounter.m_count = 0;

		for(unsigned int i = 0; i < parent.m_vendor_dist.size(); i++)
		{
			pCounter.m_macCounters.push_back(GenerateMacCounter(parent.m_vendor_dist[i].first, parent.m_vendor_dist[i].second));
		}
		for(unsigned int i = 0; i < parent.m_ports_dist.size(); i++)
		{
			pCounter.m_portCounters.push_back(GeneratePortCounter(parent.m_ports_dist[i].first, parent.m_ports_dist[i].second));
		}
		m_profileCounters.push_back(pCounter);
	}
	for(unsigned int i = 0; i < parent.m_children.size(); i++)
	{
		RecursiveGenProfileCounter(*parent.m_children[i].second);
	}
}

MacCounter NodeManager::GenerateMacCounter(const string &vendor, const double dist_val)
{
	struct MacCounter ret;
	ret.m_ethVendor = vendor;
	ret.m_increment = dist_val/100;
	ret.m_count = 0;
	return ret;
}

PortCounter NodeManager::GeneratePortCounter(const string &portName, const double dist_val)
{
	struct PortCounter ret;
	ret.m_portName = portName;
	ret.m_increment = dist_val/100;
	ret.m_count = 0;
	return ret;
}

}
