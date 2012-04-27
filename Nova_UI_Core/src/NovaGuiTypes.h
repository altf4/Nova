//============================================================================
// Name        : NovaGuiTypes.h
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

#include "HashMapStructs.h"

#include <boost/property_tree/ptree.hpp>
#include <arpa/inet.h>
#include <string>

/*********************************************************************
 - Structs and Tables for quick item access through pointers -
**********************************************************************/
#define INHERITED_MAX 8


namespace Nova
{


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
struct script
{
	std::string name;
	std::string path;
	boost::property_tree::ptree tree;
};

//Container for accessing script items
typedef google::dense_hash_map<std::string, script, std::tr1::hash<std::string>, eqstr > ScriptTable;

//used to maintain information about a port, it's type and behavior
struct port
{
	std::string portName;
	std::string portNum;
	std::string type;
	std::string behavior;
	std::string scriptName;
	std::string proxyIP;
	std::string proxyPort;
	boost::property_tree::ptree tree;

	// This is for the Javascript web interface bindings
	inline std::string GetPortName() {return portName;}
	inline std::string GetPortNum() {return portNum;}
	inline std::string GetType() {return type;}
	inline std::string GetBehavior() {return behavior;}
	inline std::string GetScriptName() {return scriptName;}
	inline std::string GetProxyIP() {return proxyIP;}
	inline std::string GetProxyPort() {return proxyPort;}

};
//Container for accessing port items
typedef google::dense_hash_map<std::string, port, std::tr1::hash<std::string>, eqstr > PortTable;

//used to keep track of subnet gui items and allow for easy access
struct subnet
{
	std::string name;
	std::string address;
	std::string mask;
	int maskBits;
	in_addr_t base;
	in_addr_t max;
	bool enabled;
	bool isRealDevice;
	std::vector<std::string> nodes;
	boost::property_tree::ptree tree;
};

//Container for accessing subnet items
typedef google::dense_hash_map<std::string, subnet, std::tr1::hash<std::string>, eqstr > SubnetTable;


//used to keep track of haystack profile gui items and allow for easy access
struct profile
{
	std::string name;
	std::string tcpAction;
	std::string udpAction;
	std::string icmpAction;
	std::string personality;
	std::string ethernet;
	std::string uptimeMin;
	std::string uptimeMax;
	std::string dropRate;
	bool inherited[INHERITED_MAX];
	std::vector<std::pair<std::string, bool> > ports;
	std::string parentProfile;
	boost::property_tree::ptree tree;



	// This is for the Javascript bindings in the web interface
	inline std::string GetName() {return name;}
	inline std::string GetTcpAction() {return tcpAction;}
	inline std::string GetUdpAction() {return udpAction;}
	inline std::string GetIcmpAction() {return icmpAction;}
	inline std::string GetPersonality() {return personality;}
	inline std::string GetEthernet() {return ethernet;}
	inline std::string GetUptimeMin() {return uptimeMin;}
	inline std::string GetUptimeMax() {return uptimeMax;}
	inline std::string GetDropRate() {return dropRate;}
	inline std::string GetParentProfile() {return parentProfile;}

	inline std::vector<bool> GetInheritance()
	{
		std::vector<bool> ret;
		for (int i = 0; i < INHERITED_MAX; i++)
		{
			ret.push_back(inherited[i]);
		}
		return ret;
	}

	inline bool isTcpActionInherited() {return inherited[TCP_ACTION];}
	inline bool isUdpActionInherited() {return inherited[UDP_ACTION];}
	inline bool isIcmpActionInherited() {return inherited[ICMP_ACTION];}
	inline bool isPersonalityInherited() {return inherited[PERSONALITY];}
	inline bool isEthernetInherited() {return inherited[ETHERNET];}
	inline bool isUptimeInherited() {return inherited[UPTIME];}
	inline bool isDropRateInherited() {return inherited[DROP_RATE];}

	// Work around for inability to get the std::pair to javascript
	inline std::vector<std::string> GetPortNames()
	{
		std::vector<std::string> ret;
		for (std::vector<std::pair<std::string, bool>>::iterator it = ports.begin(); it != ports.end(); it++)
		{
			ret.push_back(it->first);
		}
		return ret;
	}

	// Work around for inability to get the std::pair to javascript
	inline std::vector<bool> GetPortInheritance()
	{
		std::vector<bool> ret;
		for (std::vector<std::pair<std::string, bool>>::iterator it = ports.begin(); it != ports.end(); it++)
		{
			ret.push_back(it->second);
		}
		return ret;
	}
};

//Container for accessing profile items
typedef google::dense_hash_map<std::string, profile, std::tr1::hash<std::string>, eqstr > ProfileTable;

//used to keep track of haystack node gui items and allow for easy access
struct Node
{
	std::string name;
	std::string sub;
	std::string interface;
	std::string pfile;
	std::string IP;
	std::string MAC;
	in_addr_t realIP;
	bool enabled;
	boost::property_tree::ptree tree;

	// This is for the Javascript bindings in the web interface
	inline std::string GetName() {return name;}
	inline std::string GetSubnet() {return sub;}
	inline std::string GetInterface() {return interface;}
	inline std::string GetProfile() {return pfile;}
	inline std::string GetIP() {return IP;}
	inline std::string GetMAC() {return MAC;}
	inline bool IsEnabled() {return enabled;}
};

//Container for accessing node items
typedef google::dense_hash_map<std::string, Node, std::tr1::hash<std::string>, eqstr > NodeTable;

}


#endif /* NOVAGUITYPES_H_ */
