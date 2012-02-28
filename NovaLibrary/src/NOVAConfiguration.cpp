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

#include "NOVAConfiguration.h"
#include <fstream>
#include "Logger.h"
#include <syslog.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sstream>

using namespace std;

namespace Nova
{

const string NOVAConfiguration::m_prefixes[] = 	{ "INTERFACE", "HS_HONEYD_CONFIG",
		"TCP_TIMEOUT", "TCP_CHECK_FREQ", "READ_PCAP", "PCAP_FILE", "GO_TO_LIVE",
		"USE_TERMINALS", "CLASSIFICATION_TIMEOUT", "SILENT_ALARM_PORT", "K", "EPS",
		"IS_TRAINING", "CLASSIFICATION_THRESHOLD", "DATAFILE", "SA_MAX_ATTEMPTS",
		"SA_SLEEP_DURATION", "DM_HONEYD_CONFIG", "DOPPELGANGER_IP", "DOPPELGANGER_INTERFACE",
		"DM_ENABLED", "ENABLED_FEATURES","TRAINING_CAP_FOLDER", "THINNING_DISTANCE",
		"SAVE_FREQUENCY", "DATA_TTL", "CE_SAVE_FILE"};

// Files we need to run (that will be loaded with defaults if deleted)
const string NOVAConfiguration::m_requiredFiles[] = {
		"/settings",
		"/Config/NOVAConfig.txt",
		"/Data/data.txt",

		"/scripts.xml",
		"/templates/ports.xml",
		"/templates/profiles.xml",
		"/templates/routes.xml",
		"/templates/nodes.xml"
};

// Loads the configuration file into the class's state data
void NOVAConfiguration::LoadConfig()
{
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
					m_enabledFeatures = line;
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
		}
	}
	else
	{
		syslog(SYSL_INFO, "Line: %d No configuration file found.", __LINE__);
	}


	for (uint i = 0; i < sizeof(m_prefixes)/sizeof(m_prefixes[0]); i++)
	{
		if (!isValid[i])
		{
			// TODO: Make this say which config option is invalid again
			syslog(SYSL_INFO, "Line: %d Configuration option %s is invalid in the configuration file", __LINE__, m_prefixes[i].c_str());
		}
	}
	closelog();
}

bool NOVAConfiguration::SaveConfig() {
	string line, prefix;

	//Rewrite the config file with the new settings
	string configurationBackup = m_configFilePath + ".tmp";
	string copyCommand = "cp -f " + m_configFilePath + " " + configurationBackup;
	system(copyCommand.c_str());
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
		closelog();
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
	system("rm -f Config/.NOVAConfig.tmp");

	return true;
}



void NOVAConfiguration::SetDefaults()
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
	m_enabledFeatures = "111111111";
	m_thinningDistance = 0;
	m_saveFreq = 1440;
	m_dataTTL = 0;
}

// Checks to see if the current user has a ~/.nova directory, and creates it if not, along with default config files
//	Returns: True if (after the function) the user has all necessary ~/.nova config files
//		IE: Returns false only if the user doesn't have configs AND we weren't able to make them
bool NOVAConfiguration::InitUserConfigs(string homeNovaPath)
{
	bool returnValue = true;
	struct stat fileAttr;
	//TODO: Do a proper check to make sure all config files exist, not just the .nova dir
	if ( stat( homeNovaPath.c_str(), &fileAttr ) == 0)
	{
		// Do all of the important files exist?
		for (uint i = 0; i < sizeof(m_requiredFiles)/sizeof(m_requiredFiles[0]); i++)
		{
			string fullPath = homeNovaPath + NOVAConfiguration::m_requiredFiles[i];
			if (stat (fullPath.c_str(), &fileAttr ) != 0)
			{
				string defaultLocation = "/etc/nova/.nova" + NOVAConfiguration::m_requiredFiles[i];
				string copyCommand = "cp -fr " + defaultLocation + " " + fullPath;
				syslog(SYSL_ERR, "Warning: The file %s does not exist but is required for Nova to function. Restoring default file from %s",fullPath.c_str(), defaultLocation.c_str());
				if (system(copyCommand.c_str()) == -1)
				{
					syslog(SYSL_ERR, "Error: Unable to copy file %s to %s.",fullPath.c_str(), defaultLocation.c_str());
				}
			}
		}
		return returnValue;
	}
	else
	{
		if( system("gksudo --description 'Add your user to the privileged nova user group. "
				"(Required for Nova to run)'  \"usermod -a -G nova $USER\"") != 0)
		{
			syslog(SYSL_ERR, "File: %s Line: %d bind: %s", __FILE__, __LINE__, "Was not able to assign user root privileges");
			returnValue = false;
		}

		//TODO: Do this command programmatically. Not by calling system()
		if( system("cp -rf /etc/nova/.nova $HOME") == -1)
		{
			syslog(SYSL_ERR, "File: %s Line: %d bind: %s", __FILE__, __LINE__, "Was not able to create user $HOME/.nova directory");
			returnValue = false;
		}

		//Check the ~/.nova dir again
		if ( stat( homeNovaPath.c_str(), &fileAttr ) == 0)
		{
			return returnValue;
		}
		else
		{
			syslog(SYSL_ERR, "File: %s Line: %d bind: %s", __FILE__, __LINE__, "Was not able to create user $HOME/.nova directory");
			returnValue = false;
		}
	}

	return returnValue;
}

string NOVAConfiguration::toString()
{
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

	return ss.str();
}

NOVAConfiguration::NOVAConfiguration(string configFilePath)
{
	this->m_configFilePath = configFilePath;
	SetDefaults();
	LoadConfig();
}

NOVAConfiguration::~NOVAConfiguration()
{
}

double NOVAConfiguration::getClassificationThreshold() const
{
	return m_classificationThreshold;
}

int NOVAConfiguration::getClassificationTimeout() const
{
	return m_classificationTimeout;
}

string NOVAConfiguration::getConfigFilePath() const
{
	return m_configFilePath;
}

int NOVAConfiguration::getDataTTL() const
{
	return m_dataTTL;
}

string NOVAConfiguration::getDoppelInterface() const
{
	return m_doppelInterface;
}

string NOVAConfiguration::getDoppelIp() const
{
	return m_doppelIp;
}

string NOVAConfiguration::getEnabledFeatures() const
{
	return m_enabledFeatures;
}

double NOVAConfiguration::getEps() const
{
	return m_eps;
}

bool NOVAConfiguration::getGotoLive() const
{
	return m_gotoLive;
}

string NOVAConfiguration::getInterface() const
{
	return m_interface;
}

bool NOVAConfiguration::getIsDmEnabled() const
{
	return m_isDmEnabled;
}

bool NOVAConfiguration::getIsTraining() const
{
	return m_isTraining;
}

int NOVAConfiguration::getK() const
{
	return m_k;
}

string NOVAConfiguration::getPathCESaveFile() const
{
	return m_pathCESaveFile;
}

string NOVAConfiguration::getPathConfigHoneydDm() const
{
	return m_pathConfigHoneydDm;
}

string NOVAConfiguration::getPathConfigHoneydHs() const
{
	return m_pathConfigHoneydHs;
}

string NOVAConfiguration::getPathPcapFile() const
{
	return m_pathPcapFile;
}

string NOVAConfiguration::getPathTrainingCapFolder() const
{
	return m_pathTrainingCapFolder;
}

string NOVAConfiguration::getPathTrainingFile() const
{
	return m_pathTrainingFile;
}

bool NOVAConfiguration::getReadPcap() const
{
	return m_readPcap;
}

int NOVAConfiguration::getSaMaxAttempts() const
{
	return m_saMaxAttempts;
}

int NOVAConfiguration::getSaPort() const
{
	return m_saPort;
}

double NOVAConfiguration::getSaSleepDuration() const
{
	return m_saSleepDuration;
}

int NOVAConfiguration::getSaveFreq() const
{
	return m_saveFreq;
}

int NOVAConfiguration::getTcpCheckFreq() const
{
	return m_tcpCheckFreq;
}

int NOVAConfiguration::getTcpTimout() const
{
	return m_tcpTimout;
}

int NOVAConfiguration::getThinningDistance() const
{
	return m_thinningDistance;
}

bool NOVAConfiguration::getUseTerminals() const
{
	return m_useTerminals;
}

void NOVAConfiguration::setClassificationThreshold(double classificationThreshold)
{
	this->m_classificationThreshold = classificationThreshold;
}

void NOVAConfiguration::setClassificationTimeout(int classificationTimeout)
{
	this->m_classificationTimeout = classificationTimeout;
}

void NOVAConfiguration::setConfigFilePath(string configFilePath)
{
	this->m_configFilePath = configFilePath;
}

void NOVAConfiguration::setDataTTL(int dataTTL)
{
	this->m_dataTTL = dataTTL;
}

void NOVAConfiguration::setDoppelInterface(string doppelInterface)
{
	this->m_doppelInterface = doppelInterface;
}

void NOVAConfiguration::setDoppelIp(string doppelIp)
{
	this->m_doppelIp = doppelIp;
}

void NOVAConfiguration::setEnabledFeatures(string enabledFeatures)
{
	this->m_enabledFeatures = enabledFeatures;
}

void NOVAConfiguration::setEps(double eps)
{
	this->m_eps = eps;
}

void NOVAConfiguration::setGotoLive(bool gotoLive)
{
	this->m_gotoLive = gotoLive;
}

void NOVAConfiguration::setInterface(string interface)
{
	this->m_interface = interface;
}

void NOVAConfiguration::setIsDmEnabled(bool isDmEnabled)
{
	this->m_isDmEnabled = isDmEnabled;
}

void NOVAConfiguration::setIsTraining(bool isTraining)
{
	this->m_isTraining = isTraining;
}

void NOVAConfiguration::setK(int k)
{
	this->m_k = k;
}

void NOVAConfiguration::setPathCESaveFile(string pathCESaveFile)
{
	this->m_pathCESaveFile = pathCESaveFile;
}

void NOVAConfiguration::setPathConfigHoneydDm(string pathConfigHoneydDm)
{
	this->m_pathConfigHoneydDm = pathConfigHoneydDm;
}

void NOVAConfiguration::setPathConfigHoneydHs(string pathConfigHoneydHs)
{
	this->m_pathConfigHoneydHs = pathConfigHoneydHs;
}

void NOVAConfiguration::setPathPcapFile(string pathPcapFile)
{
	this->m_pathPcapFile = pathPcapFile;
}

void NOVAConfiguration::setPathTrainingCapFolder(string pathTrainingCapFolder)
{
	this->m_pathTrainingCapFolder = pathTrainingCapFolder;
}

void NOVAConfiguration::setPathTrainingFile(string pathTrainingFile)
{
	this->m_pathTrainingFile = pathTrainingFile;
}

void NOVAConfiguration::setReadPcap(bool readPcap)
{
	this->m_readPcap = readPcap;
}

void NOVAConfiguration::setSaMaxAttempts(int saMaxAttempts)
{
	this->m_saMaxAttempts = saMaxAttempts;
}

void NOVAConfiguration::setSaPort(int saPort)
{
	this->m_saPort = saPort;
}

void NOVAConfiguration::setSaSleepDuration(double saSleepDuration)
{
	this->m_saSleepDuration = saSleepDuration;
}

void NOVAConfiguration::setSaveFreq(int saveFreq)
{
	this->m_saveFreq = saveFreq;
}

void NOVAConfiguration::setTcpCheckFreq(int tcpCheckFreq)
{
	this->m_tcpCheckFreq = tcpCheckFreq;
}

void NOVAConfiguration::setTcpTimout(int tcpTimout)
{
	this->m_tcpTimout = tcpTimout;
}

void NOVAConfiguration::setThinningDistance(int thinningDistance)
{
	this->m_thinningDistance = thinningDistance;
}

void NOVAConfiguration::setUseTerminals(bool useTerminals)
{
	this->m_useTerminals = useTerminals;
}

}

