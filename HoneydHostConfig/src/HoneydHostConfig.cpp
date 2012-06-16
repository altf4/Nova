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
#include <fstream>
#include <ctype.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "PersonalityTree.h"
#include "HoneydHostConfig.h"

#define DIGIT_OFFSET 48
#define LOWER_OFFSET 87

using namespace std;
using namespace Nova;

vector<uint16_t> leftoverHostspace;
uint16_t tempHostspace;
string localMachine;

vector<Subnet> subnetsToAdd;

PersonalityTable personalities;

void Nova::Shift(u_char *m, uint cond, u_char val)
{
	if(m == NULL)
	{
		return;
	}
	if(cond % 3 == 0)
	{
		u_char b = val;
		b = b << 4;
		*m |= b;
	}
	else if(cond % 3 == 1)
	{
		u_char b = val;
		*m |= b;
	}
}

ErrCode Nova::ParseHost(boost::property_tree::ptree pt2)
{
	using boost::property_tree::ptree;

	// Instantiate Personality object here, populate it from the code below
	Personality *person = new Personality();

	// For the personality table, increment the number of hosts found
	// and decrement the number of hosts available, so at the end
	// of the configuration process we don't over-allocate space
	// in the hostspace for the subnets, clobbering existing DHCP
	// allocations.
	personalities.m_num_of_hosts++;
	personalities.m_host_addrs_avail--;

	int i = 0;

	BOOST_FOREACH(ptree::value_type &v, pt2.get_child(""))
	{
		try
		{
			// If we've found the <address> tags within the <host> xml node
			if(!v.first.compare("address"))
			{
				// and then find the ipv4 address, add it to the addresses
				// vector for the personality object;
				if(!v.second.get<string>("<xmlattr>.addrtype").compare("ipv4"))
				{
					if(localMachine.compare(v.second.get<string>("<xmlattr>.addr")))
					{
						person->m_addresses.push_back(v.second.get<string>("<xmlattr>.addr"));
					}
					else
					{
						person->m_addresses.push_back(v.second.get<string>("<xmlattr>.addr"));

						ifstream ifs("/sys/class/net/eth0/address");
						char mac[18];
						ifs.getline(mac, 18);
						string forLoop = string(mac);

						uint counter = 0;
						person->m_macs.push_back(forLoop);

						// There may be a more efficient way to do this,
						// merely for if we absolutely have to have the MAC as a string
						// right now.
						// It does, however, add the profile of the host
						// into the tree correctly.
						uint passToVendor = 0;
						u_char m = 0;
						for(uint i = 0; i < forLoop.length() && counter < 3; i++)
						{
							if(isdigit(mac[i]) && mac[i] != '0')
							{
								Shift(&m, i, mac[i] - DIGIT_OFFSET);
							}
							else if(islower(mac[i]) && mac[i] < 'g')
							{
								Shift(&m, i, mac[i] - LOWER_OFFSET);
							}
							else if(mac[i] != ':' && mac[i] != '0')
							{
								cout << "Unknown character found in MAC address string" << endl;
							}
							if((i % 3) == 2 || i == forLoop.length() - 1)
							{
								uint temp = 0 | m;
								passToVendor |= (temp << (16 - (8 * counter)));
								m = 0;
								counter++;
							}
						}

						VendorMacDb *vmd = new VendorMacDb();
						vmd->LoadPrefixFile();

						person->AddVendor(vmd->LookupVendor(passToVendor));
					}
				}
				// if we've found the mac, add the hardware address to the MACs
				// vector in the Personality object and then add the vendor to
				// the MAC_Table inside the object as well.
				else if(!v.second.get<string>("<xmlattr>.addrtype").compare("mac"))
				{
					person->m_macs.push_back(v.second.get<string>("<xmlattr>.addr"));
					person->AddVendor(v.second.get<string>("<xmlattr>.vendor"));
				}
			}
			// If we've found the <ports> tag within the <host> xml node
			else if(!v.first.compare("ports"))
			{
				BOOST_FOREACH(ptree::value_type &c, v.second.get_child(""))
				{
					try
					{
						// try, for every <port> tag in <ports>, to get the
						// port number, the protocol (tcp, udp, sctp, etc...)
						// and the name of the <service> running on the port
						// and add the port to the Port_Table in the Personality
						// object.
						stringstream ss;
						ss << c.second.get<int>("<xmlattr>.portid");
						string port_key = ss.str() + "_" + boost::to_upper_copy(c.second.get<string>("<xmlattr>.protocol"));
						ss.str("");
						string port_service = c.second.get<string>("service.<xmlattr>.name");
						person->AddPort(port_key, port_service);
						i++;
					}
					catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
					{
						cout << "Caught Exception : " << e.what() << endl;
					}
				}
			}
			// If we've found the <os> tags in the <host> xml node
			else if(!v.first.compare("os"))
			{
				// try to get the name, type, osgen, osfamily and vendor for the most
				// accurate guess by nmap. This will (provided the xml is structured correctly)
				// push back the values in the following form:
				// personality_name, type, osgen, osfamily, vendor
				// This order must be properly maintained for use in the PersonalityTree class.
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
		catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
		{
			cout << "Caught Exception : " << e.what() << endl;
			//return PARSINGERROR;
		}
	}

	// Assign the number of ports found to the m_port_count member variable,
	// for use
	person->m_port_count = i;

	// If personalityClass vector is empty, nmap wasn't able to generate
	// a personality for a host, and therefore we don't want to include it
	// into any structure that would use the Personality object populated above.
	// Just return and let the scoping take care of deallocating the object.
	if(person->m_personalityClass.empty())
	{
		return NOMATCHEDPERSONALITY;
	}


	for(uint i = 0; i < person->m_personalityClass.size() - 1; i++)
	{
		person->m_osclass += person->m_personalityClass[i] + " | ";
	}

	person->m_osclass += person->m_personalityClass[person->m_personalityClass.size() - 1];

	// Call AddHost() on the Personality object created at the beginning of this method
	personalities.AddHost(person);

	return OKAY;
}

void Nova::LoadNmap(const string &filename)
{
	using boost::property_tree::ptree;
	ptree pt;

	// Read the nmap xml output file (given by the filename parameter) into a boost
	// property tree for parsing of the found hosts.
	read_xml(filename, pt);

	BOOST_FOREACH(ptree::value_type &host, pt.get_child("nmaprun"))
	{
		if(!host.first.compare("host"))
		{
			ptree pt2 = host.second;

			// Call ParseHost on the <host> subtree within the xml file.
			switch(ParseHost(pt2))
			{
				// Output for alerting user that a found host had incomplete
				// personality data, and thus was not added to the PersonalityTable.
				case NOMATCHEDPERSONALITY:
					std::cout << "Couldn't obtain personality data on host " << pt2.get<std::string>("address.<xmlattr>.addr") << "." << std::endl;
					break;
				// Everything parsed fine, don't output anything.
				case OKAY:
					break;
				// Execution should never get here; if you're adding ErrCodes
				// that could occur in ParseHost, add a case for it
				// with an appropriate output detailing what went wrong.
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

	// for each element in recv
	for(uint i = 0; i < subnetsToAdd.size(); i++)
	{
		persTree.AddSubnet(&subnetsToAdd[i]);
	}

	persTree.ToString();

	persTree.DebugPrintProfileTable();

	persTree.ToXmlTemplate();

	persTree.DebugPrintProfileTable();

	return errVar;
}

Nova::ErrCode Nova::LoadPersonalityTable(vector<string> recv)
{
	stringstream ss;

	// For each element in recv (which contains strings of the subnets),
	// do an OS fingerprinting scan and output the results into a different
	// xml file for each subnet.
	for(uint16_t i = 0; i < recv.size(); i++)
	{
		ss << i;
		string scan = "sudo nmap -O --osscan-guess -oX subnet" + ss.str() + ".xml " + recv[i] + " >/dev/null";
		while(system(scan.c_str()));
		try
		{
			string file = "subnet" + ss.str() + ".xml";

			// Call LoadNmap on the generated filename so that we
			// can add hosts to the personality table, provided they
			// contain enough data.
			LoadNmap(file);
		}
		catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
		{
			cout << "Caught Exception : " << e.what() << endl;
			//return PARSINGERROR;
		}
		ss.str("");
	}
	return OKAY;
}

void Nova::PrintRecv(vector<string> recv)
{
	// Debug method to output what subnets were found by
	// the GetSubnetsToScan() method.
	cout << "Subnets to be scanned: " << endl;
	for(uint16_t i = 0; i < recv.size(); i++)
	{
		cout << recv[i] << endl;
	}
	cout << endl;
}

vector<string> Nova::GetSubnetsToScan(Nova::ErrCode *errVar)
{
	struct ifaddrs *devices = NULL;
	ifaddrs *curIf = NULL;
	stringstream ss;
	vector<string> addresses;
	if(errVar == NULL)
	{
		return addresses;
	}
	addresses.clear();
	char host[NI_MAXHOST];
	char bmhost[NI_MAXHOST];
	struct in_addr address;
	struct in_addr bitmask;
	struct in_addr basestruct;
	struct in_addr maxstruct;
	uint32_t ntohl_address;
	uint32_t ntohl_bitmask;
	bool there = false;

	cout << endl;

	// If we call getifaddrs and it fails to obtain any information, no point in proceeding.
	// Return the empty addresses vector and set the ErrorCode to AUTODETECTFAIL.
	if(getifaddrs(&devices))
	{
		cout << "Ethernet Interface Auto-Detection failed" << endl;
		*errVar = AUTODETECTFAIL;
		return addresses;
	}

	vector<string> interfaces;

	// For every found interface, we need to do some processing.
	for(curIf = devices; curIf != NULL; curIf = curIf->ifa_next)
	{
		// If we've found an interface that has an IPv4 address and is NOT a loopback,
		if(!(curIf->ifa_flags & IFF_LOOPBACK) && ((int)curIf->ifa_addr->sa_family == AF_INET))
		{
			Subnet add;
			// start processing it to generate the subnet for the interface.
			there = false;
			interfaces.push_back(string(curIf->ifa_name));

			// Get the string representation of the interface's IP address,
			// and put it into the host character array.
			int s = getnameinfo(curIf->ifa_addr, sizeof(sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

			if(s != 0)
			{
				// If getnameinfo returned an error, stop processing for the
				// method, assign the proper errorCode, and return an empty
				// vector.
				cout << "Getting Name info of Interface IP failed" << endl;
				*errVar = GETNAMEINFOFAIL;
				return addresses;
			}

			// Do the same thing as the above, but for the netmask of the interface
			// as opposed to the IP address.
			s = getnameinfo(curIf->ifa_netmask, sizeof(sockaddr_in), bmhost, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

			if(s != 0)
			{
				// If getnameinfo returned an error, stop processing for the
				// method, assign the proper errorCode, and return an empty
				// vector.
				cout << "Getting Name info of Interface Netmask failed" << endl;
				*errVar = GETBITMASKFAIL;
				return addresses;
			}
			// Convert the bitmask and host address character arrays to strings
			// for use later
			string bmhost_push = string(bmhost);
			string host_push = string(host);
			localMachine = host_push;

			// Spurious debug prints for now. May change them to be used
			// in UI hooks later.
			cout << "Interface: " << curIf->ifa_name << endl;
			cout << "Address: " << host_push << endl;
			cout << "Netmask: " << bmhost_push << endl;

			// Put the network ordered address values into the
			// address and bitmaks in_addr structs, and then
			// convert them to host longs for use in
			// determining how much hostspace is empty
			// on this interface's subnet.
			inet_aton(host_push.c_str(), &address);
			inet_aton(bmhost_push.c_str(), &bitmask);
			ntohl_address = ntohl(address.s_addr);
			ntohl_bitmask = ntohl(bitmask.s_addr);

			// Get the base address for the subnet
			uint32_t base = ntohl_bitmask & ntohl_address;
			basestruct.s_addr = htonl(base);

			// and the max address
			uint32_t max = ~(ntohl_bitmask) + base;
			maxstruct.s_addr = htonl(max);

			// and then add max - base (minus three, for the current
			// host, the .0 address, and the .255 address)
			// into the PersonalityTable's aggregate coutn of
			// available host address space.
			personalities.m_host_addrs_avail += max - base - 3;

			// Find out how many bits there are to work with in
			// the subnet (i.e. X.X.X.X/24? X.X.X.X/31?).
			uint32_t mask = ~ntohl_bitmask;
			int i = 32;

			while(mask != 0)
			{
				mask /= 2;
				i--;
			}

			ss << i;

			// Generate a string of the form X.X.X.X/## for use in nmap scans later
			string push = string(inet_ntoa(basestruct)) + "/" + ss.str();

			ss.str("");

			add.m_address = push;
			add.m_mask = string(inet_ntoa(bitmask));
			add.m_maskBits = i;
			add.m_base = basestruct.s_addr;
			add.m_max = maxstruct.s_addr;
			add.m_name = string(curIf->ifa_name);
			add.m_enabled = (curIf->ifa_flags & IFF_UP);
			add.m_isRealDevice = true;

			// If we have two interfaces that point the same subnet, we only want
			// to scan once; so, change the "there" flag to reflect that the subnet
			// exists to prevent it from being pushed again.
			for(uint16_t j = 0; j < addresses.size() && !there; j++)
			{
				if(!push.compare(addresses[j]))
				{
					there = true;
				}
			}

			// If the subnet isn't in the addresses vector, push it in.
			if(!there)
			{
				addresses.push_back(push);
				subnetsToAdd.push_back(add);
			}
			// Otherwise, don't do anything.
			else
			{
			}
		}
	}

	// Deallocate the devices struct from the beginning of the method,
	// and return the vector containing the subnets from each interface.
	freeifaddrs(devices);
	return addresses;
}
