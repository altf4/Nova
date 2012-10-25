//============================================================================
// Name        : ScannedHost.cpp
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
// Description : Represents one or more hosts that have been scanned by Nmap and
//	identified as the same OS.
//============================================================================

#include "ScannedHost.h"

using namespace std;

namespace Nova
{
ScannedHost::ScannedHost()
{
	m_count = 1;
	m_distribution = 100;
	m_port_count = 0;
	m_osclass = "";
	m_vendors.set_empty_key("");
}

ScannedHost::~ScannedHost()
{

}

void ScannedHost::AddVendor(const string &vendor)
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

}
