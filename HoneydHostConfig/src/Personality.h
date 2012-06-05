/*
 * Personality.h
 *
 *  Created on: Jun 1, 2012
 *      Author: victim
 */

#include "AutoConfigHashMaps.h"

namespace Nova
{

class Personality
{
public:

	Personality();
	~Personality();

	void AddVendor(std::string);

	void AddPort(std::string, std::string);

	unsigned int m_count;
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

