//============================================================================
// Name        : NOVAConfiguration.h
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Class to load and parse the NOVA configuration file
//============================================================================/*

#ifndef NOVACONFIGURATION_H_
#define NOVACONFIGURATION_H_


#include "NovaUtil.h"

using namespace std;

namespace Nova {

// Represents a configuration option
struct NovaOption {
	bool isValid; // Was the data read and parsed okay?
	string data; // Value for the configuration variable
};

// Maps the configuration prefix (e.g. INTERFACE) to a NovaOption instance
typedef google::dense_hash_map<string, struct NovaOption, tr1::hash<string>, eqstr > optionsMap;
typedef vector <pair<string, string> > defaultVector;

class NOVAConfiguration {
public:
	NOVAConfiguration();
	~NOVAConfiguration();

	// Loads and parses a NOVA configuration file
	//		configFilePath - Path to the NOVA configuration file
	//		homeNovaPath - Path to the /home/user/.nova folder
	//      module - added s.t. rsyslog  will output NovaConfig messages as the parent process that called LoadConfig
	void LoadConfig(char* configFilePath, string homeNovaPath, string module);
	// Checks through the optionsMap options and sets the default value for any
	// configuration variable that has a false isValid attribute
	void SetDefaults();

public:
	// Map of configuration variable name to NovaOption (isValid and data)
	optionsMap options;
	defaultVector defaults;
};

}

#endif /* NOVACONFIGURATION_H_ */
