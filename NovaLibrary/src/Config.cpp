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
//============================================================================/*

#include <sys/stat.h>
#include <sys/un.h>
#include <fstream>
#include <sstream>
#include <math.h>

#include "Config.h"
#include "Logger.h"
#include "NovaUtil.h"

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
	"HAYSTACK_STORAGE"
};

// Files we need to run (that will be loaded with defaults if deleted)
string Config::m_requiredFiles[] =
{
	"/settings",
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
	pthread_rwlock_wrlock(&m_lock);

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

						FILE * out = popen("netstat -rn", "r");
						if(out != NULL)
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
		}
	}
	else
	{
		// Do not call LOG here, Config and Logger are not yet initialized
		cout << "CRITICAL ERROR: No configuration file found!" << endl;
	}


	for(uint i = 0; i < sizeof(m_prefixes)/sizeof(m_prefixes[0]); i++)
	{
		if(!isValid[i])
		{
			// Do not call LOG here, Config and Logger are not yet initialized
			cout << "Invalid configuration option" << m_prefixes[i] << " is invalid in the configuration file." << endl;
		}
	}
	pthread_rwlock_unlock(&m_lock);
}

bool Config::LoadUserConfig()
{
	pthread_rwlock_wrlock(&m_lock);

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

	pthread_rwlock_unlock(&m_lock);
	return returnValue;
}

bool Config::LoadPaths()
{
	pthread_rwlock_wrlock(&m_lock);

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
	pthread_rwlock_unlock(&m_lock);

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
	pthread_rwlock_rdlock(&m_lock);
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
		pthread_rwlock_unlock(&m_lock);
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
	pthread_rwlock_unlock(&m_lock);
	return true;
}



void Config::SetDefaults()
{
	m_interface = "default";
	m_pathConfigHoneydHs 	= "Config/haystack.config";
	m_pathPcapFile 		= "../pcapfile";
	m_pathTrainingFile 	= "Data/data.txt";
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

// Checks to see if the current user has a ~/.nova directory, and creates it if not, along with default config files
//	Returns: True if(after the function) the user has all necessary ~/.nova config files
//		IE: Returns false only if the user doesn't have configs AND we weren't able to make them
bool Config::InitUserConfigs(string homeNovaPath)
{
	bool returnValue = true;
	struct stat fileAttr;

	// Important note
	// This is called before the logger is initialized. Calling LOG here will likely result in a crash. Just use cout instead.

	// Does ~/.nova exist?
	if(stat(homeNovaPath.c_str(), &fileAttr ) == 0)
	{
		// Do all of the important files exist?
		for(uint i = 0; i < sizeof(m_requiredFiles)/sizeof(m_requiredFiles[0]); i++)
		{
			string fullPath = homeNovaPath + Config::m_requiredFiles[i];
			if(stat (fullPath.c_str(), &fileAttr ) != 0)
			{
				string defaultLocation = "/etc/nova/.nova" + Config::m_requiredFiles[i];
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
		if(system("cp -rf /etc/nova/.nova /usr/share/nova") == -1)
		{
			cout << "Was unable to create directory /usr/share/nova/.nova" << endl;
			returnValue = false;
		}

		//Check the ~/.nova dir again
		if(stat(homeNovaPath.c_str(), &fileAttr) == 0)
		{
			return returnValue;
		}
		else
		{
			cout << "Was unable to create directory /usr/share/nova/.nova" << endl;
			returnValue = false;
		}
	}

	return returnValue;
}

string Config::ToString()
{
	pthread_rwlock_rdlock(&m_lock);

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

	pthread_rwlock_unlock(&m_lock);
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

}


std::string Config::ReadSetting(std::string key)
{
	pthread_rwlock_wrlock(&m_lock);

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
		pthread_rwlock_unlock(&m_lock);
		return value;
	}
	else
	{
		LOG(ERROR, "Unable to read configuration file", "");
		pthread_rwlock_unlock(&m_lock);
		return "";
	}
}

bool Config::WriteSetting(std::string key, std::string value)
{
	pthread_rwlock_wrlock(&m_lock);
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

	pthread_rwlock_unlock(&m_lock);

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
	pthread_rwlock_rdlock(&m_lock);
	double classificationThreshold = m_classificationThreshold;
	pthread_rwlock_unlock(&m_lock);
	return classificationThreshold;
}

int Config::GetClassificationTimeout()
{
	pthread_rwlock_rdlock(&m_lock);
	int classificationTimeout = m_classificationTimeout;
	pthread_rwlock_unlock(&m_lock);
	return classificationTimeout;
}

string Config::GetConfigFilePath()
{
	pthread_rwlock_rdlock(&m_lock);
	string configFilePath = m_configFilePath;
	pthread_rwlock_unlock(&m_lock);
	return configFilePath;
}

int Config::GetDataTTL()
{
	pthread_rwlock_rdlock(&m_lock);
	int dataTTL = m_dataTTL;
	pthread_rwlock_unlock(&m_lock);
	return dataTTL;
}

string Config::GetDoppelInterface()
{
	pthread_rwlock_rdlock(&m_lock);
	string doppelInterface = m_doppelInterface;
	pthread_rwlock_unlock(&m_lock);
	return doppelInterface;
}

string Config::GetDoppelIp()
{
	pthread_rwlock_rdlock(&m_lock);
	string doppelIp = m_doppelIp;
	pthread_rwlock_unlock(&m_lock);
	return doppelIp;
}

string Config::GetEnabledFeatures()
{
	pthread_rwlock_rdlock(&m_lock);
	string enabledFeatureMask = m_enabledFeatureMask;
	pthread_rwlock_unlock(&m_lock);
	return enabledFeatureMask;
}

uint Config::GetEnabledFeatureCount()
{
	pthread_rwlock_rdlock(&m_lock);
	uint enabledFeatureCount = m_enabledFeatureCount;
	pthread_rwlock_unlock(&m_lock);
	return enabledFeatureCount;
}

bool Config::IsFeatureEnabled(int i)
{
	pthread_rwlock_rdlock(&m_lock);
	bool isFeatureEnabled = m_isFeatureEnabled[i];
	pthread_rwlock_unlock(&m_lock);
	return isFeatureEnabled;
}

double Config::GetEps()
{
	pthread_rwlock_rdlock(&m_lock);
	double eps = m_eps;
	pthread_rwlock_unlock(&m_lock);
	return eps;
}

bool Config::GetGotoLive()
{
	pthread_rwlock_rdlock(&m_lock);
	bool gotoLive = m_gotoLive;
	pthread_rwlock_unlock(&m_lock);
	return gotoLive;
}

string Config::GetInterface()
{
	pthread_rwlock_rdlock(&m_lock);
	string interface = m_interface;
	pthread_rwlock_unlock(&m_lock);
	return interface;
}

bool Config::GetIsDmEnabled()
{
	pthread_rwlock_rdlock(&m_lock);
	bool isDmEnabled = m_isDmEnabled;
	pthread_rwlock_unlock(&m_lock);
	return isDmEnabled;
}

bool Config::GetIsTraining()
{
	pthread_rwlock_rdlock(&m_lock);
	bool isTraining = m_isTraining;
	pthread_rwlock_unlock(&m_lock);
	return isTraining;
}

int Config::GetK()
{
	pthread_rwlock_rdlock(&m_lock);
	int k = m_k;
	pthread_rwlock_unlock(&m_lock);
	return k;
}

string Config::GetPathCESaveFile()
{
	pthread_rwlock_rdlock(&m_lock);
	string pathCESaveFile = m_pathCESaveFile;
	pthread_rwlock_unlock(&m_lock);
	return pathCESaveFile;
}

string Config::GetPathConfigHoneydUser()
{
	pthread_rwlock_rdlock(&m_lock);
	string pathConfigHoneydDm = m_pathConfigHoneydUser;
	pthread_rwlock_unlock(&m_lock);
	return pathConfigHoneydDm;
}

string Config::GetPathConfigHoneydHS()
{
	pthread_rwlock_rdlock(&m_lock);
	string pathConfigHoneydHs = m_pathConfigHoneydHs;
	pthread_rwlock_unlock(&m_lock);
	return pathConfigHoneydHs;
}

string Config::GetPathPcapFile()
{
	pthread_rwlock_rdlock(&m_lock);
	string pathPcapFile = m_pathPcapFile;
	pthread_rwlock_unlock(&m_lock);
	return pathPcapFile;
}

string Config::GetPathTrainingCapFolder()
{
	pthread_rwlock_rdlock(&m_lock);
	string pathTrainingCapFolder = m_pathTrainingCapFolder;
	pthread_rwlock_unlock(&m_lock);
	return pathTrainingCapFolder;
}

string Config::GetPathTrainingFile()
{
	pthread_rwlock_rdlock(&m_lock);
	string pathTrainingFile = m_pathTrainingFile;
	pthread_rwlock_unlock(&m_lock);
	return pathTrainingFile;
}

bool Config::GetReadPcap()
{
	pthread_rwlock_rdlock(&m_lock);
	bool readPcap = m_readPcap;
	pthread_rwlock_unlock(&m_lock);
	return readPcap;
}

int Config::GetSaMaxAttempts()
{
	pthread_rwlock_rdlock(&m_lock);
	int saMaxAttempts = m_saMaxAttempts;
	pthread_rwlock_unlock(&m_lock);
	return saMaxAttempts;
}

int Config::GetSaPort()
{
	pthread_rwlock_rdlock(&m_lock);
	int saPort = m_saPort;
	pthread_rwlock_unlock(&m_lock);
	return saPort;
}

double Config::GetSaSleepDuration()
{
	pthread_rwlock_rdlock(&m_lock);
	double saSleepDuration = m_saSleepDuration;
	pthread_rwlock_unlock(&m_lock);
	return saSleepDuration;
}

int Config::GetSaveFreq()
{
	pthread_rwlock_rdlock(&m_lock);
	int saveFreq = m_saveFreq;
	pthread_rwlock_unlock(&m_lock);
	return saveFreq;
}

int Config::GetTcpCheckFreq()
{
	pthread_rwlock_rdlock(&m_lock);
	int tcpCheckFreq = m_tcpCheckFreq;
	pthread_rwlock_unlock(&m_lock);
	return tcpCheckFreq;
}

int Config::GetTcpTimout()
{
	pthread_rwlock_rdlock(&m_lock);
	int tcpTimout = m_tcpTimout;
	pthread_rwlock_unlock(&m_lock);
	return tcpTimout;
}

int Config::GetThinningDistance()
{
	pthread_rwlock_rdlock(&m_lock);
	int thinningDistance = m_thinningDistance;
	pthread_rwlock_unlock(&m_lock);
	return thinningDistance;
}

string Config::GetKey()
{
	pthread_rwlock_rdlock(&m_lock);
	string key = m_key;
	pthread_rwlock_unlock(&m_lock);
	return key;
}

vector<in_addr_t> Config::GetNeighbors()
{
	pthread_rwlock_rdlock(&m_lock);
	vector<in_addr_t> neighbors = m_neighbors;
	pthread_rwlock_unlock(&m_lock);
	return neighbors;
}

string Config::GetGroup()
{
	pthread_rwlock_rdlock(&m_lock);
	string group = m_group;
	pthread_rwlock_unlock(&m_lock);
	return group;
}

void Config::SetClassificationThreshold(double classificationThreshold)
{
	pthread_rwlock_wrlock(&m_lock);
	m_classificationThreshold = classificationThreshold;
	pthread_rwlock_unlock(&m_lock);
}

void Config::SetClassificationTimeout(int classificationTimeout)
{
	pthread_rwlock_wrlock(&m_lock);
	m_classificationTimeout = classificationTimeout;
	pthread_rwlock_unlock(&m_lock);
}

void Config::SetConfigFilePath(string configFilePath)
{
	pthread_rwlock_wrlock(&m_lock);
	m_configFilePath = configFilePath;
	pthread_rwlock_unlock(&m_lock);
}

void Config::SetDataTTL(int dataTTL)
{
	pthread_rwlock_wrlock(&m_lock);
	m_dataTTL = dataTTL;
	pthread_rwlock_unlock(&m_lock);
}

void Config::SetDoppelInterface(string doppelInterface)
{
	pthread_rwlock_wrlock(&m_lock);
	m_doppelInterface = doppelInterface;
	pthread_rwlock_unlock(&m_lock);
}

void Config::SetDoppelIp(string doppelIp)
{
	pthread_rwlock_wrlock(&m_lock);
	m_doppelIp = doppelIp;
	pthread_rwlock_unlock(&m_lock);
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
	pthread_rwlock_wrlock(&m_lock);
	SetEnabledFeatures_noLocking(enabledFeatureMask);
	pthread_rwlock_rdlock(&m_lock);
}

void Config::SetEps(double eps)
{
	pthread_rwlock_wrlock(&m_lock);
	m_eps = eps;
	pthread_rwlock_unlock(&m_lock);
}

void Config::SetGotoLive(bool gotoLive)
{
	pthread_rwlock_wrlock(&m_lock);
	m_gotoLive = gotoLive;
	pthread_rwlock_unlock(&m_lock);
}

void Config::SetInterface(string interface)
{
	pthread_rwlock_wrlock(&m_lock);
	m_interface = interface;
	pthread_rwlock_unlock(&m_lock);
}

void Config::SetIsDmEnabled(bool isDmEnabled)
{
	pthread_rwlock_wrlock(&m_lock);
	m_isDmEnabled = isDmEnabled;
	pthread_rwlock_unlock(&m_lock);
}

void Config::SetIsTraining(bool isTraining)
{
	pthread_rwlock_wrlock(&m_lock);
	m_isTraining = isTraining;
	pthread_rwlock_unlock(&m_lock);
}

void Config::SetK(int k)
{
	pthread_rwlock_wrlock(&m_lock);
	m_k = k;
	pthread_rwlock_unlock(&m_lock);
}

void Config::SetPathCESaveFile(string pathCESaveFile)
{
	pthread_rwlock_wrlock(&m_lock);
	m_pathCESaveFile = pathCESaveFile;
	pthread_rwlock_unlock(&m_lock);
}

void Config::SetPathConfigHoneydUser(string pathConfigHoneydUser)
{
	pthread_rwlock_wrlock(&m_lock);
	m_pathConfigHoneydUser = pathConfigHoneydUser;
	pthread_rwlock_unlock(&m_lock);
}

void Config::SetPathConfigHoneydHs(string pathConfigHoneydHs)
{
	pthread_rwlock_wrlock(&m_lock);
	m_pathConfigHoneydHs = pathConfigHoneydHs;
	pthread_rwlock_unlock(&m_lock);
}

void Config::SetPathPcapFile(string pathPcapFile)
{
	pthread_rwlock_wrlock(&m_lock);
	m_pathPcapFile = pathPcapFile;
	pthread_rwlock_unlock(&m_lock);
}

void Config::SetPathTrainingCapFolder(string pathTrainingCapFolder)
{
	pthread_rwlock_wrlock(&m_lock);
	m_pathTrainingCapFolder = pathTrainingCapFolder;
	pthread_rwlock_unlock(&m_lock);
}

void Config::SetPathTrainingFile(string pathTrainingFile)
{
	pthread_rwlock_wrlock(&m_lock);
	m_pathTrainingFile = pathTrainingFile;
	pthread_rwlock_unlock(&m_lock);
}

void Config::SetReadPcap(bool readPcap)
{
	pthread_rwlock_wrlock(&m_lock);
	m_readPcap = readPcap;
	pthread_rwlock_unlock(&m_lock);
}

void Config::SetSaMaxAttempts(int saMaxAttempts)
{
	pthread_rwlock_wrlock(&m_lock);
	m_saMaxAttempts = saMaxAttempts;
	pthread_rwlock_unlock(&m_lock);
}

void Config::SetSaPort(int saPort)
{
	pthread_rwlock_wrlock(&m_lock);
	m_saPort = saPort;
	pthread_rwlock_unlock(&m_lock);
}

void Config::SetSaSleepDuration(double saSleepDuration)
{
	pthread_rwlock_wrlock(&m_lock);
	m_saSleepDuration = saSleepDuration;
	pthread_rwlock_unlock(&m_lock);
}

void Config::SetSaveFreq(int saveFreq)
{
	pthread_rwlock_wrlock(&m_lock);
	m_saveFreq = saveFreq;
	pthread_rwlock_unlock(&m_lock);
}

void Config::SetTcpCheckFreq(int tcpCheckFreq)
{
	pthread_rwlock_wrlock(&m_lock);
	m_tcpCheckFreq = tcpCheckFreq;
	pthread_rwlock_unlock(&m_lock);
}

void Config::SetTcpTimout(int tcpTimout)
{
	pthread_rwlock_wrlock(&m_lock);
	m_tcpTimout = tcpTimout;
	pthread_rwlock_unlock(&m_lock);
}

void Config::SetThinningDistance(int thinningDistance)
{
	pthread_rwlock_wrlock(&m_lock);
	m_thinningDistance = thinningDistance;
	pthread_rwlock_unlock(&m_lock);
}

void Config::SetKey(string key)
{
	pthread_rwlock_wrlock(&m_lock);
	m_key = key;
	pthread_rwlock_unlock(&m_lock);
}

void Config::SetNeigbors(vector<in_addr_t> neighbors)
{
	pthread_rwlock_wrlock(&m_lock);
	m_neighbors = neighbors;
	pthread_rwlock_unlock(&m_lock);
}

void Config::SetGroup(string group)
{
	pthread_rwlock_wrlock(&m_lock);
	m_group = group;
	pthread_rwlock_unlock(&m_lock);
}

string Config::GetLoggerPreferences()
{
	pthread_rwlock_rdlock(&m_lock);
	string loggerPreferences = m_loggerPreferences;
	pthread_rwlock_unlock(&m_lock);
	return loggerPreferences;
}

string Config::GetSMTPAddr()
{
	pthread_rwlock_rdlock(&m_lock);
	string SMTPAddr = m_SMTPAddr;
	pthread_rwlock_unlock(&m_lock);
	return SMTPAddr;
}

string Config::GetSMTPDomain()
{
	pthread_rwlock_rdlock(&m_lock);
	string SMTPDomain = m_SMTPDomain;
	pthread_rwlock_unlock(&m_lock);
	return SMTPDomain;
}

vector<string> Config::GetSMTPEmailRecipients()
{
	pthread_rwlock_rdlock(&m_lock);
	vector<string> SMTPEmailRecipients = m_SMTPEmailRecipients;
	pthread_rwlock_unlock(&m_lock);
	return SMTPEmailRecipients;
}

in_port_t Config::GetSMTPPort()
{
	pthread_rwlock_rdlock(&m_lock);
	in_port_t SMTPPort = m_SMTPPort;
	pthread_rwlock_unlock(&m_lock);
	return SMTPPort;
}

void Config::SetLoggerPreferences(string loggerPreferences)
{
	pthread_rwlock_wrlock(&m_lock);
	m_loggerPreferences = loggerPreferences;
	pthread_rwlock_unlock(&m_lock);
}

void Config::SetSMTPAddr(string SMTPAddr)
{
	pthread_rwlock_wrlock(&m_lock);
	m_SMTPAddr = SMTPAddr;
	pthread_rwlock_unlock(&m_lock);
}

void Config::SetSMTPDomain(string SMTPDomain)
{
	pthread_rwlock_wrlock(&m_lock);
	m_SMTPDomain = SMTPDomain;
	pthread_rwlock_unlock(&m_lock);
}

void Config::SetSMTPEmailRecipients(vector<string> SMTPEmailRecipients)
{
	pthread_rwlock_wrlock(&m_lock);
	m_SMTPEmailRecipients = SMTPEmailRecipients;
	pthread_rwlock_unlock(&m_lock);
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
	pthread_rwlock_wrlock(&m_lock);
	m_SMTPPort = SMTPPort;
	pthread_rwlock_unlock(&m_lock);
}

string Config::GetPathBinaries()
{
	pthread_rwlock_rdlock(&m_lock);
	string pathBinaries = m_pathBinaries;
	pthread_rwlock_unlock(&m_lock);
	return pathBinaries;
}

string Config::GetPathWriteFolder()
{
	pthread_rwlock_rdlock(&m_lock);
	string pathWriteFolder = m_pathWriteFolder;
	pthread_rwlock_unlock(&m_lock);
	return pathWriteFolder;
}

string Config::GetPathReadFolder()
{
	pthread_rwlock_rdlock(&m_lock);
	string pathReadFolder = m_pathReadFolder;
	pthread_rwlock_unlock(&m_lock);
	return pathReadFolder;
}

string Config::GetPathIcon()
{
	pthread_rwlock_rdlock(&m_lock);
	string pathIcon= m_pathIcon;
	pthread_rwlock_unlock(&m_lock);
	return pathIcon;
}

string Config::GetPathHome()
{
	pthread_rwlock_rdlock(&m_lock);
	string pathHome = m_pathHome;
	pthread_rwlock_unlock(&m_lock);
	return pathHome;
}

double Config::GetSqurtEnabledFeatures()
{
	pthread_rwlock_rdlock(&m_lock);
	double squrtEnabledFeatures= m_squrtEnabledFeatures;
	pthread_rwlock_unlock(&m_lock);
	return squrtEnabledFeatures;
}

char Config::GetHaystackStorage()
{
	pthread_rwlock_rdlock(&m_lock);
	char haystackStorage = m_haystackStorage;
	pthread_rwlock_unlock(&m_lock);
	return haystackStorage;
}

void Config::SetHaystackStorage(char haystackStorage)
{
	pthread_rwlock_wrlock(&m_lock);
	m_haystackStorage = haystackStorage;
	pthread_rwlock_unlock(&m_lock);
}

string Config::GetUserPath()
{
	pthread_rwlock_rdlock(&m_lock);
	string userPath = m_userPath;
	pthread_rwlock_unlock(&m_lock);
	return userPath;
}

void Config::SetUserPath(string userPath)
{
	pthread_rwlock_wrlock(&m_lock);
	m_userPath = userPath;
	pthread_rwlock_unlock(&m_lock);
}
}

