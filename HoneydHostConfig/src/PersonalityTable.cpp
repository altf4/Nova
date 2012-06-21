//============================================================================
// Name        : PersonalityTable.cpp
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
// Description : This source file defines the functions for the PersonalityTable
//               object that contains a map of Personality objects as well as
//               ways to add and view information for the table.
//============================================================================

#include "PersonalityTable.h"

using namespace std;

namespace Nova
{

PersonalityTable::PersonalityTable()
{
	m_personalities.set_empty_key("");
	m_num_of_hosts = 0;
	m_num_used_hosts = 0;
	m_host_addrs_avail = 0;
}

PersonalityTable::~PersonalityTable()
{

}

void PersonalityTable::ListInfo()
{
	// Just a simple print method for outputting the
	// relevant information for each element of the PersonalityTable,
	// as well as some information about the table itself.
	std::cout << std::endl;
	std::cout << "Number of hosts found: " << m_num_of_hosts << "." << std::endl;
	std::cout << "Number of hosts that yielded personalities: " << m_num_used_hosts << "." << std::endl;
	std::cout << "Hostspace left over: " << m_host_addrs_avail << " addresses." << std::endl;

	for(Personality_Table::iterator it = m_personalities.begin(); it != m_personalities.end(); it++)
	{
		std::cout << std::endl;

		std::cout << it->second->m_personalityClass[0] << ": ";

		for(uint16_t i = it->second->m_personalityClass.size() - 1; i > 1 ; i--)
		{
			std::cout << it->second->m_personalityClass[i] << " | ";
		}

		std::cout << it->second->m_personalityClass[1];

		std::cout << std::endl;

		std::cout << "Appeared " << it->second->m_count << " time(s) on the network" << std::endl;

		std::cout << "Associated IPs: " << std::endl;
		for(uint16_t i = 0; i < it->second->m_addresses.size(); i++)
		{
			std::cout << "\t" << it->second->m_addresses[i] << std::endl;
		}

		std::cout << "Associated MACs: " << std::endl;

		for(uint16_t j = 0; j < it->second->m_macs.size(); j++)
		{
			std::cout << "\t" << it->second->m_macs[j] << std::endl;
		}

		std::cout << "All MAC vendors associated with this Personality: " << std::endl;

		for(MAC_Table::iterator it2 = it->second->m_vendors.begin(); it2 != it->second->m_vendors.end(); it2++)
		{
			std::cout << "\t" << it2->first << " occurred " << it2->second << " time(s)." << std::endl;
		}

		std::cout << "Ports for this Personality: " << std::endl;

		for(PortsTable::iterator it2 = it->second->m_ports.begin(); it2 != it->second->m_ports.end(); it2++)
		{
			std::cout << "\t" << it2->first << " occurred " << it2->second.first << " time(s)." << std::endl;
		}
	}
}

// Add a single Host
void PersonalityTable::AddHost(Personality *add)
{
	// We've gotten to the point where we're adding a host that
	// has personality data, so increment the number of useful
	// hosts.
	m_num_used_hosts++;

	// If the personality pointer that we're passing to the function
	// doesn't exist in the personalities table, just create a new
	// instance using the bracket operator and assign the Value to
	// the pointer we're passing.
	if(m_personalities.find(add->m_personalityClass[0]) == m_personalities.end())
	{
		m_personalities[add->m_personalityClass[0]] = add;
	}
	// If the personality does exist, however, then
	// we need to do some extra work to aggregate its attributes
	// with the existing entry.
	else
	{
		// Create a copy of the pointer that exists within the table
		// to modify.
		Personality *cur = m_personalities[add->m_personalityClass[0]];
		// Since we're adding a host, just grab the single IP and MAC
		// address that exist inside of the Personality object and
		// increment the number of times a host has had this personality
		// by one.
		cur->m_macs.push_back(add->m_macs[0]);
		cur->m_addresses.push_back(add->m_addresses[0]);
		cur->m_count++;

		// Iterate through the Port_Table in the copy and update the
		// counts for the occurrence of each port
		for(PortsTable::iterator it = add->m_ports.begin(); it != add->m_ports.end(); it++)
		{
			if(cur->m_ports.find(it->first) == cur->m_ports.end())
			{
				cur->m_ports[it->first].first = 1;
				cur->m_ports[it->first].second = it->second.second;
			}
			else
			{
				cur->m_ports[it->first].first++;
			}
			cur->m_port_count++;
		}
		// and do the same for the MAC vendors in the MAC_Table.
		for(MAC_Table::iterator it = add->m_vendors.begin(); it != add->m_vendors.end(); it++)
		{
			if(cur->m_vendors.find(it->first) == cur->m_vendors.end())
			{
				cur->m_vendors[it->first] = 1;
			}
			else
			{
				cur->m_vendors[it->first]++;
			}
		}
		// Finally, delete the pointer we passed to deallocate
		// its memory.
		delete add;
	}
}

}
