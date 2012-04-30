//============================================================================
// Name        : VendorMacDb.cpp
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
// Description : Class for using the nmap MAC prefix file
//============================================================================

#include "VendorMacDb.h"
#include "NovaUtil.h"
#include "Config.h"

#include <errno.h>
#include <fstream>
#include <math.h>
#include <sstream>
#include <string.h>

using namespace std;
using namespace Nova;

VendorMacDb::VendorMacDb()
{
	m_macVendorFile =  Config::Inst()->GetPathReadFolder() + "/nmap-mac-prefixes";
	m_MACVendorTable.set_empty_key(16777216); //2^24, invalid MAC prefix.
	m_vendorMACTable.set_empty_key("");
}

VendorMacDb::VendorMacDb(string MacVendorFile)
{
	m_macVendorFile = MacVendorFile;
	m_MACVendorTable.set_empty_key(16777216); //2^24, invalid MAC prefix.
	m_vendorMACTable.set_empty_key("");
}

void VendorMacDb::LoadPrefixFile()
{
	ifstream MACPrefixes(m_macVendorFile.c_str());
	string line, vendor, prefixStr;
	char * notUsed;
	uint prefix;
	if(MACPrefixes.is_open())
	{
		while(MACPrefixes.good())
		{
			getline(MACPrefixes, prefixStr, ' ');
			/* From 'man strtoul'  Since strtoul() can legitimately return 0 or  LONG_MAX  (LLONG_MAX  for
		       strtoull()) on both success and failure, the calling program should set
		       errno to 0 before the call, and then determine if an error occurred  by
		       checking whether errno has a nonzero value after the call. */
			errno = 0;
			prefix = strtoul(prefixStr.c_str(), &notUsed, 16);
			if(errno)
				continue;
			getline(MACPrefixes, vendor);
			m_MACVendorTable[prefix] = vendor;
			if(m_vendorMACTable.find(vendor) != m_vendorMACTable.end())
			{
				m_vendorMACTable[vendor]->push_back(prefix);
			}
			else
			{
				vector<uint> * vect = new vector<uint>;
				vect->push_back(prefix);
				m_vendorMACTable[vendor] = vect;
			}
		}
	}
}


//Randomly selects one of the ranges associated with vendor and generates the remainder of the MAC address
// *note conflicts are only checked for locally, weird things may happen if the address is already being used.
string VendorMacDb::GenerateRandomMAC(string vendor)
{
	char addrBuffer[8];
	stringstream addrStrm;
	pair<uint32_t,uint32_t> addr;
	VendorToMACTable::iterator it;
	string testStr;

	//If we can resolve the vendor to a range
	if((it = m_vendorMACTable.find(vendor)) != m_vendorMACTable.end())
	{
		//Randomly select one of the ranges
		uint j;
		uint i = rand() % it->second->size();
		addr.first = it->second->at(i);
		i = 0;

		//Convert the first part to a string and format it for output
		sprintf(addrBuffer, "%x", addr.first);
		testStr = string(addrBuffer);
		for(j = 0; j < (6-testStr.size()); j++)
		{
			if(!(i%2)&& i )
			{
				addrStrm << ":";
			}
			addrStrm << "0";
			i++;
		}
		j = 0;
		if(testStr.size() > 6)
			j = testStr.size() - 6;
		for(; j < testStr.size(); j++)
		{
			if(!(i%2)&& i )
			{
				addrStrm << ":";
			}
			addrStrm << addrBuffer[j];
			i++;
		}

		//Randomly generate the remaining portion
		addr.second = ((uint)rand() & (uint)(pow(2.0,24)-1));

		//Convert the second part to a string and format it for output
		bzero(addrBuffer, 8);
		sprintf(addrBuffer, "%x", addr.second);
		testStr = string(addrBuffer);
		i = 0;
		for(j = 0; j < (6-testStr.size()); j++)
		{
			if(!(i%2)&& i )
			{
				addrStrm << ":";
			}
			addrStrm << "0";
			i++;
		}
		j = 0;
		if(testStr.size() > 6)
			j = testStr.size() - 6;
		for(; j < testStr.size(); j++)
		{
			if(!(i%2)&& (i!=6))
			{
				addrStrm << ":";
			}
			addrStrm << addrBuffer[j];
			i++;
		}
	}
	return addrStrm.str();
}

//Resolve the first 3 bytes of a MAC Address to a MAC vendor that owns the range, returns the vendor string
string VendorMacDb::LookupVendor(uint MACPrefix)
{
	if(m_MACVendorTable.find((uint)(MACPrefix)) != m_MACVendorTable.end())
		return m_MACVendorTable[(uint)(MACPrefix)];
	else
		return "";
}

bool VendorMacDb::IsVendorValid(string vendor)
	{
		return (m_vendorMACTable.find(vendor)) == m_vendorMACTable.end();
	}

vector<string> VendorMacDb::SearchVendors(string partialVendorName)
{
	vector<string> matches;
	bool matched;

	for(VendorToMACTable::iterator it = m_vendorMACTable.begin(); it != m_vendorMACTable.end(); it++)
	{
		matched = true;
		if(strcasestr(it->first.c_str(), partialVendorName.c_str()) == NULL)
			matched = false;

		if(matched)
			matches.push_back(it->first);
	}

	return matches;
}

vector<string> VendorMacDb::GetVendorNames() {
	vector<string> names;
	for (VendorToMACTable::iterator it = m_vendorMACTable.begin(); it != m_vendorMACTable.end(); it++)
	{
		names.push_back(it->first);
	}
	return names;
}
