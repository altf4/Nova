//============================================================================
// Name        : MacAddressDb.h
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
// Description : Object for using the nmap MAC prefix file
//============================================================================


#ifndef MACADDRESSDB_H_
#define MACADDRESSDB_H_

#include "HashMapStructs.h"

typedef google::dense_hash_map<uint, string, tr1::hash<uint>, eqint> MACToVendorTable;
typedef google::dense_hash_map<string, vector<uint> *,  tr1::hash<string>, eqstr > VendorToMACTable;

class VendorMacDb
{
public:
	VendorMacDb();

	// Uses a user defined vendor -> mac prefix file
	//		macVendorFile: Prefix file path. Contains MAC prefix followed by the name of the vendor
	VendorMacDb(string macVendorFile);

	// Loads the prefix file and populates it's internal state
	void LoadPrefixFile();

	// Generates a random MAC address for a vendor
	//		vendor: Name of vendor to use for the prefix
	string GenerateRandomMAC(string vendor);

	// Finds a vendor based on a MAC prefix
	string LookupVendor(uint MACPrefix);

	// Searches for a vendor that matches a string
	//		partialVendorName: Any part of the vendor name
	vector <string> SearchVendors(string partialVendorName);

	// Checks if a vendor name is known
	bool IsVendorValid(string vendor);


private:
	MACToVendorTable m_MACVendorTable;
	VendorToMACTable m_vendorMACTable;
	string m_macVendorFile;
};


#endif /* MACADDRESSDB_H_ */
