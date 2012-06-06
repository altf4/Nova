//============================================================================
// Name        : HoneydHostConfig.cpp
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
// Description : Main HH_CONFIG file, performs the subnet acquisition, scanning, and
//               parsing of the resultant .xml output into a PersonalityTable object
//============================================================================


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
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <string>
#include <exception>
#include <algorithm>

#include "ScriptTable.h"
#include "HoneydHostConfig.h"
#include "VendorMacDb.h"

using namespace std;
using namespace Nova;

std::vector<uint16_t> leftoverHostspace;
uint16_t tempHostspace;
Nova::PersonalityTable personalities;
std::string localMachine;
//VendorMacDb * macs;

ErrCode Nova::ParseHost(boost::property_tree::ptree pt2)
{
	using boost::property_tree::ptree;

	// instantiate Personality object here, populate it from the code below
	Personality * person = new Personality();

	int i = 0;

	BOOST_FOREACH(ptree::value_type &v, pt2.get_child(""))
	{
		try
		{
			if(!v.first.compare("address"))
			{
				if(!v.second.get<std::string>("<xmlattr>.addrtype").compare("ipv4"))
				{
					if(localMachine.compare(v.second.get<std::string>("<xmlattr>.addr")))
					{
						person->m_addresses.push_back(v.second.get<std::string>("<xmlattr>.addr"));
					}
					else
					{
						return DONTADDSELF;
					}
				}
				else if(!v.second.get<std::string>("<xmlattr>.addrtype").compare("mac"))
				{
					person->m_macs.push_back(v.second.get<std::string>("<xmlattr>.addr"));
					person->AddVendor(v.second.get<std::string>("<xmlattr>.vendor"));
				}
			}
			else if(!v.first.compare("ports"))
			{
				BOOST_FOREACH(ptree::value_type &c, v.second.get_child(""))
				{
					try
					{
						std::stringstream ss;
						ss << c.second.get<int>("<xmlattr>.portid");
						std::string port_key = ss.str() + "_" + boost::to_upper_copy(c.second.get<std::string>("<xmlattr>.protocol"));
						ss.str("");
						std::string port_service = c.second.get<std::string>("service.<xmlattr>.name");
						person->AddPort(port_key);
						i++;
					}
					catch(std::exception &e)
					{

					}
				}
			}
			else if(!v.first.compare("os"))
			{
				// Need to determine what to do in the case that an OS class designation doesn't
				// have all four fields required
				if(pt2.get<std::string>("os.osmatch.<xmlattr>.name").compare(""))
				{
					person->m_personalityClass.push_back(pt2.get<std::string>("os.osmatch.<xmlattr>.name"));
				}
				if(pt2.get<std::string>("os.osmatch.osclass.<xmlattr>.type").compare(""))
				{
					person->m_personalityClass.push_back(pt2.get<std::string>("os.osmatch.osclass.<xmlattr>.type"));
				}
				if(pt2.get<std::string>("os.osmatch.osclass.<xmlattr>.osgen").compare(""))
				{
					person->m_personalityClass.push_back(pt2.get<std::string>("os.osmatch.osclass.<xmlattr>.osgen"));
				}
				if(pt2.get<std::string>("os.osmatch.osclass.<xmlattr>.osfamily").compare(""))
				{
					person->m_personalityClass.push_back(pt2.get<std::string>("os.osmatch.osclass.<xmlattr>.osfamily"));
				}
				if(pt2.get<std::string>("os.osmatch.osclass.<xmlattr>.vendor").compare(""))
				{
					person->m_personalityClass.push_back(pt2.get<std::string>("os.osmatch.osclass.<xmlattr>.vendor"));
				}
			}
			else
			{}
		}
		catch(std::exception &e)
		{

		}
	}

	person->m_port_count = i;

	// call AddHost() on the Personality object created at the beginning of this method

	if(person->m_personalityClass.empty())
	{
		std::cout << "Failed to match any personality information to " << person->m_addresses[0] << ", not adding to Personality Table." << std::endl;
		return NOMATCHEDPERSONALITY;
	}

	personalities.AddHost(person);

	return OKAY;
}

void Nova::LoadNmap(const std::string &filename)
{
	using boost::property_tree::ptree;
	ptree pt;

	read_xml(filename, pt);

	BOOST_FOREACH(ptree::value_type &host, pt.get_child("nmaprun"))
	{
		if(!host.first.compare("host"))
		{
			ptree pt2 = host.second;
			ParseHost(pt2);
		}
	}
}

int main(int argc, char ** argv)
{
	ErrCode errVar = OKAY;
	std::vector<std::string> recv = GetSubnetsToScan(&errVar);

	PrintRecv(recv);

	errVar = LoadPersonalityTable(recv);

	if(errVar != OKAY)
	{
		std::cout << "Unable to load personality table" << std::endl;
		return errVar;
	}

	return errVar;
}

Nova::ErrCode Nova::LoadPersonalityTable(std::vector<std::string> recv)
{
	std::stringstream ss;

	for(uint16_t i = 0; i < recv.size(); i++)
	{
		ss << i;
		std::string scan = "sudo nmap -O --osscan-guess -oX subnet" + ss.str() + ".xml " + recv[i] + " >/dev/null";
		while(system(scan.c_str()));
		try
		{
			std::string file = "subnet" + ss.str() + ".xml";

			LoadNmap(file);

			calculateDistributionMetrics();
		}
		catch(std::exception &e)
		{

		}

		ss.str();
	}

	//macs = new VendorMacDb();

	//macs->LoadPrefixFile();

	personalities.ListInfo();

	return OKAY;
}

void Nova::PrintRecv(std::vector<std::string> recv)
{
	std::cout << "Subnets to be scanned: " << std::endl;
	for(uint16_t i = 0; i < recv.size(); i++)
	{
		std::cout << recv[i] << std::endl;
	}
	std::cout << std::endl;
}

void Nova::calculateDistributionMetrics()
{

}

std::vector<std::string> Nova::GetSubnetsToScan(Nova::ErrCode * errVar)
{
	struct ifaddrs * devices = NULL;
	ifaddrs *curIf = NULL;
	std::stringstream ss;
	std::vector<std::string> addresses;
	addresses.clear();
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
		*errVar = AUTODETECTFAIL;
		return addresses;
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
				*errVar = GETNAMEINFOFAIL;
				return addresses;
			}

			s = getnameinfo(curIf->ifa_netmask, sizeof(sockaddr_in), bmhost, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

			if(s != 0)
			{
				std::cout << "Getting Name info of Interface Netmask failed" << std::endl;
				*errVar = GETBITMASKFAIL;
				return addresses;
			}

			std::string bmhost_push = std::string(bmhost);
			std::string host_push = std::string(host);
			localMachine = host_push;
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
