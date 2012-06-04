#ifndef HONEYDHOSTCONFIG_
#define HONEYDHOSTCONFIG_

#include "HoneydConfiguration.h"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <netdb.h>
#include <iostream>
#include <net/if.h>
#include <sys/un.h>
#include <ifaddrs.h>
#include <exception>
#include <algorithm>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

std::vector<struct profile_read> profile_vec;
std::vector<uint16_t> leftoverHostspace;
uint16_t tempHostspace;
typedef Nova::HashMap<std::string, uint16_t, std::tr1::hash<std::string>, eqstr > Pers_Table;
typedef Nova::HashMap<std::string, uint16_t, std::tr1::hash<std::string>, eqstr > Ports_Table;
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

#endif
