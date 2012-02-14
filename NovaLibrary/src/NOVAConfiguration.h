//============================================================================
// Name        : NOVAConfiguration.h
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
// Description : Class to load and parse the NOVA configuration file
//============================================================================/*

#ifndef NOVACONFIGURATION_H_
#define NOVACONFIGURATION_H_


#include "NovaUtil.h"
#include "Logger.h"

using namespace std;

namespace Nova {

// Represents a configuration option
struct NovaOption {
	bool isValid; // Was the data read and parsed okay?
	string data; // Value for the configuration variable
};

using namespace google;
// Maps the configuration prefix (e.g. INTERFACE) to a NovaOption instance
typedef dense_hash_map<string, struct NovaOption, tr1::hash<string>, eqstr > optionsMap;
typedef vector <pair<string, string> > defaultVector;

class NOVAConfiguration {
public:
	NOVAConfiguration();
	~NOVAConfiguration();

	// Loads and parses a NOVA configuration file
	//		configFilePath - Path to the NOVA configuration file
	//		homeNovaPath - Path to the /home/user/.nova folder
	//      module - added s.t. rsyslog  will output NovaConfig messages as the parent process that called LoadConfig
	void LoadConfig(char const* configFilePath, string homeNovaPath, string module);

	// Checks through the optionsMap options and sets the default value for any
	// configuration variable that has a false isValid attribute
	int SetDefaults();

	// Checks to see if the current user has a ~/.nova directory, and creates it if not, along with default config files
	//	Returns: True if (after the function) the user has all necessary ~/.nova config files
	//		IE: Returns false only if the user doesn't have configs AND we weren't able to make them
	bool static InitUserConfigs(string homeNovaPath);

public:
	// Map of configuration variable name to NovaOption (isValid and data)
	optionsMap options;
	// associative array for the defaults that can be statically set.
	defaultVector defaults;

private:

	static const string prefixes[];
};

}

#endif /* NOVACONFIGURATION_H_ */
