//============================================================================
// Name        : Logger.h
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
// Description : Class to generate messages based on events inside the program,
// and maintain information needed for the sending of those events, mostly
// networking information that is not readily available
//============================================================================/*

#ifndef Logger_H_
#define Logger_H_


#include "NovaUtil.h"

using namespace std;
using namespace google;

namespace Nova
{

// may not need this, trying to determine if this is a good way to
// tell the class how to allocate service use or if using the enum
// below would be better
enum Services {NO_SERV, SYSLOG, LIBNOTIFY, LIBNOTIFY_BELOW, EMAIL, EMAIL_SYSLOG, EMAIL_LIBNOTIFY, EMAIL_BELOW};
// enum for NovaMessaging to use
enum Levels {EMERGENCY, ALERT, CRITICAL, ERROR, WARNING, NOTICE, INFO, DEBUG};

typedef vector<pair<uint16_t, string> > levelsMap;
typedef vector<pair<Nova::Levels, Nova::Services> > userMap;


//There will be more than this, most likely
struct MessageOptions
{
	// the SMTP server domain name for display purposes
	string smtp_domain;
	// the email address that will be set as sender
	string smtp_addr;
	// the port for SMTP send; normally 25 if I'm not mistaken, may take this out
	in_port_t smtp_port;
	// TODO: Write the comment here lol
	userMap service_preferences;
	// a vector containing the email recipients; may move this into the actual classes
	// as opposed to being in this struct
	vector<string> email_recipients;
};

typedef struct MessageOptions optionsInfo;

class Logger
{
public:
	Logger(string parent, char const * configFilePath, bool init);
	~Logger();
	// Load Configuration: loads the SMTP info and service level preferences
	// from NovaConfig.txt.
	// args:   char const * filename. This tells the method where the config
	// file is for parsing.
	// return: uint16_t ( 0 | 1). On 0, one of the options was not set, and has
	// no default. On 1, everything's fine.
	uint16_t LoadConfiguration(char const* filename);
	void SaveLoggerConfiguration(string filename);
	vector<string> ParseAddressesString(string addresses);
	// This is the hub method that will take in data from the processes,
	// use it to determine what services and levels and such need to be used, then call the private methods
	// from there
	void Logging(Nova::Levels messageLevel, string message);
	// methods for assigning the log preferences from different places
	// into the user map inside MessageOptions struct.
	// args: string logPrefString: this method is used for reading from the config file
	// args: Nova::Levels messageLevel, Nova::Services services: this is for individual
	//       adjustments to the struct from the GUI; may change the name but as they
	//		 are both used to set the preferences I figured it'd be okay.
	void setUserLogPreferences(string logPrefString);
	void setUserLogPreferences(Nova::Levels messageLevel, Nova::Services services);

private:
	// Notify makes use of the libnotify methods to produce
	// a notification with the desired level to the desktop.
	void Notify(uint16_t level, string message);
	// Log will be the method that calls syslog
	void Log(uint16_t level, string message);
	// Mail will, obviously, email people.
	void Mail(uint16_t level, string message);
	// clean the elements in the toClean vector.
	vector<string> CleanAddresses(vector<string> toClean);
	// for use in the logging public method. Purely used to reduce number
	// of arguments for void Logging().
	// TODO: write a function definition for this
	Nova::Services setServiceLevel(Nova::Levels messageLevel);
	//
	Nova::Services parseServicesFromChar(char parse);

public:
	optionsInfo messageInfo;
	levelsMap levels;

private:
	static const string prefixes[];
	string parentName;
};

}

#endif /* Logger_H_ */
