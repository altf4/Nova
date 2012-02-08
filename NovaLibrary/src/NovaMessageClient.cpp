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

#include "NovaMessageClient.h"

using namespace std;

namespace Nova
{

	const string NovaMessageClient::prefixes[] = { "SMTP_ADDR", "SMTP_PORT", "SMTP_DOMAIN", "RECIPIENTS", "SERVICE_PREFERENCES" };

// Loads the configuration file into the class's state data
	uint16_t NovaMessageClient::LoadConfiguration(char const* configFilePath)
	{
		string line;
		string prefix;
		uint16_t prefixIndex;

		//openlog(use.c_str(), LOG_CONS | LOG_PID | LOG_NDELAY | LOG_PERROR, LOG_AUTHPRIV);

		//syslog(SYSL_INFO, "Loading file %s in homepath %s", configFilePath, homeNovaPath.c_str());

		ifstream config(configFilePath);

		//populate the defaultVector. I know it looks a little messy, maybe
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
						messageInfo.smtp_addr = ((in_addr_t) atoi(line.c_str()));
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
					}

					continue;
				}

				prefixIndex++;
				prefix = prefixes[prefixIndex];

				//RECIPIENTS
				if(!line.substr(0, prefix.size()).compare(prefix))
				{
					line = line.substr(prefix.size() + 1, line.size());

					if(line.size() > 0)
					{
						messageInfo.email_recipients = parseAddressesString(line);
					}

					continue;
				}

				prefixIndex++;
				prefix = prefixes[prefixIndex];

				//SERVICE_PREFERENCES
				if(!line.substr(0, prefix.size()).compare(prefix))
				{
					line = line.substr(prefix.size() + 1, line.size());

					if(line.size() > 0)
					{
						messageInfo.service_preferences = ((uint16_t) atoi(line.c_str()));
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
	// TODO: use find and substring instead of trying to use strtok
	vector<string> parseAddressesString(string addresses)
	{
		 vector<string> returnAddresses;
		 char * add;
		 add = strtok(addresses, ",");

		 while(add != NULL)
		 {
			 returnAddresses.push_back(add);
			 add = strtok(NULL, ",");
		 }

		 return returnAddresses;
	}

	void saveMessageConfiguration(string filename)
	{

	}

	NovaMessageClient::NovaMessageClient(string parent)
	{
		parentName = parent;
	}

	NovaMessageClient::~NovaMessageClient()
	{
	}

}

