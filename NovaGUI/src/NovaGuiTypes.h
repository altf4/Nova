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
#include <QtGui/QTreeWidgetItem>
#include <arpa/inet.h>
#include <string>

/*********************************************************************
 - Structs and Tables for quick item access through pointers -
**********************************************************************/
#define INHERITED_MAX 8

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
	QTreeWidgetItem * item;
	std::string portName;
	std::string portNum;
	std::string type;
	std::string behavior;
	std::string scriptName;
	std::string proxyIP;
	std::string proxyPort;
	boost::property_tree::ptree tree;
};
//Container for accessing port items
typedef google::dense_hash_map<std::string, port, std::tr1::hash<std::string>, eqstr > PortTable;

//used to keep track of subnet gui items and allow for easy access
struct subnet
{
	QTreeWidgetItem * item;
	QTreeWidgetItem * nodeItem;
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
	QTreeWidgetItem * item;
	QTreeWidgetItem * profileItem;
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
};

//Container for accessing profile items
typedef google::dense_hash_map<std::string, profile, std::tr1::hash<std::string>, eqstr > ProfileTable;

//used to keep track of haystack node gui items and allow for easy access
struct node
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
};

//Container for accessing node items
typedef google::dense_hash_map<std::string, node, std::tr1::hash<std::string>, eqstr > NodeTable;

#endif /* NOVAGUITYPES_H_ */
