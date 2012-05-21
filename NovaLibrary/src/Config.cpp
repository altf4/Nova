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
// Description : Class to load and parse the NOVA configuration file
//============================================================================

#include <sys/stat.h>
#include <sys/un.h>
#include <fstream>
#include <sstream>
#include <math.h>

#include "Config.h"
#include "Logger.h"
#include "NovaUtil.h"
#include "Lock.h"

using namespace std;

namespace Nova
{

string Config::m_prefixes[] =
{
	"INTERFACE",
	"HS_HONEYD_CONFIG",
	"TCP_TIMEOUT",
	"TCP_CHECK_FREQ",
	"READ_PCAP",
	"PCAP_FILE",
	"GO_TO_LIVE",
	"CLASSIFICATION_TIMEOUT",
	"SILENT_ALARM_PORT",
	"K",
	"EPS",
	"IS_TRAINING",
	"CLASSIFICATION_THRESHOLD",
	"DATAFILE",
	"SA_MAX_ATTEMPTS",
	"SA_SLEEP_DURATION",
	"USER_HONEYD_CONFIG",
	"DOPPELGANGER_IP",
	"DOPPELGANGER_INTERFACE",
	"DM_ENABLED",
	"ENABLED_FEATURES",
	"TRAINING_CAP_FOLDER",
	"THINNING_DISTANCE",
	"SAVE_FREQUENCY",
	"DATA_TTL",
	"CE_SAVE_FILE",
	"SMTP_ADDR",
	"SMTP_PORT",
	"SMTP_DOMAIN",
	"RECIPIENTS",
	"SERVICE_PREFERENCES",
	"HAYSTACK_STORAGE",
	"WHITELIST_FILE"
};

// Files we need to run (that will be loaded with defaults if deleted)
string Config::m_requiredFiles[] =
{
	"/settings",
	"/Config",
	"/keys",
	"/templates",
	"/Config/NOVAConfig.txt",
	"/scripts.xml",
	"/templates/ports.xml",
	"/templates/profiles.xml",
	"/templates/routes.xml",
	"/templates/nodes.xml"
};

Config* Config::m_instance = NULL;

Config* Config::Inst()
{
	if(m_instance == NULL)
	{
		m_instance = new Config();
	}
	return m_instance;
}

// Loads the configuration file into the class's state data
void Config::LoadConfig()
{
	Lock lock(&m_lock, false);

	string line;
	string prefix;
	int prefixIndex;

	bool isValid[sizeof(m_prefixes)/sizeof(m_prefixes[0])];

	ifstream config(m_configFilePath.c_str());

	if(config.is_open())
	{
		while (config.good())
		{
			getline(config, line);
			prefixIndex = 0;
			prefix = m_prefixes[prefixIndex];

			// INTERFACE
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					// Try and detect a default interface by checking what the default route in the IP kernel's IP table is
					if(strcmp(line.c_str(), "default") == 0)
					{

						FILE *out = popen("netstat -rn", "r");
						if(out != NULL)
						{
							char buffer[2048];
							char *column;
							int currentColumn;
							char *line = fgets(buffer, sizeof(buffer), out);

							while (line != NULL)
							{
								currentColumn = 0;
								column = strtok(line, " \t\n");

								// Wait until we have the default route entry, ignore other entries
								if(strcmp(column, "0.0.0.0") == 0)
								{
									while (column != NULL)
									{
										// Get the column that has the interface name
										if(currentColumn == 7)
										{

											m_interface = column;
											isValid[prefixIndex] = true;
										}

										column = strtok(NULL, " \t\n");
										currentColumn++;
									}
								}

								line = fgets(buffer, sizeof(buffer), out);
							}

							delete column;
							delete line;
						}
						pclose(out);
					}
					else
					{
						m_interface = line;
					}
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// HS_HONEYD_CONFIG
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_pathConfigHoneydHs  = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// TCP_TIMEOUT
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(atoi(line.c_str()) > 0)
				{
					m_tcpTimout = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// TCP_CHECK_FREQ
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(atoi(line.c_str()) > 0)
				{
					m_tcpCheckFreq = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// READ_PCAP
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(atoi(line.c_str()) == 0 || atoi(line.c_str()) == 1)
				{
					m_readPcap = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// PCAP_FILE
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_pathPcapFile = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// GO_TO_LIVE
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(atoi(line.c_str()) >= 0)
				{
					m_gotoLive = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// CLASSIFICATION_TIMEOUT
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(atoi(line.c_str()) >= 0)
				{
					m_classificationTimeout = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// SILENT_ALARM_PORT
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				if(line.size() == prefix.size())
				{
					line += " ";
				}

				line = line.substr(prefix.size() + 1, line.size());

				if(atoi(line.c_str()) > 0)
				{
					m_saPort = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// K
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(atoi(line.c_str()) > 0)
				{
					m_k = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// EPS
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(atof(line.c_str()) >= 0)
				{
					m_eps = atof(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// IS_TRAINING
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(atoi(line.c_str()) == 0 || atoi(line.c_str()) == 1)
				{
					m_isTraining = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// CLASSIFICATION_THRESHOLD
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(atof(line.c_str()) >= 0)
				{
					m_classificationThreshold= atof(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// DATAFILE
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0 && !line.substr(line.size() - 4,
						line.size()).compare(".txt"))
				{
					m_pathTrainingFile = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// SA_MAX_ATTEMPTS
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(atoi(line.c_str()) > 0)
				{
					m_saMaxAttempts = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// SA_SLEEP_DURATION
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(atof(line.c_str()) >= 0)
				{
					m_saSleepDuration = atof(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// USER_HONEYD_CONFIG
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_pathConfigHoneydUser = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}


			// DOPPELGANGER_IP
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_doppelIp = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// DOPPELGANGER_INTERFACE
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_doppelInterface = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// DM_ENABLED
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(atoi(line.c_str()) == 0 || atoi(line.c_str()) == 1)
				{
					m_isDmEnabled = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;

			}

			// ENABLED_FEATURES
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() == DIM)
				{
					SetEnabledFeatures_noLocking(line);
					isValid[prefixIndex] = true;
				}
				continue;

			}

			// TRAINING_CAP_FOLDER
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_pathTrainingCapFolder = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// THINNING_DISTANCE
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(atof(line.c_str()) > 0)
				{
					m_thinningDistance = atof(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// SAVE_FREQUENCY
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(atoi(line.c_str()) > 0)
				{
					m_saveFreq = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// DATA_TTL
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(atoi(line.c_str()) >= 0)
				{
					m_dataTTL = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// CE_SAVE_FILE
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_pathCESaveFile = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}


			// SMTP_ADDR
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());

				if(line.size() > 0)
				{
					m_SMTPAddr = line;
					isValid[prefixIndex] = true;
				}

				continue;
			}


			//SMTP_PORT
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());

				if(line.size() > 0)
				{
					m_SMTPPort = (((in_port_t) atoi(line.c_str())));
					isValid[prefixIndex] = true;
				}

				continue;
			}

			//SMTP_DOMAIN
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());

				if(line.size() > 0)
				{
					m_SMTPDomain = line;
					isValid[prefixIndex] = true;
				}

				continue;
			}

			//RECIPIENTS
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());

				if(line.size() > 0)
				{
					SetSMTPEmailRecipients_noLocking(line);
					isValid[prefixIndex] = true;
				}

				continue;
			}

			// SERVICE_PREFERENCES
			//TODO: make method for parsing string to map criticality level to service
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());

				if(line.size() > 0)
				{
					m_loggerPreferences = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// HAYSTACK_STORAGE
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());

				if(line.size() > 0)
				{

					switch(line.at(0))
					{
						case 'E'://E will be implemented with multiple configuration support
						case 'M':
						{
							m_haystackStorage = line.at(0);
							//Needs a file or dir path
							if(line.size() > 2)
							{
								m_userPath = line.substr(2, line.size());
								isValid[prefixIndex] = true;
							}
							break;
						}
						case 'I':
						{
							m_haystackStorage = 'I';
							m_userPath = m_pathHome + "/Config/haystack_honeyd.config";
							isValid[prefixIndex] = true;
							break;
						}
						default:
						{
							//Invalid entry
							break;
						}
					}
				}
				continue;
			}


			// WHITELIST_FILE
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0 && !line.substr(line.size() - 4,
						line.size()).compare(".txt"))
				{
					m_pathWhitelistFile = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}
		}
	}
	else
	{
		// Do not call LOG here, Config and Logger are not yet initialized
		cout << "CRITICAL ERROR: No configuration file found!" << endl;
	}

	config.close();

	for(uint i = 0; i < sizeof(m_prefixes)/sizeof(m_prefixes[0]); i++)
	{
		if(!isValid[i])
		{
			// Do not call LOG here, Config and Logger are not yet initialized
			cout << "Invalid configuration option" << m_prefixes[i] << " is invalid in the configuration file." << endl;
		}
	}
}

bool Config::LoadUserConfig()
{
	Lock lock(&m_lock, false);

	string prefix, line;
	uint i = 0;
	bool returnValue = true;
	ifstream settings(m_userConfigFilePath.c_str());
	in_addr_t nbr;

	if(settings.is_open())
	{
		while(settings.good())
		{
			getline(settings, line);
			i++;
			prefix = "neighbor";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				if(line.size() > 0)
				{
					//Note inet_addr() not compatible with 255.255.255.255 as this is == the error condition of -1
					nbr = inet_addr(line.c_str());

					if((int)nbr == -1)
					{
						stringstream ss;
						ss << "Invalid IP address parsed on line " << i << " of the settings file.";
						LOG(ERROR, "Invalid IP address parsed.", ss.str());
						returnValue = false;
					}
					else if(nbr)
					{
						m_neighbors.push_back(nbr);
					}
					nbr = 0;
				}
			}
			prefix = "key";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				//TODO Key should be 256 characters, hard check for this once implemented
				if((line.size() > 0) && (line.size() < 257))
				{
					m_key = line;
				}
				else
				{
					stringstream ss;
					ss << "Invalid Key parsed on line " << i << " of the settings file.";
					LOG(ERROR, "Invalid Key parsed.", ss.str());
					returnValue = false;
				}
			}

			prefix = "group";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				m_group = line;
				continue;
			}
		}
	}
	else
	{
		returnValue = false;
	}
	settings.close();


	return returnValue;
}

bool Config::LoadPaths()
{
	Lock lock(&m_lock, false);

	//Get locations of nova files
	ifstream *paths =  new ifstream(PATHS_FILE);
	string prefix, line;

	if(paths->is_open())
	{
		while(paths->good())
		{
			getline(*paths,line);

			prefix = "NOVA_HOME";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				m_pathHome = ResolvePathVars(line);
				continue;
			}

			prefix = "NOVA_RD";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				m_pathReadFolder = ResolvePathVars(line);
				continue;
			}

			prefix = "NOVA_WR";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				m_pathWriteFolder = ResolvePathVars(line);
				continue;
			}

			prefix = "NOVA_BIN";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				m_pathBinaries = ResolvePathVars(line);
				continue;
			}

			prefix = "NOVA_ICON";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				m_pathIcon = ResolvePathVars(line);
				continue;
			}
		}
	}
	paths->close();
	delete paths;


	return true;
}

string Config::ResolvePathVars(string path)
{
	int start = 0;
	int end = 0;
	string var = "";

	while((start = path.find("$",end)) != -1)
	{
		end = path.find("/", start);
		//If no path after environment var
		if(end == -1)
		{

			var = path.substr(start+1, path.size());
			var = getenv(var.c_str());
			path = path.substr(0,start) + var;
		}
		else
		{
			var = path.substr(start+1, end-1);
			var = getenv(var.c_str());
			var = var + path.substr(end, path.size());
			if(start > 0)
			{
				path = path.substr(0,start)+var;
			}
			else
			{
				path = var;
			}
		}
	}
	if(var.compare(""))
	{
		return var;
	}
	else
	{
		return path;
	}
}

bool Config::SaveConfig()
{
	Lock lock(&m_lock, true);
	string line, prefix;

	//Rewrite the config file with the new settings
	string configurationBackup = m_configFilePath + ".tmp";
	string copyCommand = "cp -f " + m_configFilePath + " " + configurationBackup;
	if(system(copyCommand.c_str()) != 0)
	{
		LOG(ERROR, "Problem saving current configuration.","System Call "+copyCommand+" has failed.");
	}

	ifstream *in = new ifstream(configurationBackup.c_str());
	ofstream *out = new ofstream(m_configFilePath.c_str());

	if(out->is_open() && in->is_open())
	{
		while(in->good())
		{
			if(!getline(*in, line))
			{
				continue;
			}

			prefix = "DM_ENABLED";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				if(GetIsDmEnabled())
				{
					*out << "DM_ENABLED 1"<<endl;
				}
				else
				{
					*out << "DM_ENABLED 0"<<endl;
				}
				continue;
			}

			prefix = "IS_TRAINING";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				if(GetIsTraining())
				{
					*out << "IS_TRAINING 1"<< endl;
				}
				else
				{
					*out << "IS_TRAINING 0"<<endl;
				}
				continue;
			}

			prefix = "INTERFACE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << GetInterface() << endl;
				continue;
			}

			prefix = "DATAFILE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << GetPathTrainingFile() << endl;
				continue;
			}

			prefix = "SA_SLEEP_DURATION";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << GetSaSleepDuration() << endl;
				continue;
			}

			prefix = "SA_MAX_ATTEMPTS";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << GetSaMaxAttempts() << endl;
				continue;
			}

			prefix = "SILENT_ALARM_PORT";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << GetSaPort() << endl;
				continue;
			}

			prefix = "K";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << GetK() << endl;
				continue;
			}

			prefix = "EPS";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << GetEps() << endl;
				continue;
			}

			prefix = "CLASSIFICATION_TIMEOUT";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << GetClassificationTimeout() << endl;
				continue;
			}

			prefix = "CLASSIFICATION_THRESHOLD";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << GetClassificationThreshold() << endl;
				continue;
			}

			prefix = "USER_HONEYD_CONFIG";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << GetPathConfigHoneydUser() << endl;
				continue;
			}

			prefix = "DOPPELGANGER_IP";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << GetDoppelIp() << endl;
				continue;
			}

			prefix = "HS_HONEYD_CONFIG";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << GetPathConfigHoneydHS() << endl;
				continue;
			}

			prefix = "TCP_TIMEOUT";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << GetTcpTimout() << endl;
				continue;
			}

			prefix = "TCP_CHECK_FREQ";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << GetTcpCheckFreq()  << endl;
				continue;
			}

			prefix = "PCAP_FILE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " <<  GetPathPcapFile() << endl;
				continue;
			}

			prefix = "ENABLED_FEATURES";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << GetEnabledFeatures() << endl;
				continue;
			}

			prefix = "READ_PCAP";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				if(GetReadPcap())
				{
					*out << "READ_PCAP 1"<<  endl;
				}
				else
				{
					*out << "READ_PCAP 0"<< endl;
				}
				continue;
			}

			prefix = "GO_TO_LIVE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				if(GetGotoLive())
				{
					*out << "GO_TO_LIVE 1" << endl;
				}
				else
				{
					*out << "GO_TO_LIVE 0" << endl;
				}
				continue;
			}
			prefix = "HAYSTACK_STORAGE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " <<  m_haystackStorage;
				if(m_haystackStorage == 'M')
				{
					*out << " " << m_userPath;
				}
				*out << endl;
				continue;
			}
			*out << line << endl;
		}
	}
	else
	{
		LOG(ERROR, "Problem saving current configuration.", "");
		in->close();
		out->close();
		delete in;
		delete out;

		return false;
	}

	in->close();
	out->close();
	delete in;
	delete out;
	if(system("rm -f Config/.NOVAConfig.tmp") != 0)
	{
		LOG(WARNING, "Problem saving current configuration.", "System Command rm -f Config/.NOVAConfig.tmp has failed.");
	}

	return true;
}



void Config::SetDefaults()
{
	m_interface = "default";
	m_pathConfigHoneydHs 	= "Config/haystack.config";
	m_pathPcapFile 		= "../pcapfile";
	m_pathTrainingFile 	= "Config/data.txt";
	m_pathWhitelistFile = "Config/whitelist.txt";
	m_pathConfigHoneydUser	= "Config/doppelganger.config";
	m_pathTrainingCapFolder = "Data";
	m_pathCESaveFile = "ceStateSave";

	m_tcpTimout = 7;
	m_tcpCheckFreq = 3;
	m_readPcap = false;
	m_gotoLive = true;
	m_isDmEnabled = true;

	m_classificationTimeout = 3;
	m_saPort = 12024;
	m_k = 3;
	m_eps = 0.01;
	m_isTraining = 0;
	m_classificationThreshold = .5;
	m_saMaxAttempts = 3;
	m_saSleepDuration = .5;
	m_doppelIp = "10.0.0.1";
	m_doppelInterface = "lo";
	m_enabledFeatureMask = "111111111";
	m_thinningDistance = 0;
	m_saveFreq = 1440;
	m_dataTTL = 0;
}

// Checks to see if the current user has a nova directory, and creates it if not, along with default config files
//	Returns: True if(after the function) the user has all necessary nova config files
//		IE: Returns false only if the user doesn't have configs AND we weren't able to make them
bool Config::InitUserConfigs(string homeNovaPath)
{
	bool returnValue = true;
	struct stat fileAttr;

	// Important note
	// This is called before the logger is initialized. Calling LOG here will likely result in a crash. Just use cout instead.

	// Does the nova folder exist?
	if(stat(homeNovaPath.c_str(), &fileAttr ) == 0)
	{
		// Do all of the important files exist?
		for(uint i = 0; i < sizeof(m_requiredFiles)/sizeof(m_requiredFiles[0]); i++)
		{
			string fullPath = homeNovaPath + Config::m_requiredFiles[i];
			if(stat (fullPath.c_str(), &fileAttr ) != 0)
			{
				string defaultLocation = "/etc/nova/nova" + Config::m_requiredFiles[i];
				string copyCommand = "cp -fr " + defaultLocation + " " + fullPath;

				cout << "The required file " << fullPath << " does not exist. Copying it from the defaults folder." << endl;
				if(system(copyCommand.c_str()) != 0)
				{
					cout << "Unable to load defaults from " << defaultLocation << "System Command " << copyCommand <<" has failed." << endl;
					returnValue = false;
				}
			}
		}
	}
	else
	{
		//TODO: Do this command programmatically. Not by calling system()
		if(system("cp -rf /etc/nova/nova /usr/share/nova") == -1)
		{
			cout << "Was unable to create directory /usr/share/nova/nova" << endl;
			returnValue = false;
		}

		//Check the nova dir again
		if(stat(homeNovaPath.c_str(), &fileAttr) == 0)
		{
			return returnValue;
		}
		else
		{
			cout << "Was unable to create directory /usr/share/nova/nova" << endl;
			returnValue = false;
		}
	}

	return returnValue;
}

string Config::ToString()
{
	Lock lock(&m_lock, true);

	std::stringstream ss;
	ss << "getConfigFilePath() " << GetConfigFilePath() << endl;
	ss << "getDoppelInterface() " << GetDoppelInterface() << endl;
	ss << "getDoppelIp() " << GetDoppelIp() << endl;
	ss << "getEnabledFeatures() " << GetEnabledFeatures() << endl;
	ss << "getInterface() " << GetInterface() << endl;
	ss << "getPathCESaveFile() " << GetPathCESaveFile() << endl;
	ss << "getPathConfigHoneydDm() " << GetPathConfigHoneydUser() << endl;
	ss << "getPathConfigHoneydHs() " << GetPathConfigHoneydHS() << endl;
	ss << "getPathPcapFile() " << GetPathPcapFile() << endl;
	ss << "getPathTrainingCapFolder() " << GetPathTrainingCapFolder() << endl;
	ss << "getPathTrainingFile() " << GetPathTrainingFile() << endl;

	ss << "getReadPcap() " << GetReadPcap() << endl;
	ss << "GetIsDmEnabled() " << GetIsDmEnabled() << endl;
	ss << "getIsTraining() " << GetIsTraining() << endl;
	ss << "getGotoLive() " << GetGotoLive() << endl;

	ss << "getClassificationTimeout() " << GetClassificationTimeout() << endl;
	ss << "getDataTTL() " << GetDataTTL() << endl;
	ss << "getK() " << GetK() << endl;
	ss << "getSaMaxAttempts() " << GetSaMaxAttempts() << endl;
	ss << "getSaPort() " << GetSaPort() << endl;
	ss << "getSaveFreq() " << GetSaveFreq() << endl;
	ss << "getTcpCheckFreq() " << GetTcpCheckFreq() << endl;
	ss << "getTcpTimout() " << GetTcpTimout() << endl;
	ss << "getThinningDistance() " << GetThinningDistance() << endl;

	ss << "getClassificationThreshold() " << GetClassificationThreshold() << endl;
	ss << "getSaSleepDuration() " << GetSaSleepDuration() << endl;
	ss << "getEps() " << GetEps() << endl;


	return ss.str();
}

Config::Config()
{
	pthread_rwlock_init(&m_lock, NULL);
	LoadPaths();

	if(!InitUserConfigs(GetPathHome()))
	{
		// Do not call LOG here, Config and logger are not yet initialized
		cout << "CRITICAL ERROR: InitUserConfigs failed" << endl;
	}

	m_configFilePath = GetPathHome() + "/Config/NOVAConfig.txt";
	m_userConfigFilePath = GetPathHome() + "/settings";
	SetDefaults();
	LoadConfig();
	LoadUserConfig();
}

Config::~Config()
{
	delete m_instance;
}


std::string Config::ReadSetting(std::string key)
{
	Lock lock(&m_lock, false);

	string line;
	string value;

	ifstream config(m_configFilePath.c_str());

	if(config.is_open())
	{
		while (config.good())
		{
			getline(config, line);

			if(!line.substr(0, key.size()).compare(key))
			{
				line = line.substr(key.size() + 1, line.size());
				if(line.size() > 0)
				{
					value = line;
					break;
				}
				continue;
			}
		}

		config.close();

		return value;
	}
	else
	{
		LOG(ERROR, "Unable to read configuration file", "");

		return "";
	}
}

bool Config::WriteSetting(std::string key, std::string value)
{
	Lock lock(&m_lock, false);
	string line;
	bool error = false;

	//Rewrite the config file with the new settings
	string configurationBackup = m_configFilePath + ".tmp";
	string copyCommand = "cp -f " + m_configFilePath + " " + configurationBackup;
	if(system(copyCommand.c_str()) != 0)
	{
		LOG(ERROR, "Problem saving current configuration.","System Call "+copyCommand+" has failed.");
	}

	ifstream *in = new ifstream(configurationBackup.c_str());
	ofstream *out = new ofstream(m_configFilePath.c_str());

	if(out->is_open() && in->is_open())
	{
		while(in->good())
		{
			if(!getline(*in, line))
			{
				continue;
			}


			if(!line.substr(0,key.size()).compare(key))
			{
				*out << key << " " << value << endl;
				continue;
			}

			*out << line << endl;
		}
	}
	else
	{
		error = true;
	}

	in->close();
	out->close();
	delete in;
	delete out;

	if(system("rm -f Config/.NOVAConfig.tmp") != 0)
	{
		LOG(WARNING, "Problem saving current configuration.", "System Command rm -f Config/.NOVAConfig.tmp has failed.");
	}



	if (error)
	{
		LOG(ERROR, "Problem saving current configuration.", "");
		return false;
	}
	else
	{
		return true;
	}
}

double Config::GetClassificationThreshold()
{
	Lock lock(&m_lock, true);
	return m_classificationThreshold;
}

int Config::GetClassificationTimeout()
{
	Lock lock(&m_lock, true);
	return m_classificationTimeout;
}

string Config::GetConfigFilePath()
{
	Lock lock(&m_lock, true);
	return m_configFilePath;
}

int Config::GetDataTTL()
{
	Lock lock(&m_lock, true);
	return m_dataTTL;
}

string Config::GetDoppelInterface()
{
	Lock lock(&m_lock, true);
	return m_doppelInterface;
}

string Config::GetDoppelIp()
{
	Lock lock(&m_lock, true);
	return m_doppelIp;
}

string Config::GetEnabledFeatures()
{
	Lock lock(&m_lock, true);
	return m_enabledFeatureMask;
}

uint Config::GetEnabledFeatureCount()
{
	Lock lock(&m_lock, true);
	return m_enabledFeatureCount;
}

bool Config::IsFeatureEnabled(int i)
{
	Lock lock(&m_lock, true);
	return m_isFeatureEnabled[i];
}

double Config::GetEps()
{
	Lock lock(&m_lock, true);
	return m_eps;
}

bool Config::GetGotoLive()
{
	Lock lock(&m_lock, true);
	return m_gotoLive;
}

string Config::GetInterface()
{
	Lock lock(&m_lock, true);
	return m_interface;
}

bool Config::GetIsDmEnabled()
{
	Lock lock(&m_lock, true);
	return m_isDmEnabled;
}

bool Config::GetIsTraining()
{
	Lock lock(&m_lock, true);
	return m_isTraining;
}

int Config::GetK()
{
	Lock lock(&m_lock, true);
	return m_k;
}

string Config::GetPathCESaveFile()
{
	Lock lock(&m_lock, true);
	return m_pathCESaveFile;
}

string Config::GetPathConfigHoneydUser()
{
	Lock lock(&m_lock, true);
	return m_pathConfigHoneydUser;
}

string Config::GetPathConfigHoneydHS()
{
	Lock lock(&m_lock, true);
	return m_pathConfigHoneydHs;
}

string Config::GetPathPcapFile()
{
	Lock lock(&m_lock, true);
	return m_pathPcapFile;
}

string Config::GetPathTrainingCapFolder()
{
	Lock lock(&m_lock, true);
	return m_pathTrainingCapFolder;
}

string Config::GetPathTrainingFile()
{
	Lock lock(&m_lock, true);
	return m_pathTrainingFile;
}

string Config::GetPathWhitelistFile()
{
	Lock lock(&m_lock, true);
	return m_pathWhitelistFile;
}

bool Config::GetReadPcap()
{
	Lock lock(&m_lock, true);
	return m_readPcap;
}

int Config::GetSaMaxAttempts()
{
	Lock lock(&m_lock, true);
	return m_saMaxAttempts;
}

int Config::GetSaPort()
{
	Lock lock(&m_lock, true);
	return m_saPort;
}

double Config::GetSaSleepDuration()
{
	Lock lock(&m_lock, true);
	return m_saSleepDuration;
}

int Config::GetSaveFreq()
{
	Lock lock(&m_lock, true);
	return m_saveFreq;
}

int Config::GetTcpCheckFreq()
{
	Lock lock(&m_lock, true);
	return m_tcpCheckFreq;
}

int Config::GetTcpTimout()
{
	Lock lock(&m_lock, true);
	return m_tcpTimout;
}

double Config::GetThinningDistance()
{
	Lock lock(&m_lock, true);
	return m_thinningDistance;
}

string Config::GetKey()
{
	Lock lock(&m_lock, true);
	return m_key;
}

vector<in_addr_t> Config::GetNeighbors()
{
	Lock lock(&m_lock, true);
	return m_neighbors;
}

string Config::GetGroup()
{
	Lock lock(&m_lock, true);
	return m_group;
}

void Config::SetClassificationThreshold(double classificationThreshold)
{
	Lock lock(&m_lock, false);
	m_classificationThreshold = classificationThreshold;
}

void Config::SetClassificationTimeout(int classificationTimeout)
{
	Lock lock(&m_lock, false);
	m_classificationTimeout = classificationTimeout;
}

void Config::SetConfigFilePath(string configFilePath)
{
	Lock lock(&m_lock, false);
	m_configFilePath = configFilePath;
}

void Config::SetDataTTL(int dataTTL)
{
	Lock lock(&m_lock, false);
	m_dataTTL = dataTTL;
}

void Config::SetDoppelInterface(string doppelInterface)
{
	Lock lock(&m_lock, false);
	m_doppelInterface = doppelInterface;
}

void Config::SetDoppelIp(string doppelIp)
{
	Lock lock(&m_lock, false);
	m_doppelIp = doppelIp;
}

void Config::SetEnabledFeatures_noLocking(string enabledFeatureMask)
{
	m_enabledFeatureCount = 0;
	for(uint i = 0; i < DIM; i++)
	{
		if('1' == enabledFeatureMask.at(i))
		{
			m_isFeatureEnabled[i] = true;
			m_enabledFeatureCount++;
		}
		else
		{
			m_isFeatureEnabled[i] = false;
		}
	}

	m_squrtEnabledFeatures = sqrt(m_enabledFeatureCount);
	m_enabledFeatureMask = enabledFeatureMask;
}


void Config::SetEnabledFeatures(string enabledFeatureMask)
{
	Lock lock(&m_lock, false);
	SetEnabledFeatures_noLocking(enabledFeatureMask);
}

void Config::SetEps(double eps)
{
	Lock lock(&m_lock, false);
	m_eps = eps;
}

void Config::SetGotoLive(bool gotoLive)
{
	Lock lock(&m_lock, false);
	m_gotoLive = gotoLive;
}

void Config::SetInterface(string interface)
{
	Lock lock(&m_lock, false);
	m_interface = interface;
}

void Config::SetIsDmEnabled(bool isDmEnabled)
{
	Lock lock(&m_lock, false);
	m_isDmEnabled = isDmEnabled;
}

void Config::SetIsTraining(bool isTraining)
{
	Lock lock(&m_lock, false);
	m_isTraining = isTraining;
}

void Config::SetK(int k)
{
	Lock lock(&m_lock, false);
	m_k = k;
}

void Config::SetPathCESaveFile(string pathCESaveFile)
{
	Lock lock(&m_lock, false);
	m_pathCESaveFile = pathCESaveFile;
}

void Config::SetPathConfigHoneydUser(string pathConfigHoneydUser)
{
	Lock lock(&m_lock, false);
	m_pathConfigHoneydUser = pathConfigHoneydUser;
}

void Config::SetPathConfigHoneydHs(string pathConfigHoneydHs)
{
	Lock lock(&m_lock, false);
	m_pathConfigHoneydHs = pathConfigHoneydHs;
}

void Config::SetPathPcapFile(string pathPcapFile)
{
	Lock lock(&m_lock, false);
	m_pathPcapFile = pathPcapFile;
}

void Config::SetPathTrainingCapFolder(string pathTrainingCapFolder)
{
	Lock lock(&m_lock, false);
	m_pathTrainingCapFolder = pathTrainingCapFolder;
}

void Config::SetPathTrainingFile(string pathTrainingFile)
{
	Lock lock(&m_lock, false);
	m_pathTrainingFile = pathTrainingFile;
}

void Config::SetPathWhitelistFile(string pathWhitelistFile)
{
	Lock lock(&m_lock, false);
	m_pathWhitelistFile = pathWhitelistFile;
}


void Config::SetReadPcap(bool readPcap)
{
	Lock lock(&m_lock, false);
	m_readPcap = readPcap;
}

void Config::SetSaMaxAttempts(int saMaxAttempts)
{
	Lock lock(&m_lock, false);
	m_saMaxAttempts = saMaxAttempts;
}

void Config::SetSaPort(int saPort)
{
	Lock lock(&m_lock, false);
	m_saPort = saPort;
}

void Config::SetSaSleepDuration(double saSleepDuration)
{
	Lock lock(&m_lock, false);
	m_saSleepDuration = saSleepDuration;
}

void Config::SetSaveFreq(int saveFreq)
{
	Lock lock(&m_lock, false);
	m_saveFreq = saveFreq;
}

void Config::SetTcpCheckFreq(int tcpCheckFreq)
{
	Lock lock(&m_lock, false);
	m_tcpCheckFreq = tcpCheckFreq;
}

void Config::SetTcpTimout(int tcpTimout)
{
	Lock lock(&m_lock, false);
	m_tcpTimout = tcpTimout;
}

void Config::SetThinningDistance(double thinningDistance)
{
	Lock lock(&m_lock, false);
	m_thinningDistance = thinningDistance;
}

void Config::SetKey(string key)
{
	Lock lock(&m_lock, false);
	m_key = key;
}

void Config::SetNeigbors(vector<in_addr_t> neighbors)
{
	Lock lock(&m_lock, false);
	m_neighbors = neighbors;
}

void Config::SetGroup(string group)
{
	Lock lock(&m_lock, false);
	m_group = group;
}

string Config::GetLoggerPreferences()
{
	Lock lock(&m_lock, true);
	return m_loggerPreferences;
}

string Config::GetSMTPAddr()
{
	Lock lock(&m_lock, true);
	return m_SMTPAddr;
}

string Config::GetSMTPDomain()
{
	Lock lock(&m_lock, true);
	return m_SMTPDomain;
}

vector<string> Config::GetSMTPEmailRecipients()
{
	Lock lock(&m_lock, true);
	return m_SMTPEmailRecipients;
}

in_port_t Config::GetSMTPPort()
{
	Lock lock(&m_lock, true);
	return m_SMTPPort;
}

void Config::SetLoggerPreferences(string loggerPreferences)
{
	Lock lock(&m_lock, false);
	m_loggerPreferences = loggerPreferences;

}

void Config::SetSMTPAddr(string SMTPAddr)
{
	Lock lock(&m_lock, false);
	m_SMTPAddr = SMTPAddr;

}

void Config::SetSMTPDomain(string SMTPDomain)
{
	Lock lock(&m_lock, false);
	m_SMTPDomain = SMTPDomain;

}

void Config::SetSMTPEmailRecipients(vector<string> SMTPEmailRecipients)
{
	Lock lock(&m_lock, false);
	m_SMTPEmailRecipients = SMTPEmailRecipients;

}

void Config::SetSMTPEmailRecipients_noLocking(string SMTPEmailRecipients)
{
	vector<string> addresses;
	istringstream iss(SMTPEmailRecipients);

	copy(istream_iterator<string>(iss),
			istream_iterator<string>(),
			back_inserter<vector <string> >(addresses));

	vector<string> out = addresses;

	for(uint16_t i = 0; i < addresses.size(); i++)
	{
		uint16_t endSubStr = addresses[i].find(",", 0);

		if(endSubStr != addresses[i].npos)
		{
			out[i] = addresses[i].substr(0, endSubStr);
		}
	}

	m_SMTPEmailRecipients = out;
}

void Config::SetSMTPPort(in_port_t SMTPPort)
{
	Lock lock(&m_lock, false);
	m_SMTPPort = SMTPPort;

}

string Config::GetPathBinaries()
{
	Lock lock(&m_lock, true);
	return m_pathBinaries;
}

string Config::GetPathWriteFolder()
{
	Lock lock(&m_lock, true);
	return m_pathWriteFolder;
}

string Config::GetPathReadFolder()
{
	Lock lock(&m_lock, true);
	return m_pathReadFolder;
}

string Config::GetPathIcon()
{
	Lock lock(&m_lock, true);
	return m_pathIcon;
}

string Config::GetPathHome()
{
	Lock lock(&m_lock, true);
	return m_pathHome;
}

double Config::GetSqurtEnabledFeatures()
{
	Lock lock(&m_lock, true);
	return m_squrtEnabledFeatures;
}

char Config::GetHaystackStorage()
{
	Lock lock(&m_lock, true);
	return m_haystackStorage;
}

void Config::SetHaystackStorage(char haystackStorage)
{
	Lock lock(&m_lock, false);
	m_haystackStorage = haystackStorage;
}

string Config::GetUserPath()
{
	Lock lock(&m_lock, true);
	return m_userPath;
}

void Config::SetUserPath(string userPath)
{
	Lock lock(&m_lock, false);
	m_userPath = userPath;
}
}

