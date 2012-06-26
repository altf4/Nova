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

#include "NodeManager.h"
#include "HoneydHostConfig.h"
#include "Logger.h"

#define DIGIT_OFFSET 48
#define LOWER_OFFSET 87
#define UPPER_OFFSET 65

using namespace std;
using namespace Nova;

vector<uint16_t> leftoverHostspace;
uint16_t tempHostspace;
string localMachine;

vector<Subnet> subnetsDetected;

PersonalityTable personalities;

//Converts the first three bytes of a MAC address from a string to a unsigned int
//	MAC: string of the MAC address you wish to conver must be at least 8 characters
//	return: unsigned integervalue of the MAC prefix
//	note: must be valid hex charactar pairs separated by colons, ex: '09:af:AF'
uint Nova::AtoMACPrefix(string MAC)
{
	if(MAC.size() < 8)
	{
		return 0;
	}
	u_char tempBuf = 0;
	uint ret = 0;
	for(uint i = 0; (i < MAC.length()) && (i < 8); i++)
	{
		uint val = 0;
		if(isdigit(MAC[i]) && MAC[i] != '0')
		{
			val = MAC[i] - DIGIT_OFFSET;
		}
		else if(islower(MAC[i]) && MAC[i] < 'g')
		{
			val = MAC[i] - LOWER_OFFSET;
		}
		else if(isupper(MAC[i]) && MAC[i] < 'G')
		{
			val = MAC[i] - UPPER_OFFSET;
		}
		else if(MAC[i] != ':' && MAC[i] != '0')
		{
			return 0;
		}
		switch(i%3)
		{
			case 0:
			{
				tempBuf += val;
				tempBuf = tempBuf << 4;
				break;
			}
			case 1:
			{
				tempBuf += val;
				break;
			}
			case 2:
			{
				ret += tempBuf;
				ret = ret << 8;
				break;
			}
			default:
			{
				return 0;
			}
		}
	}
	return ret;
}

ErrCode Nova::ParseHost(boost::property_tree::ptree propTree)
{
	using boost::property_tree::ptree;

	// Instantiate Personality object here, populate it from the code below
	Personality *persObject = new Personality();

	// For the personality table, increment the number of hosts found
	// and decrement the number of hosts available, so at the end
	// of the configuration process we don't over-allocate space
	// in the hostspace for the subnets, clobbering existing DHCP
	// allocations.
	personalities.m_num_of_hosts++;
	personalities.m_numAddrsAvail--;

	int i = 0;

	BOOST_FOREACH(ptree::value_type &value, propTree.get_child(""))
	{
		try
		{
			// If we've found the <address> tags within the <host> xml node
			if(!value.first.compare("address"))
			{
				// and then find the ipv4 address, add it to the addresses
				// vector for the personality object;
				if(!value.second.get<string>("<xmlattr>.addrtype").compare("ipv4"))
				{
					// If we're not parsing ourself, just get the address in the
					// <addr> tag
					if(localMachine.compare(value.second.get<string>("<xmlattr>.addr")))
					{
						persObject->m_addresses.push_back(value.second.get<string>("<xmlattr>.addr"));
					}
					// If, however, we're parsing ourself, we need to do some extra work.
					// The IP address will be in the nmap XML structure, but the MAC will not.
					// Thus, we need to get it through other means. This will grab a string
					// representation of the MAC address, convert the first three hex pairs into
					// an unsigned integer, and then find the MAC vendor using the VendorMacDb class
					// of Nova proper.
					else
					{
						persObject->m_addresses.push_back(value.second.get<string>("<xmlattr>.addr"));

						ifstream ifs("/sys/class/net/eth0/address");
						char macBuffer[18];
						ifs.getline(macBuffer, 18);

						string macString = string(macBuffer);
						persObject->m_macs.push_back(macString);
						uint rawMACPrefix = AtoMACPrefix(macString);

						VendorMacDb *macVendorDB = new VendorMacDb();
						macVendorDB->LoadPrefixFile();
						persObject->AddVendor(macVendorDB->LookupVendor(rawMACPrefix));
					}
				}
				// if we've found the MAC, add the hardware address to the MACs
				// vector in the Personality object and then add the vendor to
				// the MAC_Table inside the object as well.
				else if(!value.second.get<string>("<xmlattr>.addrtype").compare("mac"))
				{
					persObject->m_macs.push_back(value.second.get<string>("<xmlattr>.addr"));
					persObject->AddVendor(value.second.get<string>("<xmlattr>.vendor"));
				}
			}
			// If we've found the <ports> tag within the <host> xml node
			else if(!value.first.compare("ports"))
			{
				BOOST_FOREACH(ptree::value_type &portValue, value.second.get_child(""))
				{
					try
					{
						// try, for every <port> tag in <ports>, to get the
						// port number, the protocol (tcp, udp, sctp, etc...)
						// and the name of the <service> running on the port
						// and add the port to the Port_Table in the Personality
						// object.
						stringstream ss;
						ss << portValue.second.get<int>("<xmlattr>.portid");
						string port_key = ss.str() + "_" + boost::to_upper_copy(portValue.second.get<string>("<xmlattr>.protocol"));
						ss.str("");
						string port_service = portValue.second.get<string>("service.<xmlattr>.name");
						persObject->AddPort(port_key, port_service);
						i++;
					}
					catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
					{
						LOG(DEBUG, "Caught Exception : " + string(e.what()), "");
					}
				}
			}
			// If we've found the <os> tags in the <host> xml node
			else if(!value.first.compare("os"))
			{
				// try to get the name, type, osgen, osfamily and vendor for the most
				// accurate guess by nmap. This will (provided the xml is structured correctly)
				// push back the values in the following form:
				// personality_name, type, osgen, osfamily, vendor
				// This order must be properly maintained for use in the PersonalityTree class.
				if(propTree.get<string>("os.osmatch.<xmlattr>.name").compare(""))
				{
					persObject->m_personalityClass.push_back(propTree.get<string>("os.osmatch.<xmlattr>.name"));
				}
				if(propTree.get<string>("os.osmatch.osclass.<xmlattr>.type").compare(""))
				{
					persObject->m_personalityClass.push_back(propTree.get<string>("os.osmatch.osclass.<xmlattr>.type"));
				}
				if(propTree.get<string>("os.osmatch.osclass.<xmlattr>.osgen").compare(""))
				{
					persObject->m_personalityClass.push_back(propTree.get<string>("os.osmatch.osclass.<xmlattr>.osgen"));
				}
				if(propTree.get<string>("os.osmatch.osclass.<xmlattr>.osfamily").compare(""))
				{
					persObject->m_personalityClass.push_back(propTree.get<string>("os.osmatch.osclass.<xmlattr>.osfamily"));
				}
				if(propTree.get<string>("os.osmatch.osclass.<xmlattr>.vendor").compare(""))
				{
					persObject->m_personalityClass.push_back(propTree.get<string>("os.osmatch.osclass.<xmlattr>.vendor"));
				}
			}
		}
		catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
		{
			LOG(DEBUG, "Caught Exception : " + string(e.what()), "");
		}
	}

	// Assign the number of ports found to the m_port_count member variable,
	// for use
	persObject->m_port_count = i;

	// If personalityClass vector is empty, nmap wasn't able to generate
	// a personality for a host, and therefore we don't want to include it
	// into any structure that would use the Personality object populated above.
	// Just return and let the scoping take care of deallocating the object.
	if(persObject->m_personalityClass.empty())
	{
		return NOMATCHEDPERSONALITY;
	}

	// Generate OS Class strings for use later down the line; used primarily
	// for matching open ports to scripts in the script table. So, say 22_TCP is open
	// on a host, we'll use the m_personalityClass string to match the OS and open port
	// to a script and then assign that script automatically.
	for(uint i = 0; i < persObject->m_personalityClass.size() - 1; i++)
	{
		persObject->m_osclass += persObject->m_personalityClass[i] + " | ";
	}

	persObject->m_osclass += persObject->m_personalityClass[persObject->m_personalityClass.size() - 1];

	// Call AddHost() on the Personality object created at the beginning of this method
	personalities.AddHost(persObject);

	return OKAY;
}

void Nova::LoadNmap(const string &filename)
{
	using boost::property_tree::ptree;
	ptree propTree;

	// Read the nmap xml output file (given by the filename parameter) into a boost
	// property tree for parsing of the found hosts.
	read_xml(filename, propTree);

	BOOST_FOREACH(ptree::value_type &host, propTree.get_child("nmaprun"))
	{
		if(!host.first.compare("host"))
		{
			ptree tempPropTree = host.second;

			// Call ParseHost on the <host> subtree within the xml file.
			switch(ParseHost(tempPropTree))
			{
				// Output for alerting user that a found host had incomplete
				// personality data, and thus was not added to the PersonalityTable.
				case NOMATCHEDPERSONALITY:
				{
					LOG(WARNING, "Unable to obtain personality data for host "
						+ tempPropTree.get<std::string>("address.<xmlattr>.addr") + ".", "");
					break;
				}
				// Everything parsed fine, don't output anything.
				case OKAY:
				{
					break;
				}
				// Execution should never get here; if you're adding ErrCodes
				// that could occur in ParseHost, add a case for it
				// with an appropriate output detailing what went wrong.
				default:
				{
					LOG(ERROR, "Unknown return value.", "");
					break;
				}
			}
		}
	}
}

int main(int argc, char ** argv)
{
	if(argc != 2)
	{
		cout << "Usage: ./honeydhostconfig NUM_NODES_TO_CREATE" << endl;
		exit(INCORRECTNUMBERARGS);
	}

	for(uint i = 0; i < sizeof(argv[1])/sizeof(char) - 1; i++)
	{
		cout << (uint)argv[1][i] << endl;
		if((!isdigit(argv[1][i])) && (argv[1][i] != 0))
		{
			cout << "The argument for number of nodes must be an integer." << endl;
			exit(NONINTEGERARG);
		}
	}

	ErrCode errVar = OKAY;

	vector<string> subnetNames = GetSubnetsToScan(&errVar);

	if(errVar != OKAY || subnetNames.empty())
	{
		LOG(ERROR, "There was a problem determining the subnets to scan, or there are no interfaces to scan on. Stopping execution.", "");
		return errVar;
	}

	errVar = LoadPersonalityTable(subnetNames);

	if(errVar != OKAY)
	{
		LOG(ERROR, "There was a problem loading the personality table. Stopping execution.", "");
		return errVar;
	}

	PersonalityTree persTree = PersonalityTree(&personalities, subnetsDetected);

	NodeManager nodeBuilder = NodeManager(&persTree);
	nodeBuilder.GenerateNodes(atoi(argv[1]));

	persTree.ToXmlTemplate();

	return errVar;
}

Nova::ErrCode Nova::LoadPersonalityTable(vector<string> subnetNames)
{
	stringstream ss;

	// For each element in recv (which contains strings of the subnets),
	// do an OS fingerprinting scan and output the results into a different
	// xml file for each subnet.
	for(uint16_t i = 0; i < subnetNames.size(); i++)
	{
		ss << i;
		string executionString = "sudo nmap -O --osscan-guess -oX subnet" + ss.str() + ".xml " + subnetNames[i] + " >/dev/null";
		while(system(executionString.c_str()));
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

void Nova::PrintStringVector(vector<string> stringVector)
{
	// Debug method to output what subnets were found by
	// the GetSubnetsToScan() method.
	cout << "Subnets to be scanned: " << '\n';
	for(uint16_t i = 0; i < stringVector.size(); i++)
	{
		cout << stringVector[i] << '\n';
	}
	cout << endl;
}

vector<string> Nova::GetSubnetsToScan(Nova::ErrCode *errVar)
{
	struct ifaddrs *devices = NULL;
	ifaddrs *curIf = NULL;
	stringstream ss;
	vector<string> hostAddrStrings;
	if(errVar == NULL)
	{
		return hostAddrStrings;
	}
	hostAddrStrings.clear();
	char addrBuffer[NI_MAXHOST];
	char bitmaskBuffer[NI_MAXHOST];
	struct in_addr netOrderAddrStruct;
	struct in_addr netOrderBitmaskStruct;
	struct in_addr minAddrInRange;
	struct in_addr maxAddrInRange;
	uint32_t hostOrderAddr;
	uint32_t hostOrderBitmask;
	bool subnetExists = false;

	// If we call getifaddrs and it fails to obtain any information, no point in proceeding.
	// Return the empty addresses vector and set the ErrorCode to AUTODETECTFAIL.
	if(getifaddrs(&devices))
	{
		cout << "Ethernet Interface Auto-Detection failed" << endl;
		*errVar = AUTODETECTFAIL;
		return hostAddrStrings;
	}

	vector<string> interfaces;

	// For every found interface, we need to do some processing.
	for(curIf = devices; curIf != NULL; curIf = curIf->ifa_next)
	{
		// IF we've found a loopback address with an IPv4 address
		if((curIf->ifa_flags & IFF_LOOPBACK) && ((int)curIf->ifa_addr->sa_family == AF_INET))
		{
			Subnet newSubnet;
			// start processing it to generate the subnet for the interface.
			subnetExists = false;
			interfaces.push_back(string(curIf->ifa_name));

			// Get the string representation of the interface's IP address,
			// and put it into the host character array.
			int socket = getnameinfo(curIf->ifa_addr, sizeof(sockaddr_in), addrBuffer, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

			if(socket != 0)
			{
				// If getnameinfo returned an error, stop processing for the
				// method, assign the proper errorCode, and return an empty
				// vector.
				cout << "Getting Name info of Interface IP failed" << endl;
				*errVar = GETNAMEINFOFAIL;
				return hostAddrStrings;
			}

			// Do the same thing as the above, but for the netmask of the interface
			// as opposed to the IP address.
			socket = getnameinfo(curIf->ifa_netmask, sizeof(sockaddr_in), bitmaskBuffer, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

			if(socket != 0)
			{
				// If getnameinfo returned an error, stop processing for the
				// method, assign the proper errorCode, and return an empty
				// vector.
				cout << "Getting Name info of Interface Netmask failed" << endl;
				*errVar = GETBITMASKFAIL;
				return hostAddrStrings;
			}
			// Convert the bitmask and host address character arrays to strings
			// for use later
			string bitmaskString = string(bitmaskBuffer);
			string addrString = string(addrBuffer);
			localMachine = addrString;

			// Spurious debug prints for now. May change them to be used
			// in UI hooks later.
			cout << "Interface: " << curIf->ifa_name << endl;
			cout << "Address: " << addrString << endl;
			cout << "Netmask: " << bitmaskString << endl;

			// Put the network ordered address values into the
			// address and bitmaks in_addr structs, and then
			// convert them to host longs for use in
			// determining how much hostspace is empty
			// on this interface's subnet.
			inet_aton(addrString.c_str(), &netOrderAddrStruct);
			inet_aton(bitmaskString.c_str(), &netOrderBitmaskStruct);
			hostOrderAddr = ntohl(netOrderAddrStruct.s_addr);
			hostOrderBitmask = ntohl(netOrderBitmaskStruct.s_addr);

			// Get the base address for the subnet
			uint32_t hostOrderMinAddrInRange = hostOrderBitmask & hostOrderAddr;
			minAddrInRange.s_addr = htonl(hostOrderMinAddrInRange);

			// and the max address
			uint32_t hostOrderMaxAddrInRange = ~(hostOrderBitmask) + hostOrderMinAddrInRange;
			maxAddrInRange.s_addr = htonl(hostOrderMaxAddrInRange);

			// and then add max - base (minus three, for the current
			// host, the .0 address, and the .255 address)
			// into the PersonalityTable's aggregate coutn of
			// available host address space.
			personalities.m_numAddrsAvail += hostOrderMaxAddrInRange - hostOrderMinAddrInRange - 3;

			// Find out how many bits there are to work with in
			// the subnet (i.e. X.X.X.X/24? X.X.X.X/31?).
			uint32_t tempRawMask = ~hostOrderBitmask;
			int i = 32;

			while(tempRawMask != 0)
			{
				tempRawMask /= 2;
				i--;
			}

			ss << i;

			// Generate a string of the form X.X.X.X/## for use in nmap scans later
			addrString = string(inet_ntoa(minAddrInRange)) + "/" + ss.str();

			ss.str("");

			// Populate the subnet struct for use in the SubnetTable of the HoneydConfiguration
			// object.
			newSubnet.m_address = addrString;
			newSubnet.m_mask = string(inet_ntoa(netOrderBitmaskStruct));
			newSubnet.m_maskBits = i;
			newSubnet.m_base = minAddrInRange.s_addr;
			newSubnet.m_max = maxAddrInRange.s_addr;
			newSubnet.m_name = string(curIf->ifa_name);
			newSubnet.m_enabled = (curIf->ifa_flags & IFF_UP);
			newSubnet.m_isRealDevice = true;

			// If we have two interfaces that point the same subnet, we only want
			// to scan once; so, change the "there" flag to reflect that the subnet
			// exists to prevent it from being pushed again.
			for(uint16_t j = 0; j < hostAddrStrings.size() && !subnetExists; j++)
			{
				if(!addrString.compare(hostAddrStrings[j]))
				{
					subnetExists = true;
				}
			}

			// Want to add loopbacks to the subnets (for Doppelganger) but not to the
			// addresses to scan vector
			if(!subnetExists)
			{
				subnetsDetected.push_back(newSubnet);
			}
			// Otherwise, don't do anything.
			else
			{
			}
		}
		// If we've found an interface that has an IPv4 address and isn't a loopback
		if(!(curIf->ifa_flags & IFF_LOOPBACK) && ((int)curIf->ifa_addr->sa_family == AF_INET))
		{
			Subnet newSubnet;
			// start processing it to generate the subnet for the interface.
			subnetExists = false;
			interfaces.push_back(string(curIf->ifa_name));

			// Get the string representation of the interface's IP address,
			// and put it into the host character array.
			int s = getnameinfo(curIf->ifa_addr, sizeof(sockaddr_in), addrBuffer, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

			if(s != 0)
			{
				// If getnameinfo returned an error, stop processing for the
				// method, assign the proper errorCode, and return an empty
				// vector.
				cout << "Getting Name info of Interface IP failed" << endl;
				*errVar = GETNAMEINFOFAIL;
				return hostAddrStrings;
			}

			// Do the same thing as the above, but for the netmask of the interface
			// as opposed to the IP address.
			s = getnameinfo(curIf->ifa_netmask, sizeof(sockaddr_in), bitmaskBuffer, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

			if(s != 0)
			{
				// If getnameinfo returned an error, stop processing for the
				// method, assign the proper errorCode, and return an empty
				// vector.
				cout << "Getting Name info of Interface Netmask failed" << endl;
				*errVar = GETBITMASKFAIL;
				return hostAddrStrings;
			}
			// Convert the bitmask and host address character arrays to strings
			// for use later
			string bitmaskString = string(bitmaskBuffer);
			string addrString = string(addrBuffer);
			localMachine = addrString;

			// Spurious debug prints for now. May change them to be used
			// in UI hooks later.
			cout << "Interface: " << curIf->ifa_name << endl;
			cout << "Address: " << addrString << endl;
			cout << "Netmask: " << bitmaskString << endl;

			// Put the network ordered address values into the
			// address and bitmaks in_addr structs, and then
			// convert them to host longs for use in
			// determining how much hostspace is empty
			// on this interface's subnet.
			inet_aton(addrString.c_str(), &netOrderAddrStruct);
			inet_aton(bitmaskString.c_str(), &netOrderBitmaskStruct);
			hostOrderAddr = ntohl(netOrderAddrStruct.s_addr);
			hostOrderBitmask = ntohl(netOrderBitmaskStruct.s_addr);

			// Get the base address for the subnet
			uint32_t hostOrderMinAddrInRange = hostOrderBitmask & hostOrderAddr;
			minAddrInRange.s_addr = htonl(hostOrderMinAddrInRange);

			// and the max address
			uint32_t hostOrderMaxAddrInRange = ~(hostOrderBitmask) + hostOrderMinAddrInRange;
			maxAddrInRange.s_addr = htonl(hostOrderMaxAddrInRange);

			// and then add max - base (minus three, for the current
			// host, the .0 address, and the .255 address)
			// into the PersonalityTable's aggregate coutn of
			// available host address space.
			personalities.m_numAddrsAvail += hostOrderMaxAddrInRange - hostOrderMinAddrInRange - 3;

			// Find out how many bits there are to work with in
			// the subnet (i.e. X.X.X.X/24? X.X.X.X/31?).
			uint32_t mask = ~hostOrderBitmask;
			int i = 32;

			while(mask != 0)
			{
				mask /= 2;
				i--;
			}

			ss << i;

			// Generate a string of the form X.X.X.X/## for use in nmap scans later
			string push = string(inet_ntoa(minAddrInRange)) + "/" + ss.str();

			ss.str("");

			newSubnet.m_address = push;
			newSubnet.m_mask = string(inet_ntoa(netOrderBitmaskStruct));
			newSubnet.m_maskBits = i;
			newSubnet.m_base = minAddrInRange.s_addr;
			newSubnet.m_max = maxAddrInRange.s_addr;
			newSubnet.m_name = string(curIf->ifa_name);
			newSubnet.m_enabled = (curIf->ifa_flags & IFF_UP);
			newSubnet.m_isRealDevice = true;

			// If we have two interfaces that point the same subnet, we only want
			// to scan once; so, change the "there" flag to reflect that the subnet
			// exists to prevent it from being pushed again.
			for(uint16_t j = 0; j < hostAddrStrings.size() && !subnetExists; j++)
			{
				if(!push.compare(hostAddrStrings[j]))
				{
					subnetExists = true;
				}
			}

			// If the subnet isn't in the addresses vector, push it in.
			if(!subnetExists)
			{
				hostAddrStrings.push_back(push);
				subnetsDetected.push_back(newSubnet);
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
	return hostAddrStrings;
}
