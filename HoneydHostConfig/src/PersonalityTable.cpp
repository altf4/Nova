#include "PersonalityTable.h"

using namespace std;

namespace Nova
{
PersonalityTable::PersonalityTable()
{
	m_personalities.set_empty_key("");
	m_num_of_hosts = 0;
	m_host_addrs_avail = 0;
}

PersonalityTable::~PersonalityTable()
{}

// Add a single Host
void PersonalityTable::AddHost(Personality * add)
{
	m_num_of_hosts++;
	m_host_addrs_avail--;

	if(m_personalities.find(add->m_personalityClass[0]) == m_personalities.end())
	{
		m_personalities[add->m_personalityClass[0]] = add;
	}
	else
	{
		Personality * cur = m_personalities[add->m_personalityClass[0]];
		cur->m_macs.push_back(add->m_macs[0]);
		cur->m_addresses.push_back(add->m_addresses[0]);
		cur->m_count++;

		for(Port_Table::iterator it = add->m_ports.begin(); it != add->m_ports.end(); it++)
		{
			if(cur->m_ports.find(it->first) == cur->m_ports.end())
			{
				pair<uint16_t, string> push;
				push.first = 1;
				push.second = it->second.second;
				cur->m_ports[it->first] = push;
			}
			else
			{
				cur->m_ports[it->first].first++;
			}
			cur->m_port_count++;
		}


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
		// rest of stuff

		delete add;
	}
}
}
