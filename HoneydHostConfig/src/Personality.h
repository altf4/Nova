/*
 * Personality.h
 *
 *  Created on: Jun 1, 2012
 *      Author: victim
 */

#include "HashMapStructs.h"
#include "HashMap.h"

using namespace std;

class Personality
{

public:

	Personality();
	~Personality();

	unsigned int m_count;
	unsigned long long int m_port_count;
	std::string m_personalityClass;

	//HashMAP[std::string key]; key == MAC Vendor, val == num of times mac vendor is seen for hosts
	typedef Nova::HashMap<std::string, std::pair<uint16_t,std::string>, std::tr1::hash<std::string>, eqstr > MAC_Table;
	//HashMap[std::string key]; key == port in format <NUM>_<PROTOCOL>, val == pair<uint count, string of nmap service info>
	typedef Nova::HashMap<std::string, std::pair<uint16_t, std::string>, std::tr1::hash<std::string>, eqstr > Port_Table;
	// vector of MAC addresses
	// vector of IP addresses

};


