//============================================================================
// Name        : NOVAConfiguration.h
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Class to load and parse the NOVA configuration file
//============================================================================/*

#include "NOVAConfiguration.h"
#include <fstream>
#include <iostream>
#include <string.h>
using namespace log4cxx;
using namespace log4cxx::xml;
using namespace std;

namespace Nova
{

// Loads the configuration file into the class's state data
void NOVAConfiguration::LoadConfig(char* configFilePath, string homeNovaPath)
{
	string line;
	string prefix;
	int prefixIndex;
	LoggerPtr m_logger(Logger::getLogger("main"));

	cout << "Loading file " << configFilePath << " in homepath " << homeNovaPath << endl;

	ifstream config(configFilePath);

	const string prefixes[] =
	{ "INTERFACE", "HS_HONEYD_CONFIG", "TCP_TIMEOUT", "TCP_CHECK_FREQ",
			"READ_PCAP", "PCAP_FILE", "GO_TO_LIVE", "USE_TERMINALS",
			"CLASSIFICATION_TIMEOUT", "SILENT_ALARM_PORT",
			"K", "EPS", "IS_TRAINING", "CLASSIFICATION_THRESHOLD", "DATAFILE",
			"SA_MAX_ATTEMPTS", "SA_SLEEP_DURATION", "DM_HONEYD_CONFIG",
			"DOPPELGANGER_IP", "DM_ENABLED", "ENABLED_FEATURES" };

	// Populate the options map
	for (uint i = 0; i < sizeof(prefixes)/sizeof(prefixes[0]); i++)
	{
		NovaOption * currentOption = new NovaOption();
		currentOption->isValid = false;
		options[prefixes[i]] = *currentOption;
	}

	if (config.is_open())
	{
		while (config.good())
		{
			getline(config, line);
			prefixIndex = 0;
			prefix = prefixes[prefixIndex];


			// INTERFACE
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (line.size() > 0)
				{
					// Try and detect a default interface by checking what the default route in the IP kernel's IP table is
					if (strcmp(line.c_str(), "default") == 0)
					{

						FILE * out = popen("netstat -rn", "r");
						if (out != NULL)
						{
							char buffer[2048];
							char * column;
							int currentColumn;
							char * line = fgets(buffer, sizeof(buffer), out);

							while (line != NULL)
							{
								currentColumn = 0;
								column = strtok(line, " \t\n");

								// Wait until we have the default route entry, ignore other entries
								if (strcmp(column, "0.0.0.0") == 0)
								{
									while (column != NULL)
									{
										// Get the column that has the interface name
										if (currentColumn == 7)
										{

											options[prefix].data = column;
											// LOG4CXX_INFO(m_logger, "Interface Device defaulted to '" + dev + "'");
										}

										column = strtok(NULL, " \t\n");
										currentColumn++;
									}
								}

								line = fgets(buffer, sizeof(buffer), out);

							}
						}
						pclose(out);
					}
					else
						options[prefix].data = line;

					if (!options[prefix].data.empty())
						options[prefix].isValid = true;
				}
				continue;
			}

			// HS_HONEYD_CONFIG
			prefixIndex++;
			prefix = prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (line.size() > 0)
				{
					options[prefix].data = homeNovaPath + "/" + line;
					options[prefix].isValid = true;
				}
				continue;
			}

			// TCP_TIMEOUT
			prefixIndex++;
			prefix = prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (atoi(line.c_str()) > 0)
				{
					options[prefix].data = line.c_str();
					options[prefix].isValid = true;
				}
				continue;
			}

			// TCP_CHECK_FREQ
			prefixIndex++;
			prefix = prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (atoi(line.c_str()) > 0)
				{
					options[prefix].data = line.c_str();
					options[prefix].isValid = true;
				}
				continue;
			}

			// READ_PCAP
			prefixIndex++;
			prefix = prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (atoi(line.c_str()) == 0 || atoi(line.c_str()) == 1)
				{
					options[prefix].data = line.c_str();
					options[prefix].isValid = true;
				}
				continue;
			}

			// PCAP_FILE
			prefixIndex++;
			prefix = prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (line.size() > 0)
				{
					options[prefix].data = homeNovaPath + "/" + line;
					options[prefix].isValid = true;
				}
				continue;
			}

			// GO_TO_LIVE
			prefixIndex++;
			prefix = prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (line.size() > 0)
				{
					options[prefix].data = line.c_str();
					options[prefix].isValid = true;
				}
				continue;
			}

			// USE_TERMINALS
			prefixIndex++;
			prefix = prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (atoi(line.c_str()) == 0 || atoi(line.c_str()) == 1)
				{
					options[prefix].data = line.c_str();
					options[prefix].isValid = true;
				}
				continue;
			}

			// CLASSIFICATION_TIMEOUT
			prefixIndex++;
			prefix = prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (atoi(line.c_str()) > 0)
				{
					options[prefix].data = line.c_str();
					options[prefix].isValid = true;
				}
				continue;
			}

			// SILENT_ALARM_PORT
			prefixIndex++;
			prefix = prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				// This is a quick fix for the case when there's no space; it will append the required space and
				// the default port value, and then display an INFO message for the user
				if(line.size() == prefix.size())
				{
					line += " 12024";
					LOG4CXX_INFO(m_logger, "Value for " << prefix << " was either 0 or not set. Using default port.");
				}

				line = line.substr(prefix.size() + 1, line.size());

				if (atoi(line.c_str()) > 0)
				{
					options[prefix].data = line.c_str();
					options[prefix].isValid = true;
				}
				// If the address given is 0, then just assign the default and send it through
				else if(atoi(line.c_str()) == 0)
				{
					options[prefix].data = "12024";
					LOG4CXX_INFO(m_logger, "Value for " << prefix << " was either 0 or not set. Using default port.");
					options[prefix].isValid = true;
				}
				continue;
			}

			// K
			prefixIndex++;
			prefix = prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (atoi(line.c_str()) > 0)
				{
					options[prefix].data = line.c_str();
					options[prefix].isValid = true;
				}
				continue;
			}

			// EPS
			prefixIndex++;
			prefix = prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (atof(line.c_str()) >= 0)
				{
					options[prefix].data = line.c_str();
					options[prefix].isValid = true;
				}
				continue;
			}

			// IS_TRAINING
			prefixIndex++;
			prefix = prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (atoi(line.c_str()) == 0 || atoi(line.c_str()) == 1)
				{
					options[prefix].data = line.c_str();
					options[prefix].isValid = true;
				}
				continue;
			}

			// CLASSIFICATION_THRESHOLD
			prefixIndex++;
			prefix = prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (atof(line.c_str()) >= 0)
				{
					options[prefix].data = line.c_str();
					options[prefix].isValid = true;
				}
				continue;
			}

			// DATAFILE
			prefixIndex++;
			prefix = prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (line.size() > 0 && !line.substr(line.size() - 4,
						line.size()).compare(".txt"))
				{
					options[prefix].data = line.c_str();
					options[prefix].isValid = true;
				}
				continue;
			}

			// SA_MAX_ATTEMPTS
			prefixIndex++;
			prefix = prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (atoi(line.c_str()) > 0)
				{
					options[prefix].data = line.c_str();
					options[prefix].isValid = true;
				}
				continue;
			}

			// SA_SLEEP_DURATION
			prefixIndex++;
			prefix = prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (atof(line.c_str()) >= 0)
				{
					options[prefix].data = line.c_str();
					options[prefix].isValid = true;
				}
				continue;
			}

			// DM_HONEYD_CONFIG
			prefixIndex++;
			prefix = prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (line.size() > 0)
				{
					options[prefix].data = homeNovaPath + "/" + line;
					options[prefix].isValid = true;
				}
				continue;
			}

			// DOPPELGANGER_IP
			prefixIndex++;
			prefix = prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (line.size() > 0)
				{
					options[prefix].data = line.c_str();
					options[prefix].isValid = true;
				}
				continue;
			}

			// DM_ENABLED
			prefixIndex++;
			prefix = prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (atoi(line.c_str()) == 0 || atoi(line.c_str()) == 1)
				{
					options[prefix].data = line.c_str();
					options[prefix].isValid = true;
				}
				continue;

			}

			// ENABLED_FEATURES
			prefixIndex++;
			prefix = prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (line.size() == DIM)
				{
					options[prefix].data = line.c_str();
					options[prefix].isValid = true;
				}
				continue;

			}

		}
	}
	else
	{
		// TODO: Change this to use the logger. Need to figure out the home path
		// to get the logger set up, so putting this off until the that's moved to a utility class.
		cout << "No configuration file found" << endl;
		//LOG4CXX_INFO(m_logger, "No configuration file detected.");
	}
}

NOVAConfiguration::NOVAConfiguration()
{
	options.set_empty_key("0");
}

NOVAConfiguration::~NOVAConfiguration()
{
}

}

