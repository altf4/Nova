
// REQUIRES NMAP 6

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
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <string>
#include <exception>
#include <algorithm>

#include "PersonalityTable.h"
#include "ScriptTable.h"
#include "Personality.h"

std::vector<std::string> getSubnetsToScan();
void calculateDistributionMetrics();
enum ERRCODES {OKAY, AUTODETECTFAIL, GETNAMEINFOFAIL, GETBITMASKFAIL};

std::vector<std::pair<std::string, std::string> > aggregate_profiles;
std::vector<std::string> aggregate_ethvendors;
std::vector<std::pair<int, std::string> > aggregate_port_services;
std::vector<std::pair<int, std::string> > aggregate_port_state;

struct port_read
{
	int open_ports;
	std::vector<std::pair<int, std::string> > port_services;
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

struct profile_read parseHost(boost::property_tree::ptree pt2)
{
	using boost::property_tree::ptree;

	struct profile_read ret;

	int i = 0;

	BOOST_FOREACH(ptree::value_type &v, pt2.get_child(""))
	{
		try
		{	if(!v.first.compare("address"))
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
						std::pair<int, std::string> push;
						std::string state = c.second.get<std::string>("state.<xmlattr>.state");
						ret.ports.port_state.push_back(state);
						push.first = c.second.get<int>("<xmlattr>.portid");
						push.second = c.second.get<std::string>("service.<xmlattr>.name");
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
			std::vector<struct profile_read> profile_vec;

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
				aggregate_ethvendors.push_back(profile_vec[j].ethernet_vendor);
				std::cout << "Personality guess: " << profile_vec[j].personality << std::endl;
				std::cout << "Personality class: " << profile_vec[j].personality_class << std::endl;
				std::pair<std::string, std::string> push_profile;
				push_profile.first = profile_vec[j].personality;
				push_profile.second = profile_vec[j].personality_class;
				aggregate_profiles.push_back(push_profile);

				for(uint16_t k = 0; k < profile_vec[j].ports.port_services.size(); k++)
				{
					std::cout << "Port " << profile_vec[j].ports.port_services[k].first << " is running " << profile_vec[j].ports.port_services[k].second << ", state is " << profile_vec[j].ports.port_state[k] << std::endl;
					std::pair<int, std::string> push_services;
					push_services.first = profile_vec[j].ports.port_services[k].first;
					push_services.second = profile_vec[j].ports.port_services[k].second;
					aggregate_port_services.push_back(push_services);
					push_services.second = profile_vec[j].ports.port_state[k];
					aggregate_port_state.push_back(push_services);
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
	sort(aggregate_profiles.begin(), aggregate_profiles.end());

	std::string comp = aggregate_profiles[0].first;

	std::vector<std::pair<std::string, int> > profileDistribution;

	std::pair<std::string, int> input;

	input.first = comp;
	input.second = 0;

	for(uint16_t i = 0; i < aggregate_profiles.size(); i++)
	{
		if(aggregate_profiles[i].first.compare(comp))
		{
			comp = aggregate_profiles[i].first;
			profileDistribution.push_back(input);
			input.first = comp;
			input.second = 0;
		}
		else
		{
			input.second++;
		}
	}

	for(uint16_t i = 0; i < profileDistribution.size(); i++)
	{
		std::cout << profileDistribution[i].first << " accounts for " << (100 * ((float)profileDistribution[i].second / (float)profileDistribution.size())) << "% of the scanned network." << std::endl;
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
	//struct in_addr maxstruct;
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

			//uint32_t max = ~(ntohl_bitmask) + base;
			//maxstruct.s_addr = htonl(max);

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
