//============================================================================
// Name        : HoneydHostConfig.h
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
// Description : Header for the HoneydHostConfig.cpp file, defines some variables
//               and provides function declarations
//============================================================================
#ifndef HONEYDHOSTCONFIG_
#define HONEYDHOSTCONFIG_

#include "HashMapStructs.h"
#include "HashMap.h"
#include <boost/property_tree/ptree.hpp>

namespace Nova
{
enum ErrCode : int
{
	OKAY = 0,
	AUTODETECTFAIL,
	GETNAMEINFOFAIL,
	GETBITMASKFAIL,
	NOMATCHEDPERSONALITY,
	PARSINGERROR,
	INCORRECTNUMBERARGS,
	NONINTEGERARG
};

// Loads the nmap xml output into a ptree and passes <host> child nodes to ParseHost
//  const std::string &filename - string of the xml filename to read
// Returns nothing
void LoadNmap(const std::string &filename);

// Puts the integer val into m, and shifts it
// depending on the value of cond. Used for converting
// a MAC address string into a uint for use with the MAC Vendor Db object.
uint AtoMACPrefix(std::string MAC);

// Takes a <host> sub-ptree and parses it for the requisite information, placing said information
// into a Personality object which then gets passed into the PersonalityTable object
//  ptree pt2 - <host> subtree of the highest level node in the nmap xml files
// Returns ErrCode to determine what went wrong
ErrCode ParseHost(boost::property_tree::ptree pt2);

// Determines what interfaces are present, and the subnets that they're connected to
//  ErrCode errVar - ptr to an error code variable so that we can inspect it's value afterward
//   after if the vector is empty
// Returns a vector containings strings of the subnet addresses
std::vector<std::string> GetSubnetsToScan(ErrCode *errVar);

// Prints out the subnets that're found during GetSubnetsToScan(errVar)
//  vector<string> recv - vector of subnets found
// Only prints, no return value
void PrintStringVector(std::vector<std::string> recv);

// After the subnets are found, pass recv into this function to do the actual scanning
// and generation of the XML files for parsing
//  vector<string> recv - vector of subnets
// Returns an ErrCode signifying success or failure
ErrCode LoadPersonalityTable(std::vector<std::string> recv);
}
#endif
