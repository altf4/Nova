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

#include "Config.h"
#include <fstream>
#include "Logger.h"
#include <syslog.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sstream>
#include "NovaUtil.h"
#include <math.h>

using namespace std;

namespace Nova
{

string Config::m_prefixes[] = {
		"INTERFACE",
		"HS_HONEYD_CONFIG",
		"TCP_TIMEOUT",
		"TCP_CHECK_FREQ",
		"READ_PCAP",
		"PCAP_FILE",
		"GO_TO_LIVE",
		"USE_TERMINALS",
		"CLASSIFICATION_TIMEOUT",
		"SILENT_ALARM_PORT",
		"K",
		"EPS",
		"IS_TRAINING",
		"CLASSIFICATION_THRESHOLD",
		"DATAFILE",
		"SA_MAX_ATTEMPTS",
		"SA_SLEEP_DURATION",
		"DM_HONEYD_CONFIG",
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
		"SERVICE_PREFERENCES"
};

// Files we need to run (that will be loaded with defaults if deleted)
string Config::m_requiredFiles[] = {
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
	if (m_instance == NULL)
		m_instance = new Config();
	return m_instance;
}

// Loads the configuration file into the class's state data
void Config::LoadConfig()
{
	pthread_rwlock_wrlock(&lock);

	string line;
	string prefix;
	int prefixIndex;

	bool isValid[sizeof(m_prefixes)/sizeof(m_prefixes[0])];

	openlog("Novaconfiguration", LOG_CONS | LOG_PID | LOG_NDELAY | LOG_PERROR, LOG_AUTHPRIV);

	ifstream config(m_configFilePath.c_str());

	if (config.is_open())
	{
		while (config.good())
		{
			getline(config, line);
			prefixIndex = 0;
			prefix = m_prefixes[prefixIndex];


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
						m_interface = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// HS_HONEYD_CONFIG
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (line.size() > 0)
				{
					m_pathConfigHoneydHs  = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// TCP_TIMEOUT
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (atoi(line.c_str()) > 0)
				{
					m_tcpTimout = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// TCP_CHECK_FREQ
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (atoi(line.c_str()) > 0)
				{
					m_tcpCheckFreq = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// READ_PCAP
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (atoi(line.c_str()) == 0 || atoi(line.c_str()) == 1)
				{
					m_readPcap = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// PCAP_FILE
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (line.size() > 0)
				{
					m_pathPcapFile = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// GO_TO_LIVE
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (line.size() > 0)
				{
					m_gotoLive = line.c_str();
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// USE_TERMINALS
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (atoi(line.c_str()) == 0 || atoi(line.c_str()) == 1)
				{
					m_useTerminals = line.c_str();
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// CLASSIFICATION_TIMEOUT
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (atoi(line.c_str()) >= 0)
				{
					m_classificationTimeout = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// SILENT_ALARM_PORT
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				if(line.size() == prefix.size())
				{
					line += " ";
				}

				line = line.substr(prefix.size() + 1, line.size());

				if (atoi(line.c_str()) > 0)
				{
					m_saPort = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// K
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (atoi(line.c_str()) > 0)
				{
					m_k = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// EPS
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (atof(line.c_str()) >= 0)
				{
					m_eps = atof(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// IS_TRAINING
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (atoi(line.c_str()) == 0 || atoi(line.c_str()) == 1)
				{
					m_isTraining = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// CLASSIFICATION_THRESHOLD
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (atof(line.c_str()) >= 0)
				{
					m_classificationThreshold= atof(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// DATAFILE
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (line.size() > 0 && !line.substr(line.size() - 4,
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
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (atoi(line.c_str()) > 0)
				{
					m_saMaxAttempts = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// SA_SLEEP_DURATION
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (atof(line.c_str()) >= 0)
				{
					m_saSleepDuration = atof(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// DM_HONEYD_CONFIG
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (line.size() > 0)
				{
					m_pathConfigHoneydDm = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}


			// DOPPELGANGER_IP
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (line.size() > 0)
				{
					m_doppelIp = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// DOPPELGANGER_INTERFACE
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (line.size() > 0)
				{
					m_doppelInterface = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// DM_ENABLED
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (atoi(line.c_str()) == 0 || atoi(line.c_str()) == 1)
				{
					m_isDmEnabled = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;

			}

			// ENABLED_FEATURES
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (line.size() == DIM)
				{
					setEnabledFeatures_noLocking(line);
					isValid[prefixIndex] = true;
				}
				continue;

			}

			// TRAINING_CAP_FOLDER
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (line.size() > 0)
				{
					m_pathTrainingCapFolder = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// THINNING_DISTANCE
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (atof(line.c_str()) > 0)
				{
					m_thinningDistance = atof(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// SAVE_FREQUENCY
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (atoi(line.c_str()) > 0)
				{
					m_saveFreq = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// DATA_TTL
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (atoi(line.c_str()) >= 0)
				{
					m_dataTTL = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// CE_SAVE_FILE
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (line.size() > 0)
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
					SMTPAddr = line;
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
					SMTPPort = (((in_port_t) atoi(line.c_str())));
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
					SMTPDomain = line;
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
					setSMTPEmailRecipients_noLocking(line);
					isValid[prefixIndex] = true;
				}

				continue;
			}

			//SERVICE_PREFERENCES
			//TODO: make method for parsing string to map criticality level to service
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());

				if(line.size() > 0)
				{
					loggerPreferences = line;
					isValid[prefixIndex] = true;
				}

				continue;
			}
		}
	}
	else
	{
		//TODO replace with LOG()
		syslog(SYSL_INFO, "Line: %d No configuration file found.", __LINE__); //TODO replace with LOG()
	}


	for (uint i = 0; i < sizeof(m_prefixes)/sizeof(m_prefixes[0]); i++)
	{
		if (!isValid[i])
		{
			// TODO: Make this say which config option is invalid again
			syslog(SYSL_INFO, "Line: %d Configuration option %s is invalid in the configuration file", __LINE__, m_prefixes[i].c_str());
			//TODO replace with LOG()
		}
	}
	closelog();

	pthread_rwlock_unlock(&lock);
}

bool Config::LoadUserConfig()
{
	pthread_rwlock_wrlock(&lock);

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
						syslog(SYSL_ERR, "Line: %d Invalid IP address parsed on line %d of the settings file.", __LINE__, i);
						//TODO replace with LOG()
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
					m_key = line;
				else
				{
					syslog(SYSL_ERR, "Line: %d Invalid Key parsed on line %d of the settings file.", __LINE__, i);
					//TODO replace with LOG()
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

	pthread_rwlock_unlock(&lock);
	return returnValue;
}

bool Config::LoadPaths()
{
	pthread_rwlock_wrlock(&lock);

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
				pathHome = ResolvePathVars(line);
				continue;
			}

			prefix = "NOVA_RD";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				pathReadFolder = ResolvePathVars(line);
				continue;
			}

			prefix = "NOVA_WR";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				pathWriteFolder = ResolvePathVars(line);
				continue;
			}

			prefix = "NOVA_BIN";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				pathBinaries = ResolvePathVars(line);
				continue;
			}

			prefix = "NOVA_ICON";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				pathIcon = ResolvePathVars(line);
				continue;
			}
		}
	}
	paths->close();
	delete paths;
	pthread_rwlock_unlock(&lock);

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
		return var;
	else return path;
}

bool Config::SaveConfig()
{
	pthread_rwlock_rdlock(&lock);
	string line, prefix;

	//Rewrite the config file with the new settings
	string configurationBackup = m_configFilePath + ".tmp";
	string copyCommand = "cp -f " + m_configFilePath + " " + configurationBackup;
	if(system(copyCommand.c_str()) != 0)
	{
		//TODO ERROR handling
	}

	ifstream *in = new ifstream(configurationBackup.c_str());
	ofstream *out = new ofstream(m_configFilePath.c_str());

	if(out->is_open() && in->is_open())
	{
		while(in->good())
		{
			if (!getline(*in, line))
			{
				continue;
			}

			prefix = "DM_ENABLED";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				if(getIsDmEnabled())
					*out << "DM_ENABLED 1"<<endl;
				else
					*out << "DM_ENABLED 0"<<endl;
				continue;
			}

			prefix = "IS_TRAINING";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				if(getIsTraining())
					*out << "IS_TRAINING 1"<<endl;
				else
					*out << "IS_TRAINING 0"<<endl;
				continue;
			}

			prefix = "INTERFACE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << getInterface() << endl;
				continue;
			}

			prefix = "DATAFILE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << getPathTrainingFile() << endl;
				continue;
			}

			prefix = "SA_SLEEP_DURATION";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << getSaSleepDuration() << endl;
				continue;
			}

			prefix = "SA_MAX_ATTEMPTS";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << getSaMaxAttempts() << endl;
				continue;
			}

			prefix = "SILENT_ALARM_PORT";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << getSaPort() << endl;
				continue;
			}

			prefix = "K";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << getK() << endl;
				continue;
			}

			prefix = "EPS";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << getEps() << endl;
				continue;
			}

			prefix = "CLASSIFICATION_TIMEOUT";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << getClassificationTimeout() << endl;
				continue;
			}

			prefix = "CLASSIFICATION_THRESHOLD";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << getClassificationThreshold() << endl;
				continue;
			}

			prefix = "DM_HONEYD_CONFIG";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << getPathConfigHoneydDm() << endl;
				continue;
			}

			prefix = "DOPPELGANGER_IP";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << getDoppelIp() << endl;
				continue;
			}

			prefix = "HS_HONEYD_CONFIG";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << getPathConfigHoneydHs() << endl;
				continue;
			}

			prefix = "TCP_TIMEOUT";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << getTcpTimout() << endl;
				continue;
			}

			prefix = "TCP_CHECK_FREQ";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << getTcpCheckFreq()  << endl;
				continue;
			}

			prefix = "PCAP_FILE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " <<  getPathPcapFile() << endl;
				continue;
			}

			prefix = "ENABLED_FEATURES";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << getEnabledFeatures() << endl;
				continue;
			}

			prefix = "READ_PCAP";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				if(getReadPcap())
					*out << "READ_PCAP 1"<< endl;
				else
					*out << "READ_PCAP 0"<< endl;
				continue;
			}

			prefix = "GO_TO_LIVE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				if(getGotoLive())
					*out << "GO_TO_LIVE 1" << endl;
				else
					*out << "GO_TO_LIVE 0" << endl;
				continue;
			}

			prefix = "USE_TERMINALS";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				if(getUseTerminals())
					*out << "USE_TERMINALS 1" << endl;
				else
					*out << "USE_TERMINALS 0" << endl;
				continue;
			}

			*out << line << endl;
		}
	}
	else
	{
		openlog("Novaconfiguration", OPEN_SYSL, LOG_AUTHPRIV);
		syslog(SYSL_ERR, "File: %s Line: %d Error writing to Nova config file.", __FILE__, __LINE__);
		//TODO replace with LOG()
		closelog();
		in->close();
		out->close();
		delete in;
		delete out;

		pthread_rwlock_unlock(&lock);
		return false;
	}

	in->close();
	out->close();
	delete in;
	delete out;
	if(system("rm -f Config/.NOVAConfig.tmp") != 0)
	{
		//TODO ERROR handling.
	}
	pthread_rwlock_unlock(&lock);
	return true;
}



void Config::SetDefaults()
{
	openlog(__FUNCTION__, OPEN_SYSL, LOG_AUTHPRIV);

	m_interface = "default";
	m_pathConfigHoneydHs 	= "Config/haystack.config";
	m_pathPcapFile 		= "../pcapfile";
	m_pathTrainingFile 	= "Data/data.txt";
	m_pathConfigHoneydDm	= "Config/doppelganger.config";
	m_pathTrainingCapFolder = "Data";
	m_pathCESaveFile = "ceStateSave";

	m_tcpTimout = 7;
	m_tcpCheckFreq = 3;
	m_readPcap = false;
	m_gotoLive = true;
	m_useTerminals = false;
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

bool Config::AddUserToGroup()
{
	bool returnValue = true;
	if( system("gksudo --description 'Add your user to the privileged nova user group. "
			"(Required for Nova to run)'  \"usermod -a -G nova $USER\"") != 0)
	{
		syslog(SYSL_ERR, "File: %s Line: %d bind: %s", __FILE__, __LINE__, "Was not able to add user to the 'nova' group");
		//TODO replace with LOG()
		returnValue = false;
	}
	else
	{
		//TODO replace with LOG()
		syslog(SYSL_INFO, "Added your user to the 'nova' group. You may need to log in and out for this to take affect.");
	}
	return returnValue;
}

// Checks to see if the current user has a ~/.nova directory, and creates it if not, along with default config files
//	Returns: True if (after the function) the user has all necessary ~/.nova config files
//		IE: Returns false only if the user doesn't have configs AND we weren't able to make them
bool Config::InitUserConfigs(string homeNovaPath)
{
	bool returnValue = true;
	struct stat fileAttr;
	char buffer[256];

	// Does ~/.nova exist?
	if ( stat( homeNovaPath.c_str(), &fileAttr ) == 0)
	{
		// Are we in the nova group? Run 'groups' and parse output
		string groupsResult = "";
		FILE* pipe = popen("groups", "r");
		while(!feof(pipe))
			if(fgets(buffer, 256, pipe) != NULL)
				groupsResult += buffer;

		stringstream ss(groupsResult);
		string group;
		bool found = false;
		while (ss >> group)
			if (group == "nova")
				found = true;

		// Add them to the group if need be
		if (!found)
			if (!AddUserToGroup())
				returnValue = false;

		// Do all of the important files exist?
		for (uint i = 0; i < sizeof(m_requiredFiles)/sizeof(m_requiredFiles[0]); i++)
		{
			string fullPath = homeNovaPath + Config::m_requiredFiles[i];
			if (stat (fullPath.c_str(), &fileAttr ) != 0)
			{
				string defaultLocation = "/etc/nova/.nova" + Config::m_requiredFiles[i];
				string copyCommand = "cp -fr " + defaultLocation + " " + fullPath;
				//TODO replace with LOG()
				syslog(SYSL_ERR, "Warning: The file %s does not exist but is required for Nova to function. Restoring default file from %s",fullPath.c_str(), defaultLocation.c_str());
				if (system(copyCommand.c_str()) == -1)
				{
					//TODO replace with LOG()
					syslog(SYSL_ERR, "Error: Unable to copy file %s to %s.",fullPath.c_str(), defaultLocation.c_str());
				}
			}
		}
	}
	else
	{
		// Try adding the user to the nova group
		if (!AddUserToGroup())
			returnValue = false;

		//TODO: Do this command programmatically. Not by calling system()
		if( system("cp -rf /etc/nova/.nova $HOME") == -1)
		{
			syslog(SYSL_ERR, "File: %s Line: %d bind: %s", __FILE__, __LINE__, "Was not able to create user $HOME/.nova directory");
			//TODO replace with LOG()
			returnValue = false;
		}

		//Check the ~/.nova dir again
		if ( stat( homeNovaPath.c_str(), &fileAttr ) == 0)
			return returnValue;
		else
		{
			syslog(SYSL_ERR, "File: %s Line: %d bind: %s", __FILE__, __LINE__, "Was not able to create user $HOME/.nova directory");
			//TODO replace with LOG()
			returnValue = false;
		}
	}

	return returnValue;
}

string Config::toString()
{
	pthread_rwlock_rdlock(&lock);

	std::stringstream ss;
	ss << "getConfigFilePath() " << getConfigFilePath() << endl;
	ss << "getDoppelInterface() " << getDoppelInterface() << endl;
	ss << "getDoppelIp() " << getDoppelIp() << endl;
	ss << "getEnabledFeatures() " << getEnabledFeatures() << endl;
	ss << "getInterface() " << getInterface() << endl;
	ss << "getPathCESaveFile() " << getPathCESaveFile() << endl;
	ss << "getPathConfigHoneydDm() " << getPathConfigHoneydDm() << endl;
	ss << "getPathConfigHoneydHs() " << getPathConfigHoneydHs() << endl;
	ss << "getPathPcapFile() " << getPathPcapFile() << endl;
	ss << "getPathTrainingCapFolder() " << getPathTrainingCapFolder() << endl;
	ss << "getPathTrainingFile() " << getPathTrainingFile() << endl;

	ss << "getReadPcap() " << getReadPcap() << endl;
	ss << "getUseTerminals() " << getUseTerminals() << endl;
	ss << "getIsDmEnabled() " << getIsDmEnabled() << endl;
	ss << "getIsTraining() " << getIsTraining() << endl;
	ss << "getGotoLive() " << getGotoLive() << endl;

	ss << "getClassificationTimeout() " << getClassificationTimeout() << endl;
	ss << "getDataTTL() " << getDataTTL() << endl;
	ss << "getK() " << getK() << endl;
	ss << "getSaMaxAttempts() " << getSaMaxAttempts() << endl;
	ss << "getSaPort() " << getSaPort() << endl;
	ss << "getSaveFreq() " << getSaveFreq() << endl;
	ss << "getTcpCheckFreq() " << getTcpCheckFreq() << endl;
	ss << "getTcpTimout() " << getTcpTimout() << endl;
	ss << "getThinningDistance() " << getThinningDistance() << endl;

	ss << "getClassificationThreshold() " << getClassificationThreshold() << endl;
	ss << "getSaSleepDuration() " << getSaSleepDuration() << endl;
	ss << "getEps() " << getEps() << endl;

	pthread_rwlock_unlock(&lock);
	return ss.str();
}

Config::Config()
{
	pthread_rwlock_init(&lock, NULL);
	LoadPaths();

	openlog("NovaConfigurator", OPEN_SYSL, LOG_AUTHPRIV);

	if(!this->InitUserConfigs(this->getPathHome()))
	{
		//TODO replace with LOG()
		syslog(SYSL_ERR, "Error: InitUserConfigs failed. Your home folder and permissions may not have been configured properly");
		//exit(EXIT_FAILURE);
	}

	m_configFilePath = getPathHome() + "/Config/NOVAConfig.txt";
	m_userConfigFilePath = getPathHome() + "/settings";
	SetDefaults();
	LoadConfig();
	LoadUserConfig();
}

Config::~Config()
{
}

double Config::getClassificationThreshold()
{
	pthread_rwlock_rdlock(&lock);
	double m_classificationThreshold = this->m_classificationThreshold;
	pthread_rwlock_unlock(&lock);
	return m_classificationThreshold;
}

int Config::getClassificationTimeout()
{
	pthread_rwlock_rdlock(&lock);
	int m_classificationTimeout = this->m_classificationTimeout;
	pthread_rwlock_unlock(&lock);
	return m_classificationTimeout;
}

string Config::getConfigFilePath()
{
	pthread_rwlock_rdlock(&lock);
	string m_configFilePath = this->m_configFilePath;
	pthread_rwlock_unlock(&lock);
	return m_configFilePath;
}

int Config::getDataTTL()
{
	pthread_rwlock_rdlock(&lock);
	int m_dataTTL = this->m_dataTTL;
	pthread_rwlock_unlock(&lock);
	return m_dataTTL;
}

string Config::getDoppelInterface()
{
	pthread_rwlock_rdlock(&lock);
	string m_doppelInterface = this->m_doppelInterface;
	pthread_rwlock_unlock(&lock);
	return m_doppelInterface;
}

string Config::getDoppelIp()
{
	pthread_rwlock_rdlock(&lock);
	string m_doppelIp = this->m_doppelIp;
	pthread_rwlock_unlock(&lock);
	return m_doppelIp;
}

string Config::getEnabledFeatures()
{
	pthread_rwlock_rdlock(&lock);
	string m_enabledFeatureMask = this->m_enabledFeatureMask;
	pthread_rwlock_unlock(&lock);
	return m_enabledFeatureMask;
}

uint Config::getEnabledFeatureCount()
{
	pthread_rwlock_rdlock(&lock);
	uint m_enabledFeatureCount = this->m_enabledFeatureCount;
	pthread_rwlock_unlock(&lock);
	return m_enabledFeatureCount;
}

bool Config::isFeatureEnabled(int i)
{
	return m_isFeatureEnabled[i];
}

double Config::getEps()
{
	pthread_rwlock_rdlock(&lock);
	double m_eps = this->m_eps;
	pthread_rwlock_unlock(&lock);
	return m_eps;
}

bool Config::getGotoLive()
{
	pthread_rwlock_rdlock(&lock);
	bool m_gotoLive = this->m_gotoLive;
	pthread_rwlock_unlock(&lock);
	return m_gotoLive;
}

string Config::getInterface()
{
	pthread_rwlock_rdlock(&lock);
	string m_interface = this->m_interface;
	pthread_rwlock_unlock(&lock);
	return m_interface;
}

bool Config::getIsDmEnabled()
{
	pthread_rwlock_rdlock(&lock);
	bool m_isDmEnabled = this->m_isDmEnabled;
	pthread_rwlock_unlock(&lock);
	return m_isDmEnabled;
}

bool Config::getIsTraining()
{
	pthread_rwlock_rdlock(&lock);
	bool m_isTraining = this->m_isTraining;
	pthread_rwlock_unlock(&lock);
	return m_isTraining;
}

int Config::getK()
{
	pthread_rwlock_rdlock(&lock);
	int m_k = this->m_k;
	pthread_rwlock_unlock(&lock);
	return m_k;
}

string Config::getPathCESaveFile()
{
	pthread_rwlock_rdlock(&lock);
	string m_pathCESaveFile = this->m_pathCESaveFile;
	pthread_rwlock_unlock(&lock);
	return m_pathCESaveFile;
}

string Config::getPathConfigHoneydDm()
{
	pthread_rwlock_rdlock(&lock);
	string m_pathConfigHoneydDm = this->m_pathConfigHoneydDm;
	pthread_rwlock_unlock(&lock);
	return m_pathConfigHoneydDm;
}

string Config::getPathConfigHoneydHs()
{
	pthread_rwlock_rdlock(&lock);
	string m_pathConfigHoneydHs = this->m_pathConfigHoneydHs;
	pthread_rwlock_unlock(&lock);
	return m_pathConfigHoneydHs;
}

string Config::getPathPcapFile()
{
	pthread_rwlock_rdlock(&lock);
	string m_pathPcapFile = this->m_pathPcapFile;
	pthread_rwlock_unlock(&lock);
	return m_pathPcapFile;
}

string Config::getPathTrainingCapFolder()
{
	pthread_rwlock_rdlock(&lock);
	string m_pathTrainingCapFolder = this->m_pathTrainingCapFolder;
	pthread_rwlock_unlock(&lock);
	return m_pathTrainingCapFolder;
}

string Config::getPathTrainingFile()
{
	pthread_rwlock_rdlock(&lock);
	string m_pathTrainingFile = this->m_pathTrainingFile;
	pthread_rwlock_unlock(&lock);
	return m_pathTrainingFile;
}

bool Config::getReadPcap()
{
	pthread_rwlock_rdlock(&lock);
	bool m_readPcap = this->m_readPcap;
	pthread_rwlock_unlock(&lock);
	return m_readPcap;
}

int Config::getSaMaxAttempts()
{
	pthread_rwlock_rdlock(&lock);
	int m_saMaxAttempts = this->m_saMaxAttempts;
	pthread_rwlock_unlock(&lock);
	return m_saMaxAttempts;
}

int Config::getSaPort()
{
	pthread_rwlock_rdlock(&lock);
	int m_saPort = this->m_saPort;
	pthread_rwlock_unlock(&lock);
	return m_saPort;
}

double Config::getSaSleepDuration()
{
	pthread_rwlock_rdlock(&lock);
	double m_saSleepDuration = this->m_saSleepDuration;
	pthread_rwlock_unlock(&lock);
	return m_saSleepDuration;
}

int Config::getSaveFreq()
{
	pthread_rwlock_rdlock(&lock);
	int m_saveFreq = this->m_saveFreq;
	pthread_rwlock_unlock(&lock);
	return m_saveFreq;
}

int Config::getTcpCheckFreq()
{
	pthread_rwlock_rdlock(&lock);
	int m_tcpCheckFreq = this->m_tcpCheckFreq;
	pthread_rwlock_unlock(&lock);
	return m_tcpCheckFreq;
}

int Config::getTcpTimout()
{
	pthread_rwlock_rdlock(&lock);
	int m_tcpTimout = this->m_tcpTimout;
	pthread_rwlock_unlock(&lock);
	return m_tcpTimout;
}

int Config::getThinningDistance()
{
	pthread_rwlock_rdlock(&lock);
	int m_thinningDistance = this->m_thinningDistance;
	pthread_rwlock_unlock(&lock);
	return m_thinningDistance;
}

bool Config::getUseTerminals()
{
	pthread_rwlock_rdlock(&lock);
	bool m_useTerminals = this->m_useTerminals;
	pthread_rwlock_unlock(&lock);
	return m_useTerminals;
}

string Config::getKey()
{
	pthread_rwlock_rdlock(&lock);
	string m_key = this->m_key;
	pthread_rwlock_unlock(&lock);
	return m_key;
}

vector<in_addr_t> Config::getNeighbors()
{
	pthread_rwlock_rdlock(&lock);
	vector<in_addr_t> m_neighbors = this->m_neighbors;
	pthread_rwlock_unlock(&lock);
	return m_neighbors;
}

string Config::getGroup()
{
	pthread_rwlock_rdlock(&lock);
	string m_group = this->m_group;
	pthread_rwlock_unlock(&lock);
	return m_group;
}

void Config::setClassificationThreshold(double classificationThreshold)
{
	pthread_rwlock_wrlock(&lock);
	this->m_classificationThreshold = classificationThreshold;
	pthread_rwlock_unlock(&lock);
}

void Config::setClassificationTimeout(int classificationTimeout)
{
	pthread_rwlock_wrlock(&lock);
	this->m_classificationTimeout = classificationTimeout;
	pthread_rwlock_unlock(&lock);
}

void Config::setConfigFilePath(string configFilePath)
{
	pthread_rwlock_wrlock(&lock);
	this->m_configFilePath = configFilePath;
	pthread_rwlock_unlock(&lock);
}

void Config::setDataTTL(int dataTTL)
{
	pthread_rwlock_wrlock(&lock);
	this->m_dataTTL = dataTTL;
	pthread_rwlock_unlock(&lock);
}

void Config::setDoppelInterface(string doppelInterface)
{
	pthread_rwlock_wrlock(&lock);
	this->m_doppelInterface = doppelInterface;
	pthread_rwlock_unlock(&lock);
}

void Config::setDoppelIp(string doppelIp)
{
	pthread_rwlock_wrlock(&lock);
	this->m_doppelIp = doppelIp;
	pthread_rwlock_unlock(&lock);
}

void Config::setEnabledFeatures_noLocking(string enabledFeatureMask)
{
	m_enabledFeatureCount = 0;
	for (uint i = 0; i < DIM; i++)
	{
		if ('1' == enabledFeatureMask.at(i))
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


void Config::setEnabledFeatures(string enabledFeatureMask)
{
	pthread_rwlock_wrlock(&lock);
	setEnabledFeatures_noLocking(enabledFeatureMask);
	pthread_rwlock_rdlock(&lock);
}

void Config::setEps(double eps)
{
	pthread_rwlock_wrlock(&lock);
	this->m_eps = eps;
	pthread_rwlock_unlock(&lock);
}

void Config::setGotoLive(bool gotoLive)
{
	pthread_rwlock_wrlock(&lock);
	this->m_gotoLive = gotoLive;
	pthread_rwlock_unlock(&lock);
}

void Config::setInterface(string interface)
{
	pthread_rwlock_wrlock(&lock);
	this->m_interface = interface;
	pthread_rwlock_unlock(&lock);
}

void Config::setIsDmEnabled(bool isDmEnabled)
{
	pthread_rwlock_wrlock(&lock);
	this->m_isDmEnabled = isDmEnabled;
	pthread_rwlock_unlock(&lock);
}

void Config::setIsTraining(bool isTraining)
{
	pthread_rwlock_wrlock(&lock);
	this->m_isTraining = isTraining;
	pthread_rwlock_unlock(&lock);
}

void Config::setK(int k)
{
	pthread_rwlock_wrlock(&lock);
	this->m_k = k;
	pthread_rwlock_unlock(&lock);
}

void Config::setPathCESaveFile(string pathCESaveFile)
{
	pthread_rwlock_wrlock(&lock);
	this->m_pathCESaveFile = pathCESaveFile;
	pthread_rwlock_unlock(&lock);
}

void Config::setPathConfigHoneydDm(string pathConfigHoneydDm)
{
	pthread_rwlock_wrlock(&lock);
	this->m_pathConfigHoneydDm = pathConfigHoneydDm;
	pthread_rwlock_unlock(&lock);
}

void Config::setPathConfigHoneydHs(string pathConfigHoneydHs)
{
	pthread_rwlock_wrlock(&lock);
	this->m_pathConfigHoneydHs = pathConfigHoneydHs;
	pthread_rwlock_unlock(&lock);
}

void Config::setPathPcapFile(string pathPcapFile)
{
	pthread_rwlock_wrlock(&lock);
	this->m_pathPcapFile = pathPcapFile;
	pthread_rwlock_unlock(&lock);
}

void Config::setPathTrainingCapFolder(string pathTrainingCapFolder)
{
	pthread_rwlock_wrlock(&lock);
	this->m_pathTrainingCapFolder = pathTrainingCapFolder;
	pthread_rwlock_unlock(&lock);
}

void Config::setPathTrainingFile(string pathTrainingFile)
{
	pthread_rwlock_wrlock(&lock);
	this->m_pathTrainingFile = pathTrainingFile;
	pthread_rwlock_unlock(&lock);
}

void Config::setReadPcap(bool readPcap)
{
	pthread_rwlock_wrlock(&lock);
	this->m_readPcap = readPcap;
	pthread_rwlock_unlock(&lock);
}

void Config::setSaMaxAttempts(int saMaxAttempts)
{
	pthread_rwlock_wrlock(&lock);
	this->m_saMaxAttempts = saMaxAttempts;
	pthread_rwlock_unlock(&lock);
}

void Config::setSaPort(int saPort)
{
	pthread_rwlock_wrlock(&lock);
	this->m_saPort = saPort;
	pthread_rwlock_unlock(&lock);
}

void Config::setSaSleepDuration(double saSleepDuration)
{
	pthread_rwlock_wrlock(&lock);
	this->m_saSleepDuration = saSleepDuration;
	pthread_rwlock_unlock(&lock);
}

void Config::setSaveFreq(int saveFreq)
{
	pthread_rwlock_wrlock(&lock);
	this->m_saveFreq = saveFreq;
	pthread_rwlock_unlock(&lock);
}

void Config::setTcpCheckFreq(int tcpCheckFreq)
{
	pthread_rwlock_wrlock(&lock);
	this->m_tcpCheckFreq = tcpCheckFreq;
	pthread_rwlock_unlock(&lock);
}

void Config::setTcpTimout(int tcpTimout)
{
	pthread_rwlock_wrlock(&lock);
	this->m_tcpTimout = tcpTimout;
	pthread_rwlock_unlock(&lock);
}

void Config::setThinningDistance(int thinningDistance)
{
	pthread_rwlock_wrlock(&lock);
	this->m_thinningDistance = thinningDistance;
	pthread_rwlock_unlock(&lock);
}

void Config::setUseTerminals(bool useTerminals)
{
	pthread_rwlock_wrlock(&lock);
	this->m_useTerminals = useTerminals;
	pthread_rwlock_unlock(&lock);
}

void Config::setKey(string key)
{
	pthread_rwlock_wrlock(&lock);
	this->m_key = key;
	pthread_rwlock_unlock(&lock);
}

void Config::setNeigbors(vector<in_addr_t> neighbors)
{
	pthread_rwlock_wrlock(&lock);
	this->m_neighbors = neighbors;
	pthread_rwlock_unlock(&lock);
}

void Config::setGroup(string group)
{
	pthread_rwlock_wrlock(&lock);
	m_group = group;
	pthread_rwlock_unlock(&lock);
}

string Config::getLoggerPreferences()
{
	pthread_rwlock_rdlock(&lock);
	string loggerPreferences = this->loggerPreferences;
	pthread_rwlock_unlock(&lock);
	return loggerPreferences;
}

string Config::getSMTPAddr()
{
	pthread_rwlock_rdlock(&lock);
	string SMTPAddr = this->SMTPAddr;
	pthread_rwlock_unlock(&lock);
	return SMTPAddr;
}

string Config::getSMTPDomain()
{
	pthread_rwlock_rdlock(&lock);
	string SMTPDomain = this->SMTPDomain;
	pthread_rwlock_unlock(&lock);
	return SMTPDomain;
}

vector<string> Config::getSMTPEmailRecipients()
{
	pthread_rwlock_rdlock(&lock);
	vector<string> SMTPEmailRecipients = this->SMTPEmailRecipients;
	pthread_rwlock_unlock(&lock);
	return SMTPEmailRecipients;
}

in_port_t Config::getSMTPPort()
{
	pthread_rwlock_rdlock(&lock);
	in_port_t SMTPPort = this->SMTPPort;
	pthread_rwlock_unlock(&lock);
	return SMTPPort;
}

void Config::setLoggerPreferences(string loggerPreferences)
{
	pthread_rwlock_wrlock(&lock);
	this->loggerPreferences = loggerPreferences;
	pthread_rwlock_unlock(&lock);
}

void Config::setSMTPAddr(string SMTPAddr)
{
	pthread_rwlock_wrlock(&lock);
	this->SMTPAddr = SMTPAddr;
	pthread_rwlock_unlock(&lock);
}

void Config::setSMTPDomain(string SMTPDomain)
{
	pthread_rwlock_wrlock(&lock);
	this->SMTPDomain = SMTPDomain;
	pthread_rwlock_unlock(&lock);
}

void Config::setSMTPEmailRecipients(vector<string> SMTPEmailRecipients)
{
	pthread_rwlock_wrlock(&lock);
	this->SMTPEmailRecipients = SMTPEmailRecipients;
	pthread_rwlock_unlock(&lock);
}

void Config::setSMTPEmailRecipients_noLocking(string SMTPEmailRecipients)
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

	this->SMTPEmailRecipients = out;
}

void Config::setSMTPPort(in_port_t SMTPPort)
{
	pthread_rwlock_wrlock(&lock);
	this->SMTPPort = SMTPPort;
	pthread_rwlock_unlock(&lock);
}

string Config::getPathBinaries()
{
	pthread_rwlock_rdlock(&lock);
	string pathBinaries = this->pathBinaries;
	pthread_rwlock_unlock(&lock);
	return pathBinaries;
}

string Config::getPathWriteFolder()
{
	pthread_rwlock_rdlock(&lock);
	string pathWriteFolder = this->pathWriteFolder;
	pthread_rwlock_unlock(&lock);
	return pathWriteFolder;
}

string Config::getPathReadFolder()
{
	pthread_rwlock_rdlock(&lock);
	string pathReadFolder = this->pathReadFolder;
	pthread_rwlock_unlock(&lock);
	return pathReadFolder;
}

string Config::getPathIcon()
{
	pthread_rwlock_rdlock(&lock);
	string pathIcon= this->pathIcon;
	pthread_rwlock_unlock(&lock);
	return pathIcon;
}

string Config::getPathHome()
{
	pthread_rwlock_rdlock(&lock);
	string pathHome = this->pathHome;
	pthread_rwlock_unlock(&lock);
	return pathHome;
}

double Config::getSqurtEnabledFeatures()
{
	pthread_rwlock_rdlock(&lock);
	double m_squrtEnabledFeatures= this->m_squrtEnabledFeatures;
	pthread_rwlock_unlock(&lock);
	return m_squrtEnabledFeatures;
}

}

