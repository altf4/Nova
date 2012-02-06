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

// may not need this, trying to determine if this is a good way to
// tell the class how to allocate service use or if using the enum
// below would be better
enum {NO_SERV, SYSLOG, LIBNOTIFY, LIBNOTIFY_BELOW, EMAIL, EMAIL_SYSLOG, EMAIL_LIBNOTIFY, EMAIL_BELOW};
// enum for NovaMessaging to use
enum {DEBUG, LOG, ERROR, CRITICAL, ALERT, EMERGENCY};

struct MessageOptions
{
	string smtp_domain;
	in_addr_t smtp_addr;
	in_port_t port;
};

using namespace std;

namespace Nova {

class NovaMessageClient {
public:
	NovaMessageClient();
	~NovaMessageClient();
	void ConfigureMessaging();
	void LoadConfiguration(string filename);
	void saveMessageConfiguration(string filename);
	vector<string> parseAddressesString(string addresses);
	void NovaMessaging(int messageLevel, char inclusive);
	void setDefaults();

private:
	// NovaNotify makes use of the libnotify methods to produce
	// a notification with the desired level to the desktop.
	//
	void NovaNotify(int level, string message);
	void NovaLog(int level, string message);
	void NovaMail(int level, string message);

public:

private:
	uint4_t whichService;
	vector<string> email_addrs;
	string prefixes[];
};

}

#endif /* NOVAMESSAGECLIENT_H_ */
