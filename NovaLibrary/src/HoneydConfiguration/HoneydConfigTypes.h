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
//============================================================================

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
std::string PortBehaviorToString(enum PortBehavior behavior);

//Returns a PortBehavior enum which is represented by the given string
//	returns PORT_ERROR on error
enum PortBehavior StringToPortBehavior(std::string behavior);

enum PortProtocol
{
	PROTOCOL_UDP = 0
	, PROTOCOL_TCP
	, PROTOCOL_ICMP
	, PROTOCOL_ERROR
};
//Returns a string representation of the given PortProtocol
//	Returns one of: "udp" "tcp" "icmp" or "" for error
std::string PortProtocolToString(enum PortProtocol protocol);

//Returns a PortProtocol enum which is represented by the given string
//	returns PROTOCOL_ERROR on error
enum PortProtocol StringToPortProtocol(std::string protocol);

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

}

#endif /* NOVAGUITYPES_H_ */
