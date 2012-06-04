/*
 * Personality.h
 *
 *  Created on: Jun 1, 2012
 *      Author: victim
 */

#include "HashMapStructs.h"
#include "HashMap.h"

namespace Nova
{
typedef Nova::HashMap<std::string, uint16_t, std::tr1::hash<std::string>, eqstr > MAC_Table;
typedef Nova::HashMap<std::string, std::pair<uint16_t, std::string>, std::tr1::hash<std::string>, eqstr > Port_Table;

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
	//HashMap of ports; Key is port (format: <NUM>_<PROTOCOL>), Value is a pair consisting of <uint16_t count, std::string nmap_service>
	Port_Table m_ports;
};
}

