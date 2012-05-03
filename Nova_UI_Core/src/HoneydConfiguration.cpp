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
//============================================================================/*

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
string HoneydConfiguration::AddPort(uint16_t portNum, portProtocol isTCP, portBehavior behavior, string scriptName)
{
	port pr;
	//Check the validity and assign the port number
	if(!portNum)
	{
		LOG(ERROR, "Cannot create port: Port Number of 0 is Invalid.", "");
		return string("");
	}
	stringstream ss;
	ss << portNum;
	pr.portNum = ss.str();

	//Assign the port type (UDP or TCP)
	if(isTCP)
	{
		pr.type = "TCP";
	}
	else
	{
		pr.type = "UDP";
	}

	//Check and assign the port behavior
	switch(behavior)
	{
		case BLOCK:
		{
			pr.behavior = "block";
			break;
		}
		case OPEN:
		{
			pr.behavior = "open";
			break;
		}
		case RESET:
		{
			pr.behavior = "reset";
			if(!pr.type.compare("UDP"))
			{
				pr.behavior = "block";
			}
			break;
		}
		case SCRIPT:
		{
			//If the script does not exist
			if(m_scripts.find(scriptName) == m_scripts.end())
			{
				LOG(ERROR, "Cannot create port: specified script "+ scriptName +" does not exist.", "");
				return "";
			}
			pr.behavior = "script";
			pr.scriptName = scriptName;
			break;
		}
		default:
		{
			LOG(ERROR, "Cannot create port: Attempting to use unknown port behavior", "");
			return string("");
		}
	}

	//	Creates the ports unique identifier these names won't collide unless the port is the same
	if(!pr.behavior.compare("script"))
	{
		pr.portName = pr.portNum + "_" + pr.type + "_" + pr.scriptName;
	}
	else
	{
		pr.portName = pr.portNum + "_" + pr.type + "_" + pr.behavior;
	}

	//Checks if the port already exists
	if(m_ports.find(pr.portName) != m_ports.end())
	{
		LOG(DEBUG, "Cannot create port: Specified port " + pr.portName + " already exists.", "");
		return "";
	}

	//Adds the port into the table
	m_ports[pr.portName] = pr;
	return pr.portName;
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

	return true;
}

//Loads ports from file
void HoneydConfiguration::LoadPortsTemplate()
{
	using boost::property_tree::ptree;
	using boost::property_tree::xml_parser::trim_whitespace;

	m_portTree.clear();
	try
	{
		read_xml(m_homePath+"/templates/ports.xml", m_portTree, boost::property_tree::xml_parser::trim_whitespace);

		BOOST_FOREACH(ptree::value_type &v, m_portTree.get_child("ports"))
		{
			port p;
			p.tree = v.second;
			//Required xml entries
			p.portName = v.second.get<std::string>("name");

			if(!p.portName.compare(""))
			{
				LOG(ERROR, "Problem loading honeyd XML files.", "");
				continue;
			}

			p.portNum = v.second.get<std::string>("number");
			p.type = v.second.get<std::string>("type");
			p.behavior = v.second.get<std::string>("behavior");

			//If this port uses a script, find and assign it.
			if(!p.behavior.compare("script") || !p.behavior.compare("internal"))
			{
				p.scriptName = v.second.get<std::string>("script");
			}
			//If the port works as a proxy, find destination
			else if(!p.behavior.compare("proxy"))
			{
				p.proxyIP = v.second.get<std::string>("IP");
				p.proxyPort = v.second.get<std::string>("Port");
			}
			m_ports[p.portName] = p;
		}
	}
	catch(std::exception &e)
	{
		LOG(ERROR, "Problem loading ports: "+string(e.what())+".", "");
	}
}


//Loads the subnets and nodes from file for the currently specified group
void HoneydConfiguration::LoadNodesTemplate()
{
	using boost::property_tree::ptree;
	using boost::property_tree::xml_parser::trim_whitespace;

	m_groupTree.clear();
	ptree ptr;

	try
	{
		read_xml(m_homePath+"/templates/nodes.xml", m_groupTree, boost::property_tree::xml_parser::trim_whitespace);
		BOOST_FOREACH(ptree::value_type &v, m_groupTree.get_child("groups"))
		{
			//Find the specified group
			if(!v.second.get<std::string>("name").compare(Config::Inst()->GetGroup()))
			{
				try //Null Check
				{
					//Load Subnets first, they are needed before we can load nodes
					m_subnetTree = v.second.get_child("subnets");
					LoadSubnets(&m_subnetTree);

					try //Null Check
					{
						//If subnets are loaded successfully, load nodes
						m_nodesTree = v.second.get_child("nodes");
						LoadNodes(&m_nodesTree);
					}
					catch(std::exception &e)
					{
						LOG(ERROR, "Problem loading nodes: "+string(e.what())+".", "");
					}
				}
				catch(std::exception &e)
				{
					LOG(ERROR, "Problem loading subnets: "+string(e.what())+".", "");
				}
			}
		}
	}
	catch(std::exception &e)
	{
		LOG(ERROR, "Problem loading groups: "+Config::Inst()->GetGroup()+" - "+string(e.what()) +".", "");
	}
}


//Sets the configuration of 'set' values for profile that called it
void HoneydConfiguration::LoadProfileSettings(ptree *ptr, profile *p)
{
	string prefix;
	try
	{
		BOOST_FOREACH(ptree::value_type &v, ptr->get_child(""))
		{
			prefix = "TCP";
			if(!string(v.first.data()).compare(prefix))
			{
				p->tcpAction = v.second.data();
				p->inherited[TCP_ACTION] = false;
				continue;
			}
			prefix = "UDP";
			if(!string(v.first.data()).compare(prefix))
			{
				p->udpAction = v.second.data();
				p->inherited[UDP_ACTION] = false;
				continue;
			}
			prefix = "ICMP";
			if(!string(v.first.data()).compare(prefix))
			{
				p->icmpAction = v.second.data();
				p->inherited[ICMP_ACTION] = false;
				continue;
			}
			prefix = "personality";
			if(!string(v.first.data()).compare(prefix))
			{
				p->personality = v.second.data();
				p->inherited[PERSONALITY] = false;
				continue;
			}
			prefix = "ethernet";
			if(!string(v.first.data()).compare(prefix))
			{
				p->ethernet = v.second.data();
				p->inherited[ETHERNET] = false;
				continue;
			}
			prefix = "uptimeMin";
			if(!string(v.first.data()).compare(prefix))
			{
				p->uptimeMin = v.second.data();
				p->inherited[UPTIME] = false;
				continue;
			}
			prefix = "uptimeMax";
			if(!string(v.first.data()).compare(prefix))
			{
				p->uptimeMax = v.second.data();
				p->inherited[UPTIME] = false;
				continue;
			}
			prefix = "dropRate";
			if(!string(v.first.data()).compare(prefix))
			{
				p->dropRate = v.second.data();
				p->inherited[DROP_RATE] = false;
				continue;
			}
		}
	}
	catch(std::exception &e)
	{
		LOG(ERROR, "Problem loading profile set parameters: "+string(e.what())+".", "");
	}
}

//Adds specified ports and subsystems
// removes any previous port with same number and type to avoid conflicts
void HoneydConfiguration::LoadProfileServices(ptree *ptr, profile *p)
{
	string prefix;
	port * prt;

	try
	{
		for(uint i = 0; i < p->ports.size(); i++)
		{
			p->ports[i].second = true;
		}
		BOOST_FOREACH(ptree::value_type &v, ptr->get_child(""))
		{
			//Checks for ports
			prefix = "ports";
			if(!string(v.first.data()).compare(prefix))
			{
				//Iterates through the ports
				BOOST_FOREACH(ptree::value_type &v2, ptr->get_child("ports"))
				{
					prt = &m_ports[v2.second.data()];

					//Checks inherited ports for conflicts
					for(uint i = 0; i < p->ports.size(); i++)
					{
						//Erase inherited port if a conflict is found
						if(!prt->portNum.compare(m_ports[p->ports[i].first].portNum) && !prt->type.compare(m_ports[p->ports[i].first].type))
						{
							p->ports.erase(p->ports.begin()+i);
						}
					}
					//Add specified port
					pair<string, bool> portPair;
					portPair.first = prt->portName;
					portPair.second = false;
					if(!p->ports.size())
					{
						p->ports.push_back(portPair);
					}
					else
					{
						uint i = 0;
						for(i = 0; i < p->ports.size(); i++)
						{
							port * temp = &m_ports[p->ports[i].first];
							if((atoi(temp->portNum.c_str())) < (atoi(prt->portNum.c_str())))
							{
								continue;
							}
							break;
						}
						if(i < p->ports.size())
						{
							p->ports.insert(p->ports.begin()+i, portPair);
						}
						else
						{
							p->ports.push_back(portPair);
						}
					}
				}
				continue;
			}

			//Checks for a subsystem
			prefix = "subsystem"; //TODO
			if(!string(v.first.data()).compare(prefix))
			{
				continue;
			}
		}
	}
	catch(std::exception &e)
	{
		LOG(ERROR, "Problem loading profile add parameters: "+string(e.what())+".", "");
	}
}

//Recursive descent down a profile tree, inherits parent, sets values and continues if not leaf.
void HoneydConfiguration::LoadProfileChildren(string parent)
{
	ptree ptr = m_profiles[parent].tree;
	try
	{
		BOOST_FOREACH(ptree::value_type &v, ptr.get_child("profiles"))
		{
			ptree *ptr2;

			//Inherits parent,
			profile prof = m_profiles[parent];
			prof.tree = v.second;
			prof.parentProfile = parent;

			//Gets name, initializes DHCP
			prof.name = v.second.get<std::string>("name");

			if(!prof.name.compare(""))
			{
				LOG(ERROR, "Problem loading honeyd XML files.", "");
				continue;
			}

			for(uint i = 0; i < INHERITED_MAX; i++)
			{
				prof.inherited[i] = true;
			}


			try //Conditional: If profile has set configurations different from parent
			{
				ptr2 = &v.second.get_child("set");
				LoadProfileSettings(ptr2, &prof);
			}
			catch(...){}

			try //Conditional: If profile has port or subsystems different from parent
			{
				ptr2 = &v.second.get_child("add");
				LoadProfileServices(ptr2, &prof);
			}
			catch(...){}

			//Saves the profile
			m_profiles[prof.name] = prof;

			try //Conditional: if profile has children (not leaf)
			{
				LoadProfileChildren(prof.name);
			}
			catch(...){}
		}
	}
	catch(std::exception &e)
	{
		LOG(ERROR, "Problem loading sub profiles: "+string(e.what())+".", "");
	}
}


//Loads scripts from file
void HoneydConfiguration::LoadScriptsTemplate()
{
	using boost::property_tree::ptree;
	using boost::property_tree::xml_parser::trim_whitespace;
	m_scriptTree.clear();
	try
	{
		read_xml(m_homePath+"/scripts.xml", m_scriptTree, boost::property_tree::xml_parser::trim_whitespace);

		BOOST_FOREACH(ptree::value_type &v, m_scriptTree.get_child("scripts"))
		{
			script s;
			s.tree = v.second;
			//Each script consists of a name and path to that script
			s.name = v.second.get<std::string>("name");

			if(!s.name.compare(""))
			{
				LOG(ERROR, "Problem loading honeyd XML files.","");
				continue;
			}

			s.path = v.second.get<std::string>("path");
			m_scripts[s.name] = s;
		}
	}
	catch(std::exception &e)
	{
		LOG(ERROR, "Problem loading scripts: "+string(e.what())+".", "");
	}
}


/************************************************
 * Save Honeyd XML Configuration Functions
 ************************************************/

//Saves the current configuration information to XML files

//**Important** this function assumes that unless it is a new item (ptree pointer == NULL) then
// all required fields exist and old fields have been removed. Ex: if a port previously used a script
// but now has a behavior of open, at that point the user should have erased the script field.
// inverserly if a user switches to script the script field must be created.

//To summarize this function only populates the xml data for the values it contains unless it is a new item,
// it does not clean up, and only creates if it's a new item and then only for the fields that are needed.
// it does not track profile inheritance either, that should be created when the heirarchy is modified.
bool HoneydConfiguration::SaveAllTemplates()
{
	using boost::property_tree::ptree;
	ptree pt;

	//Scripts
	m_scriptTree.clear();
	for(ScriptTable::iterator it = m_scripts.begin(); it != m_scripts.end(); it++)
	{
		pt = it->second.tree;
		pt.put<std::string>("name", it->second.name);
		pt.put<std::string>("path", it->second.path);
		m_scriptTree.add_child("scripts.script", pt);
	}

	//Ports
	m_portTree.clear();
	for(PortTable::iterator it = m_ports.begin(); it != m_ports.end(); it++)
	{
		pt = it->second.tree;
		pt.put<std::string>("name", it->second.portName);
		pt.put<std::string>("number", it->second.portNum);
		pt.put<std::string>("type", it->second.type);
		pt.put<std::string>("behavior", it->second.behavior);
		//If this port uses a script, save it.
		if(!it->second.behavior.compare("script") || !it->second.behavior.compare("internal"))
		{
			pt.put<std::string>("script", it->second.scriptName);
		}
		//If the port works as a proxy, save destination
		else if(!it->second.behavior.compare("proxy"))
		{
			pt.put<std::string>("IP", it->second.proxyIP);
			pt.put<std::string>("Port", it->second.proxyPort);
		}
		m_portTree.add_child("ports.port", pt);
	}

	m_subnetTree.clear();
	for(SubnetTable::iterator it = m_subnets.begin(); it != m_subnets.end(); it++)
	{
		pt = it->second.tree;

		//TODO assumes subnet is interface, need to discover and handle if virtual
		pt.put<std::string>("name", it->second.name);
		pt.put<bool>("enabled",it->second.enabled);
		pt.put<bool>("isReal", it->second.isRealDevice);

		//Remove /## format mask from the address then put it in the XML.
		stringstream ss;
		ss << "/" << it->second.maskBits;
		int i = ss.str().size();
		string temp = it->second.address.substr(0,(it->second.address.size()-i));
		pt.put<std::string>("IP", temp);

		//Gets the mask from mask bits then put it in XML
		in_addr_t mask = ::pow(2, 32-it->second.maskBits) - 1;
		//If maskBits is 24 then we have 2^8 -1 = 0x000000FF
		mask = ~mask; //After getting the inverse of this we have the mask in host addr form.
		//Convert to network order, put in in_addr struct
		//call ntoa to get char * and make string
		in_addr tempMask;
		tempMask.s_addr = htonl(mask);
		temp = string(inet_ntoa(tempMask));
		pt.put<std::string>("mask", temp);
		m_subnetTree.add_child("interface", pt);
	}

	//Nodes
	m_nodesTree.clear();
	for(NodeTable::iterator it = m_nodes.begin(); it != m_nodes.end(); it++)
	{
		pt = it->second.tree;

		// No need to save names besides the doppel, we can derive them
		if(it->second.name == "Doppelganger")
		{
			// Make sure the IP reflects whatever is being used right now
			it->second.IP = Config::Inst()->GetDoppelIp();
			pt.put<std::string>("name", it->second.name);
		}

		//Required xml entires
		pt.put<std::string>("interface", it->second.interface);
		pt.put<std::string>("IP", it->second.IP);
		pt.put<bool>("enabled", it->second.enabled);
		pt.put<std::string>("MAC", it->second.MAC);
		pt.put<std::string>("profile.name", it->second.pfile);
		m_nodesTree.add_child("node",pt);
	}
	using boost::property_tree::ptree;
	BOOST_FOREACH(ptree::value_type &v, m_groupTree.get_child("groups"))
	{
		//Find the specified group
		if(!v.second.get<std::string>("name").compare(Config::Inst()->GetGroup()))
		{
			//Load Subnets first, they are needed before we can load nodes
			v.second.put_child("subnets", m_subnetTree);
			v.second.put_child("nodes",m_nodesTree);
		}
	}
	m_profileTree.clear();
	for(ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
	{
		if(it->second.parentProfile == "")
		{
			pt = it->second.tree;
			m_profileTree.add_child("profiles.profile", pt);
		}
	}
	boost::property_tree::xml_writer_settings<char> settings('\t', 1);
	write_xml(m_homePath+"/scripts.xml", m_scriptTree, std::locale(), settings);
	write_xml(m_homePath+"/templates/ports.xml", m_portTree, std::locale(), settings);
	write_xml(m_homePath+"/templates/nodes.xml", m_groupTree, std::locale(), settings);
	write_xml(m_homePath+"/templates/profiles.xml", m_profileTree, std::locale(), settings);
	return true;
}

//Writes the current configuration to honeyd configs
bool HoneydConfiguration::WriteHoneydConfiguration(string path)
{
	stringstream out;

	vector<string> profilesParsed;

	for (ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
	{
		if(!it->second.parentProfile.compare(""))
		{
			string pString = ProfileToString(&it->second);
			out << pString;
			profilesParsed.push_back(it->first);
		}
	}

	while (profilesParsed.size() < m_profiles.size())
	{
		for (ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
		{
			bool selfMatched = false;
			bool parentFound = false;
			for (uint i = 0; i < profilesParsed.size(); i++)
			{
				if(!it->second.parentProfile.compare(profilesParsed[i]))
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
	for (NodeTable::iterator it = m_nodes.begin(); it != m_nodes.end(); it++)
	{
		if(!it->second.enabled)
		{
			continue;
		}
		//We write the dopp regardless of whether or not it is enabled so that it can be toggled during runtime.
		else if(!it->second.name.compare("Doppelganger"))
		{
			string pString = DoppProfileToString(&m_profiles[it->second.pfile]);
			out << endl << pString;
			out << "bind " << it->second.IP << " DoppelgangerReservedTemplate" << endl << endl;
			//Use configured or discovered loopback
		}
		else
		{
			// No IP address, use DHCP
			if(it->second.IP == "DHCP" && it->second.MAC == "RANDOM")
			{
				out << "dhcp " << it->second.pfile << " on " << it->second.interface << endl;
			}
			else if(it->second.IP == "DHCP" && it->second.MAC != "RANDOM")
			{
				out << "dhcp " << it->second.pfile << " on " << it->second.interface << " ethernet \"" << it->second.MAC << "\"" << endl;
			}
			else if(it->second.IP != "DHCP" && it->second.MAC == "RANDOM")
			{
				out << "bind " << it->second.IP << " " <<  it->second.pfile << endl;
			}
			else if(it->second.IP != "DHCP" && it->second.MAC != "RANDOM")
			{
				out << "clone " << it->second.pfile << it->second.IP << " " << it->second.pfile << endl;
				out << "set " << it->second.pfile << it->second.IP << " ethernet \"" << it->second.MAC << "\"" << endl;
			}
		}
	}
	ofstream outFile(path);
	outFile << out.str() << endl;
	outFile.close();
	return true;
}


//loads subnets from file for current group
void HoneydConfiguration::LoadSubnets(ptree *ptr)
{
	try
	{
		BOOST_FOREACH(ptree::value_type &v, ptr->get_child(""))
		{
			//If real interface
			if(!string(v.first.data()).compare("interface"))
			{
				subnet sub;
				sub.tree = v.second;
				sub.isRealDevice =  v.second.get<bool>("isReal");
				//Extract the data
				sub.name = v.second.get<std::string>("name");
				sub.address = v.second.get<std::string>("IP");
				sub.mask = v.second.get<std::string>("mask");
				sub.enabled = v.second.get<bool>("enabled");

				//Gets the IP address in uint32 form
				in_addr_t baseTemp = ntohl(inet_addr(sub.address.c_str()));

				//Converting the mask to uint32 allows a simple bitwise AND to get the lowest IP in the subnet.
				in_addr_t maskTemp = ntohl(inet_addr(sub.mask.c_str()));
				sub.base = (baseTemp & maskTemp);
				//Get the number of bits in the mask
				sub.maskBits = GetMaskBits(maskTemp);
				//Adding the binary inversion of the mask gets the highest usable IP
				sub.max = sub.base + ~maskTemp;
				stringstream ss;
				ss << sub.address << "/" << sub.maskBits;
				sub.address = ss.str();

				//Save subnet
				m_subnets[sub.name] = sub;
			}
			//If virtual honeyd subnet
			else if(!string(v.first.data()).compare("virtual"))
			{
				//TODO Implement and test
				/*subnet sub;
				sub.tree = v.second;
				sub.isRealDevice = false;
				//Extract the data
				sub.name = v.second.get<std::string>("name");
				sub.address = v.second.get<std::string>("IP");
				sub.mask = v.second.get<std::string>("mask");
				sub.enabled = v.second.get<bool>("enabled");

				//Gets the IP address in uint32 form
				in_addr_t baseTemp = ntohl(inet_addr(sub.address.c_str()));

				//Converting the mask to uint32 allows a simple bitwise AND to get the lowest IP in the subnet.
				in_addr_t maskTemp = ntohl(inet_addr(sub.mask.c_str()));
				sub.base = (baseTemp & maskTemp);
				//Get the number of bits in the mask
				sub.maskBits = GetMaskBits(maskTemp);
				//Adding the binary inversion of the mask gets the highest usable IP
				sub.max = sub.base + ~maskTemp;
				stringstream ss;
				ss << sub.address << "/" << sub.maskBits;
				sub.address = ss.str();

				//Save subnet
				subnets[sub.name] = sub;*/
			}
			else
			{
				LOG(ERROR, "Unexpected Entry in file: "+string(v.first.data())+".", "");
			}
		}
	}
	catch(std::exception &e)
	{
		LOG(ERROR, "Problem loading subnets: "+string(e.what()), "");
	}
}


//loads haystack nodes from file for current group
void HoneydConfiguration::LoadNodes(ptree *ptr)
{
	profile p;
	//ptree * ptr2;
	try
	{
		BOOST_FOREACH(ptree::value_type &v, ptr->get_child(""))
		{
			if(!string(v.first.data()).compare("node"))
			{
				Node n;
				stringstream ss;
				uint j = 0;
				j = ~j; // 2^32-1

				n.tree = v.second;
				//Required xml entires
				n.interface = v.second.get<std::string>("interface");
				n.IP = v.second.get<std::string>("IP");
				n.enabled = v.second.get<bool>("enabled");
				n.pfile = v.second.get<std::string>("profile.name");

				if(!n.pfile.compare(""))
				{
					LOG(ERROR, "Problem loading honeyd XML files.", "");
					continue;
				}

				p = m_profiles[n.pfile];

				//Get mac if present
				try //Conditional: has "set" values
				{
					//ptr2 = &v.second.get_child("MAC");
					//pass 'set' subset and pointer to this profile
					n.MAC = v.second.get<std::string>("MAC");
				}
				catch(...){}
				if(!n.IP.compare(Config::Inst()->GetDoppelIp()))
				{
					n.name = "Doppelganger";
					n.sub = n.interface;
					n.realIP = htonl(inet_addr(n.IP.c_str())); //convert ip to uint32
					//save the node in the table
					m_nodes[n.name] = n;

					//Put address of saved node in subnet's list of nodes.
					m_subnets[m_nodes[n.name].sub].nodes.push_back(n.name);
				}
				else
				{
					n.name = n.IP + " - " + n.MAC;

					if(n.name == "DHCP - RANDOM")
					{
						//Finds a unique identifier
						uint i = 1;
						while((m_nodes.keyExists(n.name)) && (i < j))
						{
							i++;
							ss.str("");
							ss << "DHCP - RANDOM(" << i << ")";
							n.name = ss.str();
						}
					}

					if(n.IP != "DHCP")
					{
						n.realIP = htonl(inet_addr(n.IP.c_str())); //convert ip to uint32
						n.sub = FindSubnet(n.realIP);
						//If we have a subnet and node is unique
						if(n.sub != "")
						{
							//Put address of saved node in subnet's list of nodes.
							m_subnets[n.sub].nodes.push_back(n.name);
						}
						//If no subnet found, can't use node unless it's doppelganger.
						else
						{
							LOG(ERROR, "Node at IP: "+ n.IP+"is outside all valid subnet ranges.", "");
						}
					}
					else
					{
						n.sub = n.interface; //TODO virtual subnets will need to be handled when implemented
						// If no valid subnet/interface found
						if(!n.sub.compare(""))
						{
							LOG(ERROR, "DHCP Enabled Node with MAC: "+n.name+" is unable to resolve it's interface.","");
							continue;
						}

						//Put address of saved node in subnet's list of nodes.
						m_subnets[n.sub].nodes.push_back(n.name);
					}
				}

				if(!n.name.compare(""))
				{
					LOG(ERROR, "Problem loading honeyd XML files.", "");
					continue;
				}
				else
				{
					//save the node in the table
					m_nodes[n.name] = n;
				}
			}
			else
			{
				LOG(ERROR, "Unexpected Entry in file: "+string(v.first.data()), "");
			}
		}
	}
	catch(std::exception &e)
	{
		LOG(ERROR, "Problem loading nodes: "+ string(e.what()), "");
	}
}

string HoneydConfiguration::FindSubnet(in_addr_t ip)
{
	string subnet = "";
	int max = 0;
	//Check each subnet
	for(SubnetTable::iterator it = m_subnets.begin(); it != m_subnets.end(); it++)
	{
		//If node falls outside a subnets range skip it
		if((ip < it->second.base) || (ip > it->second.max))
		{
			continue;
		}
		//If this is the smallest range
		if(it->second.maskBits > max)
		{
			subnet = it->second.name;
		}
	}

	return subnet;
}

void HoneydConfiguration::LoadProfilesTemplate()
{
	using boost::property_tree::ptree;
	using boost::property_tree::xml_parser::trim_whitespace;
	ptree * ptr;
	m_profileTree.clear();
	try
	{
		read_xml(m_homePath+"/templates/profiles.xml", m_profileTree, boost::property_tree::xml_parser::trim_whitespace);

		BOOST_FOREACH(ptree::value_type &v, m_profileTree.get_child("profiles"))
		{
			//Generic profile, essentially a honeyd template
			if(!string(v.first.data()).compare("profile"))
			{
				profile p;
				//Root profile has no parent
				p.parentProfile = "";
				p.tree = v.second;

				//Name required, DCHP boolean intialized (set in loadProfileSet)
				p.name = v.second.get<std::string>("name");

				if(!p.name.compare(""))
				{
					LOG(ERROR, "Problem loading honeyd XML files.", "");
					continue;
				}

				p.ports.clear();
				for(uint i = 0; i < INHERITED_MAX; i++)
				{
					p.inherited[i] = false;
				}

				try //Conditional: has "set" values
				{
					ptr = &v.second.get_child("set");
					//pass 'set' subset and pointer to this profile
					LoadProfileSettings(ptr, &p);
				}
				catch(...){}

				try //Conditional: has "add" values
				{
					ptr = &v.second.get_child("add");
					//pass 'add' subset and pointer to this profile
					LoadProfileServices(ptr, &p);
				}
				catch(...){}

				//Save the profile
				m_profiles[p.name] = p;

				try //Conditional: has children profiles
				{
					//start recurisive descent down profile tree with this profile as the root
					//pass subtree and pointer to parent
					LoadProfileChildren(p.name);
				}
				catch(...){}

			}

			//Honeyd's implementation of switching templates based on conditions
			else if(!string(v.first.data()).compare("dynamic"))
			{
				//TODO
			}
			else
			{
				LOG(ERROR, "Invalid XML Path " +string(v.first.data())+".", "");
			}
		}
	}
	catch(std::exception &e)
	{
		LOG(ERROR, "Problem loading Profiles: "+string(e.what())+".", "");
	}
}

string HoneydConfiguration::ProfileToString(profile* p)
{
	stringstream out;

	if(!p->parentProfile.compare("default") || !p->parentProfile.compare(""))
	{
		out << "create " << p->name << endl;
	}
	else
	{
		out << "clone " << p->parentProfile << " " << p->name << endl;
	}

	out << "set " << p->name  << " default tcp action " << p->tcpAction << endl;
	out << "set " << p->name  << " default udp action " << p->udpAction << endl;
	out << "set " << p->name  << " default icmp action " << p->icmpAction << endl;

	if(p->personality.compare(""))
	{
		out << "set " << p->name << " personality \"" << p->personality << '"' << endl;
	}

	if(p->ethernet.compare(""))
	{
		out << "set " << p->name << " ethernet \"" << p->ethernet << '"' << endl;
	}


	if(p->dropRate.compare(""))
	{
		out << "set " << p->name << " droprate in " << p->dropRate << endl;
	}

	for (uint i = 0; i < p->ports.size(); i++)
	{
		// Only include non-inherited ports
		if(!p->ports[i].second)
		{
			out << "add " << p->name;
			if(!m_ports[p->ports[i].first].type.compare("TCP"))
			{
				out << " tcp port ";
			}
			else
			{
				out << " udp port ";
			}
			out << m_ports[p->ports[i].first].portNum << " ";

			if(!(m_ports[p->ports[i].first].behavior.compare("script")))
			{
				string scriptName = m_ports[p->ports[i].first].scriptName;

				if(m_scripts[scriptName].path.compare(""))
				{
					out << '"' << m_scripts[scriptName].path << '"'<< endl;
				}
				else
				{
					LOG(ERROR, "Error writing profile port script.", "Path to script "+scriptName+" is null.");
				}
			}
			else
			{
				out << m_ports[p->ports[i].first].behavior << endl;
			}
		}
	}
	out << endl;
	return out.str();
}

//
string HoneydConfiguration::DoppProfileToString(profile* p)
{
	stringstream out;
	out << "create DoppelgangerReservedTemplate" << endl;

	out << "set DoppelgangerReservedTemplate default tcp action " << p->tcpAction << endl;
	out << "set DoppelgangerReservedTemplate default udp action " << p->udpAction << endl;
	out << "set DoppelgangerReservedTemplate default icmp action " << p->icmpAction << endl;

	if(p->personality.compare(""))
	{
		out << "set DoppelgangerReservedTemplate" << " personality \"" << p->personality << '"' << endl;
	}


	if(p->dropRate.compare(""))
	{
		out << "set DoppelgangerReservedTemplate" << " droprate in " << p->dropRate << endl;
	}

	for (uint i = 0; i < p->ports.size(); i++)
	{
		// Only include non-inherited ports
		if(!p->ports[i].second)
		{
			out << "add DoppelgangerReservedTemplate";
			if(!m_ports[p->ports[i].first].type.compare("TCP"))
			{
				out << " tcp port ";
			}
			else
			{
				out << " udp port ";
			}
			out << m_ports[p->ports[i].first].portNum << " ";

			if(!(m_ports[p->ports[i].first].behavior.compare("script")))
			{
				string scriptName = m_ports[p->ports[i].first].scriptName;

				if(m_scripts[scriptName].path.compare(""))
				{
					out << '"' << m_scripts[scriptName].path << '"'<< endl;
				}
				else
				{
					LOG(ERROR, "Error writing profile port script.", "Path to script "+scriptName+" is null.");
				}
			}
			else
			{
				out << m_ports[p->ports[i].first].behavior << endl;
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

	for (ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
	{
		if(it->second.parentProfile == parent)
		{
			childProfiles.push_back(it->second.name);
		}
	}

	return childProfiles;
}

std::vector<std::string> HoneydConfiguration::GetProfileNames()
{
	vector<std::string> childProfiles;

	for (ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
	{
		childProfiles.push_back(it->second.name);
	}

	return childProfiles;
}

Nova::profile * HoneydConfiguration::GetProfile(std::string profileName)
{
	if(!m_profiles.keyExists(profileName))
	{
		return NULL;
	}
	else
	{
		profile *ret = new profile();
		*ret = m_profiles[profileName];
		return ret;
	}
}

Nova::port * HoneydConfiguration::GetPort(std::string portName)
{
	if(m_ports.keyExists(portName))
	{
		port *p = new port();
		*p = m_ports[portName];
		return p;
	}
	return NULL;
}

std::vector<std::string> HoneydConfiguration::GetNodeNames()
{
	vector<std::string> childnodes;

	for (NodeTable::iterator it = m_nodes.begin(); it != m_nodes.end(); it++)
	{
		childnodes.push_back(it->second.name);
	}

	return childnodes;
}


std::vector<std::string> HoneydConfiguration::GetSubnetNames()
{
	vector<std::string> childSubnets;

	for (SubnetTable::iterator it = m_subnets.begin(); it != m_subnets.end(); it++)
	{
		childSubnets.push_back(it->second.name);
	}

	return childSubnets;
}


/*std::pair<hdConfigReturn, std::string> HoneydConfiguration::GetEthernet(profileName profile)
{
	pair<hdConfigReturn, string> ret;

	// Make sure the input profile name exists
	if(!m_profiles.keyExists(profile))
	{
		ret.first = NO_SUCH_KEY;
		ret.second = "";
		return ret;
	}

	ret.first = NOT_INHERITED;
	profileName parent = profile;

	while (m_profiles[parent].ethernet == "")
	{
		ret.first = INHERITED;
		parent = m_profiles[parent].parentProfile;
	}

	ret.second = m_profiles[parent].ethernet;

	return ret;
}

std::pair<hdConfigReturn, std::string> HoneydConfiguration::GetPersonality(profileName profile)
{
	pair<hdConfigReturn, string> ret;

	// Make sure the input profile name exists
	if(!m_profiles.keyExists(profile))
	{
		ret.first = NO_SUCH_KEY;
		ret.second = "";
		return ret;
	}

	ret.first = NOT_INHERITED;
	profileName parent = profile;

	while (m_profiles[parent].personality == "")
	{
		ret.first = INHERITED;
		parent = m_profiles[parent].parentProfile;
	}

	ret.second = m_profiles[parent].personality;

	return ret;
}

std::pair<hdConfigReturn, std::string> HoneydConfiguration::GetActionTCP(profileName profile)
{
	pair<hdConfigReturn, string> ret;

	// Make sure the input profile name exists
	if(!m_profiles.keyExists(profile))
	{
		ret.first = NO_SUCH_KEY;
		ret.second = "";
		return ret;
	}

	ret.first = NOT_INHERITED;
	profileName parent = profile;

	while (m_profiles[parent].tcpAction == "")
	{
		ret.first = INHERITED;
		parent = m_profiles[parent].parentProfile;
	}

	ret.second = m_profiles[parent].tcpAction;

	return ret;

}

std::pair<hdConfigReturn, std::string> HoneydConfiguration::GetActionUDP(profileName profile)
{
	pair<hdConfigReturn, string> ret;

	// Make sure the input profile name exists
	if(!m_profiles.keyExists(profile))
	{
		ret.first = NO_SUCH_KEY;
		ret.second = "";
		return ret;
	}

	ret.first = NOT_INHERITED;
	profileName parent = profile;

	while (m_profiles[parent].udpAction == "")
	{
		ret.first = INHERITED;
		parent = m_profiles[parent].parentProfile;
	}

	ret.second = m_profiles[parent].udpAction;

	return ret;
}

std::pair<hdConfigReturn, std::string> HoneydConfiguration::GetActionICMP(profileName profile)
{
	pair<hdConfigReturn, string> ret;

	// Make sure the input profile name exists
	if(!m_profiles.keyExists(profile))
	{
		ret.first = NO_SUCH_KEY;
		ret.second = "";
		return ret;
	}

	ret.first = NOT_INHERITED;
	profileName parent = profile;

	while (m_profiles[parent].icmpAction == "")
	{
		ret.first = INHERITED;
		parent = m_profiles[parent].parentProfile;
	}

	ret.second = m_profiles[parent].icmpAction;

	return ret;

}

std::pair<hdConfigReturn, string> HoneydConfiguration::GetUptimeMin(profileName profile)
{
	pair<hdConfigReturn, string> ret;

	// Make sure the input profile name exists
	if(!m_profiles.keyExists(profile))
	{
		ret.first = NO_SUCH_KEY;
		ret.second = "";
		return ret;
	}

	ret.first = NOT_INHERITED;
	profileName parent = profile;

	while (m_profiles[parent].uptimeMin == "")
	{
		ret.first = INHERITED;
		parent = m_profiles[parent].parentProfile;
	}

	ret.second = m_profiles[parent].uptimeMin;

	return ret;
}

std::pair<hdConfigReturn, string> HoneydConfiguration::GetUptimeMax(profileName profile)
{
	pair<hdConfigReturn, string> ret;

	// Make sure the input profile name exists
	if(!m_profiles.keyExists(profile))
	{
		ret.first = NO_SUCH_KEY;
		ret.second = "";
		return ret;
	}

	ret.first = NOT_INHERITED;
	profileName parent = profile;

	while (m_profiles[parent].uptimeMax == "")
	{
		ret.first = INHERITED;
		parent = m_profiles[parent].parentProfile;
	}

	ret.second = m_profiles[parent].uptimeMax;

	return ret;
}*/


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
		if(!it->second.MAC.compare(mac))
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
		if(!it->second.IP.compare(ip) && it->second.name.compare(ip))
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
		if(!it->second.pfile.compare(profileName))
		{
			return true;
		}
	}
	return false;
}

void HoneydConfiguration::GenerateMACAddresses(string profileName)
{
	for(NodeTable::iterator it = m_nodes.begin(); it != m_nodes.end(); it++)
	{
		if(!it->second.pfile.compare(profileName))
		{
			it->second.MAC = GenerateUniqueMACAddress(m_profiles[profileName].ethernet);
		}
	}
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
bool HoneydConfiguration::AddProfile(profile * profile)
{
	if(!m_profiles.keyExists(profile->name))
	{
		m_profiles[profile->name] = *profile;
		CreateProfileTree(profile->name);
		UpdateProfileTree(profile->name, ALL);
		return true;
	}
	return false;
}

bool HoneydConfiguration::RenameProfile(string oldName, string newName)
{
	//If item text and profile name don't match, we need to update
	if(oldName.compare(newName) && (m_profiles.keyExists(oldName)))
	{
		//Set the profile to the correct name and put the profile in the table
		m_profiles[oldName].name = newName;
		m_profiles[newName] = m_profiles[oldName];

		//Find all nodes who use this profile and update to the new one
		for(NodeTable::iterator it = m_nodes.begin(); it != m_nodes.end(); it++)
		{
			if(!it->second.pfile.compare(oldName))
			{
				it->second.pfile = newName;
			}
		}

		for(ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
		{
			if(!it->second.parentProfile.compare(oldName))
			{
				it->second.parentProfile = newName;
			}
		}
		DeleteProfile(oldName);
		UpdateProfileTree(newName, ALL);
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
			string oldParent = m_profiles[child].parentProfile;
			m_profiles[child].parentProfile = parent;
			//If the child has an old parent
			if((oldParent.compare("")) && (m_profiles.keyExists(oldParent)))
			{
				UpdateProfileTree(parent, ALL);
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
		if(!it->second.parentProfile.compare(""))
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

	m_nodes[nodeName].enabled = true;

	// Enable the subnet of this node
	m_subnets[m_nodes[nodeName].sub].enabled = true;

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

	m_nodes[nodeName].enabled = false;
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
	subnet * s = &m_subnets[m_nodes[nodeName].sub];
	for(uint i = 0; i < s->nodes.size(); i++)
	{
		if(!s->nodes[i].compare(nodeName))
		{
			s->nodes.erase(s->nodes.begin()+i);
		}
	}

	//Delete the node
	m_nodes.erase(nodeName);
	return true;
}

Node * HoneydConfiguration::GetNode(std::string nodeName)
{
	// Make sure the node exists
	if(m_nodes.find(nodeName) == m_nodes.end())
	{
		LOG(ERROR, string("There was an attempt to retrieve a honeyd node (name = ")
			+ nodeName + string(" that doesn't exist"), "");
		return NULL;
	}
	Node *ret = new Node();
	*ret = m_nodes[nodeName];
	return ret;
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
	return m_nodes[nodeName].sub;
}

void HoneydConfiguration::DisableProfileNodes(std::string profileName)
{
	for(NodeTable::iterator it = m_nodes.begin(); it != m_nodes.end(); it++)
	{
		if(!it->second.pfile.compare(profileName))
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
			for(uint i = 0; (i < jt->second.ports.size()) && !found; i++)
			{
				if(!jt->second.ports[i].first.compare(it->first))
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

bool HoneydConfiguration::AddNewNode(std::string profileName, string ipAddress, std::string macAddress, std::string interface, std::string subnet)
{
	Node newNode;
	newNode.IP = ipAddress;
	newNode.interface = interface;
	cout << "Adding new node " << profileName << ipAddress << macAddress << interface << subnet <<endl;

	if(newNode.IP != "DHCP")
	{
		newNode.realIP = htonl(inet_addr(newNode.IP.c_str()));
	}

	// Figure out it's subnet
	if(subnet == "")
	{
		if(newNode.IP == "DHCP")
		{
			newNode.sub = newNode.interface;
		}
		else
		{
			newNode.sub = FindSubnet(newNode.realIP);
		}
	}
	else
	{
		newNode.sub = subnet;
	}


	newNode.pfile = profileName;
	newNode.enabled = true;
	newNode.MAC = macAddress;

	newNode.name = newNode.IP + " - " + newNode.MAC;

	uint j = ~0;
	stringstream ss;
	if(newNode.name == "DHCP - RANDOM")
	{
		//Finds a unique identifier
		uint i = 1;
		while((m_nodes.keyExists(newNode.name)) && (i < j))
		{
			i++;
			ss.str("");
			ss << "DHCP - RANDOM(" << i << ")";
			newNode.name = ss.str();
		}
	}

	m_nodes[newNode.name] = newNode;
	if(newNode.sub != "")
	{
		m_subnets[newNode.sub].nodes.push_back(newNode.name);
	}
	else
	{
		LOG(WARNING, "No subnet was set for new node. This could make certain features unstable", "");
	}

	//TODO add error checking
	return true;
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
		return false;
	}
	//Recursive descent to find and call delete on any children of the profile
	for(ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
	{
		//If the profile at the iterator is a child of this profile
		if(!it->second.parentProfile.compare(profileName))
		{
			DeleteProfile(it->first, false);
		}
	}
	profile p = m_profiles[profileName];

	//Delete any nodes using the profile
	vector<string> delList;
	for(NodeTable::iterator it = m_nodes.begin(); it != m_nodes.end(); it++)
	{
		if(!it->second.pfile.compare(p.name))
		{
			delList.push_back(it->second.name);
		}
	}
	while(!delList.empty())
	{
		m_nodes.erase(m_nodes[delList.back()].name);
		delList.pop_back();
	}
	m_profiles.erase(profileName);

	//If it is not the original profile to be deleted skip this part
	if(originalCall)
	{
		//If this profile has a parent
		if(m_profiles.keyExists(p.parentProfile))
		{
			//save a copy of the parent
			profile parent = m_profiles[p.parentProfile];

			//point to the profiles subtree of parent-copy ptree and clear it
			ptree * pt = &parent.tree.get_child("profiles");
			pt->clear();

			//Find all profiles still in the table that are sibilings of deleted profile
			//* We should be using an iterator to find the original profile and erase it
			//* but boost's iterator implementation doesn't seem to be able to access data
			//* correctly and are frequently invalidated.

			for(ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
			{
				if(!it->second.parentProfile.compare(parent.name))
				{
					//Put sibiling profiles into the tree
					pt->add_child("profile", it->second.tree);
				}
			}	//parent-copy now has the ptree of all children except deleted profile

			//point to the original parent's profiles subtree and replace it with our new ptree
			ptree * treePtr = &m_profiles[p.parentProfile].tree.get_child("profiles");
			treePtr->clear();
			*treePtr = *pt;

			//Updates all ancestors with the deletion
			UpdateProfileTree(profileName, UP);
		}
		else
		{
			LOG(ERROR, string("Parent profile with name: ") + p.parentProfile + string(" doesn't exist"), "");
		}
	}
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
	else if(m_profiles[profileName].name.compare(profileName))
	{
		LOG(DEBUG, "Profile key: " + profileName + " does not match profile name of: "
			+ m_profiles[profileName].name + ". Setting profile name to the value of profile key.", "");
			m_profiles[profileName].name = profileName;
	}
	//Copy the profile
	profile p = m_profiles[profileName];
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
			if(!it->second.parentProfile.compare(p.name))
			{
				CreateProfileTree(it->second.name);
				//Update the child
				UpdateProfileTree(it->second.name, DOWN);
				//Put the child in the parent's ptree
				p.tree.add_child("profiles.profile", it->second.tree);
			}
		}
		m_profiles[profileName] = p;
	}
	//If the original calling profile has a parent to update
	if(p.parentProfile.compare("") && up)
	{
		//Get the parents name and create an empty ptree
		profile parent = m_profiles[p.parentProfile];
		ptree pt;
		pt.clear();
		pt.add_child("profile", p.tree);

		//Find all children of the parent and put them in the empty ptree
		// Ideally we could just replace the individual child but the data structure doesn't seem
		// to support this very well when all keys in the ptree (ie. profiles.profile) are the same
		// because the ptree iterators just don't seem to work correctly and documentation is very poor
		for(ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
		{
			if(!it->second.parentProfile.compare(parent.name))
			{
				pt.add_child("profile", it->second.tree);
			}
		}
		//Replace the parent's profiles subtree (stores all children) with the new one
		parent.tree.put_child("profiles", pt);
		m_profiles[parent.name] = parent;
		//Recursively ascend to update all ancestors
		CreateProfileTree(parent.name);
		UpdateProfileTree(parent.name, UP);
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
	profile p = m_profiles[profileName];
	if(p.name.compare(""))
	{
		temp.put<std::string>("name", p.name);
	}
	if(p.tcpAction.compare("") && !p.inherited[TCP_ACTION])
	{
		temp.put<std::string>("set.TCP", p.tcpAction);
	}
	if(p.udpAction.compare("") && !p.inherited[UDP_ACTION])
	{
		temp.put<std::string>("set.UDP", p.udpAction);
	}
	if(p.icmpAction.compare("") && !p.inherited[ICMP_ACTION])
	{
		temp.put<std::string>("set.ICMP", p.icmpAction);
	}
	if(p.personality.compare("") && !p.inherited[PERSONALITY])
	{
		temp.put<std::string>("set.personality", p.personality);
	}
	if(p.ethernet.compare("") && !p.inherited[ETHERNET])
	{
		temp.put<std::string>("set.ethernet", p.ethernet);
	}
	if(p.uptimeMin.compare("") && !p.inherited[UPTIME])
	{
		temp.put<std::string>("set.uptimeMin", p.uptimeMin);
	}
	if(p.uptimeMax.compare("") && !p.inherited[UPTIME])
	{
		temp.put<std::string>("set.uptimeMax", p.uptimeMax);
	}
	if(p.dropRate.compare("") && !p.inherited[DROP_RATE])
	{
		temp.put<std::string>("set.dropRate", p.dropRate);
	}

	//Populates the ports, if none are found create an empty field because it is expected.
	ptree pt;
	if(p.ports.size())
	{
		temp.put_child("add.ports",pt);
		for(uint i = 0; i < p.ports.size(); i++)
		{
			//If the port isn't inherited
			if(!p.ports[i].second)
			{
				temp.add<std::string>("add.ports.port", p.ports[i].first);
			}
		}
	}
	else
	{
		temp.put_child("add.ports",pt);
	}
	//put empty ptree in profiles as well because it is expected, does not matter that it is the same
	// as the one in add.ports if profile has no ports, since both are empty.
	temp.put_child("profiles", pt);

	for(ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
	{
		if(!it->second.parentProfile.compare(profileName))
		{
			temp.add_child("profiles.profile", it->second.tree);
		}
	}

	//copy the tree over and update ancestors
	p.tree = temp;
	m_profiles[profileName] = p;
	return true;
}
}


