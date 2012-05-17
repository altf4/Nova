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
//============================================================================

#include "Logger.h"
#include "Config.h"
#include "Lock.h"

#include <fstream>
#include <sstream>
#include <syslog.h>
#include <string.h>
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
		m_messageInfo.smtp_addr = Config::Inst()->GetSMTPAddr();
		m_messageInfo.smtp_port = Config::Inst()->GetSMTPPort();
		m_messageInfo.smtp_domain = Config::Inst()->GetSMTPDomain();
		m_messageInfo.m_email_recipients = Config::Inst()->GetSMTPEmailRecipients();
		SetUserLogPreferences(Config::Inst()->GetLoggerPreferences());

		return 1;
	}

	void Logger::Log(Nova::Levels messageLevel, const char* messageBasic,  const char* messageAdv,
			const char* file,  const int& line)
	{
		Lock lock(&m_logLock, false);
		string mask = getBitmask(messageLevel);
		string tempStr = (string)messageAdv;
		stringstream ss;
		ss << "File " << file << " at line " << line << ": ";
		// No advanced message? Log the same thing to both
		if(tempStr == "")
		{
			ss << (string)messageBasic;
		}
		else
		{
			ss << (string)messageAdv;
		}
		tempStr = messageBasic;

		if(mask.at(LIBNOTIFY) == '1')
		{
			Notify(messageLevel, tempStr);
		}

		if(mask.at(SYSLOG) == '1')
		{
			LogToFile(messageLevel, ss.str());
		}

		if(mask.at(EMAIL) == '1')
		{
			Mail(messageLevel, tempStr);
		}
	}

	void Logger::Notify(uint16_t level, string message)
	{
		NotifyNotification *note;
		string notifyHeader = m_levels[level].second;
		notify_init("Nova");
		#ifdef NOTIFY_CHECK_VERSION
		#if NOTIFY_CHECK_VERSION (0, 7, 0)
		note = notify_notification_new(notifyHeader.c_str(), message.c_str(), Config::Inst()->GetPathIcon().c_str());
		#else
		note = notify_notification_new(notifyHeader.c_str(), message.c_str(), Config::Inst()->GetPathIcon().c_str(), NULL);
		#endif
		#else
		note = notify_notification_new(notifyHeader.c_str(), message.c_str(), Config::Inst()->GetPathIcon().c_str(), NULL);
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
		openlog("NovaMail", OPEN_SYSL, LOG_AUTHPRIV);
		syslog(level, "%s %s", (m_levels[level].second).c_str(), message.c_str());
		closelog();
	}

	void Logger::SetUserLogPreferences(string logPrefString)
	{
		uint16_t size = logPrefString.size() + 1;
		char *tokens;
		char *parse;
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
			m_messageInfo.m_service_preferences.push_back(push);
			parse = strtok(NULL, ";");
			j++;
		}

		delete parse;
		delete[] tokens;
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

	void Logger::SetUserLogPreferences(Nova::Services services, Nova::Levels messageTypeLevel, char upDown)
	{
		char *tokens;
		char *parse;
		uint16_t j = 0;
		pair <pair <Nova::Services, Nova::Levels>, char> push;
		pair <Nova::Services, Nova::Levels> insert;
		char logPref[16];

		strcpy(logPref, Config::Inst()->GetLoggerPreferences().c_str());

		//If we didn't get a null string from the above statement,
		// continue with the parsing.
		if(strlen(logPref) > 0)
		{
			//This for-loop will traverse through the string searching for the
			// character numeric representation of the services enum member passed
			// as an argument to the function.
			for(uint16_t i = 0; i < strlen(logPref); i += 4)
			{
				//If it finds it...
				if(logPref[i] == ';')
				{
					i++;
				}
				if(logPref[i] == (char)(services + '0'))
				{
					//It replaces the pair's constituent message level with the messageTypeLevel
					// argument that was passed.
					logPref[i + 2] = (char)(messageTypeLevel + '0');

					//Now we have to deal with some formatting issues:
					//If a change to the current range modifier
					// is requested, and there is a '+' or '-' at
					// the requisite place in the string, replace it and
					// move on the the next pair
					if(upDown != '0' and logPref[i + 3] != ';')
					{
						logPref[i + 3] = upDown;
						i++;
					}
					//Else, if there's a change requested and there's currently no
					// range modifier, shift everything to the right one (and out
					// of the range modifers spot) and place the range modifier into the
					// character array
					else if(upDown != '0' and logPref[i + 3] == ';')
					{

						char temp[16];
						strcpy(temp, logPref);
						logPref[i + 3] = upDown;

						for(int j = i + 4; j < 16; j++)
						{
							logPref[j] = temp[j - 1];
						}

						i++;
					}
					//If nullification was requested, and there's a range modifier, remove it,
					// shift everything to the left and move on
					else if(upDown == '0' and logPref[i + 3] != ';')
					{
						char temp[16];
						strcpy(temp, logPref);
						logPref[i + 3] = ';';

						for(int j = i + 4; j < 16; j++)
						{
							logPref[j] = temp[j + 1];
						}

						i++;
					}
					//Else if there's a 0 and no range modifer, do nothing.
					else if(upDown == '0' and logPref[i + 3] == ';')
					{
						continue;
					}
				}
			}

			//Set the Config class instance's logger preference string to the new one
			Config::Inst()->SetLoggerPreferences(std::string(logPref));

			tokens = new char[strlen(logPref) + 1];
			strcpy(tokens, logPref);

			parse = strtok(tokens, ";");
			m_messageInfo.m_service_preferences.clear();

			//Parsing to update the m_messageInfo struct used in the logger class
			// to dynamically determine what services are called for for a given log
			// message's level
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
				m_messageInfo.m_service_preferences.push_back(push);
				parse = strtok(NULL, ";");
				j++;
			}

			delete tokens;
		}
		else
		{
			//log preference string in Config is null, log error and kick out of function
			LOG(WARNING, "Unable to set new user preferences.",
			"Unable to change user log preferences, due to NULL in Config class; check that the Config file is formatted correctly");
		}
	}

	string Logger::getBitmask(Nova::Levels level)
	{
		string mask = "";
		char upDown = '0';

		for(uint16_t i = 0; i < m_messageInfo.m_service_preferences.size(); i++)
		{
			upDown = m_messageInfo.m_service_preferences[i].second;

			if(m_messageInfo.m_service_preferences[i].first.first == SYSLOG)
			{
				if(m_messageInfo.m_service_preferences[i].first.second == level)
				{
					mask.append("1");
				}
				else if(m_messageInfo.m_service_preferences[i].first.second < level && upDown == '+')
				{
					mask.append("1");
				}
				else if(m_messageInfo.m_service_preferences[i].first.second > level && upDown == '-')
				{
					mask.append("1");
				}
				else
				{
					mask.append("0");
				}
			}
			else if(m_messageInfo.m_service_preferences[i].first.first == LIBNOTIFY)
			{
				if(m_messageInfo.m_service_preferences[i].first.second == level)
				{
					mask.append("1");
				}
				else if(m_messageInfo.m_service_preferences[i].first.second < level && upDown == '+')
				{
					mask.append("1");
				}
				else if(m_messageInfo.m_service_preferences[i].first.second > level && upDown == '-')
				{
					mask.append("1");
				}
				else
				{
					mask.append("0");
				}
			}
			else if(m_messageInfo.m_service_preferences[i].first.first == EMAIL)
			{
				if(m_messageInfo.m_service_preferences[i].first.second == level)
				{
					mask.append("1");
				}
				else if(m_messageInfo.m_service_preferences[i].first.second < level && upDown == '+')
				{
					mask.append("1");
				}
				else if(m_messageInfo.m_service_preferences[i].first.second > level && upDown == '-')
				{
					mask.append("1");
				}
				else
				{
					mask.append("0");
				}
			}
		}
		return mask;
	}

	void Logger::SetEmailRecipients(std::vector<std::string> recs)
	{
		fstream recipients("/usr/share/nova/emails", ios::in | ios::out | ios::trunc);

		for(uint16_t i = 0; i < recs.size(); i++)
		{
			recipients.write(recs[i].c_str(), recs[i].size());
			recipients.put('\n');
		}

		recipients.close();
	}

	void Logger::AppendEmailRecipients(std::vector<std::string> recs)
	{
		fstream repeat("/usr/share/nova/emails", ios::in | ios::out);
		std::vector<std::string> check;

		while(repeat.good())
		{
			std::string temp;
			getline(repeat, temp);
			check.push_back(temp);
		}

		repeat.close();

		for(uint16_t i = 0; i < check.size(); i++)
		{
				for(uint16_t j = 0; j < recs.size(); j++)
				{
					if(check[i].compare(recs[j]) == 0)
					{
						recs.erase(recs.begin() + j);
					}
				}
		}

		fstream recipients("/usr/share/nova/emails", ios::in | ios::out | ios::app);

		if(!recipients.fail())
		{
			for(uint16_t i = 0; i < recs.size(); i++)
			{
				recipients.write(recs[i].c_str(), recs[i].size());
				recipients.put('\n');
			}
		}

		recipients.close();
	}

	/*void Logger::ModifyEmailRecipients(std::vector<std::string> remove, std::vector<std::string> append)
	{
		// I'll do this one later, the functionality is more of a luxury than a necessity anyways
	}*/

	void Logger::RemoveEmailRecipients(std::vector<std::string> recs)
	{
		//check through the vector; if an email is in the vector and present in the file,
		// do not rewrite to the new emails file. If and email isn't there, don't do anything.
		// If an email in the vector doesn't correspond to any email in the file, do nothing.
		fstream recipients("/usr/share/nova/emails", ios::in | ios::out);
		std::vector<std::string> check;
		bool print = true;

		while(recipients.good())
		{
			std::string temp;
			getline(recipients, temp);
			check.push_back(temp);
		}

		recipients.close();

		fstream writer("/usr/share/nova/emails", ios::in | ios::out | ios::trunc);

		for(uint16_t i = 0; i < check.size(); i++)
		{
			print = true;

			for(uint16_t j = 0; j < recs.size(); j++)
			{
				if(check[i].compare(recs[j]) == 0)
				{
					print = false;
				}
			}

			if(print)
			{
				writer.write(check[i].c_str(), check[i].size());
				writer.put('\n');
			}
		}

		writer.close();
	}

	void Logger::ClearEmailRecipients()
	{
		//open the file with ios::trunc and then close it
		fstream recipients("/usr/share/nova/emails", ios::in | ios::out | ios::trunc);
		recipients.close();
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
		delete m_loggerInstance;
	}

}

