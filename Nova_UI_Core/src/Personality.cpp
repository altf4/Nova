//============================================================================
// Name        : Personality.cpp
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
// Description : Source file that contains the definition of a Personality object;
//               This object aggregates all the information for a given nmap personality
//               across the subnet(s). The source file adds the object to Nova namespace
//               as well as defines different functions for interacting with the object.
//============================================================================

#include "Personality.h"

using namespace std;

namespace Nova
{
Personality::Personality()
{
	m_count = 1;
	m_port_count = 0;
	m_osclass = "";
	m_vendors.set_empty_key("");
	m_ports.set_empty_key("");
}

Personality::~Personality()
{

}

void Personality::AddVendor(const string &vendor)
{
	// If the vendor does not exist in MAC_Table,
	// then use the bracket operator to make a new
	// entry and set its count to 1
	if(m_vendors.find(vendor) == m_vendors.end())
	{
		m_vendors[vendor] = 1;
	}
	// Otherwise just increase the count at that key
	else
	{
		m_vendors[vendor]++;
	}
}

void Personality::AddPort(const string &port_string, const string &port_service)
{
	// If the vendor does not exist in Port_Table,
	// then use the bracket operator to make a new
	// entry and set its count to 1
	if(m_ports.find(port_string) == m_ports.end())
	{
		m_ports[port_string].first = 1;
		m_ports[port_string].second = port_service;
	}
	// Otherwise just increase the count at that key
	else
	{
		m_ports[port_string].first++;
	}
}
}
