#ifndef HONEYDHOSTCONFIG_
#define HONEYDHOSTCONFIG_

#include "HashMapStructs.h"
#include "HashMap.h"
#include "PersonalityTable.h"
#include <boost/property_tree/ptree.hpp>

typedef Nova::HashMap<std::string, std::pair<uint16_t,std::string>, std::tr1::hash<std::string>, eqstr > Pers_Table;
typedef Nova::HashMap<std::string, uint16_t, std::tr1::hash<std::string>, eqstr > Ports_Table;
// make a vendor MAC db to query for MAC

namespace Nova
{
enum ERRCODES {OKAY, AUTODETECTFAIL, GETNAMEINFOFAIL, GETBITMASKFAIL};
Pers_Table personalities;
Ports_Table ports;

struct port_read
{
	int open_ports;
	std::vector<std::pair<int, std::pair<std::string, std::string> > > port_services;
	std::vector<std::string> port_state;
};

struct profile_read
{
	std::string address;
	std::string personality;
	std::string personality_class;
	std::string ethernet_vendor;
	struct port_read ports;
};

std::vector<struct profile_read> load_nmap(const std::string &filename);
struct profile_read parseHost(boost::property_tree::ptree pt2);
std::vector<std::string> getSubnetsToScan();
void calculateDistributionMetrics();
}
#endif
