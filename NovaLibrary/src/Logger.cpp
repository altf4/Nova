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

		 returnAddresses = Logger::CleanAddresses(returnAddresses);

		 return returnAddresses;
	}

	void Logger::SaveLoggerConfiguration(string filename)
	{

	}

	void Logger::Logging(Nova::Levels messageLevel, userMap serv, string message, optionsInfo options)
	{
		Nova::Services services = Logger::setServiceLevel(messageLevel, serv);

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
			Mail(messageLevel, message, options);
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
		#ifdef NOTIFY_CHECK_VERSION
		#if NOTIFY_CHECK_VERSION (0, 7, 0)
		note = notify_notification_new((levels[level].second).c_str(), message.c_str(), "/usr/share/nova/icons/DataSoftIcon.jpg");
		#else
		note = notify_notification_new((levels[level].second).c_str(), message.c_str(), "/usr/share/nova/icons/DataSoftIcon.jpg", NULL);
		#endif
		#else
		note = notify_notification_new((levels[level].second).c_str(), message.c_str(), "/usr/share/nova/icons/DataSoftIcon.jpg", NULL);
		#endif
		notify_notification_set_timeout(note, 3000);
		notify_notification_show(note, NULL);
		g_object_unref(G_OBJECT(note));
	}

	void Logger::Log(uint16_t level, string message)
	{
		openlog("Logger", OPEN_SYSL, LOG_AUTHPRIV);
		syslog(level, "%s %s: %s", (levels[level].second).c_str(), parentName.c_str(), message.c_str());
		closelog();
	}

	void Logger::Mail(uint16_t level, string message, optionsInfo options)
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
		if(logPrefString.size() != 32)
		{
			openlog("Logger", OPEN_SYSL, LOG_AUTHPRIV);
			syslog(SYSL_ERR, "The service preferences line is not configured correctly; it should be of the form \"LEVEL:SERVICES;\" (without quotes); for each level, 0 through 7, and each service, 1 through 7.");
			closelog();
			exit(EXIT_FAILURE);
		}
		else
		{
			for(uint16_t i = 0; i < logPrefString.size(); i += 4)
			{
					if(isdigit(logPrefString.at(i)))
					{
						switch(logPrefString.at(i))
						{
							case '0': messageInfo.service_preferences.push_back(pair<Nova::Levels, Nova::Services>(EMERGENCY, parseServicesFromChar(logPrefString.at(i + 2))));
									break;
							case '1': messageInfo.service_preferences.push_back(pair<Nova::Levels, Nova::Services>(ALERT, parseServicesFromChar(logPrefString.at(i + 2))));
									break;
							case '2': messageInfo.service_preferences.push_back(pair<Nova::Levels, Nova::Services>(CRITICAL, parseServicesFromChar(logPrefString.at(i + 2))));
									break;
							case '3': messageInfo.service_preferences.push_back(pair<Nova::Levels, Nova::Services>(ERROR, parseServicesFromChar(logPrefString.at(i + 2))));
									break;
							case '4': messageInfo.service_preferences.push_back(pair<Nova::Levels, Nova::Services>(WARNING, parseServicesFromChar(logPrefString.at(i + 2))));
									break;
							case '5': messageInfo.service_preferences.push_back(pair<Nova::Levels, Nova::Services>(NOTICE, parseServicesFromChar(logPrefString.at(i + 2))));
									break;
							case '6': messageInfo.service_preferences.push_back(pair<Nova::Levels, Nova::Services>(INFO, parseServicesFromChar(logPrefString.at(i + 2))));
									break;
							case '7': messageInfo.service_preferences.push_back(pair<Nova::Levels, Nova::Services>(DEBUG, parseServicesFromChar(logPrefString.at(i + 2))));
									break;
						}
					}
					else
					{
						openlog("Logger", OPEN_SYSL, LOG_AUTHPRIV);
						syslog(SYSL_ERR, "The service preferences line is not configured correctly; it should be of the form \"LEVEL:SERVICES;\" (without quotes) for each level, 0 through 7, and each service, 1 through 7.");
						closelog();
						exit(EXIT_FAILURE);
					}
			}
		}
	}

	Nova::Services Logger::parseServicesFromChar(char parse)
	{
		switch((int)(parse - 48))
		{
			case 0: return NO_SERV;
					break;
			case 1: return SYSLOG;
					break;
			case 2: return LIBNOTIFY;
					break;
			case 3: return LIBNOTIFY_BELOW;
					break;
			case 4: return EMAIL;
					break;
			case 5: return EMAIL_SYSLOG;
					break;
			case 6: return EMAIL_LIBNOTIFY;
					break;
			case 7: return EMAIL_BELOW;
					break;
			default: return NO_SERV;
		}
		return NO_SERV;
	}

	userMap Logger::setUserLogPreferences(Nova::Levels messageTypeLevel, Nova::Services services, userMap inModuleSettings)
	{
		userMap output = inModuleSettings;
		bool end = false;

		for(uint16_t i = 0; i < inModuleSettings.size() && !end; i++)
		{
			if(inModuleSettings[i].first == messageTypeLevel)
			{
				output[i].second = services;
				end = true;
			}
		}

		return output;
	}

	Nova::Services Logger::setServiceLevel(Nova::Levels messageLevel, userMap serv)
	{
		for(uint16_t i = 0; i < serv.size(); i++)
		{
			if(messageLevel == serv[i].first)
			{
				return serv[i].second;
			}
		}

		return NO_SERV;
	}

	Logger::Logger(string parent, char const * configFilePath, bool init)
	{
		parentName = parent.substr(7, (parent.length() - 11));;

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
				exit(EXIT_FAILURE);
			}
		}
	}

	Logger::~Logger()
	{
	}

}

