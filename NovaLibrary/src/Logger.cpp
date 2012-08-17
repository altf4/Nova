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

#include <ctime>
#include <vector>
#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <syslog.h>
#include <string.h>
#include <curl/curl.h>
#include <libnotify/notify.h>

using namespace std;

namespace Nova
{
	Logger *Logger::m_loggerInstance = NULL;

	Logger *Logger::Inst()
	{
		if(m_loggerInstance == NULL)
			m_loggerInstance = new Logger();
		return m_loggerInstance;
	}

// Loads the configuration file into the class's state data
	uint16_t Logger::LoadConfiguration()
	{
		UpdateDateString();
		// TODO: get Getter functions and a safe way to store
		// the user and pass for the SMTP server
		m_messageInfo.smtp_user = Config::Inst()->GetSMTPUser();
		m_messageInfo.smtp_pass = Config::Inst()->GetSMTPPass();
		m_messageInfo.smtp_addr = Config::Inst()->GetSMTPAddr();
		m_messageInfo.smtp_port = Config::Inst()->GetSMTPPort();
		m_messageInfo.smtp_domain = Config::Inst()->GetSMTPDomain();
		m_messageInfo.m_email_recipients = Config::Inst()->GetSMTPEmailRecipients();
		SetUserLogPreferences(Config::Inst()->GetLoggerPreferences());

		return 1;
	}

	void Logger::Log(Nova::Levels messageLevel, const char *messageBasic,  const char *messageAdv,
			const char *file,  const int& line)
	{
		Lock lock(&m_logLock, WRITE_LOCK);
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
		// Don't think this is needed. If it is, it won't compile in Fedora since it isn't defined symbol
		// g_object_unref(G_OBJECT(note));
	}

	void Logger::LogToFile(uint16_t level, string message)
	{
		openlog("Nova", OPEN_SYSL, LOG_AUTHPRIV);
		syslog(level, "%s %s", (m_levels[level].second).c_str(), message.c_str());
		closelog();
	}

	void Logger::Mail(uint16_t level, string message)
	{
		CURL *curl;
	    CURLM *mcurl;
	    int still_running = 1;
	    struct timeval mp_start;
	    struct Writer counter;
	    struct curl_slist *rcpt_list = NULL;

	    int MULTI_PERFORM_HANG_TIMEOUT = 60*1000;

	    SetMailMessage(message);

	    counter.count = 0;

	    curl_global_init(CURL_GLOBAL_DEFAULT);

	    curl = curl_easy_init();
	    if(!curl)
	    {
	    	LOG(ERROR, "Curl could not initialize for Email alert", "");
	    	return;
	    }

	    mcurl = curl_multi_init();
	    if(!mcurl)
	    {
	    	LOG(ERROR, "Multi Curl could not initialize for Email alert", "");
	    	return;
	    }

	    if(m_messageInfo.m_email_recipients.size() > 0)
	    {
	    	for(uint16_t i = 0; i < m_messageInfo.m_email_recipients.size(); i++)
			{
	    		rcpt_list = curl_slist_append(rcpt_list, ("<" + m_messageInfo.m_email_recipients[i] + ">").c_str());
			}
	    }
	    else
	    {
	    	LOG(ERROR, "An email alert was attempted with no set recipients.", "");
	    	return;
	    }

		std::stringstream ss;

		ss << m_messageInfo.smtp_port;

		std::string domain = Config::Inst()->GetSMTPDomain() + ":" + ss.str();

		ss.str("");

		curl_easy_setopt(curl, CURLOPT_URL, domain.c_str());
		curl_easy_setopt(curl, CURLOPT_USERNAME, Config::Inst()->GetSMTPUser().c_str());
		curl_easy_setopt(curl, CURLOPT_PASSWORD, Config::Inst()->GetSMTPPass().c_str());
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, ReadCallback);
		curl_easy_setopt(curl, CURLOPT_MAIL_FROM, ("<" + Config::Inst()->GetSMTPAddr() + ">").c_str());
		curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, rcpt_list);
		curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1L);
		curl_easy_setopt(curl, CURLOPT_READDATA, &counter);

		// Use this for verbose output from curl
		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);

		curl_easy_setopt(curl, CURLOPT_SSLVERSION, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_SESSIONID_CACHE, 0L);
		curl_multi_add_handle(mcurl, curl);

		mp_start = tvnow();

		curl_multi_perform(mcurl, &still_running);

		while(still_running)
		{
			struct timeval timeout;
			int rc;

			fd_set fdread;
			fd_set fdwrite;
			fd_set fdexcep;
			int maxfd = -1;

			long curl_timeo = -1;

			FD_ZERO(&fdread);
			FD_ZERO(&fdwrite);
			FD_ZERO(&fdexcep);

			timeout.tv_sec = 1;
			timeout.tv_usec = 0;

			curl_multi_timeout(mcurl, &curl_timeo);

			if(curl_timeo >= 0)
			{
				timeout.tv_sec = curl_timeo / 1000;

				if(timeout.tv_sec > 1)
				{
					timeout.tv_sec = 1;
				}
				else
				{
					timeout.tv_usec = (curl_timeo % 1000)*1000;
				}
			}

			curl_multi_fdset(mcurl, &fdread, &fdwrite, &fdexcep, &maxfd);

			rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);

			if(tvdiff(tvnow(), mp_start) > MULTI_PERFORM_HANG_TIMEOUT)
			{
				LOG(ERROR, "Email alert is hanging, are you sure your credentials for the SMTP server are correct?", "");
				return;
			}

			switch(rc)
			{
				case -1:
					std::cout << "rc == -1" << std::endl;
					break;
				case 0:
				default:
					curl_multi_perform(mcurl, &still_running);
					break;
			}
		}

	  curl_slist_free_all(rcpt_list);
	  curl_multi_remove_handle(mcurl, curl);
	  curl_multi_cleanup(mcurl);
	  curl_easy_cleanup(curl);
	  curl_global_cleanup();
	}

	std::string Logger::GenerateDateString()
	{
		time_t t = time(0);
		struct tm *now = localtime(&t);

		std::string year;
		std::string month;
		std::string day;

		std::string ret;

		std::stringstream ss;
		ss << (now->tm_year + 1900);
		year = ss.str();
		ss.str("");

		switch(now->tm_mon + 1)
		{
			case 1:
				month = "January";
				break;
			case 2:
				month = "February";
				break;
			case 3:
				month = "March";
				break;
			case 4:
				month = "April";
				break;
			case 5:
				month = "May";
				break;
			case 6:
				month = "June";
				break;
			case 7:
				month = "July";
				break;
			case 8:
				month = "August";
				break;
			case 9:
				month = "September";
				break;
			case 10:
				month = "October";
				break;
			case 11:
				month = "November";
				break;
			case 12:
				month = "December";
				break;
			default:
				month = "";
				break;
		}

		ss << now->tm_mday;
		day = ss.str();
		ss.str("");

		ret = "Date: " + day + " " + month + " " + year + "\n";

		return ret;
	}

	void Logger::UpdateDateString()
	{
		SetDateString(GenerateDateString());
	}

	void Logger::SetDateString(std::string toDate)
	{
		m_dateString = toDate;
	}

	std::string Logger::GetDateString()
	{
		return m_dateString;
	}

	std::string Logger::GetRecipient()
	{
		return "To: <" + m_messageInfo.m_email_recipients[0] + ">\n";
	}

	std::string Logger::GetMailMessage()
	{
		return m_mailMessage;
	}

	void Logger::SetMailMessage(std::string message)
	{
		m_mailMessage = message + "\n";
	}

	std::string Logger::GetSenderString()
	{
		std::string ret = "From: <" + Config::Inst()->GetSMTPAddr() + ">\n";
		return ret;
	}

	std::string Logger::GetCcString()
	{
		if(m_messageInfo.m_email_recipients.size() <= 1)
		{
			return "\n";
		}

		std::string ret = "Cc: ";

		for(uint16_t i = 1; i < m_messageInfo.m_email_recipients.size(); i++)
		{
			ret += "<" + m_messageInfo.m_email_recipients[i] + ">";

			if((uint16_t)(i + 1) < m_messageInfo.m_email_recipients.size())
			{
				ret += ", ";
			}
		}

		ret += "\n";
		return ret;
	}

	uint16_t Logger::GetRecipientsLength()
	{
		return m_messageInfo.m_email_recipients.size();
	}

	size_t Logger::ReadCallback(void *ptr, size_t size, size_t nmemb, void * userp)
	{
		struct Writer *counter = (struct Writer *)userp;
		const char *data;

		std::string debug3 = Logger::Inst()->GetMailMessage();

		const char *text[] = {
				Logger::Inst()->GenerateDateString().c_str(),
				Logger::Inst()->GetRecipient().c_str(),
				Logger::Inst()->GetSenderString().c_str(),
				"Subject: Nova Mail Alert\n",
				Logger::Inst()->GetCcString().c_str(),
				"\n",
				Logger::Inst()->GetMailMessage().c_str(),
				"\n",
				NULL};

		if(size *nmemb < 1)
		{
			return 0;
		}
		data = text[counter->count];
		if(data)
		{
			size_t len = strlen(data);
			memcpy(ptr, data, len);
			counter->count++;
			return len;
		}
		return 0;
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
		m_messageInfo.m_email_recipients = recs;

		fstream recipients(m_pathEmailFile, ios::in | ios::out | ios::trunc);

		for(uint16_t i = 0; i < recs.size(); i++)
		{
			recipients.write(recs[i].c_str(), recs[i].size());
			recipients.put('\n');
		}

		recipients.close();
	}

	void Logger::AppendEmailRecipients(std::vector<std::string> recs)
	{
		fstream repeat(m_pathEmailFile, ios::in | ios::out);
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

		fstream recipients(m_pathEmailFile, ios::in | ios::out | ios::app);

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
		fstream recipients(m_pathEmailFile, ios::in | ios::out);
		std::vector<std::string> check;
		bool print = true;

		while(recipients.good())
		{
			std::string temp;
			getline(recipients, temp);
			check.push_back(temp);
		}

		recipients.close();

		fstream writer(m_pathEmailFile, ios::in | ios::out | ios::trunc);

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
		fstream recipients(m_pathEmailFile, ios::in | ios::out | ios::trunc);
		recipients.close();
	}

	Logger::Logger()
	{
		pthread_rwlock_init(&m_logLock, NULL);

		m_pathEmailFile = Config::Inst()->GetPathHome() + "/../emails";

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

