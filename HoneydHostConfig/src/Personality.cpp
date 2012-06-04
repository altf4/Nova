#include "Personality.h"

using namespace std;

namespace Nova
{
Personality::Personality()
{
	m_count = 0;
	m_port_count = 0;
	m_vendors.set_empty_key("");
	m_ports.set_empty_key("");
}

Personality::~Personality()
{

}

void Personality::AddVendor(string vendor)
{
	if(m_vendors.find(vendor) == m_vendors.end())
	{
		m_vendors[vendor] = 1;
	}
	else
	{
		m_vendors[vendor]++;
	}
}

void Personality::AddPort(string port_string, pair<uint16_t, string> port_content)
{
	if(m_ports.find(port_string) == m_ports.end())
	{
		//probably can't do this, check and make sure
		m_ports[port_string] = port_content;
	}
	else
	{
		m_ports[port_string].first++;
	}
}
}
