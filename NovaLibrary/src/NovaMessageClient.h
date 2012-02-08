//============================================================================
// Name        : NovaMessageClient.h
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

#ifndef NOVAMESSAGECLIENT_H_
#define NOVAMESSAGECLIENT_H_


#include "NovaUtil.h"
//#include <libnotify>

// may not need this, trying to determine if this is a good way to
// tell the class how to allocate service use or if using the enum
// below would be better
enum {NO_SERV, SYSLOG, LIBNOTIFY, LIBNOTIFY_BELOW, EMAIL, EMAIL_SYSLOG, EMAIL_LIBNOTIFY, EMAIL_BELOW};
// enum for NovaMessaging to use
enum {EMERGENCY, ALERT, CRITICAL, ERROR, WARNING, NOTICE, INFO, DEBUG};

using namespace std;

namespace Nova {

struct MessageOptions
{
	// the SMTP server domain name for display purposes
	string smtp_domain;
	// the actual SMTP server address for socket use
	in_addr_t smtp_addr;
	// the port for SMTP send; normally 25 if I'm not mistaken, may take this out
	in_port_t smtp_port;
	// tried to find a smaller integer to use; essentially going to be a bitmask for
	// determining what services to use. So 1 would be the user just using syslog,
	// 4 would be just email, 5 email and syslog, et cetera.
	uint16_t service_preferences;
	// a vector containing the email recipients; may move this into the actual classes
	// as opposed to being in this struct
	vector<string> email_recipients;
};

typedef struct MessageOptions optionsInfo;

class NovaMessageClient {
public:
	NovaMessageClient(string parent);
	~NovaMessageClient();
	//void ConfigureMessaging();
	uint16_t LoadConfiguration(char const* filename);
	void saveMessageConfiguration(string filename);
	vector<string> parseAddressesString(string addresses);
	// This is the hub method that will take in data from the processes,
	// use it to determine what services and levels and such need to be used, then call the private methods
	// from there
	void NovaMessaging(int messageLevel, string syslog_facility, uint16_t services, vector<string> recipients);

private:
	// NovaNotify makes use of the libnotify methods to produce
	// a notification with the desired level to the desktop.
	void NovaNotify(int level, string message);
	// NovaLog will be the method that calls syslog
	void NovaLog(int level, string facility, string message);
	// NovaMail will, obviously, email people.
	void NovaMail(int level, string message, vector<string> recipients);

public:
	optionsInfo messageInfo;
private:
	static const string prefixes[];
	string checkLoad[];
	string parentName;
};

}

#endif /* NOVAMESSAGECLIENT_H_ */
