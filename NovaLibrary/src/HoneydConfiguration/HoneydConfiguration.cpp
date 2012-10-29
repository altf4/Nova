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
#include "../NovaUtil.h"
#include "../Logger.h"

#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <arpa/inet.h>
#include <math.h>
#include <ctype.h>
#include <netdb.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sstream>
#include <fstream>

using namespace std;
using namespace Nova;
using boost::property_tree::ptree;
using boost::property_tree::xml_parser::trim_whitespace;

namespace Nova
{

HoneydConfiguration *HoneydConfiguration::m_instance = NULL;

HoneydConfiguration *HoneydConfiguration::Inst()
{
	if(m_instance == NULL)
	{
		m_instance = new HoneydConfiguration();
	}
	return m_instance;
}

//Basic constructor for the Honeyd Configuration object
// Initializes the MAC vendor database and hash tables
// *Note: To populate the object from the file system you must call LoadAllTemplates();
HoneydConfiguration::HoneydConfiguration()
{
	m_macAddresses.LoadPrefixFile();
	m_scripts.set_empty_key("");
	m_scripts.set_deleted_key("Deleted");
}

bool HoneydConfiguration::ReadAllTemplatesXML()
{
	bool totalSuccess = true;

	if(!ReadScriptsXML())
	{
		totalSuccess = false;
	}
	if(!ReadNodesXML())
	{
		totalSuccess = false;
	}
	if(!ReadProfilesXML())
	{
		totalSuccess = false;
	}

	return totalSuccess;
}

//Loads NodeProfiles from the xml template located relative to the currently set home path
// Returns true if successful, false if not.
bool HoneydConfiguration::ReadProfilesXML()
{
	using boost::property_tree::ptree;
	using boost::property_tree::xml_parser::trim_whitespace;
	m_profileTree.clear();
	try
	{
		read_xml(Config::Inst()->GetPathHome() + "/config/templates/profiles.xml", m_profileTree, boost::property_tree::xml_parser::trim_whitespace);

		m_profiles.m_root = ReadProfilesXML_helper(m_profileTree, NULL);

		return true;
	}
	catch (boost::property_tree::xml_parser_error &e) {
		LOG(ERROR, "Problem loading profiles: " + string(e.what()) + ".", "");
		return false;
	}
	catch (boost::property_tree::ptree_error &e)
	{
		LOG(ERROR, "Problem loading profiles: " + string(e.what()) + ".", "");
		return false;
	}
	return false;
}

PersonalityTreeItem *HoneydConfiguration::ReadProfilesXML_helper(ptree &ptree, PersonalityTreeItem *parent)
{
	PersonalityTreeItem *personality = NULL;

	BOOST_FOREACH(ptree::value_type &profilePtree, ptree.get_child("profiles"))
	{
		//Must be a "profile" tag, or else error
		if(!string(profilePtree.first.data()).compare("profile"))
		{
			personality = new PersonalityTreeItem();
			try
			{
				personality->m_parent = parent;
				personality->m_isGenerated = profilePtree.second.get<bool>("generated");
				personality->m_distribution = profilePtree.second.get<double>("distribution");
				personality->m_key = profilePtree.second.get<string>("name");
				personality->m_osclass = profilePtree.second.get<string>("personality");
				personality->m_uptimeMin = profilePtree.second.get<uint>("uptimeMin");
				personality->m_uptimeMax = profilePtree.second.get<uint>("uptimeMax");

				//Ethernet Settings
				BOOST_FOREACH(ptree::value_type &ethernetVendors, profilePtree.second.get_child("ethernet"))
				{
					try
					{
						string vendorName = ethernetVendors.second.get<string>("vendor");
						double distribution = ethernetVendors.second.get<double>("distribution");

						personality->m_vendors.push_back(pair<string, double>(vendorName, distribution));
					}
					catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
					{
						LOG(ERROR, "Unable to parse required values for the NodeProfiles!", "");
					};
				}

				//Others
				//TODO

				//Port Sets
				BOOST_FOREACH(ptree::value_type &portsets, profilePtree.second.get_child("portsets"))
				{
					if(!string(portsets.first.data()).compare("portset"))
					{
						PortSet *portSet = new PortSet();
						portSet->m_name = portsets.second.get<string>("name");

						portSet->m_defaultTCPBehavior = StringToPortBehavior(portsets.second.get<string>("defaultTCPBehavior"));
						portSet->m_defaultUDPBehavior = StringToPortBehavior(portsets.second.get<string>("defaultUDPBehavior"));
						portSet->m_defaultICMPBehavior = StringToPortBehavior(portsets.second.get<string>("defaultICMPBehavior"));

						//Exceptions
						BOOST_FOREACH(ptree::value_type &ports, portsets.second.get_child("ports"))
						{
							Port port;

							port.m_name = ports.second.get<string>("name");
							port.m_service = ports.second.get<string>("service");
							port.m_scriptName = ports.second.get<string>("script");
							port.m_portNumber = ports.second.get<uint>("number");
							port.m_behavior = StringToPortBehavior(ports.second.get<string>("behavior"));
							port.m_protocol = StringToPortProtocol(ports.second.get<string>("protocol"));

							portSet->AddPort(port);
						}

						personality->m_portSets.push_back(portSet);
					}
				}

				//Recursively add children
				BOOST_FOREACH(ptree::value_type &children, profilePtree.second.get_child("profiles"))
				{
					PersonalityTreeItem *child = ReadProfilesXML_helper(children.second, personality);
					if(child != NULL)
					{
						personality->m_children.push_back(child);
					}
				}

			}
			catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
			{
				LOG(ERROR, "Unable to parse required values for the NodeProfiles!", "");
				delete personality;
				return NULL;
			};

		}
		else
		{
			LOG(ERROR, "Invalid XML Path " + string(profilePtree.first.data()) + ".", "");
		}
	}

	return personality;
}

//Loads Nodes from the xml template located relative to the currently set home path
// Returns true if successful, false if not.
bool HoneydConfiguration::ReadNodesXML()
{
	using boost::property_tree::ptree;
	using boost::property_tree::xml_parser::trim_whitespace;

	m_groupTree.clear();
	ptree propTree;

	try
	{
		read_xml(Config::Inst()->GetPathHome() + "/config/templates/nodes.xml", m_groupTree, boost::property_tree::xml_parser::trim_whitespace);

		BOOST_FOREACH(ptree::value_type &groupsPtree, m_groupTree.get_child("groups"))
		{
			string groupName = groupsPtree.second.get<string>("name");
			NodeTable table;

			//For each node tag
			BOOST_FOREACH(ptree::value_type &nodesPtree, m_groupTree.get_child("nodes"))
			{
				if(!nodesPtree.first.compare("node"))
				{
					Node node;
					node.m_interface = nodesPtree.second.get<string>("interface");
					node.m_IP = nodesPtree.second.get<string>("IP");
					node.m_enabled = nodesPtree.second.get<bool>("enabled");
					node.m_MAC = nodesPtree.second.get<string>("MAC");

					node.m_pfile = nodesPtree.second.get<string>("profile.name");
					node.m_portSetName = nodesPtree.second.get<string>("profile.portset");

					table[node.m_MAC] = node;
				}
			}

			m_nodes.push_back(pair<string, NodeTable>(groupName, table));
		}
	}
	catch(Nova::hashMapException &e)
	{
		LOG(ERROR, "Problem loading node group: " + Config::Inst()->GetGroup() + " - " + string(e.what()) + ".", "");
		return false;
	}
	catch (boost::property_tree::xml_parser_error &e) {
		LOG(ERROR, "Problem loading nodes: " + string(e.what()) + ".", "");
		return false;
	}
	catch (boost::property_tree::ptree_error &e)
	{
		LOG(ERROR, "Problem loading nodes: " + string(e.what()) + ".", "");
		return false;
	}
	return true;
}

//Loads scripts from the xml template located relative to the currently set home path
// Returns true if successful, false if not.
bool HoneydConfiguration::ReadScriptsXML()
{
	using boost::property_tree::ptree;
	using boost::property_tree::xml_parser::trim_whitespace;
	m_scriptTree.clear();
	try
	{
		read_xml(Config::Inst()->GetPathHome() + "/config/templates/scripts.xml", m_scriptTree, boost::property_tree::xml_parser::trim_whitespace);

		BOOST_FOREACH(ptree::value_type &value, m_scriptTree.get_child("scripts"))
		{
			Script script;
			script.m_tree = value.second;
			//Each script consists of a name and path to that script
			script.m_name = value.second.get<string>("name");

			if(!script.m_name.compare(""))
			{
				LOG(ERROR, "Unable to a valid script from the templates!", "");
				return false;
			}

			try
			{
				script.m_osclass = value.second.get<string>("osclass");
			}
			catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
			{
				LOG(DEBUG, "No OS class found for script '" + script.m_name + "'.", "");
			};
			try
			{
				script.m_service = value.second.get<string>("service");
			}
			catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
			{
				LOG(DEBUG, "No service found for script '" + script.m_name + "'.", "");
			};
			script.m_path = value.second.get<string>("path");
			m_scripts[script.m_name] = script;
		}
	}
	catch(Nova::hashMapException &e)
	{
		LOG(ERROR, "Problem loading scripts: " + string(e.what()) + ".", "");
		return false;
	}
	catch (boost::property_tree::xml_parser_error &e) {
		LOG(ERROR, "Problem loading scripts: " + string(e.what()) + ".", "");
		return false;
	}
	catch (boost::property_tree::ptree_error &e)
	{
		LOG(ERROR, "Problem loading scripts: " + string(e.what()) + ".", "");
		return false;
	}
	return true;
}

bool HoneydConfiguration::WriteScriptsToXML()
{
	ptree propTree;

	//Scripts
	m_scriptTree.clear();
	for(ScriptTable::iterator it = m_scripts.begin(); it != m_scripts.end(); it++)
	{
		propTree = it->second.m_tree;
		propTree.put<string>("name", it->second.m_name);
		propTree.put<string>("service", it->second.m_service);
		propTree.put<string>("osclass", it->second.m_osclass);
		propTree.put<string>("path", it->second.m_path);
		m_scriptTree.add_child("scripts.script", propTree);
	}

	try
	{
		boost::property_tree::xml_writer_settings<char> settings('\t', 1);
		string homePath = Config::Inst()->GetPathHome();
		write_xml(homePath + "/config/templates/scripts.xml", m_scriptTree, locale(), settings);
	}
	catch(boost::property_tree::xml_parser_error &e)
	{
		LOG(ERROR, "Unable to write to xml files, caught exception " + string(e.what()), "");
		return false;
	}
	return true;
}

bool HoneydConfiguration::WriteNodesToXML()
{
	//Nodes
	m_nodesTree.clear();

	ptree groups;

	for(uint i = 0; i < m_nodes.size(); i++)
	{
		for(NodeTable::iterator it = m_nodes[i].second.begin(); it != m_nodes[i].second.end(); it++)
		{
			ptree nodePtree;

			nodePtree.put<string>("interface", it->second.m_interface);
			nodePtree.put<string>("IP", it->second.m_IP);
			nodePtree.put<bool>("enabled", it->second.m_enabled);
			nodePtree.put<string>("MAC", it->second.m_MAC);

			nodePtree.put<string>("profile.name", it->second.m_pfile);
			nodePtree.put<string>("profile.portset", it->second.m_portSetName);

			groups.add_child("node", nodePtree);
		}
	}

	m_nodesTree.add_child("groups.group", groups);

	//Actually write out to file
	try
	{
		boost::property_tree::xml_writer_settings<char> settings('\t', 1);
		string homePath = Config::Inst()->GetPathHome();
		write_xml(homePath + "/config/templates/nodes.xml", m_nodesTree, locale(), settings);
	}
	catch(boost::property_tree::xml_parser_error &e)
	{
		LOG(ERROR, "Unable to write to xml files, caught exception " + string(e.what()), "");
		return false;
	}
	return true;
}

bool HoneydConfiguration::WriteProfilesToXML()
{
	m_profileTree.clear();

	if(WriteProfilesToXML_helper(m_profiles.m_root))
	{
		try
		{
			boost::property_tree::xml_writer_settings<char> settings('\t', 1);
			string homePath = Config::Inst()->GetPathHome();
			write_xml(homePath + "/config/templates/profiles.xml", m_profileTree, locale(), settings);
		}
		catch(boost::property_tree::xml_parser_error &e)
		{
			LOG(ERROR, "Unable to write to xml files, caught exception " + string(e.what()), "");
			return false;
		}
		return true;
	}
	else
	{
		return false;
	}
}

//TODO: keep an eye on this, maybe it works?
bool HoneydConfiguration::WriteProfilesToXML_helper(PersonalityTreeItem *root)
{
	if(root == NULL)
	{
		return false;
	}

	ptree propTree;
	m_profileTree.clear();

	//Write the current item into the tree
	propTree.put<string>("name", root->m_key);
	propTree.put<bool>("generated", root->m_isGenerated);
	propTree.put<double>("distribution", root->m_distribution);
	propTree.put<string>("personality", root->m_osclass);
	propTree.put<uint>("uptimeMin", root->m_uptimeMin);
	propTree.put<uint>("uptimeMax", root->m_uptimeMax);

	//Ethernet settings
	for(uint i = 0; i < root->m_vendors.size(); i++)
	{
		ptree ethVendors;

		ethVendors.put<string>("vendor", root->m_vendors[i].first);
		ethVendors.put<double>("distribution", root->m_vendors[i].second);


		propTree.add_child("profile.ethernet", ethVendors);

	}

	//Describe what port sets are available for this profile
	{
		ptree portSets;

		for(uint i = 0; i < root->m_portSets.size(); i++)
		{
			ptree portSet;

			portSet.put<string>("name", root->m_portSets[i]->m_name);

			portSet.put<string>("defaultTCPBehavior", PortBehaviorToString(root->m_portSets[i]->m_defaultTCPBehavior));
			portSet.put<string>("defaultUDPBehavior", PortBehaviorToString(root->m_portSets[i]->m_defaultUDPBehavior));
			portSet.put<string>("defaultICMPBehavior", PortBehaviorToString(root->m_portSets[i]->m_defaultICMPBehavior));

			//Foreach TCP exception
			for(uint j = 0; j < root->m_portSets[i]->m_TCPexceptions.size(); j++)
			{
				//Make a sub-tree for this Port
				ptree port;

				port.put<string>("name", root->m_portSets[i]->m_TCPexceptions[j].m_name);
				port.put<string>("service", root->m_portSets[i]->m_TCPexceptions[j].m_service);
				port.put<string>("script", root->m_portSets[i]->m_TCPexceptions[j].m_scriptName);
				port.put<uint>("number", root->m_portSets[i]->m_TCPexceptions[j].m_portNumber);
				port.put<string>("behavior", PortBehaviorToString(root->m_portSets[i]->m_TCPexceptions[j].m_behavior));
				port.put<string>("protocol", PortProtocolToString(root->m_portSets[i]->m_TCPexceptions[j].m_protocol));

				portSet.add_child("portsets.portset.port", port);
			}

			//Foreach UDP exception
			for(uint j = 0; j < root->m_portSets[i]->m_UDPexceptions.size(); j++)
			{
				//Make a sub-tree for this Port
				ptree port;

				port.put<string>("name", root->m_portSets[i]->m_UDPexceptions[j].m_name);
				port.put<string>("service", root->m_portSets[i]->m_UDPexceptions[j].m_service);
				port.put<string>("script", root->m_portSets[i]->m_UDPexceptions[j].m_scriptName);
				port.put<uint>("number", root->m_portSets[i]->m_UDPexceptions[j].m_portNumber);
				port.put<string>("behavior", PortBehaviorToString(root->m_portSets[i]->m_UDPexceptions[j].m_behavior));
				port.put<string>("protocol", PortProtocolToString(root->m_portSets[i]->m_UDPexceptions[j].m_protocol));

				portSet.add_child("portsets.portset.port", port);
			}

			portSets.add_child("ports.portset", portSet);
		}

		propTree.add_child("profile.portsets",portSets);
	}


	//Then write all of its children
	for(uint i = 0; i < root->m_children.size(); i++)
	{
		WriteProfilesToXML_helper(root->m_children[i]);
	}

	m_profileTree.add_child("profiles.profile",propTree);


	return true;
}

bool HoneydConfiguration::WriteAllTemplatesToXML()
{
	bool totalSuccess = true;

	if(!WriteScriptsToXML())
	{
		totalSuccess = false;
	}
	if(!WriteNodesToXML())
	{
		totalSuccess = false;
	}
	if(!WriteProfilesToXML())
	{
		totalSuccess = false;
	}

	return totalSuccess;
}

bool HoneydConfiguration::WriteHoneydConfiguration(string groupName, string path)
{
	if(!path.compare(""))
	{
		if(!Config::Inst()->GetPathConfigHoneydHS().compare(""))
		{
			LOG(ERROR, "Invalid path given to Honeyd configuration file!", "");
			return false;
		}
		path = Config::Inst()->GetPathHome() + "/" + Config::Inst()->GetPathConfigHoneydHS();
	}

	LOG(DEBUG, "Writing honeyd configuration to " + path, "");

	stringstream out;

	for(uint i = 0; i < m_nodes.size(); i++)
	{
		for(NodeTable::iterator it = m_nodes[i].second.begin(); it != m_nodes.end(); it++)
		{
			//Only write out nodes for the intended group
			if(!groupName.compare(m_nodes[i].first))
			{

			}
		}
	}


	// Start node section
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
			if(!pString.compare(""))
			{
				LOG(ERROR, "Unable to convert expected profile '" + it->second.m_pfile + "' into a valid Honeyd configuration string!", "");
				return false;
			}
			out << '\n' << pString;
			out << "bind " << it->second.m_IP << " DoppelgangerReservedTemplate" << '\n' << '\n';
			//Use configured or discovered loopback
		}
		else
		{
			string profName = HoneydConfiguration::SanitizeProfileName(it->second.m_pfile);

			//Clone a custom profile for a node
			out << "clone CustomNodeProfile-" << m_nodeProfileIndex << " " << profName << '\n';

			//Add any custom port settings
			for(uint i = 0; i < it->second.m_ports.size(); i++)
			{
				//Only write out ports that aren't inherited from parent profiles
				if(!it->second.m_isPortInherited[i])
				{
					PortStruct prt = m_ports[it->second.m_ports[i]];
					//Skip past invalid port objects
					if(!(prt.m_type.compare("")) || !(prt.m_portNum.compare("")) || !(prt.m_behavior.compare("")))
					{
						continue;
					}

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

					if(!prt.m_behavior.compare("script") || !prt.m_behavior.compare("tarpit script"))
					{
						string scriptName = prt.m_scriptName;

						if (!prt.m_behavior.compare("tarpit script"))
						{
							cout << "tarpit ";
						}
						if(m_scripts[scriptName].m_path.compare(""))
						{
							out << '"' << m_scripts[scriptName].m_path << '"'<< '\n';
						}
						else
						{
							LOG(ERROR, "Error writing node port script.", "Path to script " + scriptName + " is null.");
						}
					}
					else
					{
						out << prt.m_behavior << '\n';
					}
				}
			}

			//If DHCP
			if(!it->second.m_IP.compare("DHCP"))
			{
				//Wrtie dhcp line
				out << "dhcp CustomNodeProfile-" << m_nodeProfileIndex << " on " << it->second.m_interface;
				//If the node has a MAC address (not random generated)
				if(it->second.m_MAC.compare("RANDOM"))
				{
					out << " ethernet \"" << it->second.m_MAC << "\"";
				}
				out << '\n';
			}
			//If static IP
			else
			{
				//If the node has a MAC address (not random generated)
				if(it->second.m_MAC.compare("RANDOM"))
				{
					//Set the MAC for the custom node profile
					out << "set " << "CustomNodeProfile-" << m_nodeProfileIndex;
					out << " ethernet \"" << it->second.m_MAC << "\"" << '\n';
				}
				//bind the node to the IP address
				out << "bind " << it->second.m_IP << " CustomNodeProfile-" << m_nodeProfileIndex << '\n';
			}
		}
	}
	ofstream outFile(path);
	outFile << out.str() << '\n';
	outFile.close();
	return true;
}

// This function takes in the raw byte form of a network mask and converts it to the number of bits
// 	used when specifiying a subnet in the dots and slash notation. ie. 192.168.1.1/24
// 	mask: The raw numerical form of the netmask ie. 255.255.255.0 -> 0xFFFFFF00
// Returns an int equal to the number of bits that are 1 in the netmask, ie the example value for mask returns 24
int HoneydConfiguration::GetMaskBits(in_addr_t mask)
{
	mask = ~mask;
	int i = 32;
	while(mask != 0)
	{
		if((mask % 2) != 1)
		{
			LOG(ERROR, "Invalid mask passed in as a parameter!", "");
			return -1;
		}
		mask = mask/2;
		i--;
	}
	return i;
}

bool HoneydConfiguration::AddNewNode(string profileName, string ipAddress, string macAddress,
		string interface, PortSet *portSet)
{
	Node newNode;
	uint macPrefix = m_macAddresses.AtoMACPrefix(macAddress);
	string vendor = m_macAddresses.LookupVendor(macPrefix);

	//Finish populating the node
	newNode.m_interface = interface;
	newNode.m_pfile = profileName;
	newNode.m_enabled = true;

	//Check the IP  and MAC address
	if(ipAddress.compare("DHCP"))
	{
		//Lookup the mac vendor to assert a valid mac
		if(!m_macAddresses.IsVendorValid(vendor))
		{
			LOG(WARNING, "Invalid MAC string '" + macAddress + "' given!", "");
		}

		uint retVal = inet_addr(ipAddress.c_str());
		if(retVal == INADDR_NONE)
		{
			LOG(ERROR, "Invalid node IP address '" + ipAddress + "' given!", "");
			return false;
		}
		newNode.m_realIP = htonl(retVal);
	}

	//Get the name after assigning the values
	newNode.m_MAC = macAddress;
	newNode.m_IP = ipAddress;
	newNode.m_name = newNode.m_IP + " - " + newNode.m_MAC;

	//Make sure we have a unique identifier
	uint j = ~0;
	stringstream ss;
	if(!newNode.m_name.compare("DHCP - RANDOM"))
	{
		uint i = 1;
		while((m_nodes.keyExists(newNode.m_name)) && (i < j))
		{
			i++;
			ss.str("");
			ss << "DHCP - RANDOM(" << i << ")";
			newNode.m_name = ss.str();
		}
	}
	if(m_nodes.keyExists(newNode.m_name))
	{
		LOG(ERROR, "Unable to generate valid identifier for new node!", "");
		return false;
	}

	//Check for a valid interface
	vector<string> interfaces = Config::Inst()->GetInterfaces();
	if(interfaces.empty())
	{
		LOG(ERROR, "No interfaces specified for node creation!", "");
		return false;
	}
	//Iterate over the interface list and try to find one.
	for(uint i = 0; i < interfaces.size(); i++)
	{
		if(!interfaces[i].compare(newNode.m_interface))
		{
			break;
		}
		else if((i + 1) == interfaces.size())
		{
			LOG(WARNING, "No interface '" + newNode.m_interface + "' detected! Using interface '" + interfaces[0] + "' instead.", "");
			newNode.m_interface = interfaces[0];
		}
	}

	//Check validity of NodeProfile
	NodeProfile *p = &m_profiles[profileName];
	if(p == NULL)
	{
		LOG(ERROR, "Unable to find expected NodeProfile '" + profileName + "'.", "");
		return false;
	}

	//Add ports to the NodeProfile
	if(portSet != NULL)
	{
		for(uint i = 0; i < portSet->m_TCPexceptions.size(); i++)
		{
			newNode.m_ports.push_back(portSet->m_TCPexceptions[i].m_name);
			newNode.m_isPortInherited.push_back(false);
		}
		for(uint i = 0; i < portSet->m_UDPexceptions.size(); i++)
		{
			newNode.m_ports.push_back(p->m_ports[i].first);
			newNode.m_isPortInherited.push_back(false);
		}
	}

	//Assign all the values
	p->m_nodeKeys.push_back(newNode.m_name);
	m_nodes[newNode.m_name] = newNode;

	LOG(DEBUG, "Added new node '" + newNode.m_name + "'.", "");

	return true;
}

//This function allows easy access to all profiles
// Returns a vector of strings containing the names of all profiles
vector<string> HoneydConfiguration::GetProfileNames()
{
	vector<string> childProfiles;
	for(ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
	{
		childProfiles.push_back(it->first);
	}
	return childProfiles;
}

//This function allows easy access to all nodes
// Returns a vector of strings containing the names of all nodes
vector<string> HoneydConfiguration::GetNodeNames()
{
	vector<string> nodeNames;
	for(NodeTable::iterator it = m_nodes.begin(); it != m_nodes.end(); it++)
	{
		nodeNames.push_back(it->second.m_name);
	}
	return nodeNames;
}

//This function allows easy access to all scripts
// Returns a vector of strings containing the names of all scripts
vector<string> HoneydConfiguration::GetScriptNames()
{
	vector<string> scriptNames;
	for(ScriptTable::iterator it = m_scripts.begin(); it != m_scripts.end(); it++)
	{
		scriptNames.push_back(it->first);
	}
	return scriptNames;
}

//This function allows easy access to all generated profiles
// Returns a vector of strings containing the names of all generated profiles
// *Note: Used by auto configuration? may not be needed.
vector<string> HoneydConfiguration::GetGeneratedProfileNames()//XXX Needed?
{
	vector<string> childProfiles;
	for(ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
	{
		if(it->second.m_generated && !it->second.m_personality.empty() && !it->second.m_ethernetVendors.empty())
		{
			childProfiles.push_back(it->first);
		}
	}
	return childProfiles;
}

//This function allows easy access to debug strings of all generated profiles
// Returns a vector of strings containing debug outputs of all generated profiles
vector<string> HoneydConfiguration::GeneratedProfilesStrings()//XXX Needed?
{
	vector<string> returnVector;
	for(ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
	{
		if(it->second.m_generated)
		{
			stringstream currentProfileStream;
			currentProfileStream << "Name: " << it->second.m_name << "\n";
			currentProfileStream << "Personality: " << it->second.m_personality << "\n";
			for(uint i = 0; i < it->second.m_ethernetVendors.size(); i++)
			{
				currentProfileStream << "MAC Vendor:  " << it->second.m_ethernetVendors[i].first << " - " << it->second.m_ethernetVendors[i].second <<"% \n";
			}
			currentProfileStream << "Associated Nodes:\n";
			for(uint i = 0; i < it->second.m_nodeKeys.size(); i++)
			{
				currentProfileStream << "\t" << it->second.m_nodeKeys[i] << "\n";

				for(uint j = 0; j < m_nodes[it->second.m_nodeKeys[i]].m_ports.size(); j++)
				{
					currentProfileStream << "\t\t" << m_nodes[it->second.m_nodeKeys[i]].m_ports[j];
				}
			}
			returnVector.push_back(currentProfileStream.str());
		}
	}
	return returnVector;
}

//This function determines whether or not the given profile is empty
// targetProfileKey: The name of the profile being inherited
// Returns true, if valid parent and false if not
// *Note: Used by auto configuration? may not be needed.
bool HoneydConfiguration::CheckNotInheritingEmptyProfile(string targetProfileKey)
{
	if(m_profiles.keyExists(targetProfileKey))
	{
		return RecursiveCheckNotInheritingEmptyProfile(m_profiles[targetProfileKey]);
	}
	else
	{
		return false;
	}
}

//This function allows easy access to all auto-generated nodes.
// Returns a vector of node names for each node on a generated profile.
vector<string> HoneydConfiguration::GetGeneratedNodeNames()//XXX Needed?
{
	vector<string> childnodes;
	Config::Inst()->SetGroup("HaystackAutoConfig");
	ReadNodesXML();
	for(NodeTable::iterator it = m_nodes.begin(); it != m_nodes.end(); it++)
	{
		if(m_profiles[it->second.m_pfile].m_generated)
		{
			childnodes.push_back(it->second.m_name);
		}
	}
	return childnodes;
}

//This function allows access to NodeProfile objects by their name
// profileName: the name or key of the NodeProfile
// Returns a pointer to the NodeProfile object or NULL if the key doesn't exist
NodeProfile *HoneydConfiguration::GetProfile(string profileName)
{
	if(m_profiles.keyExists(profileName))
	{
		return &m_profiles[profileName];
	}
	return NULL;
}

//This function allows the caller to find out if the given MAC string is taken by a node
// mac: the string representation of the MAC address
// Returns true if the MAC is in use and false if it is not.
// *Note this function may have poor performance when there are a large number of nodes
bool HoneydConfiguration::IsMACUsed(string mac)
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
				ethPair.first = value.second.get<string>("vendor");
				ethPair.second = value.second.get<double>("ethDistribution");
				//If we inherited ethernet vendors but have our own, clear the vector
				if(nodeProf->m_inherited[ETHERNET] == true)
				{
					nodeProf->m_ethernetVendors.clear();
				}
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
	PortStruct *port;

	try
	{
		for(uint i = 0; i < nodeProf->m_ports.size(); i++)
		{
			nodeProf->m_ports[i].second.first = true;
		}
		BOOST_FOREACH(ptree::value_type &value, propTree->get_child(""))
		{
			//Checks for ports
			valueKey = "port";
			if(!string(value.first.data()).compare(valueKey))
			{
				string portName = value.second.get<string>("portName");
				port = &m_ports[portName];
				if(port == NULL)
				{
					LOG(ERROR, "Invalid port '" + portName + "' in NodeProfile '" + nodeProf->m_name + "'.","");
					return false;
				}

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
				pair<bool,double> insidePortPair;
				pair<string, pair<bool, double> > outsidePortPair;
				outsidePortPair.first = port->m_portName;
				insidePortPair.first = false;

				double tempVal = atof(value.second.get<string>("portDistribution").c_str());
				//If outside the range, set distribution to 0
				if((tempVal < 0) ||(tempVal > 100))
				{
					tempVal = 0;
				}
				insidePortPair.second = tempVal;
				outsidePortPair.second = insidePortPair;
				if(!nodeProf->m_ports.size())
				{
					nodeProf->m_ports.push_back(outsidePortPair);
				}
				else
				{
					uint i = 0;
					for(i = 0; i < nodeProf->m_ports.size(); i++)
					{
						PortStruct *tempPort = &m_ports[nodeProf->m_ports[i].first];
						if((atoi(tempPort->m_portNum.c_str())) < (atoi(port->m_portNum.c_str())))
						{
							continue;
						}
						break;
					}
					if(i < nodeProf->m_ports.size())
					{
						nodeProf->m_ports.insert(nodeProf->m_ports.begin() + i, outsidePortPair);
					}
					else
					{
						nodeProf->m_ports.push_back(outsidePortPair);
					}
				}
			}
			//Checks for a subsystem
			valueKey = "subsystem";
			if(!string(value.first.data()).compare(valueKey))
			{
				 //TODO - implement subsystem handling
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

			try
			{
				nodeProf.m_tree = value.second;
				nodeProf.m_parentProfile = parentKey;

				nodeProf.m_generated = value.second.get<bool>("generated");
				nodeProf.m_distribution = value.second.get<double>("distribution");

				//Gets name, initializes DHCP
				nodeProf.m_name = value.second.get<string>("name");
			}
			catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
			{
				// Can't get name, generated, or some needed field
				// Skip this profile
				LOG(ERROR, "Profile XML file contained invalid profile. Skipping", "");
				continue;
			};

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

vector<string> HoneydConfiguration::GetGroups()
{
	return m_groups;
}


bool HoneydConfiguration::AddGroup(string groupName)
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
		newGroup.put<string>("name", groupName);
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
	if (!oldName.compare("default") || !newName.compare("default"))
	{
		LOG(ERROR, "RenameProfile called with 'default' as an argument.", "");
	}

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

		if(!UpdateProfileTree(newName, ALL))
		{
			LOG(ERROR, string("Couldn't update " + oldName + "'s profile tree."), "");
		}
		if(!DeleteProfile(oldName))
		{
			LOG(ERROR, string("Couldn't delete profile " + oldName), "");
		}
		return true;
	}
	return false;
}

bool HoneydConfiguration::DeleteNode(string nodeName)
{
	// We don't delete the doppelganger node, only edit it
	if(!nodeName.compare("Doppelganger"))
	{
		LOG(WARNING, "Unable to delete the Doppelganger node", "");
		return false;
	}

	if (!m_nodes.keyExists(nodeName))
	{
		LOG(WARNING, "Unable to locate expected node '" + nodeName + "'.","");
		return false;
	}

	if (!m_profiles.keyExists(m_nodes[nodeName].m_pfile))
	{
		LOG(ERROR, "Unable to locate expected profile '" + m_nodes[nodeName].m_pfile + "'.","");
		return false;
	}

	vector<string> v = m_profiles[m_nodes[nodeName].m_pfile].m_nodeKeys;
	v.erase(remove( v.begin(), v.end(), nodeName), v.end());
	m_profiles[m_nodes[nodeName].m_pfile].m_nodeKeys = v;

	//Delete the node
	m_nodes.erase(nodeName);
	return true;
}

Node *HoneydConfiguration::GetNode(string nodeName)
{
	// Make sure the node exists
	if(m_nodes.keyExists(nodeName))
	{
		return &m_nodes[nodeName];
	}
	return NULL;
}

std::vector<PortSet*> GetPortSets(std::string profileName);
{
//TODO
}

ScriptTable &HoneydConfiguration::GetScriptTable()
{
	return m_scripts;
}

bool HoneydConfiguration::DeleteProfile(string profileName)
{
	if(!m_profiles.keyExists(profileName))
	{
		LOG(DEBUG, "Attempted to delete profile that does not exist", "");
		return false;
	}

	NodeProfile originalProfile = m_profiles[profileName];
	vector<string> profilesToDelete;
	GetProfilesToDelete(profileName, profilesToDelete);

	for (int i = 0; i < static_cast<int>(profilesToDelete.size()); i++)
	{
		string pfile = profilesToDelete.at(i);
		cout << "Attempting to delete profile " << pfile << endl;

		NodeProfile p = m_profiles[pfile];

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

		m_profiles.erase(pfile);
	}

	//If this profile has a parent
	if(m_profiles.keyExists(originalProfile.m_parentProfile))
	{
		//save a copy of the parent
		NodeProfile parent = m_profiles[originalProfile.m_parentProfile];

		//point to the profiles subtree of parent-copy ptree and clear it
		ptree *pt = &parent.m_tree.get_child("profiles");
		pt->clear();

		//Find all profiles still in the table that are siblings of deleted profile
		// We should be using an iterator to find the original profile and erase it
		// but boost's iterator implementation doesn't seem to be able to access data
		// correctly and are frequently invalidated.

		for(ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
		{
			if(!it->second.m_parentProfile.compare(parent.m_name))
			{
				//Put sibling profiles into the tree
				pt->add_child("profile", it->second.m_tree);
			}
		}	//parent-copy now has the ptree of all children except deleted profile


		//point to the original parent's profiles subtree and replace it with our new ptree
		ptree *treePtr = &m_profiles[originalProfile.m_parentProfile].m_tree.get_child("profiles");
		treePtr->clear();
		*treePtr = *pt;

		//Updates all ancestors with the deletion
		UpdateProfileTree(originalProfile.m_parentProfile, ALL);
	}
	else
	{
		LOG(ERROR, string("Parent profile with name: ") + originalProfile.m_parentProfile + string(" doesn't exist"), "");
	}

	return true;
}

string HoneydConfiguration::SanitizeProfileName(std::string oldName)
{
	if (!oldName.compare("default") || !oldName.compare(""))
	{
		return oldName;
	}

	string newname = "pfile" + oldName;
	ReplaceString(newname, " ", "-");
	ReplaceString(newname, ",", "COMMA");
	ReplaceString(newname, ";", "COLON");
	ReplaceString(newname, "@", "AT");
	return newname;
}

}
