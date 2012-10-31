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
			try
			{
				personality = new PersonalityTreeItem(parent, profilePtree.second.get<string>("name"));
				personality->m_isGenerated = profilePtree.second.get<bool>("generated");
				personality->m_distribution = profilePtree.second.get<double>("distribution");
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

	//Print the "default" profile
	cout << m_profiles.m_root->ToString();

	//Print all the nodes
	for(uint i = 0; i < m_nodes.size(); i++)
	{
		uint j = 0;
		for(NodeTable::iterator it = m_nodes[i].second.begin(); it != m_nodes[i].second.end(); it++)
		{
			if(!it->second.m_enabled)
			{
				continue;
			}

			string nodeName = "CustomNodeProfile-";
			nodeName += j;

			//Only write out nodes for the intended group
			if(!groupName.compare(m_nodes[i].first))
			{
				PersonalityTreeItem *item = m_profiles.GetProfile(it->second.m_pfile);
				if(item != NULL)
				{
					//Print the profile
					cout << item->ToString(it->second.m_portSetName);
					//Then we need to add node-specific information to the profile's output
					if(!it->second.m_IP.compare("DHCP"))
					{
						out << "dhcp " << nodeName << " on " << it->second.m_interface;
						//If the node has a MAC address (not random generated)
						if(it->second.m_MAC.compare("RANDOM"))
						{
							out << " ethernet \"" << it->second.m_MAC << "\"";
						}
						out << '\n';
					}
					else
					{
						//If the node has a MAC address (not random generated)
						if(it->second.m_MAC.compare("RANDOM"))
						{
							//Set the MAC for the custom node profile
							out << "set " << nodeName << " ethernet \"" << it->second.m_MAC << "\"" << '\n';
						}
						//bind the node to the IP address
						out << "bind " << it->second.m_IP << " " << nodeName << '\n';
					}
				}
			}
			j++;
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
		string interface, PortSet *portSet, string groupName)
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

	//Make sure we have a unique identifier
	stringstream ss;

	bool inserted = false;
	for(uint i = 0; i < m_nodes.size(); i++)
	{
		if(!m_nodes[i].first.compare(groupName))
		{
			if(m_nodes[i].second.keyExists(newNode.m_MAC))
			{
				//We got the right group, but the MAC already exists. So just quit
				break;
			}
			else
			{
				m_nodes[i].second[newNode.m_MAC] = newNode;
				inserted = true;
				break;
			}
		}
	}

	if(!inserted)
	{
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

	//Assign all the values

	LOG(DEBUG, "Added new node '" + newNode.m_MAC + "'.", "");

	return true;
}

//Recursive helper function to GetProfileNames() and GetGeneratedProfileNames()
vector<string> GetProfileNames_helper(PersonalityTreeItem *item, bool lookForGeneratedOnly)
{
	if(item == NULL)
	{
		//Return an empty vector
		return vector<string>();
	}

	vector<string> runningTotalProfiles;

	//Only add this profile's name if we're not only looking for generated profiles, or if it is generated anyway
	if(!lookForGeneratedOnly || item->m_isGenerated)
	{
		runningTotalProfiles.push_back(item->m_key);
	}


	//Depth first traversal of tree
	for(uint i = 0; i < item->m_children.size(); i++)
	{
		vector<string> childProfiles = GetProfileNames_helper(item->m_children[i], lookForGeneratedOnly);

		//Preallocate memory, can significantly speed up the insert
		runningTotalProfiles.reserve(runningTotalProfiles.size() + childProfiles.size());
		runningTotalProfiles.insert(runningTotalProfiles.end(), childProfiles.begin(), childProfiles.end());
	}

	return runningTotalProfiles;
}

vector<string> HoneydConfiguration::GetProfileNames()
{
	return GetProfileNames_helper(m_profiles.m_root, false);
}

vector<string> HoneydConfiguration::GetGeneratedProfileNames()
{
	return GetProfileNames_helper(m_profiles.m_root, true);
}

vector<string> HoneydConfiguration::GetScriptNames()
{
	vector<string> scriptNames;
	for(ScriptTable::iterator it = m_scripts.begin(); it != m_scripts.end(); it++)
	{
		scriptNames.push_back(it->first);
	}
	return scriptNames;
}

PersonalityTreeItem *GetProfile_helper(string profileName, PersonalityTreeItem *item)
{
	if(item == NULL)
	{
		return NULL;
	}

	if(!item->m_key.compare(profileName))
	{
		return item;
	}

	for(uint i = 0; i < item->m_children.size(); i++)
	{
		PersonalityTreeItem *profile =  GetProfile_helper(profileName, item->m_children[i]);
		if(profile != NULL)
		{
			return item->m_children[i];
		}
	}

	return NULL;
}

PersonalityTreeItem *HoneydConfiguration::GetProfile(string profileName)
{
	return GetProfile_helper(profileName, m_profiles.m_root);
}

// *Note this function may have poor performance when there are a large number of nodes
bool HoneydConfiguration::IsMACUsed(string mac, string groupName)
{
	//For each group of nodes
	for(uint i = 0; i < m_nodes.size(); i++)
	{
		//If this is our group, OR if we're looking through all groups
		if(!groupName.compare(m_nodes[i].first) || !groupName.compare(""))
		{
			//Search through every node in the group
			for(NodeTable::iterator it = m_nodes[i].second.begin(); it != m_nodes[i].second.end(); it++)
			{
				if(!it->first.compare(mac))
				{
					return true;
				}
			}
		}
	}

	return false;
}

//Inserts the profile into the honeyd configuration
//	profile: pointer to the profile you wish to add
//	Returns (true) if the profile could be created, (false) if it cannot.
bool HoneydConfiguration::AddProfile(PersonalityTreeItem *profile)
{

	return false;
}

vector<string> HoneydConfiguration::GetGroups()
{
	vector<string> groups;

	for(uint i = 0; i < m_nodes.size(); i++)
	{
		groups.push_back(m_nodes[i].first);
	}

	return groups;
}


bool HoneydConfiguration::AddGroup(string groupName)
{
	using boost::property_tree::ptree;

	//See if the group name already exists
	for(uint i = 0; i < m_nodes.size(); i++)
	{
		if(!m_nodes[i].first.compare(groupName))
		{
			return false;
		}
	}

	pair<string, NodeTable> group;
	group.first = groupName;

	m_nodes.push_back(group);

	return true;
}

bool HoneydConfiguration::RenameProfile(string oldName, string newName)
{
	PersonalityTreeItem *profile = GetProfile(oldName);
	if(profile == NULL)
	{
		return false;
	}

	profile->m_key = newName;
	return true;
}

bool HoneydConfiguration::DeleteNode(string nodeMAC)
{
	for(uint i = 0; i < m_nodes.size(); i++)
	{
		if(m_nodes[i].second.keyExists(nodeMAC))
		{
			m_nodes[i].second.erase(nodeMAC);
			return true;
		}
	}

	return false;
}

Node *HoneydConfiguration::GetNode(string nodeMAC)
{
	for(uint i = 0; i < m_nodes.size(); i++)
	{
		if(m_nodes[i].second.keyExists(nodeMAC))
		{
			//XXX: Unsafe address access into table
			return &m_nodes[i].second[nodeMAC];
		}
	}

	return NULL;
}

std::vector<PortSet*> HoneydConfiguration::GetPortSets(std::string profileName)
{
	vector<PortSet*> portSets;

	PersonalityTreeItem *profile = GetProfile(profileName);
	if(profile == NULL)
	{
		return portSets;
	}

	return profile->m_portSets;
}

ScriptTable &HoneydConfiguration::GetScriptTable()
{
	return m_scripts;
}

bool HoneydConfiguration::DeleteProfile(string profileName)
{
	PersonalityTreeItem *profile = GetProfile(profileName);
	if(profile == NULL)
	{
		return false;
	}

	//TODO: Recursively delete any nodes for children of the deleted profile

	delete profile;

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
