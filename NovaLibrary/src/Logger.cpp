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
#include "Config.h"
#include <fstream>
#include <sstream>
#include <syslog.h>
#include <libnotify/notify.h>

using namespace std;

namespace Nova
{
	Logger *Logger::m_loggerInstance = NULL;

	Logger *Logger::Inst()
	{
		if (m_loggerInstance == NULL)
			m_loggerInstance = new Logger();
		return m_loggerInstance;
	}

// Loads the configuration file into the class's state data
	uint16_t Logger::LoadConfiguration()
	{
		m_messageInfo.smtp_addr = Config::Inst()->getSMTPAddr();
		m_messageInfo.smtp_port = Config::Inst()->getSMTPPort();
		m_messageInfo.smtp_domain = Config::Inst()->getSMTPDomain();
		m_messageInfo.email_recipients = Config::Inst()->getSMTPEmailRecipients();
		setUserLogPreferences(Config::Inst()->getLoggerPreferences());

		return 1;
	}

	void Logger::SaveLoggerConfiguration(string filename)
	{

	}

	void Logger::Log(Nova::Levels messageLevel, string messageBasic, string messageAdv)
	{
		pthread_rwlock_wrlock(&m_logLock);
		string mask = getBitmask(messageLevel);

		// No advanced message? Log the same thing to both
		if (messageAdv == "")
			messageAdv = messageBasic;

		if(mask.at(0) == '1')
		{
			Notify(messageLevel, messageBasic);
		}

		if(mask.at(1) == '1')
		{
			LogToFile(messageLevel, messageAdv);
		}

		if(mask.at(2) == '1')
		{
			Mail(messageLevel, messageBasic);
		}

		pthread_rwlock_unlock(&m_logLock);
	}

	void Logger::Notify(uint16_t level, string message)
	{
		NotifyNotification *note;
		string notifyHeader = m_levels[level].second;
		notify_init("Nova");
		#ifdef NOTIFY_CHECK_VERSION
		#if NOTIFY_CHECK_VERSION (0, 7, 0)
		note = notify_notification_new(notifyHeader.c_str(), message.c_str(), Config::Inst()->getPathIcon().c_str());
		#else
		note = notify_notification_new(notifyHeader.c_str(), message.c_str(), Config::Inst()->getPathIcon().c_str(), NULL);
		#endif
		#else
		note = notify_notification_new(notifyHeader.c_str(), message.c_str(), Config::Inst()->getPathIcon().c_str(), NULL);
		#endif
		notify_notification_set_timeout(note, 3000);
		notify_notification_show(note, NULL);
		g_object_unref(G_OBJECT(note));
	}

	void Logger::LogToFile(uint16_t level, string message)
	{
		openlog("Nova", OPEN_SYSL, LOG_AUTHPRIV);
		syslog(level, "%s %s", (m_levels[level].second).c_str(), message.c_str());
		closelog();
	}

	void Logger::Mail(uint16_t level, string message)
	{

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
			m_messageInfo.service_preferences.push_back(push);
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

		for(uint16_t i = 0; i < m_messageInfo.service_preferences.size(); i++)
		{
			upDown = m_messageInfo.service_preferences[i].second;

			if(m_messageInfo.service_preferences[i].first.first == SYSLOG)
			{
				if(m_messageInfo.service_preferences[i].first.second == level)
				{
					mask += "1";
				}
				else if(m_messageInfo.service_preferences[i].first.second < level && upDown == '+')
				{
					mask += "1";
				}
				else if(m_messageInfo.service_preferences[i].first.second > level && upDown == '-')
				{
					mask += "1";
				}
				else
				{
					mask += "0";
				}
			}
			else if(m_messageInfo.service_preferences[i].first.first == LIBNOTIFY)
			{
				if(m_messageInfo.service_preferences[i].first.second == level)
				{
					mask += "1";
				}
				else if(m_messageInfo.service_preferences[i].first.second < level && upDown == '+')
				{
					mask += "1";
				}
				else if(m_messageInfo.service_preferences[i].first.second > level && upDown == '-')
				{
					mask += "1";
				}
				else
				{
					mask += "0";
				}
			}
			else if(m_messageInfo.service_preferences[i].first.first == EMAIL)
			{
				if(m_messageInfo.service_preferences[i].first.second == level)
				{
					mask += "1";
				}
				else if(m_messageInfo.service_preferences[i].first.second < level && upDown == '+')
				{
					mask += "1";
				}
				else if(m_messageInfo.service_preferences[i].first.second > level && upDown == '-')
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

	Logger::Logger()
	{
		pthread_rwlock_init(&m_logLock, NULL);

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

			m_levels.push_back(pair<uint16_t, string> (i, level));
		}

		LoadConfiguration();
	}

	Logger::~Logger()
	{
	}

}

