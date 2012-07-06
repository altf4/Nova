//============================================================================
// Name        : HoneydConfiguration.cpp
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
// Description : Object for reading and writing Honeyd XML configurations
//============================================================================

#include "HoneydConfiguration.h"
#include "NovaUtil.h"
#include "Logger.h"

#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <arpa/inet.h>
#include <math.h>

using namespace std;
using namespace Nova;
using boost::property_tree::ptree;
using boost::property_tree::xml_parser::trim_whitespace;

namespace Nova
{

HoneydConfiguration::HoneydConfiguration()
{
	m_macAddresses.LoadPrefixFile();

	m_homePath = Config::Inst()->GetPathHome();
	m_subnets.set_empty_key("");
	m_ports.set_empty_key("");
	m_nodes.set_empty_key("");
	m_profiles.set_empty_key("");
	m_scripts.set_empty_key("");
	m_subnets.set_deleted_key("Deleted");
	m_nodes.set_deleted_key("Deleted");
	m_profiles.set_deleted_key("Deleted");
	m_ports.set_deleted_key("Deleted");
	m_scripts.set_deleted_key("Deleted");
}

int HoneydConfiguration::GetMaskBits(in_addr_t mask)
{
	mask = ~mask;
	int i = 32;
	while(mask != 0)
	{
		mask = mask/2;
		i--;
	}
	return i;
}

//Adds a port with the specified configuration into the port table
//	portNum: Must be a valid port number (1-65535)
//	isTCP: if true the port uses TCP, if false it uses UDP
//	behavior: how this port treats incoming connections
//	scriptName: this parameter is only used if behavior == SCRIPT, in which case it designates
//		the key of the script it can lookup and execute for incoming connections on the port
//	Note(s): If CleanPorts is called before using this port in a profile, it will be deleted
//			If using a script it must exist in the script table before calling this function
//Returns: the port name if successful and an empty string if unsuccessful
string HoneydConfiguration::AddPort(uint16_t portNum, portProtocol isTCP, portBehavior behavior, string scriptName, string service)
{
	Port pr;
	//Check the validity and assign the port number
	if(!portNum)
	{
		LOG(ERROR, "Cannot create port: Port Number of 0 is Invalid.", "");
		return string("");
	}
	stringstream ss;
	ss << portNum;
	pr.m_portNum = ss.str();

	//Assign the port type (UDP or TCP)
	if(isTCP)
	{
		pr.m_type = "TCP";
	}
	else
	{
		pr.m_type = "UDP";
	}

	//Check and assign the port behavior
	switch(behavior)
	{
		case BLOCK:
		{
			pr.m_behavior = "block";
			break;
		}
		case OPEN:
		{
			pr.m_behavior = "open";
			break;
		}
		case RESET:
		{
			pr.m_behavior = "reset";
			if(!pr.m_type.compare("UDP"))
			{
				pr.m_behavior = "block";
			}
			break;
		}
		case SCRIPT:
		{
			//If the script does not exist
			if(m_scripts.find(scriptName) == m_scripts.end())
			{
				LOG(ERROR, "Cannot create port: specified script " + scriptName + " does not exist.", "");
				return "";
			}
			pr.m_behavior = "script";
			pr.m_scriptName = scriptName;
			break;
		}
		default:
		{
			LOG(ERROR, "Cannot create port: Attempting to use unknown port behavior", "");
			return string("");
		}
	}

	pr.m_service = service;

	//	Creates the ports unique identifier these names won't collide unless the port is the same
	if(!pr.m_behavior.compare("script"))
	{
		pr.m_portName = pr.m_portNum + "_" + pr.m_type + "_" + pr.m_scriptName;
	}
	else
	{
		pr.m_portName = pr.m_portNum + "_" + pr.m_type + "_" + pr.m_behavior;
	}

	//Checks if the port already exists
	if(m_ports.find(pr.m_portName) != m_ports.end())
	{
		LOG(DEBUG, "Cannot create port: Specified port " + pr.m_portName + " already exists.", "");
		return pr.m_portName;
	}

	//Adds the port into the table
	m_ports[pr.m_portName] = pr;
	return pr.m_portName;
}

std::string HoneydConfiguration::AddPort(Port pr)
{
	if(m_ports.find(pr.m_portName) != m_ports.end())
	{
		return pr.m_portName;
	}

	m_ports[pr.m_portName] = pr;

	return pr.m_portName;
}

//Calls all load functions
bool HoneydConfiguration::LoadAllTemplates()
{
	m_scripts.clear_no_resize();
	m_ports.clear_no_resize();
	m_profiles.clear_no_resize();
	m_nodes.clear_no_resize();
	m_subnets.clear_no_resize();

	LoadScriptsTemplate();
	LoadPortsTemplate();
	LoadProfilesTemplate();
	LoadNodesTemplate();

	LoadNodeKeys();

	return true;
}

void HoneydConfiguration::LoadNodeKeys()
{
	for(NodeTable::iterator it = m_nodes.begin(); it != m_nodes.end(); it++)
	{
		m_profiles[it->second.m_pfile].m_nodeKeys.push_back(it->first);
	}
}

vector<string> HoneydConfiguration::GetGeneratedProfileNames()
{
	vector<std::string> childProfiles;

	for(ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
	{
		if(it->second.m_generated && !it->second.m_personality.empty() && !it->second.m_ethernet.empty())
		{
			childProfiles.push_back(it->second.m_name);
		}
	}

	return childProfiles;
}

//Loads ports from file
bool HoneydConfiguration::LoadPortsTemplate()
{
	using boost::property_tree::ptree;
	using boost::property_tree::xml_parser::trim_whitespace;

	m_portTree.clear();
	try
	{
		read_xml(m_homePath + "/templates/ports.xml", m_portTree, boost::property_tree::xml_parser::trim_whitespace);

		BOOST_FOREACH(ptree::value_type &value, m_portTree.get_child("ports"))
		{
			Port port;
			port.m_tree = value.second;
			//Required xml entries
			port.m_portName = value.second.get<std::string>("name");

			if(!port.m_portName.compare(""))
			{
				LOG(ERROR, "Problem loading honeyd XML files.", "");
				continue;
			}

			port.m_portNum = value.second.get<std::string>("number");
			port.m_type = value.second.get<std::string>("type");
			port.m_service = value.second.get<std::string>("service");
			port.m_behavior = value.second.get<std::string>("behavior");

			//If this port uses a script, find and assign it.
			if(!port.m_behavior.compare("script") || !port.m_behavior.compare("internal"))
			{
				port.m_scriptName = value.second.get<std::string>("script");
			}
			//If the port works as a proxy, find destination
			else if(!port.m_behavior.compare("proxy"))
			{
				port.m_proxyIP = value.second.get<std::string>("IP");
				port.m_proxyPort = value.second.get<std::string>("Port");
			}
			m_ports[port.m_portName] = port;
		}
	}
	catch(Nova::hashMapException &e)
	{
		LOG(ERROR, "Problem loading ports: " + string(e.what()) + ".", "");
		return false;
	}

	return true;
}


//Loads the subnets and nodes from file for the currently specified group
bool HoneydConfiguration::LoadNodesTemplate()
{
	using boost::property_tree::ptree;
	using boost::property_tree::xml_parser::trim_whitespace;

	m_groupTree.clear();
	ptree propTree;

	try
	{
		read_xml(m_homePath + "/templates/nodes.xml", m_groupTree, boost::property_tree::xml_parser::trim_whitespace);
		m_groups.clear();
		BOOST_FOREACH(ptree::value_type &value, m_groupTree.get_child("groups"))
		{
			m_groups.push_back(value.second.get<std::string>("name"));
			//Find the specified group
			if(!value.second.get<std::string>("name").compare(Config::Inst()->GetGroup()))
			{
				try //Null Check
				{
					//Load Subnets first, they are needed before we can load nodes
					m_subnetTree = value.second.get_child("subnets");
					LoadSubnets(&m_subnetTree);

					try //Null Check
					{
						//If subnets are loaded successfully, load nodes
						m_nodesTree = value.second.get_child("nodes");
						LoadNodes(&m_nodesTree);
					}
					catch(Nova::hashMapException &e)
					{
						LOG(ERROR, "Problem loading nodes: " + string(e.what()) + ".", "");
						return false;
					}
				}
				catch(Nova::hashMapException &e)
				{
					LOG(ERROR, "Problem loading subnets: " + string(e.what()) + ".", "");
					return false;
				}
			}
		}
	}
	catch(Nova::hashMapException &e)
	{
		LOG(ERROR, "Problem loading groups: " + Config::Inst()->GetGroup() + " - " + string(e.what()) + ".", "");
		return false;
	}

	return true;
}

//This is used when a profile is cloned, it allows us to copy a ptree and extract all children from it
// it is exactly the same as novagui's xml extraction functions except that it gets the ptree from the
// cloned profile and it asserts a profile's name is unique and changes the name if it isn't
bool HoneydConfiguration::LoadProfilesFromTree(string parent)
{
	using boost::property_tree::ptree;
	ptree *ptr, pt = m_profiles[parent].m_tree;
	try
	{
		BOOST_FOREACH(ptree::value_type &value, pt.get_child("profiles"))
		{
			//Generic profile, essentially a honeyd template
			if(!string(value.first.data()).compare("profile"))
			{
				NodeProfile prof = m_profiles[parent];
				//Root profile has no parent
				prof.m_parentProfile = parent;
				prof.m_tree = value.second;

				for(uint i = 0; i < INHERITED_MAX; i++)
				{
					prof.m_inherited[i] = true;
				}

				//Asserts the name is unique, if it is not it finds a unique name
				// up to the range of 2^32
				string profileStr = prof.m_name;
				stringstream ss;
				uint i = 0, j = 0;
				j = ~j; //2^32-1

				while((m_profiles.keyExists(prof.m_name)) && (i < j))
				{
					ss.str("");
					i++;
					ss << profileStr << "-" << i;
					prof.m_name = ss.str();
				}
				prof.m_tree.put<std::string>("name", prof.m_name);

				prof.m_ports.clear();

				try //Conditional: has "set" values
				{
					ptr = &value.second.get_child("set");
					//pass 'set' subset and pointer to this profile
					LoadProfileSettings(ptr, &prof);
				}
				catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e) {};

				try //Conditional: has "add" values
				{
					ptr = &value.second.get_child("add");
					//pass 'add' subset and pointer to this profile
					LoadProfileServices(ptr, &prof);
				}
				catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e) {};

				//Save the profile
				m_profiles[prof.m_name] = prof;
				UpdateProfile(prof.m_name);

				try //Conditional: has children profiles
				{
					ptr = &value.second.get_child("profiles");

					//start recurisive descent down profile tree with this profile as the root
					//pass subtree and pointer to parent
					LoadProfileChildren(prof.m_name);
				}
				catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e) {};
			}

			//Honeyd's implementation of switching templates based on conditions
			else if(!string(value.first.data()).compare("dynamic"))
			{
				//TODO
			}
			else
			{
				LOG(ERROR, "Invalid XML Path "+string(value.first.data()), "");
			}
		}
		return true;
	}
	catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
	{
		LOG(ERROR, "Problem loading Profiles: "+string(e.what()), "");
		return false;
	}
}

//Sets the configuration of 'set' values for profile that called it
bool HoneydConfiguration::LoadProfileSettings(ptree *propTree, NodeProfile *nodeProf)
{
	string valueKey;
	try
	{
		BOOST_FOREACH(ptree::value_type &value, propTree->get_child(""))
		{
			valueKey = "TCP";
			if(!string(value.first.data()).compare(valueKey))
			{
				nodeProf->m_tcpAction = value.second.data();
				nodeProf->m_inherited[TCP_ACTION] = false;
				continue;
			}
			valueKey = "UDP";
			if(!string(value.first.data()).compare(valueKey))
			{
				nodeProf->m_udpAction = value.second.data();
				nodeProf->m_inherited[UDP_ACTION] = false;
				continue;
			}
			valueKey = "ICMP";
			if(!string(value.first.data()).compare(valueKey))
			{
				nodeProf->m_icmpAction = value.second.data();
				nodeProf->m_inherited[ICMP_ACTION] = false;
				continue;
			}
			valueKey = "personality";
			if(!string(value.first.data()).compare(valueKey))
			{
				nodeProf->m_personality = value.second.data();
				nodeProf->m_inherited[PERSONALITY] = false;
				continue;
			}
			valueKey = "ethernet";
			if(!string(value.first.data()).compare(valueKey))
			{
				pair<string, double> ethPair;
				ethPair.first = value.second.get<std::string>("vendor");
				ethPair.second = value.second.get<double>("distribution");
				nodeProf->m_ethernetVendors.push_back(ethPair);
				nodeProf->m_inherited[ETHERNET] = false;
				continue;
			}
			valueKey = "uptimeMin";
			if(!string(value.first.data()).compare(valueKey))
			{
				nodeProf->m_uptimeMin = value.second.data();
				nodeProf->m_inherited[UPTIME] = false;
				continue;
			}
			valueKey = "uptimeMax";
			if(!string(value.first.data()).compare(valueKey))
			{
				nodeProf->m_uptimeMax = value.second.data();
				nodeProf->m_inherited[UPTIME] = false;
				continue;
			}
			valueKey = "dropRate";
			if(!string(value.first.data()).compare(valueKey))
			{
				nodeProf->m_dropRate = value.second.data();
				nodeProf->m_inherited[DROP_RATE] = false;
				continue;
			}
		}
	}
	catch(Nova::hashMapException &e)
	{
		LOG(ERROR, "Problem loading profile set parameters: " + string(e.what()) + ".", "");
		return false;
	}

	return true;
}

//Adds specified ports and subsystems
// removes any previous port with same number and type to avoid conflicts
bool HoneydConfiguration::LoadProfileServices(ptree *propTree, NodeProfile *nodeProf)
{
	string valueKey;
	Port *port;

	try
	{
		for(uint i = 0; i < nodeProf->m_ports.size(); i++)
		{
			nodeProf->m_ports[i].second.first = true;
		}
		BOOST_FOREACH(ptree::value_type &value, propTree->get_child(""))
		{
			//Checks for ports
			valueKey = "ports";
			if(!string(value.first.data()).compare(valueKey))
			{
				//Iterates through the ports
				BOOST_FOREACH(ptree::value_type &portValue, propTree->get_child("ports"))
				{
					port = &m_ports[portValue.second.data()];

					//Checks inherited ports for conflicts
					for(uint i = 0; i < nodeProf->m_ports.size(); i++)
					{
						//Erase inherited port if a conflict is found
						if(!port->m_portNum.compare(m_ports[nodeProf->m_ports[i].first].m_portNum) && !port->m_type.compare(m_ports[nodeProf->m_ports[i].first].m_type))
						{
							nodeProf->m_ports.erase(nodeProf->m_ports.begin() + i);
						}
					}
					//Add specified port
					pair<string, pair<bool, double> > portPair;
					portPair.first = port->m_portName;
					portPair.second.first = false;
					portPair.second.second = 0;
					if(!nodeProf->m_ports.size())
					{
						nodeProf->m_ports.push_back(portPair);
					}
					else
					{
						uint i = 0;
						for(i = 0; i < nodeProf->m_ports.size(); i++)
						{
							Port *tempPort = &m_ports[nodeProf->m_ports[i].first];
							if((atoi(tempPort->m_portNum.c_str())) < (atoi(port->m_portNum.c_str())))
							{
								continue;
							}
							break;
						}
						if(i < nodeProf->m_ports.size())
						{
							nodeProf->m_ports.insert(nodeProf->m_ports.begin() + i, portPair);
						}
						else
						{
							nodeProf->m_ports.push_back(portPair);
						}
					}
				}
				continue;
			}

			//Checks for a subsystem
			valueKey = "subsystem"; //TODO
			if(!string(value.first.data()).compare(valueKey))
			{
				continue;
			}
		}
	}
	catch(Nova::hashMapException &e)
	{
		LOG(ERROR, "Problem loading profile add parameters: " + string(e.what()) + ".", "");
		return false;
	}

	return true;
}

//Recursive descent down a profile tree, inherits parent, sets values and continues if not leaf.
bool HoneydConfiguration::LoadProfileChildren(string parentKey)
{
	ptree propTree = m_profiles[parentKey].m_tree;
	try
	{
		BOOST_FOREACH(ptree::value_type &value, propTree.get_child("profiles"))
		{
			ptree *childPropTree;

			//Inherits parent,
			NodeProfile nodeProf = m_profiles[parentKey];
			nodeProf.m_tree = value.second;
			nodeProf.m_parentProfile = parentKey;

			nodeProf.m_generated = value.second.get<bool>("generated");

			//Gets name, initializes DHCP
			nodeProf.m_name = value.second.get<std::string>("name");

			if(!nodeProf.m_name.compare(""))
			{
				LOG(ERROR, "Problem loading honeyd XML files.", "");
				continue;
			}

			for(uint i = 0; i < INHERITED_MAX; i++)
			{
				nodeProf.m_inherited[i] = true;
			}

			try
			{
				childPropTree = &value.second.get_child("set");
				LoadProfileSettings(childPropTree, &nodeProf);
			}
			catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
			{
			};

			try
			{
				childPropTree = &value.second.get_child("add");
				LoadProfileServices(childPropTree, &nodeProf);
			}
			catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
			{
			};

			//Saves the profile
			m_profiles[nodeProf.m_name] = nodeProf;
			try
			{
				LoadProfileChildren(nodeProf.m_name);
			}
			catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
			{
			};
		}
	}
	catch(Nova::hashMapException &e)
	{
		LOG(ERROR, "Problem loading sub profiles: " + string(e.what()) + ".", "");
		return false;
	}
	return true;
}


//Loads scripts from file
bool HoneydConfiguration::LoadScriptsTemplate()
{
	using boost::property_tree::ptree;
	using boost::property_tree::xml_parser::trim_whitespace;
	m_scriptTree.clear();
	try
	{
		read_xml(m_homePath + "/scripts.xml", m_scriptTree, boost::property_tree::xml_parser::trim_whitespace);

		BOOST_FOREACH(ptree::value_type &value, m_scriptTree.get_child("scripts"))
		{
			Script script;
			script.m_tree = value.second;
			//Each script consists of a name and path to that script
			script.m_name = value.second.get<std::string>("name");

			if(!script.m_name.compare(""))
			{
				LOG(ERROR, "Problem loading honeyd XML files.","");
				continue;
			}

			try
			{
				script.m_osclass = value.second.get<std::string>("osclass");
			}
			catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
			{
			};
			try
			{
				script.m_service = value.second.get<std::string>("service");
			}
			catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
			{
			};
			script.m_path = value.second.get<std::string>("path");
			m_scripts[script.m_name] = script;
		}
	}
	catch(Nova::hashMapException &e)
	{
		LOG(ERROR, "Problem loading scripts: " + string(e.what()) + ".", "");
		return false;
	}
	return true;
}


/************************************************
  Save Honeyd XML Configuration Functions
 ************************************************/

//Saves the current configuration information to XML files

// Important this function assumes that unless it is a new item (ptree pointer == NULL) then
// all required fields exist and old fields have been removed. Ex: if a port previously used a script
// but now has a behavior of open, at that point the user should have erased the script field.
// inverserly if a user switches to script the script field must be created.

//To summarize this function only populates the xml data for the values it contains unless it is a new item,
// it does not clean up, and only creates if it's a new item and then only for the fields that are needed.
// it does not track profile inheritance either, that should be created when the heirarchy is modified.
bool HoneydConfiguration::SaveAllTemplates()
{
	using boost::property_tree::ptree;
	ptree propTree;

	//Scripts
	m_scriptTree.clear();
	for(ScriptTable::iterator it = m_scripts.begin(); it != m_scripts.end(); it++)
	{
		propTree = it->second.m_tree;
		propTree.put<std::string>("name", it->second.m_name);
		propTree.put<std::string>("service", it->second.m_service);
		propTree.put<std::string>("osclass", it->second.m_osclass);
		propTree.put<std::string>("path", it->second.m_path);
		m_scriptTree.add_child("scripts.script", propTree);
	}

	//Ports
	m_portTree.clear();
	for(PortTable::iterator it = m_ports.begin(); it != m_ports.end(); it++)
	{
		propTree = it->second.m_tree;
		propTree.put<std::string>("name", it->second.m_portName);
		propTree.put<std::string>("number", it->second.m_portNum);
		propTree.put<std::string>("type", it->second.m_type);
		propTree.put<std::string>("service", it->second.m_service);
		propTree.put<std::string>("behavior", it->second.m_behavior);
		//If this port uses a script, save it.
		if(!it->second.m_behavior.compare("script") || !it->second.m_behavior.compare("internal"))
		{
			propTree.put<std::string>("script", it->second.m_scriptName);
		}
		//If the port works as a proxy, save destination
		else if(!it->second.m_behavior.compare("proxy"))
		{
			propTree.put<std::string>("IP", it->second.m_proxyIP);
			propTree.put<std::string>("Port", it->second.m_proxyPort);
		}
		m_portTree.add_child("ports.port", propTree);
	}

	m_subnetTree.clear();
	for(SubnetTable::iterator it = m_subnets.begin(); it != m_subnets.end(); it++)
	{
		propTree = it->second.m_tree;

		//TODO assumes subnet is interface, need to discover and handle if virtual
		propTree.put<std::string>("name", it->second.m_name);
		propTree.put<bool>("enabled",it->second.m_enabled);
		propTree.put<bool>("isReal", it->second.m_isRealDevice);

		//Remove /## format mask from the address then put it in the XML.
		stringstream ss;
		ss << "/" << it->second.m_maskBits;
		int i = ss.str().size();
		string addrString = it->second.m_address.substr(0,(it->second.m_address.size()-i));
		propTree.put<std::string>("IP", addrString);

		//Gets the mask from mask bits then put it in XML
		in_addr_t rawBitMask = ::pow(2, 32-it->second.m_maskBits) - 1;
		//If maskBits is 24 then we have 2^8 -1 = 0x000000FF
		rawBitMask = ~rawBitMask; //After getting the inverse of this we have the mask in host addr form.
		//Convert to network order, put in in_addr struct
		//call ntoa to get char pointer and make string
		in_addr netOrderMask;
		netOrderMask.s_addr = htonl(rawBitMask);
		addrString = string(inet_ntoa(netOrderMask));
		propTree.put<std::string>("mask", addrString);
		m_subnetTree.add_child("interface", propTree);
	}

	//Nodes
	m_nodesTree.clear();
	for(NodeTable::iterator it = m_nodes.begin(); it != m_nodes.end(); it++)
	{
		propTree = it->second.m_tree;

		// No need to save names besides the doppel, we can derive them
		if(it->second.m_name == "Doppelganger")
		{
			// Make sure the IP reflects whatever is being used right now
			it->second.m_IP = Config::Inst()->GetDoppelIp();
			propTree.put<std::string>("name", it->second.m_name);
		}

		//Required xml entires
		propTree.put<std::string>("interface", it->second.m_interface);
		propTree.put<std::string>("IP", it->second.m_IP);
		propTree.put<bool>("enabled", it->second.m_enabled);
		propTree.put<std::string>("MAC", it->second.m_MAC);
		propTree.put<std::string>("profile.name", it->second.m_pfile);
		ptree newPortTree;
		newPortTree.clear();
		for(uint i = 0; i < it->second.m_ports.size(); i++)
		{
			newPortTree.add<std::string>("port", it->second.m_ports[i]);
		}
		propTree.put_child("profile.add.ports", newPortTree);
		m_nodesTree.add_child("node",propTree);
	}
	using boost::property_tree::ptree;
	BOOST_FOREACH(ptree::value_type &value, m_groupTree.get_child("groups"))
	{
		//Find the specified group
		if(!value.second.get<std::string>("name").compare(Config::Inst()->GetGroup()))
		{
			//Load Subnets first, they are needed before we can load nodes
			value.second.put_child("subnets", m_subnetTree);
			value.second.put_child("nodes",m_nodesTree);
		}
	}
	m_profileTree.clear();
	for(ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
	{
		if(it->second.m_parentProfile == "")
		{
			propTree = it->second.m_tree;
			m_profileTree.add_child("profiles.profile", propTree);
		}
	}
	boost::property_tree::xml_writer_settings<char> settings('\t', 1);
	write_xml(m_homePath + "/scripts.xml", m_scriptTree, std::locale(), settings);
	write_xml(m_homePath + "/templates/ports.xml", m_portTree, std::locale(), settings);
	write_xml(m_homePath + "/templates/nodes.xml", m_groupTree, std::locale(), settings);
	write_xml(m_homePath + "/templates/profiles.xml", m_profileTree, std::locale(), settings);

	LOG(DEBUG, "Honeyd templates have been saved" ,"");
	return true;
}

//Writes the current configuration to honeyd configs
bool HoneydConfiguration::WriteHoneydConfiguration(string path)
{
	stringstream out;

	vector<string> profilesParsed;

	for(ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
	{
		if(!it->second.m_parentProfile.compare(""))
		{
			string pString = ProfileToString(&it->second);
			out << pString;
			profilesParsed.push_back(it->first);
		}
	}

	while(profilesParsed.size() < m_profiles.size())
	{
		for(ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
		{
			bool selfMatched = false;
			bool parentFound = false;
			for(uint i = 0; i < profilesParsed.size(); i++)
			{
				if(!it->second.m_parentProfile.compare(profilesParsed[i]))
				{
					parentFound = true;
					continue;
				}
				if(!it->first.compare(profilesParsed[i]))
				{
					selfMatched = true;
					break;
				}
			}

			if(!selfMatched && parentFound)
			{
				string pString = ProfileToString(&it->second);
				out << pString;
				profilesParsed.push_back(it->first);
			}
		}
	}

	// Start node section
	m_nodeProfileIndex = 0;
	for(NodeTable::iterator it = m_nodes.begin(); (it != m_nodes.end()) && (m_nodeProfileIndex < (uint)(~0)); it++)
	{
		m_nodeProfileIndex++;
		if(!it->second.m_enabled)
		{
			continue;
		}
		//We write the dopp regardless of whether or not it is enabled so that it can be toggled during runtime.
		else if(!it->second.m_name.compare("Doppelganger"))
		{
			string pString = DoppProfileToString(&m_profiles[it->second.m_pfile]);
			out << endl << pString;
			out << "bind " << it->second.m_IP << " DoppelgangerReservedTemplate" << endl << endl;
			//Use configured or discovered loopback
		}
		else
		{
			string profName = it->second.m_pfile;
			ReplaceChar(profName, ' ', '-');
			//Clone a custom profile for a node
			out << "clone CustomNodeProfile-" << m_nodeProfileIndex << " " << profName << endl;

			//Add any custom port settings
			for(uint i = 0; i < it->second.m_ports.size(); i++)
			{
				Port prt = m_ports[it->second.m_ports[i]];
				out << "add CustomNodeProfile-" << m_nodeProfileIndex << " ";
				if(!prt.m_type.compare("TCP"))
				{
					out << " tcp port ";
				}
				else
				{
					out << " udp port ";
				}
				out << prt.m_portNum << " ";

				if(!(prt.m_behavior.compare("script")))
				{
					string scriptName = prt.m_scriptName;

					if(m_scripts[scriptName].m_path.compare(""))
					{
						out << '"' << m_scripts[scriptName].m_path << '"'<< endl;
					}
					else
					{
						LOG(ERROR, "Error writing node port script.", "Path to script " + scriptName + " is null.");
					}
				}
				else
				{
					out << prt.m_behavior << endl;
				}
			}

			// No IP address, use DHCP
			if(it->second.m_IP == "DHCP" && it->second.m_MAC == "RANDOM")
			{
				out << "dhcp CustomNodeProfile-" << m_nodeProfileIndex << " on " << it->second.m_interface << endl;
			}
			else if(it->second.m_IP == "DHCP" && it->second.m_MAC != "RANDOM")
			{
				out << "dhcp CustomNodeProfile-" << m_nodeProfileIndex << " on " << it->second.m_interface << " ethernet \"" << it->second.m_MAC << "\"" << endl;
			}
			else if(it->second.m_IP != "DHCP" && it->second.m_MAC == "RANDOM")
			{
				out << "bind " << it->second.m_IP << " CustomNodeProfile-" << m_nodeProfileIndex << endl;
			}
			else if(it->second.m_IP != "DHCP" && it->second.m_MAC != "RANDOM")
			{
				out << "set " << "CustomNodeProfile-" << m_nodeProfileIndex << " ethernet \"" << it->second.m_MAC << "\"" << endl;
				out << "bind " << it->second.m_IP << " CustomNodeProfile-" << m_nodeProfileIndex << it->second.m_IP << endl;
			}
		}
	}
	ofstream outFile(path);
	outFile << out.str() << endl;
	outFile.close();

	return true;
}


//loads subnets from file for current group
bool HoneydConfiguration::LoadSubnets(ptree *propTree)
{
	try
	{
		BOOST_FOREACH(ptree::value_type &value, propTree->get_child(""))
		{
			//If real interface
			if(!string(value.first.data()).compare("interface"))
			{
				Subnet sub;
				sub.m_tree = value.second;
				sub.m_isRealDevice =  value.second.get<bool>("isReal");
				//Extract the data
				sub.m_name = value.second.get<std::string>("name");
				sub.m_address = value.second.get<std::string>("IP");
				sub.m_mask = value.second.get<std::string>("mask");
				sub.m_enabled = value.second.get<bool>("enabled");

				//Gets the IP address in uint32 form
				in_addr_t hostOrderAddr = ntohl(inet_addr(sub.m_address.c_str()));

				//Converting the mask to uint32 allows a simple bitwise AND to get the lowest IP in the subnet.
				in_addr_t hostOrderMask = ntohl(inet_addr(sub.m_mask.c_str()));
				sub.m_base = (hostOrderAddr & hostOrderMask);
				//Get the number of bits in the mask
				sub.m_maskBits = GetMaskBits(hostOrderMask);
				//Adding the binary inversion of the mask gets the highest usable IP
				sub.m_max = sub.m_base + ~hostOrderMask;
				stringstream ss;
				ss << sub.m_address << "/" << sub.m_maskBits;
				sub.m_address = ss.str();

				//Save subnet
				m_subnets[sub.m_name] = sub;
			}
			//If virtual honeyd subnet
			else if(!string(value.first.data()).compare("virtual"))
			{
			}
			else
			{
				LOG(ERROR, "Unexpected Entry in file: " + string(value.first.data()) + ".", "");
				return false;
			}
		}
	}
	catch(Nova::hashMapException &e)
	{
		LOG(ERROR, "Problem loading subnets: " + string(e.what()), "");
		return false;
	}
	return true;
}


//loads haystack nodes from file for current group
bool HoneydConfiguration::LoadNodes(ptree *propTree)
{
	NodeProfile nodeProf;
	//ptree *ptr2;
	try
	{
		BOOST_FOREACH(ptree::value_type &value, propTree->get_child(""))
		{
			if(!string(value.first.data()).compare("node"))
			{
				Node node;
				stringstream ss;
				uint j = 0;
				j = ~j; // 2^32-1

				node.m_tree = value.second;
				//Required xml entires
				node.m_interface = value.second.get<std::string>("interface");
				node.m_IP = value.second.get<std::string>("IP");
				node.m_enabled = value.second.get<bool>("enabled");
				node.m_pfile = value.second.get<std::string>("profile.name");

				if(!node.m_pfile.compare(""))
				{
					LOG(ERROR, "Problem loading honeyd XML files.", "");
					continue;
				}

				nodeProf = m_profiles[node.m_pfile];

				//Get mac if present
				try //Conditional: has "set" values
				{
					node.m_MAC = value.second.get<std::string>("MAC");
				}
				catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
				{

				};

				try //Conditional: has "set" values
				{
					ptree nodePorts = value.second.get_child("profile.add");
					LoadProfileServices(&nodePorts, &nodeProf);

					for(uint i = 0; i < nodeProf.m_ports.size(); i++)
					{
						node.m_ports.push_back(nodeProf.m_ports[i].first);
						node.m_isPortInherited.push_back(false);
					}
				}
				catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e) {};

				if(!node.m_IP.compare(Config::Inst()->GetDoppelIp()))
				{
					node.m_name = "Doppelganger";
					node.m_sub = node.m_interface;
					node.m_realIP = htonl(inet_addr(node.m_IP.c_str())); //convert ip to uint32
					//save the node in the table
					m_nodes[node.m_name] = node;

					//Put address of saved node in subnet's list of nodes.
					m_subnets[m_nodes[node.m_name].m_sub].m_nodes.push_back(node.m_name);
				}
				else
				{
					node.m_name = node.m_IP + " - " + node.m_MAC;

					if(node.m_name == "DHCP - RANDOM")
					{
						//Finds a unique identifier
						uint i = 1;
						while((m_nodes.keyExists(node.m_name)) && (i < j))
						{
							i++;
							ss.str("");
							ss << "DHCP - RANDOM(" << i << ")";
							node.m_name = ss.str();
						}
					}

					if(node.m_IP != "DHCP")
					{
						node.m_realIP = htonl(inet_addr(node.m_IP.c_str())); //convert ip to uint32
						node.m_sub = FindSubnet(node.m_realIP);
						//If we have a subnet and node is unique
						if(node.m_sub != "")
						{
							//Put address of saved node in subnet's list of nodes.
							m_subnets[node.m_sub].m_nodes.push_back(node.m_name);
						}
						//If no subnet found, can't use node unless it's doppelganger.
						else
						{
							LOG(ERROR, "Node at IP: " + node.m_IP + "is outside all valid subnet ranges.", "");
						}
					}
					else
					{
						node.m_sub = node.m_interface; //TODO virtual subnets will need to be handled when implemented
						// If no valid subnet/interface found
						if(!node.m_sub.compare(""))
						{
							LOG(ERROR, "DHCP Enabled Node with MAC: " + node.m_name + " is unable to resolve it's interface.","");
							continue;
						}

						//Put address of saved node in subnet's list of nodes.
						m_subnets[node.m_sub].m_nodes.push_back(node.m_name);
					}
				}

				if(!node.m_name.compare(""))
				{
					LOG(ERROR, "Problem loading honeyd XML files.", "");
					continue;
				}
				else
				{
					//save the node in the table
					m_nodes[node.m_name] = node;
				}
			}
			else
			{
				LOG(ERROR, "Unexpected Entry in file: " + string(value.first.data()), "");
			}
		}
	}
	catch(Nova::hashMapException &e)
	{
		LOG(ERROR, "Problem loading nodes: " + string(e.what()), "");
		return false;
	}

	return true;
}

string HoneydConfiguration::FindSubnet(in_addr_t ip)
{
	string subnet = "";
	int max = 0;
	//Check each subnet
	for(SubnetTable::iterator it = m_subnets.begin(); it != m_subnets.end(); it++)
	{
		//If node falls outside a subnets range skip it
		if((ip < it->second.m_base) || (ip > it->second.m_max))
		{
			continue;
		}
		//If this is the smallest range
		if(it->second.m_maskBits > max)
		{
			subnet = it->second.m_name;
		}
	}

	return subnet;
}

vector<string> HoneydConfiguration::GeneratedProfilesStrings()
{
	vector<string> ret;

	for(ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
	{
		if(it->second.m_generated)
		{
			stringstream pushToReturnVector;

			pushToReturnVector << "Name: " << it->second.m_name << "\n";
			pushToReturnVector << "Personality: " << it->second.m_personality << "\n";
			for(uint i = 0; i < it->second.m_ethernetVendors.size(); i++)
			{
				pushToReturnVector << "MAC Vendor:  " << it->second.m_ethernetVendors[i].first << " - " << it->second.m_ethernetVendors[i].second <<"% \n";
			}

			if(!it->second.m_nodeKeys.empty())
			{
				pushToReturnVector << "Associated Nodes:\n";

				for(uint i = 0; i < it->second.m_nodeKeys.size(); i++)
				{
					pushToReturnVector << "\t" << it->second.m_nodeKeys[i] << "\n";

					for(uint j = 0; j < m_nodes[it->second.m_nodeKeys[i]].m_ports.size(); j++)
					{
						pushToReturnVector << "\t\t" << m_nodes[it->second.m_nodeKeys[i]].m_ports[j];
					}
				}
			}

			ret.push_back(pushToReturnVector.str());
		}
	}
	return ret;
}

bool HoneydConfiguration::LoadProfilesTemplate()
{
	using boost::property_tree::ptree;
	using boost::property_tree::xml_parser::trim_whitespace;
	ptree *propTree;
	m_profileTree.clear();
	try
	{
		read_xml(m_homePath + "/templates/profiles.xml", m_profileTree, boost::property_tree::xml_parser::trim_whitespace);

		BOOST_FOREACH(ptree::value_type &value, m_profileTree.get_child("profiles"))
		{
			//Generic profile, essentially a honeyd template
			if(!string(value.first.data()).compare("profile"))
			{
				NodeProfile nodeProf;
				//Root profile has no parent
				nodeProf.m_parentProfile = "";
				nodeProf.m_tree = value.second;

				nodeProf.m_generated = value.second.get<bool>("generated");

				//Name required, DCHP boolean intialized (set in loadProfileSet)
				nodeProf.m_name = value.second.get<std::string>("name");

				if(!nodeProf.m_name.compare(""))
				{
					LOG(ERROR, "Problem loading honeyd XML files.", "");
					continue;
				}

				nodeProf.m_ports.clear();
				for(uint i = 0; i < INHERITED_MAX; i++)
				{
					nodeProf.m_inherited[i] = false;
				}

				try //Conditional: has "set" values
				{
					propTree = &value.second.get_child("set");
					//pass 'set' subset and pointer to this profile
					LoadProfileSettings(propTree, &nodeProf);
				}
				catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
				{
				};

				try //Conditional: has "add" values
				{
					propTree = &value.second.get_child("add");
					//pass 'add' subset and pointer to this profile
					LoadProfileServices(propTree, &nodeProf);
				}
				catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
				{
				};

				//Save the profile
				m_profiles[nodeProf.m_name] = nodeProf;

				try //Conditional: has children profiles
				{
					//start recurisive descent down profile tree with this profile as the root
					//pass subtree and pointer to parent
					LoadProfileChildren(nodeProf.m_name);
				}
				catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
				{
				};

			}

			//Honeyd's implementation of switching templates based on conditions
			else if(!string(value.first.data()).compare("dynamic"))
			{
				//TODO
			}
			else
			{
				LOG(ERROR, "Invalid XML Path " + string(value.first.data()) + ".", "");
			}
		}
	}
	catch(Nova::hashMapException &e)
	{
		LOG(ERROR, "Problem loading Profiles: " + string(e.what()) + ".", "");
		return false;
	}
	return true;
}

string HoneydConfiguration::ProfileToString(NodeProfile *p)
{
	stringstream out;
	string profName = p->m_name;
	string parentProfName = p->m_parentProfile;
	ReplaceChar(profName, ' ', '-');
	ReplaceChar(parentProfName, ' ', '-');

	if(!parentProfName.compare("default") || !parentProfName.compare(""))
	{
		out << "create " << profName << endl;
	}
	else
	{
		out << "clone " << profName << " " << parentProfName << endl;
	}

	out << "set " << profName  << " default tcp action " << p->m_tcpAction << endl;
	out << "set " << profName  << " default udp action " << p->m_udpAction << endl;
	out << "set " << profName  << " default icmp action " << p->m_icmpAction << endl;

	if(p->m_personality.compare(""))
	{
		out << "set " << profName << " personality \"" << p->m_personality << '"' << endl;
	}

	string vendor = "";
	double maxDist = 0;
	for(uint i = 0; i < p->m_ethernetVendors.size(); i++)
	{
		if(p->m_ethernetVendors[i].second > maxDist)
		{
			maxDist = p->m_ethernetVendors[i].second;
			vendor = p->m_ethernetVendors[i].first;
		}
	}
	if(vendor.compare(""))
	{
		out << "set " << profName << " ethernet \"" << vendor << '"' << endl;
	}

	if(p->m_dropRate.compare(""))
	{
		out << "set " << profName << " droprate in " << p->m_dropRate << endl;
	}

	for (uint i = 0; i < p->m_ports.size(); i++)
	{
		// Only include non-inherited ports
		if(!p->m_ports[i].second.first)
		{
			out << "add " << profName;
			if(!m_ports[p->m_ports[i].first].m_type.compare("TCP"))
			{
				out << " tcp port ";
			}
			else
			{
				out << " udp port ";
			}
			out << m_ports[p->m_ports[i].first].m_portNum << " ";

			if(!(m_ports[p->m_ports[i].first].m_behavior.compare("script")))
			{
				string scriptName = m_ports[p->m_ports[i].first].m_scriptName;

				if(m_scripts[scriptName].m_path.compare(""))
				{
					out << '"' << m_scripts[scriptName].m_path << '"'<< endl;
				}
				else
				{
					LOG(ERROR, "Error writing NodeProfile port script.", "Path to script " + scriptName + " is null.");
				}
			}
			else
			{
				out << m_ports[p->m_ports[i].first].m_behavior << endl;
			}
		}
	}
	out << endl;
	return out.str();
}

//
string HoneydConfiguration::DoppProfileToString(NodeProfile *p)
{
	stringstream out;
	out << "create DoppelgangerReservedTemplate" << endl;

	out << "set DoppelgangerReservedTemplate default tcp action " << p->m_tcpAction << endl;
	out << "set DoppelgangerReservedTemplate default udp action " << p->m_udpAction << endl;
	out << "set DoppelgangerReservedTemplate default icmp action " << p->m_icmpAction << endl;

	if(p->m_personality.compare(""))
	{
		out << "set DoppelgangerReservedTemplate" << " personality \"" << p->m_personality << '"' << endl;
	}


	if(p->m_dropRate.compare(""))
	{
		out << "set DoppelgangerReservedTemplate" << " droprate in " << p->m_dropRate << endl;
	}

	for (uint i = 0; i < p->m_ports.size(); i++)
	{
		// Only include non-inherited ports
		if(!p->m_ports[i].second.first)
		{
			out << "add DoppelgangerReservedTemplate";
			if(!m_ports[p->m_ports[i].first].m_type.compare("TCP"))
			{
				out << " tcp port ";
			}
			else
			{
				out << " udp port ";
			}
			out << m_ports[p->m_ports[i].first].m_portNum << " ";

			if(!(m_ports[p->m_ports[i].first].m_behavior.compare("script")))
			{
				string scriptName = m_ports[p->m_ports[i].first].m_scriptName;

				if(m_scripts[scriptName].m_path.compare(""))
				{
					out << '"' << m_scripts[scriptName].m_path << '"'<< endl;
				}
				else
				{
					LOG(ERROR, "Error writing NodeProfile port script.", "Path to script " + scriptName + " is null.");
				}
			}
			else
			{
				out << m_ports[p->m_ports[i].first].m_behavior << endl;
			}
		}
	}
	out << endl;
	return out.str();
}

//Setter for the directory to read from and write to
void HoneydConfiguration::SetHomePath(std::string homePath)
{
	m_homePath = homePath;
}

//Getter for the directory to read from and write to
std::string HoneydConfiguration::GetHomePath()
{
	return m_homePath;
}



std::vector<std::string> HoneydConfiguration::GetProfileChildren(std::string parent)
{
	vector<std::string> childProfiles;

	for(ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
	{
		if(it->second.m_parentProfile == parent)
		{
			childProfiles.push_back(it->second.m_name);
		}
	}

	return childProfiles;
}

std::vector<std::string> HoneydConfiguration::GetProfileNames()
{
	vector<std::string> childProfiles;

	for(ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
	{
		childProfiles.push_back(it->second.m_name);
	}

	return childProfiles;
}

NodeProfile *HoneydConfiguration::GetProfile(std::string profileName)
{
	if(m_profiles.keyExists(profileName))
	{
		return &m_profiles[profileName];
	}
	return NULL;
}

Port *HoneydConfiguration::GetPort(std::string portName)
{
	if(m_ports.keyExists(portName))
	{
		return &m_ports[portName];
	}
	return NULL;
}

std::vector<std::string> HoneydConfiguration::GetNodeNames()
{
	vector<std::string> childnodes;

	Config::Inst()->SetGroup("main");

	for(NodeTable::iterator it = m_nodes.begin(); it != m_nodes.end(); it++)
	{
		childnodes.push_back(it->second.m_name);
	}

	return childnodes;
}

std::vector<std::string> HoneydConfiguration::GetGeneratedNodeNames()
{
	vector<std::string> childnodes;

	Config::Inst()->SetGroup("HaystackAutoConfig");

	for(NodeTable::iterator it = m_nodes.begin(); it != m_nodes.end(); it++)
	{
		if(m_profiles[it->second.m_pfile].m_generated)
		{
			childnodes.push_back(it->second.m_name);
		}
	}

	return childnodes;
}

std::vector<std::string> HoneydConfiguration::GetSubnetNames()
{
	vector<std::string> childSubnets;

	for(SubnetTable::iterator it = m_subnets.begin(); it != m_subnets.end(); it++)
	{
		childSubnets.push_back(it->second.m_name);
	}

	return childSubnets;
}

std::vector<std::string> HoneydConfiguration::GetScriptNames()
{
	vector<string> ret;
	for(ScriptTable::iterator it = m_scripts.begin(); it != m_scripts.end(); it++)
	{
		ret.push_back(it->first);
	}

	return ret;
}

bool HoneydConfiguration::IsMACUsed(std::string mac)
{
	for(NodeTable::iterator it = m_nodes.begin(); it != m_nodes.end(); it++)
	{
		if(!it->second.m_MAC.compare(mac))
		{
			return true;
		}
	}
	return false;
}

bool HoneydConfiguration::IsIPUsed(std::string ip)
{
	for(NodeTable::iterator it = m_nodes.begin(); it != m_nodes.end(); it++)
	{
		if(!it->second.m_IP.compare(ip) && it->second.m_name.compare(ip))
		{
			return true;
		}
	}
	return false;
}

bool HoneydConfiguration::IsProfileUsed(std::string profileName)
{
	//Find out if any nodes use this profile
	for(NodeTable::iterator it = m_nodes.begin(); it != m_nodes.end(); it++)
	{
		//if we find a node using this profile
		if(!it->second.m_pfile.compare(profileName))
		{
			return true;
		}
	}
	return false;
}

void HoneydConfiguration::UpdateMacAddressesOfProfileNodes(string profileName)
{
	//XXX
}

string HoneydConfiguration::GenerateUniqueMACAddress(string vendor)
{
	string addrStrm;
	do
	{
		addrStrm = m_macAddresses.GenerateRandomMAC(vendor);
	}while(IsMACUsed(addrStrm));

	return addrStrm;
}
//Inserts the profile into the honeyd configuration
//	profile: pointer to the profile you wish to add
//	Returns (true) if the profile could be created, (false) if it cannot.
bool HoneydConfiguration::AddProfile(NodeProfile *profile)
{
	if(!m_profiles.keyExists(profile->m_name))
	{
		m_profiles[profile->m_name] = *profile;
		CreateProfileTree(profile->m_name);
		UpdateProfileTree(profile->m_name, ALL);
		return true;
	}
	return false;
}

bool HoneydConfiguration::AddGroup(std::string groupName)
{
	using boost::property_tree::ptree;

	for(uint i = 0; i < m_groups.size(); i++)
	{
		if(!groupName.compare(m_groups[i]))
		{
			return false;
		}
	}
	m_groups.push_back(groupName);
	try
	{
		ptree newGroup, emptyTree;
		newGroup.clear();
		emptyTree.clear();
		newGroup.put<std::string>("name", groupName);
		newGroup.put_child("subnets", m_subnetTree);
		newGroup.put_child("nodes", emptyTree);
		m_groupTree.add_child("groups.group", newGroup);
	}
	catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
	{
		LOG(ERROR, "Problem adding group ports: " + string(e.what()) + ".", "");
		return false;
	}
	return true;
}

bool HoneydConfiguration::RenameProfile(string oldName, string newName)
{
	//If item text and profile name don't match, we need to update
	if(oldName.compare(newName) && (m_profiles.keyExists(oldName)) && !(m_profiles.keyExists(newName)))
	{
		//Set the profile to the correct name and put the profile in the table
		NodeProfile assign = m_profiles[oldName];
		m_profiles[newName] = assign;
		m_profiles[newName].m_name = newName;

		//Find all nodes who use this profile and update to the new one
		for(NodeTable::iterator it = m_nodes.begin(); it != m_nodes.end(); it++)
		{
			if(!it->second.m_pfile.compare(oldName))
			{
				m_nodes[it->first].m_pfile = newName;
			}
		}

		for(ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
		{
			if(!it->second.m_parentProfile.compare(oldName))
			{
				InheritProfile(it->first, newName);
			}
		}
		UpdateProfileTree(newName, ALL);
		DeleteProfile(oldName);
		return true;
	}
	return false;
}

//Makes the profile named child inherit the profile named parent
// child: the name of the child profile
// parent: the name of the parent profile
// Returns: (true) if successful, (false) if either name could not be found
bool HoneydConfiguration::InheritProfile(std::string child, std::string parent)
{
	//If the child can be found
	if(m_profiles.keyExists(child))
	{
		//If the new parent can be found
		if(m_profiles.keyExists(parent))
		{
			string oldParent = m_profiles[child].m_parentProfile;
			m_profiles[child].m_parentProfile = parent;
			//If the child has an old parent
			if((oldParent.compare("")) && (m_profiles.keyExists(oldParent)))
			{
				UpdateProfileTree(oldParent, ALL);
			}
			//Updates the child with the new inheritance and any modified values since last update
			CreateProfileTree(child);
			UpdateProfileTree(child, ALL);
			return true;
		}
	}
	return false;
}

//Iterates over the profiles, recreating the entire property tree structure
void HoneydConfiguration::UpdateAllProfiles()
{
	for(ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
	{
		//If this is a root node
		if(!it->second.m_parentProfile.compare(""))
		{
			CreateProfileTree(it->first);
			UpdateProfileTree(it->first, DOWN);
		}
	}
}

bool HoneydConfiguration::EnableNode(std::string nodeName)
{
	// Make sure the node exists
	if(!m_nodes.keyExists(nodeName))
	{
		LOG(ERROR, "There was an attempt to delete a honeyd node (name = " + nodeName + " that doesn't exist", "");
		return false;
	}

	m_nodes[nodeName].m_enabled = true;

	// Enable the subnet of this node
	m_subnets[m_nodes[nodeName].m_sub].m_enabled = true;

	return true;
}

bool HoneydConfiguration::DisableNode(std::string nodeName)
{
	// Make sure the node exists
	if(!m_nodes.keyExists(nodeName))
	{
		LOG(ERROR, string("There was an attempt to disable a honeyd node (name = ")
			+ nodeName + string(" that doesn't exist"), "");
		return false;
	}

	m_nodes[nodeName].m_enabled = false;
	return true;
}

bool HoneydConfiguration::DeleteNode(std::string nodeName)
{
	// We don't delete the doppelganger node, only edit it
	if(nodeName == "Doppelganger")
	{
		LOG(WARNING, "Unable to delete the Doppelganger node", "");
		return false;
	}

	// Make sure the node exists
	if(!m_nodes.keyExists(nodeName))
	{
		LOG(ERROR, string("There was an attempt to retrieve a honeyd node (name = ")
			+ nodeName + string(" that doesn't exist"), "");
		return false;
	}

	//Update the Subnet
	Subnet *s = &m_subnets[m_nodes[nodeName].m_sub];
	for(uint i = 0; i < s->m_nodes.size(); i++)
	{
		if(!s->m_nodes[i].compare(nodeName))
		{
			s->m_nodes.erase(s->m_nodes.begin() + i);
		}
	}

	//Delete the node
	m_nodes.erase(nodeName);
	return true;
}

Node *HoneydConfiguration::GetNode(std::string nodeName)
{
	// Make sure the node exists
	if(m_nodes.keyExists(nodeName))
	{
		return &m_nodes[nodeName];
	}
	return NULL;
}

std::string HoneydConfiguration::GetNodeSubnet(std::string nodeName)
{
	// Make sure the node exists
	if(m_nodes.find(nodeName) == m_nodes.end())
	{
		LOG(ERROR, string("There was an attempt to retrieve the subnet of a honeyd node (name = ")
			+ nodeName + string(" that doesn't exist"), "");
		return "";
	}
	return m_nodes[nodeName].m_sub;
}

void HoneydConfiguration::DisableProfileNodes(std::string profileName)
{
	for(NodeTable::iterator it = m_nodes.begin(); it != m_nodes.end(); it++)
	{
		if(!it->second.m_pfile.compare(profileName))
		{
			DisableNode(it->first);
		}
	}
}

//Checks for ports that aren't used and removes them from the table if so
void HoneydConfiguration::CleanPorts()
{
	vector<string> delList;
	bool found;
	for(PortTable::iterator it =m_ports.begin(); it != m_ports.end(); it++)
	{
		found = false;
		for(ProfileTable::iterator jt = m_profiles.begin(); (jt != m_profiles.end()) && !found; jt++)
		{
			for(uint i = 0; (i < jt->second.m_ports.size()) && !found; i++)
			{
				if(!jt->second.m_ports[i].first.compare(it->first))
				{
					found = true;
				}
			}
		}
		if(!found)
		{
			delList.push_back(it->first);
		}
	}
	while(!delList.empty())
	{
		m_ports.erase(delList.back());
		delList.pop_back();
	}
}

ScriptTable HoneydConfiguration::GetScriptTable()
{
	return m_scripts;
}

bool HoneydConfiguration::AddNewNodes(std::string profileName, string ipAddress, std::string interface, std::string subnet, int numberOfNodes)
{
	if(numberOfNodes <= 0)
	{
		LOG(ERROR, "Must create 1 or more nodes", "");
	}

	if(ipAddress == "DHCP")
	{
		for(int i = 0; i < numberOfNodes; i++)
		{
			if(!AddNewNode(profileName, "DHCP", "RANDOM", interface, subnet))
			{
				return false;
			}
		}
	}
	else
	{
		in_addr_t sAddr = ntohl(inet_addr(ipAddress.c_str()));
		//Removes un-init compiler warning given for in_addr currentAddr;
		in_addr currentAddr = *(in_addr *)&sAddr;
		for(int i = 0; i < numberOfNodes; i++)
		{
			currentAddr.s_addr = htonl(sAddr);
			if(!AddNewNode(profileName, string(inet_ntoa(currentAddr)), "RANDOM", interface, subnet))
			{
				return false;
			}
			sAddr++;
		}
	}
	return true;
}

bool HoneydConfiguration::AddNewNode(Node node)
{
	Node newNode = node;

	newNode.m_interface = "eth0";


	if(newNode.m_IP != "DHCP")
	{
		newNode.m_realIP = htonl(inet_addr(newNode.m_IP.c_str()));
	}

	// Figure out it's subnet
	if(newNode.m_sub == "")
	{
		if(newNode.m_IP == "DHCP")
		{
			newNode.m_sub = newNode.m_interface;
		}
		else
		{
			newNode.m_sub = FindSubnet(newNode.m_realIP);

			if(newNode.m_sub == "")
			{
				return false;
			}
		}
	}

	newNode.m_enabled = true;

	newNode.m_name = newNode.m_IP + " - " + newNode.m_MAC;

	cout << "Adding new node '" << newNode.m_name << "'."<< endl;

	m_profiles[newNode.m_pfile].m_nodeKeys.push_back(newNode.m_name);

	uint j = ~0;
	stringstream ss;
	if(newNode.m_name == "DHCP - RANDOM")
	{
		//Finds a unique identifier
		uint i = 1;
		while((m_nodes.keyExists(newNode.m_name)) && (i < j))
		{
			i++;
			ss.str("");
			ss << "DHCP - RANDOM(" << i << ")";
			newNode.m_name = ss.str();
		}
	}

	m_nodes[newNode.m_name] = newNode;

	if(newNode.m_sub != "")
	{
		m_subnets[newNode.m_sub].m_nodes.push_back(newNode.m_name);
	}
	else
	{
		LOG(WARNING, "No subnet was set for new node. This could make certain features unstable", "");
	}

	//TODO add error checking
	return true;
}

bool HoneydConfiguration::AddNewNode(std::string profileName, string ipAddress, std::string macAddress, std::string interface, std::string subnet)
{
	Node newNode;
	newNode.m_IP = ipAddress;
	newNode.m_interface = interface;

	if(newNode.m_IP != "DHCP")
	{
		newNode.m_realIP = htonl(inet_addr(newNode.m_IP.c_str()));
	}

	// Figure out it's subnet
	if(subnet == "")
	{
		if(newNode.m_IP == "DHCP")
		{
			newNode.m_sub = newNode.m_interface;
		}
		else
		{
			newNode.m_sub = FindSubnet(newNode.m_realIP);

			if(newNode.m_sub == "")
			{
				return false;
			}
		}
	}
	else
	{
		newNode.m_sub = subnet;
	}


	newNode.m_pfile = profileName;
	newNode.m_enabled = true;
	newNode.m_MAC = macAddress;

	newNode.m_name = newNode.m_IP + " - " + newNode.m_MAC;
	cout << "Adding new node '" << newNode.m_name << "'."<< endl;

	m_profiles[newNode.m_pfile].m_nodeKeys.push_back(newNode.m_name);

	uint j = ~0;
	stringstream ss;
	if(newNode.m_name == "DHCP - RANDOM")
	{
		//Finds a unique identifier
		uint i = 1;
		while((m_nodes.keyExists(newNode.m_name)) && (i < j))
		{
			i++;
			ss.str("");
			ss << "DHCP - RANDOM(" << i << ")";
			newNode.m_name = ss.str();
		}
	}

	m_nodes[newNode.m_name] = newNode;
	if(newNode.m_sub != "")
	{
		m_subnets[newNode.m_sub].m_nodes.push_back(newNode.m_name);
	}
	else
	{
		LOG(WARNING, "No subnet was set for new node. This could make certain features unstable", "");
	}

	//TODO add error checking
	return true;
}

bool HoneydConfiguration::AddSubnet(const Subnet &add)
{
	if(m_subnets.find(add.m_name) != m_subnets.end())
	{
		return false;
	}
	else
	{
		m_subnets[add.m_name] = add;
		return true;
	}
}

//Removes a profile and all associated nodes from the Honeyd configuration
//	profileName: name of the profile you wish to delete
//	originalCall: used internally to designate the recursion's base condition, can old be set with
//		private access. Behavior is undefined if the first DeleteProfile call has originalCall == false
// 	Returns: (true) if successful and (false) if the profile could not be found
bool HoneydConfiguration::DeleteProfile(std::string profileName, bool originalCall)
{
	if(!m_profiles.keyExists(profileName))
	{
		LOG(DEBUG, "Attempted to delete profile that does not exist", "");
		return false;
	}
	//Recursive descent to find and call delete on any children of the profile
	for(ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
	{
		//If the profile at the iterator is a child of this profile
		if(!it->second.m_parentProfile.compare(profileName))
		{
			if(!DeleteProfile(it->first, false))
			{
				return false;
			}
		}
	}
	NodeProfile p = m_profiles[profileName];

	//Delete any nodes using the profile
	vector<string> delList;
	for(NodeTable::iterator it = m_nodes.begin(); it != m_nodes.end(); it++)
	{
		if(!it->second.m_pfile.compare(p.m_name))
		{
			delList.push_back(it->second.m_name);
		}
	}
	while(!delList.empty())
	{
		if(!DeleteNode(delList.back()))
		{
			LOG(DEBUG, "Failed to delete profile because child node deletion failed", "");
			return false;
		}
		delList.pop_back();
	}
	m_profiles.erase(profileName);

	//If it is not the original profile to be deleted skip this part
	if(originalCall)
	{
		//If this profile has a parent
		if(m_profiles.keyExists(p.m_parentProfile))
		{
			//save a copy of the parent
			NodeProfile parent = m_profiles[p.m_parentProfile];

			//point to the profiles subtree of parent-copy ptree and clear it
			ptree *pt = &parent.m_tree.get_child("profiles");
			pt->clear();

			//Find all profiles still in the table that are sibilings of deleted profile
			// We should be using an iterator to find the original profile and erase it
			// but boost's iterator implementation doesn't seem to be able to access data
			// correctly and are frequently invalidated.

			for(ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
			{
				if(!it->second.m_parentProfile.compare(parent.m_name))
				{
					//Put sibiling profiles into the tree
					pt->add_child("profile", it->second.m_tree);
				}
			}	//parent-copy now has the ptree of all children except deleted profile

			//point to the original parent's profiles subtree and replace it with our new ptree
			ptree *treePtr = &m_profiles[p.m_parentProfile].m_tree.get_child("profiles");
			treePtr->clear();
			*treePtr = *pt;

			//Updates all ancestors with the deletion
			UpdateProfileTree(p.m_parentProfile, ALL);
		}
		else
		{
			LOG(ERROR, string("Parent profile with name: ") + p.m_parentProfile + string(" doesn't exist"), "");
		}
	}

	LOG(DEBUG, "Deleted profile " + profileName, "");
	return true;
}

//Recreates the profile tree of ancestors, children or both
//	Note: This needs to be called after making changes to a profile to update the hierarchy
//	Returns (true) if successful and (false) if no profile with name 'profileName' exists
bool HoneydConfiguration::UpdateProfileTree(string profileName, recursiveDirection direction)
{
	if(!m_profiles.keyExists(profileName))
	{
		return false;
	}
	else if(m_profiles[profileName].m_name.compare(profileName))
	{
		LOG(DEBUG, "Profile key: " + profileName + " does not match profile name of: "
			+ m_profiles[profileName].m_name + ". Setting profile name to the value of profile key.", "");
			m_profiles[profileName].m_name = profileName;
	}
	//Copy the profile
	NodeProfile p = m_profiles[profileName];
	bool up = false, down = false;
	switch(direction)
	{
		case UP:
		{
			up = true;
			break;
		}
		case DOWN:
		{
			down = true;
			break;
		}
		case ALL:
		default:
		{
			up = true;
			down = true;
			break;
		}
	}
	if(down)
	{
		//Find all children
		for(ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
		{
			//If child is found
			if(!it->second.m_parentProfile.compare(p.m_name))
			{
				CreateProfileTree(it->second.m_name);
				//Update the child
				UpdateProfileTree(it->second.m_name, DOWN);
				//Put the child in the parent's ptree
				p.m_tree.add_child("profiles.profile", it->second.m_tree);
			}
		}
		m_profiles[profileName] = p;
	}
	//If the original calling profile has a parent to update
	if(p.m_parentProfile.compare("") && up)
	{
		//Get the parents name and create an empty ptree
		NodeProfile parent = m_profiles[p.m_parentProfile];
		ptree pt;
		pt.clear();
		pt.add_child("profile", p.m_tree);

		//Find all children of the parent and put them in the empty ptree
		// Ideally we could just replace the individual child but the data structure doesn't seem
		// to support this very well when all keys in the ptree (ie. profiles.profile) are the same
		// because the ptree iterators just don't seem to work correctly and documentation is very poor
		for(ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
		{
			if(!it->second.m_parentProfile.compare(parent.m_name))
			{
				pt.add_child("profile", it->second.m_tree);
			}
		}
		//Replace the parent's profiles subtree (stores all children) with the new one
		// XXX There's a segfault happening here; only saw it when there was more than one subtree of default
		// that was found during scans. Goes through Linux subtree fine, hits the Windows subtree, gets the the
		// point where profileName is "Windows 7 general purpose" (which is good) and then SIGSEGV here. Might
		// be running out of memory, as it's not giving a bad tree path or data or anything.
		parent.m_tree.put_child("profiles", pt);
		m_profiles[parent.m_name] = parent;
		//Recursively ascend to update all ancestors
		CreateProfileTree(parent.m_name);
		UpdateProfileTree(parent.m_name, UP);
	}
	return true;
}

//Creates a ptree for a profile from scratch using the values found in the table
//	name: the name of the profile you wish to create a new tree for
//	Note: this only creates a leaf-node profile tree, after this call it will have no children.
//		to put the children back into the tree and place the this new tree into the parent's hierarchy
//		you must first call UpdateProfileTree(name, ALL);
//	Returns (true) if successful and (false) if no profile with name 'profileName' exists
bool HoneydConfiguration::CreateProfileTree(string profileName)
{
	ptree temp;
	if(!m_profiles.keyExists(profileName))
	{
		return false;
	}
	NodeProfile p = m_profiles[profileName];
	if(p.m_name.compare(""))
	{
		temp.put<std::string>("name", p.m_name);
	}

	temp.put<bool>("generated", p.m_generated);

	if(p.m_tcpAction.compare("") && !p.m_inherited[TCP_ACTION])
	{
		temp.put<std::string>("set.TCP", p.m_tcpAction);
	}
	if(p.m_udpAction.compare("") && !p.m_inherited[UDP_ACTION])
	{
		temp.put<std::string>("set.UDP", p.m_udpAction);
	}
	if(p.m_icmpAction.compare("") && !p.m_inherited[ICMP_ACTION])
	{
		temp.put<std::string>("set.ICMP", p.m_icmpAction);
	}
	if(p.m_personality.compare("") && !p.m_inherited[PERSONALITY])
	{
		temp.put<std::string>("set.personality", p.m_personality);
	}
	if(!p.m_inherited[ETHERNET])
	{
		for(uint i = 0; i < p.m_ethernetVendors.size(); i++)
		{
			ptree ethTemp;
			ethTemp.put<std::string>("vendor", p.m_ethernetVendors[i].first);
			ethTemp.put<double>("distribution", p.m_ethernetVendors[i].second);
			temp.add_child("set.ethernet", ethTemp);
		}
	}
	if(p.m_uptimeMin.compare("") && !p.m_inherited[UPTIME])
	{
		temp.put<std::string>("set.uptimeMin", p.m_uptimeMin);
	}
	if(p.m_uptimeMax.compare("") && !p.m_inherited[UPTIME])
	{
		temp.put<std::string>("set.uptimeMax", p.m_uptimeMax);
	}
	if(p.m_dropRate.compare("") && !p.m_inherited[DROP_RATE])
	{
		temp.put<std::string>("set.dropRate", p.m_dropRate);
	}

	//Populates the ports, if none are found create an empty field because it is expected.
	ptree pt;
	if(p.m_ports.size())
	{
		temp.put_child("add.ports",pt);
		for(uint i = 0; i < p.m_ports.size(); i++)
		{
			//If the port isn't inherited
			if(!p.m_ports[i].second.first)
			{
				temp.add<std::string>("add.ports.port", p.m_ports[i].first);
			}
		}
	}
	else
	{
		temp.put_child("add.ports",pt);
	}
	//put empty ptree in profiles as well because it is expected, does not matter that it is the same
	// as the one in add.m_ports if profile has no ports, since both are empty.
	temp.put_child("profiles", pt);

	for(ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
	{
		if(!it->second.m_parentProfile.compare(profileName))
		{
			temp.add_child("profiles.profile", it->second.m_tree);
		}
	}

	//copy the tree over and update ancestors
	p.m_tree = temp;
	m_profiles[profileName] = p;
	return true;
}
}


