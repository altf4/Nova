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

#include "HashMapStructs.h"
#include "Defines.h"
#include <cstring>

// A macro to make logging prettier
#define LOG Logger::Inst()->Log


using namespace std;
using namespace google;

namespace Nova
{

// may not need this, trying to determine if this is a good way to
// tell the class how to allocate service use or if using the enum
// below would be better
enum Services {SYSLOG, LIBNOTIFY, EMAIL};
// enum for NovaMessaging to use. May have to switch around the order to
// make newer scheme make sense
enum Levels {DEBUG, INFO, NOTICE, WARNING, ERROR, CRITICAL, ALERT, EMERGENCY};

typedef vector<pair<uint16_t, string> > levelsMap;
typedef vector<pair< pair<Nova::Services, Nova::Levels>, char > > userMap;


//There will be more than this, most likely
struct MessageOptions
{
	// the SMTP server domain name for display purposes
	string smtp_domain;
	// the email address that will be set as sender
	string smtp_addr;
	// the port for SMTP send; normally 25 if I'm not mistaken, may take this out
	in_port_t smtp_port;
	userMap service_preferences;
	// a vector containing the email recipients; may move this into the actual classes
	// as opposed to being in this struct
	vector<string> email_recipients;
};

typedef struct MessageOptions optionsInfo;

class Logger
{
public:
	// Logger is a singleton, this gets an instance of the logger
	static Logger* Inst();

	~Logger();

	// SaveLoggerConfiguration: will save LoggerConfiguration to the given filename.
	// No use for this right now, a placeholder for future gui applications. Will
	// definitely change.
	void SaveLoggerConfiguration(string filename);

	// This is the hub method that will take in data from the processes,
	// use it to determine what services and levels and such need to be used, then call the private methods
	// from there
	void Log(Nova::Levels messageLevel, string messageBasic, string messageAdv = "");

	// methods for assigning the log preferences from different places
	// into the user map inside MessageOptions struct.
	// args: 	string logPrefString: this method is used for reading from the config file
	// args: 	Nova::Levels messageLevel, Nova::Services services: this is for individual
	//       	adjustments to the struct from the GUI; may change the name but as they
	//		 	are both used to set the preferences I figured it'd be okay.
	//       	Given Nova::Levels level will have have Nova::Services services mapped to it inside
	//       	the inModuleSettings userMap, and the new map will be returned.
	void setUserLogPreferences(string logPrefString);
	void setUserLogPreferences(Nova::Levels messageLevel, Nova::Services services);

protected:
	// Constructor for the Logger class.
	Logger();

private:
	// Notify makes use of the libnotify methods to produce
	// a notification with the desired level to the desktop.
	// args: 	uint16_t level. The level of severity to print in the
	//       	libNotify message.
	//       	string message. The content of the libNotify message
	void Notify(uint16_t level, string message);

	// Log will be the method that calls syslog
	// args: 	uint16_t level. The level of severity to tell syslog to log with.
	//       	string message. The message to send to syslog in string form.
	void LogToFile(uint16_t level, string message);
	// Mail will, obviously, email people.
	// args: 	uint16_t level. The level of severity with which to apply
	// 		 	when sending the email. Used primarily to extract a string from the
	// 		 	levels map.
	//		 	string message. This will be the string that is sent as the
	// 		 	email's content.
	//       	vector<string> recipients. Who the email will be sent to.
	void Mail(uint16_t level, string message);


	// takes in a character, and returns a Services type; for use when
	// parsing the SERVICE_PREFERENCES string from the NOVAConfig.txt file.
	Nova::Levels parseLevelFromChar(char parse);

	// Load Configuration: loads the SMTP info and service level preferences
	uint16_t LoadConfiguration();
	string getBitmask(Nova::Levels level);

public:
	levelsMap m_levels;

private:
	static const string m_prefixes[];
	optionsInfo m_messageInfo;
	pthread_rwlock_t m_logLock;
	static Logger *m_loggerInstance;
};

}

#endif /* Logger_H_ */
