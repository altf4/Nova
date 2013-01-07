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
//============================================================================

#ifndef Logger_H_
#define Logger_H_

#include "HashMapStructs.h"
#include "Config.h"

/// Configs for openlog (first is with terminals, second is without
#define OPEN_SYSL (LOG_CONS | LOG_PID | LOG_NDELAY | LOG_PERROR)

// A macro to make logging prettier
#define LOG(t,s,r) Nova::Logger::Inst()->Log(t, std::string(s).c_str(), std::string(r).c_str(), __FILE__ , __LINE__)

namespace Nova
{

// may not need this, trying to determine if this is a good way to
// tell the class how to allocate service use or if using the enum
// below would be better
enum Services {SYSLOG = 0, EMAIL};
// enum for NovaMessaging to use. May have to switch around the order to
// make newer scheme make sense
enum Levels {DEBUG = 0, INFO, NOTICE, WARNING, ERROR, CRITICAL, ALERT, EMERGENCY};

typedef std::vector<std::pair<uint16_t, std::string> > levelsMap;
typedef std::vector<std::pair< std::pair<Nova::Services, Nova::Levels>, char > > userMap;


//There will be more than this, most likely
struct MessageOptions
{
	// the SMTP server domain name for display purposes
	std::string smtp_domain;
	// SMTP username for SMTP secure login
	std::string smtp_user;
	// SMTP password for SMTP secure login
	std::string smtp_pass;
	// the email address that will be set as sender
	std::string smtp_addr;
	// the port for SMTP send; normally 25 if I'm not mistaken, may take this out
	in_port_t smtp_port;
	userMap m_service_preferences;

	std::vector<std::string> m_email_recipients;
};

struct Writer
{
	int count;
};

typedef struct MessageOptions optionsInfo;

class Logger
{
public:
	// Logger is a singleton, this gets an instance of the logger
	static Logger *Inst();

	~Logger();

	// This is the hub method that will take in data from the processes,
	// use it to determine what services and levels and such need to be used, then call the private methods
	// from there
	void Log(Nova::Levels messageLevel, const char *messageBasic, const char *messageAdv,
		const char *file, const int& line);

	// methods for assigning the log preferences from different places
	// into the user map inside MessageOptions struct.
	// args: 	std::string logPrefString: this method is used for reading from the Config file
	void SetUserLogPreferences(std::string logPrefString);

	// This version serves to update the preference string one service at a time.
	// args: services: service to change
	//       messageLevel: what level to change the service to
	//       upDown: a '+' indicates a range of level or higher; a '-' indicates a range of
	//               level or lower. A '0' indicates clear previous range modifiers. Thus, if you're
	//               just wanting to change the level, and not the range modifier, you have to put the
	//               range modifier that's present in the NOVAConfig.txt file as the argument.
	void SetUserLogPreferences(Nova::Services services, Nova::Levels messageLevel, char upDown = '0');

protected:
	// Constructor for the Logger class.
	Logger();

private:

	// Log will be the method that calls syslog
	// args: 	uint16_t level. The level of severity to tell syslog to log with.
	//       	std::string message. The message to send to syslog in std::string form.
	void LogToFile(uint16_t level, std::string message);

	// Mail will, obviously, email people.
	// args: 	uint16_t level. The level of severity with which to apply
	// 		 	when sending the email. Used primarily to extract a std::string from the
	// 		 	levels map.
	//		 	std::string message. This will be the std::string that is sent as the
	// 		 	email's content.
	//       	vector<std::string> recipients. Who the email will be sent to.
	void Mail(uint16_t level, std::string message);

	// takes in a character, and returns a Services type; for use when
	// parsing the SERVICE_PREFERENCES std::string from the NOVAConfig.txt file.
	Nova::Levels parseLevelFromChar(char parse);

	// Load Configuration: loads the SMTP info and service level preferences
	uint16_t LoadConfiguration();

	std::string getBitmask(Nova::Levels level);

	static size_t ReadCallback(void *ptr, size_t size, size_t nmemb, void *userp);

	static long tvdiff(struct timeval newer, struct timeval older)
	{
	  return (newer.tv_sec-older.tv_sec)*1000+(newer.tv_usec-older.tv_usec)/1000;
	};

	static struct timeval tvnow(void)
	{
	  struct timeval now;
	  now.tv_sec = (long)time(NULL);
	  now.tv_usec = 0;
	  return now;
	};

	std::string GenerateDateString();
	std::string GetRecipient();
	std::string GetMailMessage();
	std::string GetSenderString();
	std::string GetCcString();
	void SetMailMessage(std::string message);
	uint16_t GetRecipientsLength();

public:
	levelsMap m_levels;

private:
	optionsInfo m_messageInfo;
	pthread_rwlock_t m_logLock;
	static Logger *m_loggerInstance;
	std::string m_mailMessage;
};

}
#endif /* Logger_H_ */
