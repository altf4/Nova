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
// Description : Class to generate messages based on events inside the program,
// and maintain information needed for the sending of those events, mostly
// networking information that is not readily available
//============================================================================/*

#include "Logger.h"


using namespace std;

namespace Nova
{

	const string Logger::prefixes[] = { "SMTP_ADDR", "SMTP_PORT", "SMTP_DOMAIN", "RECIPIENTS", "SERVICE_PREFERENCES" };

// Loads the configuration file into the class's state data
	uint16_t Logger::LoadConfiguration(char const* configFilePath)
	{
		string line;
		string prefix;
		uint16_t prefixIndex;
		string checkLoad[5];

		ifstream config(configFilePath);

		//populate the checkLoad array. I know it looks a little messy, maybe
		//hard code it somewhere? Just did this so that if we add or remove stuff,
		//we only have to do it here
		for(uint16_t j = 0; j < sizeof(prefixes)/sizeof(prefixes[0]); j++)
		{
			string def;

			switch(j)
			{
				case 0:
				case 1:
				case 2:
				case 3: def = "NO_DEFAULT";
						break;
				case 4: def = "0";
						break;
				default: break;
			}

			checkLoad[j] = def;
		}

		if(config.is_open())
		{
			while(config.good())
			{
				getline(config, line);

				prefixIndex = 0;
				prefix = prefixes[prefixIndex];

				// SMTP_ADDR
				if(!line.substr(0, prefix.size()).compare(prefix))
				{
					line = line.substr(prefix.size() + 1, line.size());

					if(line.size() > 0)
					{
						messageInfo.smtp_addr = line;
						checkLoad[prefixIndex] = line;
					}

					continue;
				}

				prefixIndex++;
				prefix = prefixes[prefixIndex];

				//SMTP_PORT
				if(!line.substr(0, prefix.size()).compare(prefix))
				{
					line = line.substr(prefix.size() + 1, line.size());

					if(line.size() > 0)
					{
						messageInfo.smtp_port = ((in_port_t) atoi(line.c_str()));
						checkLoad[prefixIndex] = line;
					}

					continue;
				}

				prefixIndex++;
				prefix = prefixes[prefixIndex];

				//SMTP_DOMAIN
				if(!line.substr(0, prefix.size()).compare(prefix))
				{
					line = line.substr(prefix.size() + 1, line.size());

					if(line.size() > 0)
					{
						messageInfo.smtp_domain = line;
						checkLoad[prefixIndex] = line;
					}

					continue;
				}

				prefixIndex++;
				prefix = prefixes[prefixIndex];
				vector<string> vec;
				//RECIPIENTS
				if(!line.substr(0, prefix.size()).compare(prefix))
				{
					line = line.substr(prefix.size() + 1, line.size());

					if(line.size() > 0)
					{
						messageInfo.email_recipients = Logger::ParseAddressesString(line);
						checkLoad[prefixIndex] = line;
					}

					continue;
				}

				prefixIndex++;
				prefix = prefixes[prefixIndex];

				//SERVICE_PREFERENCES
				//TODO: make method for parsing string to map criticality level to service
				if(!line.substr(0, prefix.size()).compare(prefix))
				{
					line = line.substr(prefix.size() + 1, line.size());

					if(line.size() > 0)
					{
						Logger::setUserLogPreferences(line);
						checkLoad[prefixIndex] = line;
					}

					continue;
				}
			}
		}
		else
		{
			syslog(SYSL_INFO, "Line: %d No configuration file found.", __LINE__);
		}

		for(uint16_t i = 0; i < sizeof(checkLoad)/sizeof(checkLoad[0]); i++)
		{
			if(!checkLoad[i].compare("NO_DEFAULT"))
			{
				syslog(SYSL_ERR, "Line: %d Some of the Nova messaging options were not configured, or not present.", __LINE__);
				return 0;
			}
		}

		closelog();

		return 1;
	}

	vector<string> Logger::ParseAddressesString(string addresses)
	{
		 vector<string> returnAddresses;
		 istringstream iss(addresses);

		 copy(istream_iterator<string>(iss),
		      istream_iterator<string>(),
		      back_inserter<vector <string> >(returnAddresses));

		 for(uint16_t i = 0; i < returnAddresses.size(); i++)
		 {
			 cout << returnAddresses.at(i) << endl;
		 }

		 return returnAddresses;
	}

	void Logger::SaveLoggerConfiguration(string filename)
	{

	}

	void Logger::Logging(Nova::Levels messageLevel, string message)
	{
		//Nova::Services services = Logger::setServiceLevel(messageLevel);
		// ^^^ This is temporary, still need to go through and define setServiceLevel
		Nova::Services services = LIBNOTIFY;
		if(services == LIBNOTIFY || services == LIBNOTIFY_BELOW || services == EMAIL_LIBNOTIFY || services == EMAIL_BELOW)
		{
			Notify(messageLevel, message);
		}
		if(services == SYSLOG || services == LIBNOTIFY_BELOW || services == EMAIL_SYSLOG || services == EMAIL_BELOW)
		{
			Log(messageLevel, message);
		}
		if(services == EMAIL || services == EMAIL_SYSLOG || services == EMAIL_LIBNOTIFY || services == EMAIL_BELOW)
		{
			Mail(messageLevel, message);
		}

		if(services == NO_SERV)
		{
			openlog("Logger", OPEN_SYSL, LOG_AUTHPRIV);
			syslog(SYSL_INFO, "You are not currently opting to use any logging mechanisms.");
			closelog();
		}
	}

	void Logger::Notify(uint16_t level, string message)
	{
		NotifyNotification *note;
		notify_init("Logger");
		note = notify_notification_new((levels[level].second).c_str(), message.c_str(), "/usr/share/nova/icons/DataSoftIcon.jpg", NULL);
		notify_notification_set_timeout(note, 3000);
		notify_notification_show(note, NULL);
		g_object_unref(G_OBJECT(note));
	}

	void Logger::Log(uint16_t level, string message)
	{
		openlog("Logger", OPEN_SYSL, LOG_AUTHPRIV);
		syslog(level, "%s: %s", parentName.c_str(), message.c_str());
		closelog();
	}

	void Logger::Mail(uint16_t level, string message)
	{

	}

	vector<string> Logger::CleanAddresses(vector<string> toClean)
	{
		vector<string> out = toClean;

		for(uint16_t i = 0; i < toClean.size(); i++)
		{
			uint16_t endSubStr = toClean[i].find(",", 0);

			if(endSubStr != toClean[i].npos)
			{
				out[i] = toClean[i].substr(0, endSubStr - 1);
			}
		}

		for(uint16_t i = 0; i < out.size(); i++)
		{
			cout << out.at(i) << endl;
		}

		return out;
	}

	void Logger::setUserLogPreferences(string logPrefString)
	{

	}

	void Logger::setUserLogPreferences(Nova::Levels messageTypeLevel, Nova::Services services)
	{

	}

	Logger::Logger(string parent, char const * configFilePath, bool init)
	{
		parentName = parent;

		for(uint16_t i = 0; i < 8; i++)
		{
			string level = "";
			switch(i)
			{
				case 0: level = "EMERGENCY";
						break;
				case 1: level = "ALERT";
						break;
				case 2: level = "CRITICAL";
						break;
				case 3: level = "ERROR";
						break;
				case 4: level = "WARNING";
						break;
				case 5: level = "NOTICE";
						break;
				case 6: level = "INFO";
						break;
				case 7: level = "DEBUG";
						break;
				default:break;
			}

			levels.push_back(pair<uint16_t, string> (i, level));
		}

		if(init)
		{
			if(!LoadConfiguration(configFilePath))
			{
				syslog(SYSL_ERR, "Line: %d One or more of the messaging configuration values have not been set, and have no default.", __LINE__);
				exit(1);
			}
		}
	}

	Logger::~Logger()
	{
	}

}

