//============================================================================
// Name        : HoneydConfigTypes.h
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
// Description : Common structs and typedefs for NovaGUI
//============================================================================/*

#ifndef NOVAGUITYPES_H_
#define NOVAGUITYPES_H_

#include "../HashMapStructs.h"

#include <boost/property_tree/ptree.hpp>
#include <arpa/inet.h>
#include <string>

/*********************************************************************
 - Structs and Tables for quick item access through pointers -
**********************************************************************/
#define INHERITED_MAX 8


namespace Nova
{

enum HoneydConfigReturn
{
	INHERITED
	, NOT_INHERITED
	, NO_SUCH_KEY
};

enum PortBehavior
{
	PORT_FILTERED = 0
	, PORT_CLOSED
	, PORT_OPEN
	, PORT_SCRIPT
	, PORT_TARPIT_OPEN
	, PORT_TARPIT_SCRIPT
	, PORT_ERROR
};
//Returns a string representation of the given PortBehavior
//	Returns one of: "closed" "open" "filtered" "script" "tarpit-open" "tarpit-script" or "" for error
static std::string PortBehaviorToString(enum PortBehavior behavior)
{
	switch(behavior)
	{
		case PORT_OPEN:
		{
			return "open";
		}
		case PORT_CLOSED:
		{
			return "closed";
		}
		case PORT_FILTERED:
		{
			return "filtered";
		}
		case PORT_SCRIPT:
		{
			return "script";
		}
		case PORT_TARPIT_OPEN:
		{
			return "tarpit-open";
		}
		case PORT_TARPIT_SCRIPT:
		{
			return "tarpit-script";
		}
		default:
		{
			return "";
		}
	}
}

//Returns a PortBehavior enum which is represented by the given string
//	returns PORT_ERROR on error
static enum PortBehavior StringToPortBehavior(std::string behavior)
{
	if(!behavior.compare("open"))
	{
		return PORT_OPEN;
	}
	else if(!behavior.compare("closed"))
	{
		return PORT_CLOSED;
	}
	else if(!behavior.compare("filtered"))
	{
		return PORT_FILTERED;
	}
	else if(!behavior.compare("script"))
	{
		return PORT_SCRIPT;
	}
	else if(!behavior.compare("tarpit-open"))
	{
		return PORT_TARPIT_OPEN;
	}
	else if(!behavior.compare("tarpit-script"))
	{
		return PORT_TARPIT_SCRIPT;
	}
	else
	{
		return PORT_ERROR;
	}
}

enum RecursiveDirection
{
	ALL = 0
	, UP
	, DOWN
};

enum PortProtocol
{
	PROTOCOL_UDP = 0
	, PROTOCOL_TCP
	, PROTOCOL_ICMP
	, PROTOCOL_ERROR
};
//Returns a string representation of the given PortProtocol
//	Returns one of: "udp" "tcp" "icmp" or "" for error
static std::string PortProtocolToString(enum PortProtocol protocol)
{
	switch(protocol)
	{
		case PROTOCOL_UDP:
		{
			return "udp";
		}
		case PROTOCOL_TCP:
		{
			return "tcp";
		}
		case PROTOCOL_ICMP:
		{
			return "icmp";
		}
		default:
		{
			return "";
		}
	}
}

//Returns a PortProtocol enum which is represented by the given string
//	returns PROTOCOL_ERROR on error
static enum PortProtocol StringToPortProtocol(std::string protocol)
{
	if(!protocol.compare("tcp"))
	{
		return PROTOCOL_TCP;
	}
	else if(!protocol.compare("udp"))
	{
		return PROTOCOL_UDP;
	}
	else if(!protocol.compare("icmp"))
	{
		return PROTOCOL_ICMP;
	}
	else
	{
		return PROTOCOL_ERROR;
	}
}

enum profileIndex {
	TYPE
	, TCP_ACTION
	, UDP_ACTION
	, ICMP_ACTION
	, PERSONALITY
	, ETHERNET
	, UPTIME
	, DROP_RATE
};

enum nodeConflictType : char
{
	NO_CONFLICT,
	IP_CONFLICT,
	MAC_CONFLICT
};

//used to maintain information on imported scripts
struct Script
{
	std::string m_name;
	std::string m_service;
	std::string m_osclass;
	std::string m_path;
	boost::property_tree::ptree m_tree;
};

//Container for accessing script items
typedef Nova::HashMap<std::string, Script, std::hash<std::string>, eqstr > ScriptTable;

//used to maintain information about a port, it's type and behavior
//	Useful as a compatibility layer into Quasar
struct PortStruct
{
	std::string m_portName;
	std::string m_portNum;
	std::string m_type;
	std::string m_service;
	std::string m_behavior;
	std::string m_scriptName;
	std::string m_proxyIP;
	std::string m_proxyPort;
	boost::property_tree::ptree m_tree;

	// This is only for the Javascript web interface, avoid use in C++
	bool m_isInherited;

	// This is for the Javascript web interface bindings
	inline std::string GetPortName() {return m_portName;}
	inline std::string GetPortNum() {return m_portNum;}
	inline std::string GetType() {return m_type;}
	inline std::string GetBehavior() {return m_behavior;}
	inline std::string GetScriptName() {return m_scriptName;}
	inline std::string GetProxyIP() {return m_proxyIP;}
	inline std::string GetProxyPort() {return m_proxyPort;}
	inline bool GetIsInherited() {return m_isInherited;}
	inline std::string GetService() {return m_service;}


};
//Container for accessing port items
typedef Nova::HashMap<std::string, PortStruct, std::hash<std::string>, eqstr > PortTable;

//used to keep track of subnet gui items and allow for easy access
struct Subnet
{
	std::string m_name;
	std::string m_address;
	std::string m_mask;
	int m_maskBits;
	in_addr_t m_base;
	in_addr_t m_max;
	bool m_enabled;
	bool m_isRealDevice;
	std::vector<std::string> m_nodes;
	boost::property_tree::ptree m_tree;
};


//used to keep track of haystack profile gui items and allow for easy access
struct NodeProfile
{
	std::string m_name;
	std::string m_tcpAction;
	std::string m_udpAction;
	std::string m_icmpAction;
	std::string m_personality;
	std::vector<std::pair<std::string, double>> m_ethernetVendors;
	std::string m_uptimeMin;
	std::string m_uptimeMax;
	std::string m_dropRate;
	bool m_inherited[INHERITED_MAX];
	bool m_generated;
	double m_distribution;
	std::vector<std::pair<std::string, std::pair<bool, double> > > m_ports;
	std::vector<std::string> m_nodeKeys;
	std::string m_parentProfile;
	boost::property_tree::ptree m_tree;

	// This is for the Javascript bindings in the web m_interface
	// They return bool because the templates can't handle void return types
	inline bool SetName(std::string name) {this->m_name = name; return true;}
	inline bool SetTcpAction(std::string tcpAction) {this->m_tcpAction = tcpAction; return true;}
	inline bool SetUdpAction(std::string udpAction) {this->m_udpAction = udpAction; return true;}
	inline bool SetIcmpAction(std::string icmpAction) {this->m_icmpAction = icmpAction; return true;}
	inline bool SetPersonality(std::string personality) {this->m_personality = personality; return true;}
	inline bool SetGenerated(bool generated) {this->m_generated = generated; return true;}
	inline bool SetDistribution(double distribution) {this->m_distribution = distribution; return true;}
	inline bool SetEthernet(std::string ethernet)
	{
		if(!this->m_ethernetVendors.empty())
		{
			this->m_ethernetVendors[0].first = ethernet;
			return true;
		}
		else
		{
			this->m_ethernetVendors.push_back(std::pair<std::string, double>(ethernet, 0));
			return true;
		}
	}
	inline bool SetVendors(std::vector<std::pair<std::string, double> > vendors)
	{
		m_ethernetVendors = vendors;
		return true;
	}
	inline bool SetUptimeMin(std::string uptimeMin) {this->m_uptimeMin = uptimeMin; return true;}
	inline bool SetUptimeMax(std::string uptimeMax) {this->m_uptimeMax = uptimeMax; return true;}
	inline bool SetDropRate(std::string dropRate) {this->m_dropRate = dropRate; return true;}
	inline bool SetParentProfile(std::string parentProfile) {this->m_parentProfile = parentProfile; return true;}
	inline bool AddPort(std::string portName, bool inherited, double distribution) {m_ports.push_back(std::pair<std::string, std::pair<bool, double> >(portName, std::pair<bool, double>(inherited, distribution))); return true;}

	inline bool setTcpActionInherited(bool inherit) {m_inherited[TCP_ACTION] = inherit; return true;}
	inline bool setUdpActionInherited(bool inherit) {m_inherited[UDP_ACTION] = inherit; return true;}
	inline bool setIcmpActionInherited(bool inherit) {m_inherited[ICMP_ACTION] = inherit; return true;}
	inline bool setPersonalityInherited(bool inherit) {m_inherited[PERSONALITY] = inherit; return true;}
	inline bool setEthernetInherited(bool inherit) {m_inherited[ETHERNET] = inherit; return true;}
	inline bool setUptimeInherited(bool inherit) {m_inherited[UPTIME] = inherit; return true;}
	inline bool setDropRateInherited(bool inherit) {m_inherited[DROP_RATE] = inherit; return true;}

	inline std::string GetRandomVendor()
	{
		srand(time(NULL));
		int randDist = rand() % 100;
		int breakEven = (rand() % m_ethernetVendors.size()) + 1;
		double max = 0;
		std::string ret = m_ethernetVendors[0].first;

		for(uint i = 0; i < m_ethernetVendors.size() && breakEven > 0; i++)
		{
			if(m_ethernetVendors[i].second > max && m_ethernetVendors[i].second < randDist)
			{
				max = m_ethernetVendors[i].second;
				ret = m_ethernetVendors[i].first;
				breakEven--;
			}
		}

		return ret;
	}

	// This is for the Javascript bindings in the web interface
	inline std::string GetName() {return m_name;}
	inline std::string GetTcpAction() {return m_tcpAction;}
	inline std::string GetUdpAction() {return m_udpAction;}
	inline std::string GetIcmpAction() {return m_icmpAction;}
	inline std::string GetPersonality() {return m_personality;}
	inline std::string GetEthernet() {return m_ethernetVendors[0].first;}

	inline std::vector<std::string> GetVendors()
	{
		std::vector<std::string> ret;
		for(uint i = 0; i < m_ethernetVendors.size(); i++)
		{
			ret.push_back(m_ethernetVendors[i].first);
		}
		return ret;
	}


	inline std::vector<double> GetVendorDistributions()
	{
		std::vector<double> ret;
		for(uint i = 0; i < m_ethernetVendors.size(); i++)
		{
			ret.push_back(m_ethernetVendors[i].second);
		}
		return ret;
	}
	inline std::string GetUptimeMin() {return m_uptimeMin;}
	inline std::string GetUptimeMax() {return m_uptimeMax;}
	inline std::string GetDropRate() {return m_dropRate;}
	inline std::string GetParentProfile() {return m_parentProfile;}

	inline bool GetGenerated() {return m_generated;}
	inline double GetDistribution() {return m_distribution;}

	inline std::vector<bool> GetInheritance()
	{
		std::vector<bool> ret;
		for(int i = 0; i < INHERITED_MAX; i++)
		{
			ret.push_back(m_inherited[i]);
		}
		return ret;
	}

	inline bool isTcpActionInherited() {return m_inherited[TCP_ACTION];}
	inline bool isUdpActionInherited() {return m_inherited[UDP_ACTION];}
	inline bool isIcmpActionInherited() {return m_inherited[ICMP_ACTION];}
	inline bool isPersonalityInherited() {return m_inherited[PERSONALITY];}
	inline bool isEthernetInherited() {return m_inherited[ETHERNET];}
	inline bool isUptimeInherited() {return m_inherited[UPTIME];}
	inline bool isDropRateInherited() {return m_inherited[DROP_RATE];}

	// Work around for inability to get the std::pair to javascript
	inline std::vector<std::string> GetPortNames()
	{
		std::vector<std::string> ret;
		for(std::vector<std::pair<std::string, std::pair<bool, double> > >::iterator it = m_ports.begin(); it != m_ports.end(); it++)
		{
			ret.push_back(it->first);
		}
		return ret;
	}

	// Work around for inability to get the std::pair to javascript
	inline std::vector<bool> GetPortInheritance()
	{
		std::vector<bool> ret;
		for(std::vector<std::pair<std::string, std::pair<bool, double> > >::iterator it = m_ports.begin(); it != m_ports.end(); it++)
		{
			ret.push_back(it->second.first);
		}
		return ret;
	}
};

typedef Nova::HashMap<std::string, NodeProfile, std::hash<std::string>, eqstr > ProfileTable;

//used to keep track of haystack node gui items and allow for easy access
struct Node
{
	std::string m_interface;
	std::string m_pfile;
	std::string m_portSetName;
	std::vector<bool> m_isPortInherited;
	std::string m_IP;
	std::string m_MAC;
	in_addr_t m_realIP;
	bool m_enabled;

	// This is for the Javascript bindings in the web interface
	inline std::string GetInterface() {return m_interface;}
	inline std::string GetProfile() {return m_pfile;}
	inline std::string GetIP() {return m_IP;}
	inline std::string GetMAC() {return m_MAC;}
	inline bool IsEnabled() {return m_enabled;}
};

//Container for accessing node items
//Key - String representation of the MAC address used for the node
//value - The Node object itself
typedef Nova::HashMap<std::string, Node, std::hash<std::string>, eqstr > NodeTable;

}


#endif /* NOVAGUITYPES_H_ */
