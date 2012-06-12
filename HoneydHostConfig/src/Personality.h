//============================================================================
// Name        : Personality.h
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
// Description : This header file for Personality.cpp contains the
//               function definitions and declarations of member variables
//               for the Personality object.
//============================================================================

#include "AutoConfigHashMaps.h"

namespace Nova
{

class Personality
{

public:

	Personality();
	~Personality();

	// Adds a vendor to the m_vendors member variable HashMap
	//  std::string vendor - the string representation of the vendor taken from the nmap xml
	// Returns nothing; if the vendor is there, it increments the count, if it isn't it adds it
	// and sets count to one.
	void AddVendor(std::string);

	// Adds a port to the m_ports member variable HashMap
	//  std::string port - of the format <NUM>_<PROTOCOL>, to pass to the HashMap
	// Returns nothing; if the vendor is there, it increments the count, if it isn't it adds it
	// and sets count to one.
	void AddPort(std::string);

	// count of the number of instances of this personality on the scanned subnets
	unsigned int m_count;

	// total number of ports found for this personality on the scanned subnets
	unsigned long long int m_port_count;

	// pushed in this order: name, type, osgen, osfamily, vendor
	// pops in this order: vendor, osfamily, osgen, type, name
	std::vector<std::string> m_personalityClass;

	// vector of MAC addresses
	std::vector<std::string> m_macs;

	// vector of IP addresses
	std::vector<std::string> m_addresses;

	//HashMap of MACs; Key is Vendor, Value is number of times the MAC vendor is seen for hosts of this personality type
	MAC_Table m_vendors;

	//HashMap of ports; Key is port (format: <NUM>_<PROTOCOL>), Value is a uint16_t count
	Port_Table m_ports;
};

}

