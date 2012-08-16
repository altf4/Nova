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

#include <stdio.h>
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
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "HoneydConfiguration/NodeManager.h"
#include "PersonalityTree.h"
#include "HoneydHostConfig.h"
#include "HoneydConfiguration/VendorMacDb.h"
#include "Logger.h"

using namespace std;
using namespace Nova;

vector<uint16_t> leftoverHostspace;
vector<string> interfacesToMatch;
vector<string> subnetsToAdd;
uint16_t tempHostspace;
string localMachine;
string nmapFileName;
uint numNodes;

vector<Subnet> subnetsDetected;

PersonalityTable personalities;

string lockFilePath;

int main(int argc, char ** argv)
{
	try
	{
		namespace po = boost::program_options;
		po::options_description desc("Command line options");
		desc.add_options()
				("help,h", "Show command line options")
				("num-nodes,n", po::value<uint>(&numNodes)->default_value(0), "Number of nodes to create.")
				("interface,i", po::value<vector<string> >(), "Interface(s) to use for subnet selection.")
				("additional-subnet,a", po::value<vector<string> >(), "Additional subnets to scan. Must be subnets that will return Nmap results from the AutoConfig tool's location, and of the form XXX.XXX.XXX.XXX/##")
				("nmap-xml,f", po::value<string>(), "Nmap 6.00+ XML output file to parse instead of scanning. Selecting this option skips the subnet identification and scanning phases, thus the INTERFACE and ADDITIONAL-SUBNET options will do nothing.")
		;

		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		lockFilePath = Config::Inst()->GetPathHome() + "/hhconfig.lock";
		ofstream lockFile(lockFilePath);

		bool i_flag_empty = true;
		bool a_flag_empty = true;
		bool f_flag_set = false;

		if(vm.count("help"))
		{
			cout << desc << endl;
			return HHC_CODE_OKAY;
		}
		if(vm.count("num-nodes"))
		{
			cout << "Number of nodes to create: " << numNodes << '\n';
		}
		if(vm.count("interface"))
		{
			vector<string> interfaces_flag = vm["interface"].as< vector<string> >();

			for(uint i = 0; i < interfaces_flag.size(); i++)
			{
				if(interfaces_flag[i].find(",") != string::npos)
				{
					vector<string> splitInterfacesFlagI;
					string split = interfaces_flag[i];
					boost::split(splitInterfacesFlagI, split, boost::is_any_of(","));

					for(uint j = 0; j < splitInterfacesFlagI.size(); j++)
					{
						if(!splitInterfacesFlagI[j].empty())
						{
							interfacesToMatch.push_back(splitInterfacesFlagI[j]);
						}
					}
				}
				else
				{
					interfacesToMatch.push_back(interfaces_flag[i]);
				}
			}
			i_flag_empty = false;
		}
		if(vm.count("additional-subnet"))
		{
			vector<string> addsubnets_flag = vm["additional-subnet"].as< vector<string> >();

			for(uint i = 0; i < addsubnets_flag.size(); i++)
			{
				if(addsubnets_flag[i].find(",") != string::npos)
				{
					vector<string> splitSubnetsFlagI;
					string split = addsubnets_flag[i];
					boost::split(splitSubnetsFlagI, split, boost::is_any_of(","));

					for(uint j = 0; j < splitSubnetsFlagI.size(); j++)
					{
						if(!splitSubnetsFlagI[j].empty())
						{
							subnetsToAdd.push_back(splitSubnetsFlagI[j]);
						}
					}
				}
				else
				{
					subnetsToAdd.push_back(addsubnets_flag[i]);
				}
			}

			a_flag_empty = false;
		}
		if(vm.count("nmap-xml"))
		{
			cout << "file to parse is " << vm["nmap-xml"].as< string >() << endl;
			nmapFileName = vm["nmap-xml"].as<string>();
			f_flag_set = true;
		}

		if(numNodes < 0)
		{
			lockFile.close();
			remove(lockFilePath.c_str());
			LOG(ERROR, "num-nodes argument takes an integer greater than or equal to 0. Aborting...", "");
			exit(HHC_CODE_BAD_ARG_VALUE);
		}

		HHC_ERR_CODE errVar = HHC_CODE_OKAY;

		// Arg parsing done, moving onto execution items
		if(f_flag_set)
		{
			LOG(ALERT, "Launching Honeyd Host Configuration Tool", "");
			if(!LoadNmapXML(nmapFileName))
			{
				LOG(ERROR, "LoadNmapXML failed. Aborting...", "");
				lockFile.close();
				remove(lockFilePath.c_str());
				exit(HHC_CODE_PARSING_ERROR);
			}

			GenerateConfiguration();
			lockFile.close();
			remove(lockFilePath.c_str());
			LOG(INFO, "Honeyd profile and node configuration completed.", "");
			return errVar;
		}
		else if(a_flag_empty && i_flag_empty)
		{
			errVar = HHC_CODE_REQUIRED_FLAGS_MISSING;
			lockFile.close();
			remove(lockFilePath.c_str());
			LOG(ERROR, "Must designate an Nmap XML file to parse, or provide either an interface or a subnet to scan. Aborting...", "");
			return errVar;
		}
		else
		{
			LOG(ALERT, "Launching Honeyd Host Configuration Tool", "");
			vector<string> subnetNames;

			if(!i_flag_empty)
			{
				subnetNames = GetSubnetsToScan(&errVar, interfacesToMatch);
			}
			if(!a_flag_empty)
			{
				for(uint i = 0; i < subnetsToAdd.size(); i++)
				{
					subnetNames.push_back(subnetsToAdd[i]);
				}
			}

			cout << "Scanning following subnets: " << endl;
			for(uint i = 0; i < subnetNames.size(); i++)
			{
				cout << "\t" << subnetNames[i] << endl;
			}
			cout << endl;

			errVar = LoadPersonalityTable(subnetNames);

			if(errVar != HHC_CODE_OKAY)
			{
				lockFile.close();
				remove(lockFilePath.c_str());
				LOG(ERROR, "There was a problem loading the PersonalityTable. Aborting...", "");
				return errVar;
			}

			GenerateConfiguration();
			lockFile.close();
			remove(lockFilePath.c_str());
			LOG(INFO, "Honeyd profile and node configuration completed.", "");
			return errVar;
		}
	}
	catch(exception &e)
	{
		LOG(ERROR, "Uncaught exception: " + string(e.what()) + ".", "");
	}
}

HHC_ERR_CODE Nova::ParseHost(boost::property_tree::ptree propTree)
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
						cout << "address " << value.second.get<string>("<xmlattr>.addr") << endl;
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

						string vendorString = "";

						int s;
						struct ifreq buffer;
						vector<string> interfacesFromConfig = Config::Inst()->ListInterfaces();

						for(uint j = 0; j < interfacesFromConfig.size() && !vendorString.empty(); j++)
						{
							s = socket(PF_INET, SOCK_DGRAM, 0);

							memset(&buffer, 0x00, sizeof(buffer));
							strcpy(buffer.ifr_name, interfacesFromConfig[j].c_str());
							ioctl(s, SIOCGIFHWADDR, &buffer);
							close(s);

							char macPair[3];

							stringstream ss;

							for(s = 0; s < 6; s++)
							{
								sprintf(macPair, "%2x", (unsigned char)buffer.ifr_hwaddr.sa_data[s]);
								cout << "macPair from " << s << " is " << macPair << endl;
								ss << macPair;
								if(s != 5)
								{
									ss << ":";
								}
								cout << "ss now has " << ss.str() << endl;
							}
						}

						ifstream ifs("/sys/class/net/eth0/address");
						char macBuffer[18];
						ifs.getline(macBuffer, 18);

						string macString = string(macBuffer);
						persObject->m_macs.push_back(macString);

						VendorMacDb *macVendorDB = new VendorMacDb();
						macVendorDB->LoadPrefixFile();
						uint rawMACPrefix = macVendorDB->AtoMACPrefix(macString);
						vendorString = macVendorDB->LookupVendor(rawMACPrefix);

						if(!vendorString.empty())
						{
							try
							{
								persObject->AddVendor(vendorString);
							}
							catch(emptyKeyException &e)
							{
								LOG(ERROR,("Couldn't determine MAC vendor type for local machine"), "");
							}
						}
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
		return HHC_CODE_NO_MATCHED_PERSONALITY;
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

	return HHC_CODE_OKAY;
}

bool Nova::LoadNmapXML(const string &filename)
{
	using boost::property_tree::ptree;
	ptree propTree;

	if(filename.empty())
	{
		LOG(ERROR, "Empty string passed to LoadNmapXml", "");
		return false;
	}
	// Read the nmap xml output file (given by the filename parameter) into a boost
	// property tree for parsing of the found hosts.
	try
	{
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
					case HHC_CODE_NO_MATCHED_PERSONALITY:
					{
						LOG(WARNING, "Unable to obtain personality data for host "
							+ tempPropTree.get<std::string>("address.<xmlattr>.addr") + ".", "");
						break;
					}
					// Everything parsed fine, don't output anything.
					case HHC_CODE_OKAY:
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
	catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
	{
		LOG(ERROR, "Couldn't parse XML file (bad path): " + string(e.what()) + ".", "");
		return false;
	}
	catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::xml_parser::xml_parser_error> > &e)
	{
		LOG(ERROR, "Couldn't parse from file (parser error) " + filename + ": " + string(e.what()) + ".", "");
		return false;
	}
	catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_data> > &e)
	{
		LOG(DEBUG, "Couldn't parse XML file (bad data): " + string(e.what()) + ".", "");
		return false;
	}
	return true;
}

Nova::HHC_ERR_CODE Nova::LoadPersonalityTable(vector<string> subnetNames)
{
	if(subnetNames.empty())
	{
		LOG(ERROR, "Passed empty vector of subnet names to LoadPersonalityTable.", "");
		return HHC_CODE_BAD_FUNCTION_PARAM;
	}

	stringstream ss;
	// For each element in recv (which contains strings of the subnets),
	// do an OS fingerprinting scan and output the results into a different
	// xml file for each subnet.
	for(uint16_t i = 0; i < subnetNames.size(); i++)
	{
		ss << i;
		string executionString = "sudo nmap -O --osscan-guess -oX subnet" + ss.str() + ".xml " + subnetNames[i];

		//popen here for stdout of nmap
		for(uint j = 0; j < 3; j++)
		{
			stringstream s2;
			s2 << (j + 1);
			LOG(INFO, "Attempt " + s2.str() + " to start Nmap scan.", "");
			s2.str("");

			FILE* nmap = popen(executionString.c_str(), "r");

			if(nmap == NULL)
			{
				LOG(ERROR, "Couldn't start Nmap.", "");
				continue;
			}
			else
			{
				char buffer[256];
				while(!feof(nmap))
				{
					if(fgets(buffer, 256, nmap) != NULL)
					{
						// Would avoid using endl, but need it for what this line is being used for (printing nmap output to Web UI)
						cout << string(buffer) << endl;
					}
				}
			}

			pclose(nmap);
			LOG(INFO, "Nmap scan complete.", "");
			j = 3;
		}

		try
		{
			string file = "subnet" + ss.str() + ".xml";
			// Call LoadNmap on the generated filename so that we
			// can add hosts to the personality table, provided they
			// contain enough data.
			LoadNmapXML(file);
		}
		catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e)
		{
			LOG(DEBUG, string("Caught Exception : ") + e.what(), "");
			return HHC_CODE_PARSING_ERROR;
		}
		catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_data> > &e)
		{
			LOG(DEBUG, string("Caught Exception : ") + e.what(), "");
			return HHC_CODE_PARSING_ERROR;
		}
		ss.str("");
	}
	return HHC_CODE_OKAY;
}

void Nova::PrintStringVector(vector<string> stringVector)
{
	// Debug method to output what subnets were found by
	// the GetSubnetsToScan() method.
	stringstream ss;
	ss << "Subnets to be scanned: ";
	for(uint16_t i = 0; i < stringVector.size(); i++)
	{
		ss <<  stringVector[i] << " & ";
	}
	LOG(DEBUG, ss.str(), "");
}

vector<string> Nova::GetSubnetsToScan(Nova::HHC_ERR_CODE *errVar, vector<string> interfacesToMatch)
{
	vector<string> hostAddrStrings;
	hostAddrStrings.clear();

	if(errVar == NULL || interfacesToMatch.empty())
	{
		LOG(ERROR, "errVar is NULL or empty vector of interfaces passed to GetSubnetsToScan", "");
		return hostAddrStrings;
	}

	struct ifaddrs *devices = NULL;
	ifaddrs *curIf = NULL;
	stringstream ss;

	char addrBuffer[NI_MAXHOST];
	char bitmaskBuffer[NI_MAXHOST];
	struct in_addr netOrderAddrStruct;
	struct in_addr netOrderBitmaskStruct;
	struct in_addr minAddrInRange;
	struct in_addr maxAddrInRange;
	uint32_t hostOrderAddr;
	uint32_t hostOrderBitmask;

	// If we call getifaddrs and it fails to obtain any information, no point in proceeding.
	// Return the empty addresses vector and set the ErrorCode to AUTODETECTFAIL.
	if(getifaddrs(&devices))
	{
		LOG(ERROR, "Ethernet Interface Auto-Detection failed" , "");
		*errVar = HHC_CODE_AUTODETECT_FAIL;
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
			interfaces.push_back(string(curIf->ifa_name));

			// Get the string representation of the interface's IP address,
			// and put it into the host character array.
			int socket = getnameinfo(curIf->ifa_addr, sizeof(sockaddr_in), addrBuffer, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

			if(socket != 0)
			{
				// If getnameinfo returned an error, stop processing for the
				// method, assign the proper errorCode, and return an empty
				// vector.
				LOG(WARNING, "Getting Name info of Interface IP failed", "");
				*errVar = HHC_CODE_GET_NAMEINFO_FAIL;
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
				LOG(WARNING, "Getting Name info of Interface Netmask failed", "");
				*errVar = HHC_CODE_GET_BITMASK_FAIL;
				return hostAddrStrings;
			}
			// Convert the bitmask and host address character arrays to strings
			// for use later
			string bitmaskString = string(bitmaskBuffer);
			string addrString = string(addrBuffer);

			// Spurious debug prints for now. May change them to be used
			// in UI hooks later.
			//cout << "Interface: " << curIf->ifa_name << endl;
			//cout << "Address: " << addrString << endl;
			//cout << "Netmask: " << bitmaskString << endl;

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

			// Want to add loopbacks to the subnets (for Doppelganger) but not to the
			// addresses to scan vector
			if(!CheckSubnet(hostAddrStrings, addrString))
			{
				subnetsDetected.push_back(newSubnet);
			}
		}
		// If we've found an interface that has an IPv4 address and isn't a loopback
		if(!(curIf->ifa_flags & IFF_LOOPBACK) && ((int)curIf->ifa_addr->sa_family == AF_INET))
		{
			bool go = false;

			for(uint i = 0; i < interfacesToMatch.size(); i++)
			{
				if(!string(curIf->ifa_name).compare(interfacesToMatch[i]))
				{
					go = true;
				}
			}

			if(!go)
			{
				continue;
			}

			Subnet newSubnet;
			// start processing it to generate the subnet for the interface.
			interfaces.push_back(string(curIf->ifa_name));

			// Get the string representation of the interface's IP address,
			// and put it into the host character array.
			int s = getnameinfo(curIf->ifa_addr, sizeof(sockaddr_in), addrBuffer, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

			if(s != 0)
			{
				// If getnameinfo returned an error, stop processing for the
				// method, assign the proper errorCode, and return an empty
				// vector.
				LOG(WARNING, "Getting Name info of Interface IP failed", "");
				*errVar = HHC_CODE_GET_NAMEINFO_FAIL;
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
				LOG(WARNING, "Getting Name info of Interface Netmask failed", "");
				*errVar = HHC_CODE_GET_BITMASK_FAIL;
				return hostAddrStrings;
			}
			// Convert the bitmask and host address character arrays to strings
			// for use later
			string bitmaskString = string(bitmaskBuffer);
			string addrString = string(addrBuffer);
			if(localMachine.empty())
			{
				localMachine = addrString;
			}

			// Spurious debug prints for now. May change them to be used
			// in UI hooks later.
			//cout << "Interface: " << curIf->ifa_name << endl;
			//cout << "Address: " << addrString << endl;
			//cout << "Netmask: " << bitmaskString << endl;

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
			// Generate a string of the form X.X.X.X/## for use in nmap scans later
			ss << i;
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

			// If the subnet isn't in the addresses vector, push it in.
			if(!CheckSubnet(hostAddrStrings, push))
			{
				hostAddrStrings.push_back(push);
				subnetsDetected.push_back(newSubnet);
			}
		}
	}

	// Deallocate the devices struct from the beginning of the method,
	// and return the vector containing the subnets from each interface.
	freeifaddrs(devices);
	return hostAddrStrings;
}

void Nova::GenerateConfiguration()
{
	personalities.CalculateDistributions();
	PersonalityTree persTree = PersonalityTree(&personalities, subnetsDetected);
	persTree.AddAllPorts();
	NodeManager nodeBuilder = NodeManager(persTree.GetHDConfig());
	nodeBuilder.SetNumNodesOnProfileTree(&persTree.GetHDConfig()->m_profiles["default"], numNodes);
	persTree.GetHDConfig()->SaveAllTemplates();
}

bool Nova::CheckSubnet(vector<string> &hostAddrStrings, string matchStr)
{
	if(!hostAddrStrings.empty() || matchStr.empty())
	{
		LOG(ERROR, "Empty vector of host address strings or empty string to match passed to CheckSubnet.", "");
		return false;
	}

	for(uint16_t j = 0; j < hostAddrStrings.size(); j++)
	{
		if(!matchStr.compare(hostAddrStrings[j]))
		{
			return true;
		}
	}
	return false;
}
