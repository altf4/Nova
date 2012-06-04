
// REQUIRES NMAP 6

#include "honeydhostconfig.h"
#include <arpa/inet.h>
#include <vector>
#include <sys/stat.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <sys/un.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <string>
#include <exception>
#include <algorithm>

#include "PersonalityTable.h"
#include "ScriptTable.h"
#include "Personality.h"

struct profile_read parseHost(boost::property_tree::ptree pt2)
{
	using boost::property_tree::ptree;

	struct profile_read ret;

	int i = 0;

	BOOST_FOREACH(ptree::value_type &v, pt2.get_child(""))
	{
		try
		{
			if(!v.first.compare("address"))
			{
				if(!v.second.get<std::string>("<xmlattr>.addrtype").compare("ipv4"))
				{
					ret.address = v.second.get<std::string>("<xmlattr>.addr");
				}
				else if(!v.second.get<std::string>("<xmlattr>.addrtype").compare("mac"))
				{
					ret.ethernet_vendor = v.second.get<std::string>("<xmlattr>.vendor");
				}
			}
			else if(!v.first.compare("ports"))
			{
				BOOST_FOREACH(ptree::value_type &c, v.second.get_child(""))
				{
					try
					{
						std::pair<int, std::pair<std::string, std::string> > push;
						std::string state = c.second.get<std::string>("state.<xmlattr>.state");
						ret.ports.port_state.push_back(state);
						push.first = c.second.get<int>("<xmlattr>.portid");
						push.second.first = c.second.get<std::string>("<xmlattr>.protocol");
						push.second.second = c.second.get<std::string>("service.<xmlattr>.name");
						ret.ports.port_services.push_back(push);
						i++;
					}
					catch(std::exception &e)
					{

					}
				}
			}
			else if(!v.first.compare("os"))
			{
				if(pt2.get<std::string>("os.osmatch.<xmlattr>.name").compare(""))
				{
					ret.personality = pt2.get<std::string>("os.osmatch.<xmlattr>.name");
				}
				if(pt2.get<std::string>("os.osmatch.osclass.<xmlattr>.vendor").compare(""))
				{
					ret.personality_class += pt2.get<std::string>("os.osmatch.osclass.<xmlattr>.vendor") + " | ";
				}
				if(pt2.get<std::string>("os.osmatch.osclass.<xmlattr>.osfamily").compare(""))
				{
					ret.personality_class += pt2.get<std::string>("os.osmatch.osclass.<xmlattr>.osfamily") + " | ";
				}
				if(pt2.get<std::string>("os.osmatch.osclass.<xmlattr>.osgen").compare(""))
				{
					ret.personality_class += pt2.get<std::string>("os.osmatch.osclass.<xmlattr>.osgen") + " | ";
				}
				if(pt2.get<std::string>("os.osmatch.osclass.<xmlattr>.type").compare(""))
				{
					ret.personality_class += pt2.get<std::string>("os.osmatch.osclass.<xmlattr>.type");
				}
			}
			else
			{}
		}
		catch(std::exception &e)
		{

		}
	}

	ret.ports.open_ports = i;

	return ret;
}

std::vector<struct profile_read> load_nmap(const std::string &filename)
{
	using boost::property_tree::ptree;
	ptree pt;

	std::vector<struct profile_read> ret;

	read_xml(filename, pt);

	BOOST_FOREACH(ptree::value_type &host, pt.get_child("nmaprun"))
	{
		if(!host.first.compare("host"))
		{
			struct profile_read push_struct;
			ptree pt2 = host.second;
			push_struct = parseHost(pt2);
			ret.push_back(push_struct);
		}
	}

	return ret;
}

int main(int argc, char ** argv)
{
	std::string saveLocation;
	personalities.set_empty_key("");
	ports.set_empty_key("");

	std::cout << std::endl;

	if(argc < 2)
	{
		std::cout << "Outputs will be saved in the current folder." << std::endl;
		saveLocation = "";
	}
	else if(argc == 2)
	{
		saveLocation = std::string(argv[1]);
		if(saveLocation[saveLocation.length() - 1] != '/')
		{
			saveLocation += "/";
		}
		std::cout << "Outputs will be saved in " << saveLocation << std::endl;
	}

	std::vector<std::string> recv = getSubnetsToScan();

	std::cout << "Subnets to be scanned: " << std::endl;
	for(uint16_t i = 0; i < recv.size(); i++)
	{
		std::cout << recv[i] << std::endl;
	}
	std::cout << std::endl;

	std::stringstream ss;

	for(uint16_t i = 0; i < recv.size(); i++)
	{
		ss << i;
		std::string scan = "sudo nmap -O --osscan-guess -oX " + saveLocation + "subnet" + ss.str() + ".xml " + recv[i] + " >/dev/null";
		while(system(scan.c_str()));
		try
		{
			std::string file = "subnet" + ss.str() + ".xml";

			profile_vec = load_nmap(file);

			//need to write profile_vec profile_read structs to
			//honeyd configuration format

			for(uint16_t j = 0; j < profile_vec.size(); j++)
			{
				std::cout << std::endl;

				std::cout << "Address: " << profile_vec[j].address << std::endl;
				std::cout << "Open ports: " << profile_vec[j].ports.open_ports << std::endl;
				std::cout << "Ethernet vendor: " << profile_vec[j].ethernet_vendor << std::endl;
				std::cout << "Personality guess: " << profile_vec[j].personality << std::endl;
				std::cout << "Personality class: " << profile_vec[j].personality_class << std::endl;

				for(uint16_t k = 0; k < profile_vec[j].ports.port_services.size(); k++)
				{
					ss.str("");
					ss << profile_vec[j].ports.port_services[k].first;
					std::cout << "Port " << profile_vec[j].ports.port_services[k].first << " is running " << profile_vec[j].ports.port_services[k].second.second << ", state is " << profile_vec[j].ports.port_state[k] << std::endl;
					std::string scriptString = ss.str() + "_" + boost::to_upper_copy(profile_vec[j].ports.port_services[k].second.first) + "_" + profile_vec[j].ports.port_state[k];
					ports[scriptString]++;
					ss.str("");
				}

				std::cout << std::endl;
			}

			calculateDistributionMetrics();
		}
		catch(std::exception &e)
		{

		}

		ss.str();
	}

	std::cout << std::endl;
	return OKAY;
}

void calculateDistributionMetrics()
{
	for(uint16_t i = 0; i < profile_vec.size(); i++)
	{
		if(personalities.find(profile_vec[i].personality) != personalities.end())
		{
			personalities.get(profile_vec[i].personality)++;
		}
		else
		{
			personalities[profile_vec[i].personality]++;
		}
	}

	uint16_t testSum = 0;

	for(Pers_Table::iterator it = personalities.begin(); it != personalities.end(); it++)
	{
		testSum += it->second;
	}

	std::cout << "Out of " << testSum << " hosts:" << std::endl;

	for(Pers_Table::iterator it = personalities.begin(); it != personalities.end(); it++)
	{
		std::cout << it->second << "(" << ((double)it->second.first / (double)testSum) * 100 << "%) of the hosts were " << it->first << std::endl;
	}

	for(Ports_Table::iterator it = ports.begin(); it != ports.end(); it++)
	{
		std::cout << it->first << std::endl;
	}
}

std::vector<std::string> getSubnetsToScan()
{
	struct ifaddrs * devices = NULL;
	ifaddrs *curIf = NULL;
	std::stringstream ss;
	std::vector<std::string> addresses;
	char host[NI_MAXHOST];
	char bmhost[NI_MAXHOST];
	struct in_addr address;
	struct in_addr bitmask;
	struct in_addr basestruct;
	struct in_addr maxstruct;
	uint32_t ntohl_address;
	uint32_t ntohl_bitmask;
	bool there = false;

	std::cout << std::endl;

	if(getifaddrs(&devices))
	{
		std::cout << "Ethernet Interface Auto-Detection failed" << std::endl;
		exit(AUTODETECTFAIL);
	}

	std::vector<std::string> interfaces;

	for(curIf = devices; curIf != NULL; curIf = curIf->ifa_next)
	{
		if(!(curIf->ifa_flags & IFF_LOOPBACK) && ((int)curIf->ifa_addr->sa_family == AF_INET))
		{
			there = false;
			interfaces.push_back(std::string(curIf->ifa_name));
			int s = getnameinfo(curIf->ifa_addr, sizeof(sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

			if(s != 0)
			{
				std::cout << "Getting Name info of Interface IP failed" << std::endl;
				exit(GETNAMEINFOFAIL);
			}

			s = getnameinfo(curIf->ifa_netmask, sizeof(sockaddr_in), bmhost, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

			if(s != 0)
			{
				std::cout << "Getting Name info of Interface Netmask failed" << std::endl;
				exit(GETBITMASKFAIL);
			}

			std::string bmhost_push = std::string(bmhost);
			std::string host_push = std::string(host);
			std::cout << "Interface: " << curIf->ifa_name << std::endl;
			std::cout << "Address: " << host_push << std::endl;
			std::cout << "Netmask: " << bmhost_push << std::endl;
			inet_aton(host_push.c_str(), &address);
			inet_aton(bmhost_push.c_str(), &bitmask);
			ntohl_address = ntohl(address.s_addr);
			ntohl_bitmask = ntohl(bitmask.s_addr);

			uint32_t base = ntohl_bitmask & ntohl_address;
			basestruct.s_addr = htonl(base);

			uint32_t max = ~(ntohl_bitmask) + base;
			maxstruct.s_addr = htonl(max);

			tempHostspace = max - base - 3;

			std::cout << "tempHostspace == " << tempHostspace << std::endl;

			uint32_t mask = ~ntohl_bitmask;
			int i = 32;

			while(mask != 0)
			{
				mask /= 2;
				i--;
			}

			ss << i;

			std::string push = std::string(inet_ntoa(basestruct)) + "/" + ss.str();

			ss.str("");

			std::cout << "Correct X.X.X.X/## form is: " << inet_ntoa(basestruct) << "/" << i << std::endl;

			for(uint16_t j = 0; j < addresses.size() && !there; j++)
			{
				if(!push.compare(addresses[j]))
				{
					there = true;
				}
			}

			if(!there)
			{
				addresses.push_back(push);
				std::cout << std::endl;
			}
			else
			{
				std::cout << std::endl;
			}
		}
	}

	freeifaddrs(devices);
	return addresses;
}
