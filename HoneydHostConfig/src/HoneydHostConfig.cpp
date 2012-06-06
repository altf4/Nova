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

#include <netdb.h>
#include <sstream>
#include <net/if.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "ScriptTable.h"
#include "VendorMacDb.h"
#include "PersonalityTree.h"
#include "HoneydHostConfig.h"

using namespace std;
using namespace Nova;

vector<uint16_t> leftoverHostspace;
uint16_t tempHostspace;
string localMachine;

PersonalityTable personalities;

ErrCode Nova::ParseHost(boost::property_tree::ptree pt2)
{
	using boost::property_tree::ptree;

	// instantiate Personality object here, populate it from the code below
	Personality * person = new Personality();

	personalities.m_num_of_hosts++;
	personalities.m_host_addrs_avail--;

	int i = 0;

	BOOST_FOREACH(ptree::value_type &v, pt2.get_child(""))
	{
		try
		{
			if(!v.first.compare("address"))
			{
				if(!v.second.get<string>("<xmlattr>.addrtype").compare("ipv4"))
				{
					if(localMachine.compare(v.second.get<string>("<xmlattr>.addr")))
					{
						person->m_addresses.push_back(v.second.get<string>("<xmlattr>.addr"));
					}
					else
					{
						return DONTADDSELF;
					}
				}
				else if(!v.second.get<string>("<xmlattr>.addrtype").compare("mac"))
				{
					person->m_macs.push_back(v.second.get<string>("<xmlattr>.addr"));
					person->AddVendor(v.second.get<string>("<xmlattr>.vendor"));
				}
			}
			else if(!v.first.compare("ports"))
			{
				BOOST_FOREACH(ptree::value_type &c, v.second.get_child(""))
				{
					try
					{
						stringstream ss;
						ss << c.second.get<int>("<xmlattr>.portid");
						string port_key = ss.str() + "_" + boost::to_upper_copy(c.second.get<string>("<xmlattr>.protocol"));
						ss.str("");
						string port_service = c.second.get<string>("service.<xmlattr>.name");
						person->AddPort(port_key);
						i++;
					}
					catch(exception &e)
					{

					}
				}
			}
			else if(!v.first.compare("os"))
			{
				// Need to determine what to do in the case that an OS class designation doesn't
				// have all four fields required
				if(pt2.get<string>("os.osmatch.<xmlattr>.name").compare(""))
				{
					person->m_personalityClass.push_back(pt2.get<string>("os.osmatch.<xmlattr>.name"));
				}
				if(pt2.get<string>("os.osmatch.osclass.<xmlattr>.type").compare(""))
				{
					person->m_personalityClass.push_back(pt2.get<string>("os.osmatch.osclass.<xmlattr>.type"));
				}
				if(pt2.get<string>("os.osmatch.osclass.<xmlattr>.osgen").compare(""))
				{
					person->m_personalityClass.push_back(pt2.get<string>("os.osmatch.osclass.<xmlattr>.osgen"));
				}
				if(pt2.get<string>("os.osmatch.osclass.<xmlattr>.osfamily").compare(""))
				{
					person->m_personalityClass.push_back(pt2.get<string>("os.osmatch.osclass.<xmlattr>.osfamily"));
				}
				if(pt2.get<string>("os.osmatch.osclass.<xmlattr>.vendor").compare(""))
				{
					person->m_personalityClass.push_back(pt2.get<string>("os.osmatch.osclass.<xmlattr>.vendor"));
				}
			}
		}
		catch(exception &e)
		{
			/*cout << "Caught Exception : " << e.what() << endl;
			return PARSINGERROR;*/
		}
	}

	person->m_port_count = i;

	// call AddHost() on the Personality object created at the beginning of this method

	if(person->m_personalityClass.empty())
	{
		cout << "Failed to match any personality information to " << person->m_addresses[0] << ", not adding to Personality Table." << endl;
		return NOMATCHEDPERSONALITY;
	}

	personalities.AddHost(person);

	return OKAY;
}

void Nova::LoadNmap(const string &filename)
{
	using boost::property_tree::ptree;
	ptree pt;

	read_xml(filename, pt);

	BOOST_FOREACH(ptree::value_type &host, pt.get_child("nmaprun"))
	{
		if(!host.first.compare("host"))
		{
			ptree pt2 = host.second;

			switch(ParseHost(pt2))
			{
				case DONTADDSELF:
					std::cout << "Can't add self as personality yet." << std::endl;
					break;
				case NOMATCHEDPERSONALITY:
					std::cout << "Couldn't obtain personality data on host " << pt2.get<std::string>("address.<xmlattr>.addr") << "." << std::endl;
					break;
				case OKAY:
					break;
				default:
					std::cout << "Unknown return value." << std::endl;
					break;
			}
		}
	}
}

int main(int argc, char ** argv)
{
	ErrCode errVar = OKAY;
	vector<string> recv = GetSubnetsToScan(&errVar);

	PrintRecv(recv);

	errVar = LoadPersonalityTable(recv);

	if(errVar != OKAY)
	{
		cout << "Unable to load personality table" << endl;
		return errVar;
	}
	PersonalityTree persTree = PersonalityTree(&personalities);
	persTree.ToString();
	return errVar;
}

Nova::ErrCode Nova::LoadPersonalityTable(vector<string> recv)
{
	stringstream ss;

	for(uint16_t i = 0; i < recv.size(); i++)
	{
		ss << i;
		string scan = "sudo nmap -O --osscan-guess -oX subnet" + ss.str() + ".xml " + recv[i] + " >/dev/null";
		while(system(scan.c_str()));
		try
		{
			string file = "subnet" + ss.str() + ".xml";

			LoadNmap(file);

			calculateDistributionMetrics();
		}
		catch(exception &e)
		{
			/*cout << "Caught Exception : " << e.what() << endl;
			return PARSINGERROR;*/
		}

		ss.str();
	}
	personalities.ListInfo();

	return OKAY;
}

void Nova::PrintRecv(vector<string> recv)
{
	cout << "Subnets to be scanned: " << endl;
	for(uint16_t i = 0; i < recv.size(); i++)
	{
		cout << recv[i] << endl;
	}
	cout << endl;
}

vector<string> Nova::GetSubnetsToScan(Nova::ErrCode * errVar)
{
	struct ifaddrs * devices = NULL;
	ifaddrs *curIf = NULL;
	stringstream ss;
	vector<string> addresses;
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

	cout << endl;

	if(getifaddrs(&devices))
	{
		cout << "Ethernet Interface Auto-Detection failed" << endl;
		*errVar = AUTODETECTFAIL;
		return addresses;
	}

	vector<string> interfaces;

	for(curIf = devices; curIf != NULL; curIf = curIf->ifa_next)
	{
		if(!(curIf->ifa_flags & IFF_LOOPBACK) && ((int)curIf->ifa_addr->sa_family == AF_INET))
		{
			there = false;
			interfaces.push_back(string(curIf->ifa_name));
			int s = getnameinfo(curIf->ifa_addr, sizeof(sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

			if(s != 0)
			{
				cout << "Getting Name info of Interface IP failed" << endl;
				*errVar = GETNAMEINFOFAIL;
				return addresses;
			}

			s = getnameinfo(curIf->ifa_netmask, sizeof(sockaddr_in), bmhost, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

			if(s != 0)
			{
				cout << "Getting Name info of Interface Netmask failed" << endl;
				*errVar = GETBITMASKFAIL;
				return addresses;
			}

			string bmhost_push = string(bmhost);
			string host_push = string(host);
			localMachine = host_push;
			cout << "Interface: " << curIf->ifa_name << endl;
			cout << "Address: " << host_push << endl;
			cout << "Netmask: " << bmhost_push << endl;
			inet_aton(host_push.c_str(), &address);
			inet_aton(bmhost_push.c_str(), &bitmask);
			ntohl_address = ntohl(address.s_addr);
			ntohl_bitmask = ntohl(bitmask.s_addr);

			uint32_t base = ntohl_bitmask & ntohl_address;
			basestruct.s_addr = htonl(base);

			uint32_t max = ~(ntohl_bitmask) + base;
			//maxstruct.s_addr = htonl(max);

			personalities.m_host_addrs_avail += max - base - 3;

			uint32_t mask = ~ntohl_bitmask;
			int i = 32;

			while(mask != 0)
			{
				mask /= 2;
				i--;
			}

			ss << i;

			string push = string(inet_ntoa(basestruct)) + "/" + ss.str();

			ss.str("");

			cout << "Correct X.X.X.X/## form is: " << inet_ntoa(basestruct) << "/" << i << endl;

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
				cout << endl;
			}
			else
			{
				cout << endl;
			}
		}
	}

	freeifaddrs(devices);
	return addresses;
}
