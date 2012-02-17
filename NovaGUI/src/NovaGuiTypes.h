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

using namespace std;
using boost::property_tree::ptree;

/*********************************************************************
 - Structs and Tables for quick item access through pointers -
**********************************************************************/
#define INHERITED_MAX 8

enum profileType { static_IP = 0, staticDHCP = 1, randomDHCP = 2};

enum profileIndex { TYPE = 0, TCP_ACTION = 1, UDP_ACTION = 2, ICMP_ACTION = 3,
	PERSONALITY = 4, ETHERNET = 5, UPTIME = 6, DROP_RATE = 7};

//used to maintain information on imported scripts
struct script
{
	string name;
	string path;
	ptree tree;
};

//Container for accessing script items
typedef google::dense_hash_map<string, script, tr1::hash<string>, eqstr > ScriptTable;

//used to maintain information about a port, it's type and behavior
struct port
{
	QTreeWidgetItem * item;
	string portName;
	string portNum;
	string type;
	string behavior;
	string scriptName;
	string proxyIP;
	string proxyPort;
	ptree tree;
};
//Container for accessing port items
typedef google::dense_hash_map<string, port, tr1::hash<string>, eqstr > PortTable;

//used to keep track of subnet gui items and allow for easy access
struct subnet
{
	QTreeWidgetItem * item;
	QTreeWidgetItem * nodeItem;
	string name;
	string address;
	string mask;
	int maskBits;
	in_addr_t base;
	in_addr_t max;
	bool enabled;
	bool isRealDevice;
	vector<string> nodes;
	ptree tree;
};

//Container for accessing subnet items
typedef google::dense_hash_map<string, subnet, tr1::hash<string>, eqstr > SubnetTable;


//used to keep track of haystack profile gui items and allow for easy access
struct profile
{
	QTreeWidgetItem * item;
	QTreeWidgetItem * profileItem;
	string name;
	string tcpAction;
	string udpAction;
	string icmpAction;
	string personality;
	string ethernet;
	string uptime;
	string uptimeRange;
	string dropRate;
	profileType type;
	bool inherited[INHERITED_MAX];
	vector<pair<string, bool> > ports;
	string parentProfile;
	ptree tree;
};

//Container for accessing profile items
typedef google::dense_hash_map<string, profile, tr1::hash<string>, eqstr > ProfileTable;

//used to keep track of haystack node gui items and allow for easy access
struct node
{
	string name;
	QTreeWidgetItem * item;
	QTreeWidgetItem * nodeItem;
	string sub;
	string interface;
	string pfile;
	string IP;
	string MAC;
	in_addr_t realIP;
	bool enabled;
	ptree tree;
};

//Container for accessing node items
typedef google::dense_hash_map<string, node, tr1::hash<string>, eqstr > NodeTable;

#endif /* NOVAGUITYPES_H_ */
