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
#include <fstream>
#include <sstream>
#include <syslog.h>
#include <libnotify/notify.h>

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

		openlog("Logger", OPEN_SYSL, LOG_AUTHPRIV);

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

		 returnAddresses = Logger::CleanAddresses(returnAddresses);

		 return returnAddresses;
	}

	void Logger::SaveLoggerConfiguration(string filename)
	{

	}

	void Logger::Logging(string processName, Nova::Levels messageLevel, string message)
	{
		pthread_rwlock_wrlock(&logLock);

		string mask = getBitmask(messageLevel);

		if(mask.at(0) == '1')
		{
			Notify(processName, messageLevel, message);
		}

		if(mask.at(1) == '1')
		{
			Log(processName, messageLevel, message);
		}

		if(mask.at(2) == '1')
		{
			Mail(processName, messageLevel, message);
		}

		pthread_rwlock_unlock(&logLock);
	}

	void Logger::Notify(string processName, uint16_t level, string message)
	{
		NotifyNotification *note;
		string notifyHeader = processName + ": " + levels[level].second;
		notify_init("Nova");
		#ifdef NOTIFY_CHECK_VERSION
		#if NOTIFY_CHECK_VERSION (0, 7, 0)
		note = notify_notification_new(notifyHeader.c_str(), message.c_str(), "/usr/share/nova/icons/DataSoftIcon.jpg");
		#else
		note = notify_notification_new(notifyHeader.c_str(), message.c_str(), "/usr/share/nova/icons/DataSoftIcon.jpg", NULL);
		#endif
		#else
		note = notify_notification_new(notifyHeader.c_str(), message.c_str(), "/usr/share/nova/icons/DataSoftIcon.jpg", NULL);
		#endif
		notify_notification_set_timeout(note, 3000);
		notify_notification_show(note, NULL);
		g_object_unref(G_OBJECT(note));
	}

	void Logger::Log(string processName, uint16_t level, string message)
	{
		openlog("Nova", OPEN_SYSL, LOG_AUTHPRIV);
		syslog(level, "%s: %s %s", processName.c_str(), (levels[level].second).c_str(), message.c_str());
		closelog();
	}

	void Logger::Mail(string processName, uint16_t level, string message)
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
				out[i] = toClean[i].substr(0, endSubStr);
			}
		}

		return out;
	}

	void Logger::setUserLogPreferences(string logPrefString)
	{
		uint16_t size = logPrefString.size() + 1;
		char * tokens;
		char * parse;
		uint16_t j = 0;
		pair <pair <Nova::Services, Nova::Levels>, char> push;
		pair <Nova::Services, Nova::Levels> insert;
		char upDown;

		tokens = new char[size];
		strcpy(tokens, logPrefString.c_str());

		parse = strtok(tokens, ";");

		while(parse != NULL)
		{
			switch(parse[0])
			{
				case '0': insert.first = SYSLOG;
						insert.second = parseLevelFromChar(parse[2]);
						break;
				case '1': insert.first = LIBNOTIFY;
						insert.second = parseLevelFromChar(parse[2]);
						break;
				case '2': insert.first = EMAIL;
						insert.second = parseLevelFromChar(parse[2]);
						break;
			}
			if(parse[3] == '-')
			{
				upDown = '-';
			}
			else if(parse[3] == '+')
			{
				upDown = '+';
			}
			else
			{
				upDown = '0';
			}
			push.first = insert;
			push.second = upDown;
			messageInfo.service_preferences.push_back(push);
			parse = strtok(NULL, ";");
			j++;
		}
	}

	Nova::Levels Logger::parseLevelFromChar(char parse)
	{
		switch((int)(parse - 48))
		{
			case 0: return DEBUG;
			case 1: return INFO;
			case 2: return NOTICE;
			case 3: return WARNING;
			case 4: return ERROR;
			case 5: return CRITICAL;
			case 6: return ALERT;
			case 7: return EMERGENCY;
			default: return DEBUG;
		}
		return DEBUG;
	}

	/*void Logger::setUserLogPreferences(Nova::Levels messageTypeLevel, Nova::Services services)
	{
		userMap output = messageInfo.service_preferences;
		bool end = false;

		for(uint16_t i = 0; i < messageInfo.service_preferences.size() && !end; i++)
		{
			if(messageInfo.service_preferences[i].first == messageTypeLevel)
			{
				messageInfo.service_preferences[i].second = services;
				end = true;
			}
		}
	}*/

	string Logger::getBitmask(Nova::Levels level)
	{
		string mask = "";
		char upDown;

		for(uint16_t i = 0; i < messageInfo.service_preferences.size(); i++)
		{
			upDown = messageInfo.service_preferences[i].second;

			if(messageInfo.service_preferences[i].first.first == SYSLOG)
			{
				if(messageInfo.service_preferences[i].first.second == level)
				{
					mask += "1";
				}
				else if(messageInfo.service_preferences[i].first.second < level && upDown == '+')
				{
					mask += "1";
				}
				else if(messageInfo.service_preferences[i].first.second > level && upDown == '-')
				{
					mask += "1";
				}
				else
				{
					mask += "0";
				}
			}
			else if(messageInfo.service_preferences[i].first.first == LIBNOTIFY)
			{
				if(messageInfo.service_preferences[i].first.second == level)
				{
					mask += "1";
				}
				else if(messageInfo.service_preferences[i].first.second < level && upDown == '+')
				{
					mask += "1";
				}
				else if(messageInfo.service_preferences[i].first.second > level && upDown == '-')
				{
					mask += "1";
				}
				else
				{
					mask += "0";
				}
			}
			else if(messageInfo.service_preferences[i].first.first == EMAIL)
			{
				if(messageInfo.service_preferences[i].first.second == level)
				{
					mask += "1";
				}
				else if(messageInfo.service_preferences[i].first.second < level && upDown == '+')
				{
					mask += "1";
				}
				else if(messageInfo.service_preferences[i].first.second > level && upDown == '-')
				{
					mask += "1";
				}
				else
				{
					mask += "0";
				}
			}
		}

		return mask;
	}

	Logger::Logger(char const * configFilePath, bool init)
	{
		pthread_rwlock_init(&logLock, NULL);

		for(uint16_t i = 0; i < 8; i++)
		{
			string level = "";
			switch(i)
			{
				case 0: level = "DEBUG";
						break;
				case 1: level = "INFO";
						break;
				case 2: level = "NOTICE";
						break;
				case 3: level = "WARNING";
						break;
				case 4: level = "ERROR";
						break;
				case 5: level = "CRITICAL";
						break;
				case 6: level = "ALERT";
						break;
				case 7: level = "EMERGENCY";
						break;
				default:break;
			}

			levels.push_back(pair<uint16_t, string> (i, level));
		}

		if(init)
		{
			if(!LoadConfiguration(configFilePath))
			{
				openlog("Logger", OPEN_SYSL, LOG_AUTHPRIV);
				syslog(SYSL_ERR, "Line: %d One or more of the messaging configuration values have not been set, and have no default.", __LINE__);
				exit(EXIT_FAILURE);
			}
		}
	}

	Logger::~Logger()
	{
	}

}

