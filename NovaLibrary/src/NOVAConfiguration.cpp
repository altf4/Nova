//============================================================================
// Name        : NOVAConfiguration.h
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Class to load and parse the NOVA configuration file
//============================================================================/*

#include "NOVAConfiguration.h"

using namespace std;



namespace Nova
{
// Loads the configuration file into the class's state data
void NOVAConfiguration::LoadConfig(char* configFilePath, string homeNovaPath, string module)
{
	string line;
	string prefix;
	int prefixIndex;

	string use = module.substr(7, (module.length() - 11));

	openlog(use.c_str(), LOG_CONS | LOG_PID | LOG_NDELAY | LOG_PERROR, LOG_AUTHPRIV);

	syslog(SYSL_INFO, "Loading file %s in homepath %s", configFilePath, homeNovaPath.c_str());

	ifstream config(configFilePath);

	const string prefixes[] =
	{ "INTERFACE", "HS_HONEYD_CONFIG", "TCP_TIMEOUT", "TCP_CHECK_FREQ",
			"READ_PCAP", "PCAP_FILE", "GO_TO_LIVE", "USE_TERMINALS",
			"CLASSIFICATION_TIMEOUT", "SILENT_ALARM_PORT",
			"K", "EPS", "IS_TRAINING", "CLASSIFICATION_THRESHOLD", "DATAFILE",
			"SA_MAX_ATTEMPTS", "SA_SLEEP_DURATION", "DM_HONEYD_CONFIG",
			"DOPPELGANGER_IP", "DM_ENABLED", "ENABLED_FEATURES" };

	//populate the defaultVector. I know it looks a little messy, maybe
	//hard code it somewhere? Just did this so that if we add or remove stuff,
	//we only have to do it here
	for(uint j = 0; j < sizeof(prefixes)/sizeof(prefixes[0]); j++)
	{
		string def;
		switch(j)
		{
			case 0: def = "default";
					break;
			case 1: def = "Config/haystack.config";
					break;
			case 2: def = "7";
					break;
			case 3: def = "3";
					break;
			case 4: def = "0";
					break;
			case 5: def = "../pcapfile";
					break;
			case 6: def = "1";
					break;
			case 7: def = "1";
					break;
			case 8: def = "3";
					break;
			case 9: def = "12024";
					break;
			case 10: def = "3";
					break;
			case 11: def = "0.01";
					break;
			case 12: def = "0";
					break;
			case 13: def = ".5";
					break;
			case 14: def = "Data/data.txt";
					break;
			case 15: def = "3";
					break;
			case 16: def = ".5";
					break;
			case 17: def = "Config/doppelganger.config";
					break;
			case 18: def = "10.0.0.1";
					break;
			case 19: def = "1";
					break;
			case 20: def = "111111111";
					break;
			default: break;
		}
		defaults.push_back(make_pair(prefixes[j], def));
	}

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
				// allow processing to continue without a segfault
				if(line.size() == prefix.size())
				{
					line += " ";
				}

				line = line.substr(prefix.size() + 1, line.size());

				if (atoi(line.c_str()) > 0)
				{
					options[prefix].data = line.c_str();
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
		syslog(SYSL_INFO, "Line: %d No configuration file found.", __LINE__);
		//return 0;
	}
	closelog();
}


///Checks the optionsMap generated by LoadConfig for any incorrect values;
///if there are any problems, report to syslog and set option to default
int NOVAConfiguration::SetDefaults()
{
	openlog(__FUNCTION__, OPEN_SYSL, LOG_AUTHPRIV);

	int out = 0;

	const string prefixes[] =
	{ "INTERFACE", "HS_HONEYD_CONFIG", "TCP_TIMEOUT", "TCP_CHECK_FREQ",
			"READ_PCAP", "PCAP_FILE", "GO_TO_LIVE", "USE_TERMINALS",
			"CLASSIFICATION_TIMEOUT", "SILENT_ALARM_PORT",
			"K", "EPS", "IS_TRAINING", "CLASSIFICATION_THRESHOLD", "DATAFILE",
			"SA_MAX_ATTEMPTS", "SA_SLEEP_DURATION", "DM_HONEYD_CONFIG",
			"DOPPELGANGER_IP", "DM_ENABLED", "ENABLED_FEATURES" };

	for(uint i = 0; i < options.size() && out < 2; i++)
	{
		//if the option is not valid from LoadConfig, and it is an option that has a default value, assign.
		//anything that has no static default (i.e. something that has a user defined default) will pass
		//on to a given module with isValid being false, and kick out from there. The compare doesn't have to
		//be here until we have a solid foundation determining what will have static defaults and what won't,
		//but I put it in anyways to remind myself when the time comes
		if(!options[prefixes[i]].isValid && defaults[i].second.compare("No") != 0)
		{
			syslog(SYSL_INFO, "Configuration option %s was not set, present, or valid. Setting to default value %s", prefixes[i].c_str(), defaults[i].second.c_str());
			options[prefixes[i]].data = defaults[i].second;
			options[prefixes[i]].isValid = true;
			out = 1;
		}
		if(!options[prefixes[i]].isValid && !defaults[i].second.compare("No"))
		{
			syslog(SYSL_ERR, "The configuration variable %s was not set in the configuration file, and has no default.", prefixes[i].c_str());
			out = 2;
		}
	}

	closelog();
	return out;
}

NOVAConfiguration::NOVAConfiguration()
{
	options.set_empty_key("0");
}

NOVAConfiguration::~NOVAConfiguration()
{
}

}

