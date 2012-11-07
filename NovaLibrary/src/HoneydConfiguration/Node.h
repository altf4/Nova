//============================================================================
// Name        : Node.h
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
// Description : Represents a single haystack node
//============================================================================

#ifndef NODE_H_
#define NODE_H_

namespace Nova
{

class Node
{

public:

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

#endif /* NODE_H_ */
