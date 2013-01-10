//============================================================================
// Name        : NOVAConfiguration.h
// Copyright   : DataSoft Corporation 2011-2013
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

#define BOOST_FILESYSTEM_VERSION 2

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/un.h>
#include <syslog.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>
#include <pwd.h>
#include <string>
#include <iostream>

#include "Config.h"
#include "Logger.h"
#include "NovaUtil.h"
#include "Lock.h"

using namespace std;

namespace Nova
{

string Config::m_pathsFile = "/usr/share/nova/sharedFiles/paths";
string Config::m_pathPrefix = "";

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
	"K",
	"EPS",
	"CLASSIFICATION_THRESHOLD",
	"DATAFILE",
	"USER_HONEYD_CONFIG",
	"DOPPELGANGER_IP",
	"DOPPELGANGER_INTERFACE",
	"DM_ENABLED",
	"ENABLED_FEATURES",
	"THINNING_DISTANCE",
	"SAVE_FREQUENCY",
	"DATA_TTL",
	"CE_SAVE_FILE",
	"SMTP_ADDR",
	"SMTP_PORT",
	"SMTP_DOMAIN",
	"SMTP_USEAUTH",
	"RECIPIENTS",
	"SERVICE_PREFERENCES",
	"HAYSTACK_STORAGE",
	"WHITELIST_FILE",
	"MIN_PACKET_THRESHOLD",
	"CUSTOM_PCAP_FILTER",
	"CUSTOM_PCAP_MODE",
	"TRAINING_SESSION",
	"WEB_UI_PORT",
	"CLEAR_AFTER_HOSTILE_EVENT",
	"CAPTURE_BUFFER_SIZE",
	"MASTER_UI_IP",
	"MASTER_UI_RECONNECT_TIME",
	"MASTER_UI_CLIENT_ID",
	"MASTER_UI_ENABLED",
	"MASTER_UI_PORT",
	"FEATURE_WEIGHTS",
	"CLASSIFICATION_ENGINE",
	"THRESHOLD_HOSTILE_TRIGGERS",
	"ONLY_CLASSIFY_HONEYPOT_TRAFFIC",
	"CURRENT_CONFIG",
	"EMAIL_ALERTS_ENABLED"
};

Config *Config::m_instance = NULL;

Config *Config::Inst()
{
	if(m_instance == NULL)
	{
		m_instance = new Config();
		m_instance->LoadInterfaces();
	}
	return m_instance;
}

Config::Config()
{
	pthread_rwlock_init(&m_lock, NULL);

	m_pathPrefix = GetEnvVariable("NOVA_PATH_PREFIX");
	if (m_pathPrefix.compare(""))
	{
		cout << "Using prefix '" + m_pathPrefix + "' on paths" << endl;
	}
	LoadPaths();

	if(!InitUserConfigs())
	{
		// Do not call LOG here, Config and logger are not yet initialized
		cout << "CRITICAL ERROR: InitUserConfigs failed" << endl;
	}

	m_configFilePath = m_pathHome + string("/config/NOVAConfig.txt");
	m_userConfigFilePath = m_pathHome + string("/config/settings");
	LoadUserConfig();
	LoadConfig_Internal();
	LoadVersionFile();
}

Config::~Config()
{

}

void Config::LoadConfig()
{
	LoadConfig_Internal();
	LoadVersionFile();
	LoadInterfaces();
}

vector<string> Config::GetPrefixes()
{
	vector<string> ret;
	for (uint i = 0; i < sizeof(Config::Inst()->m_prefixes)/sizeof(Config::Inst()->m_prefixes[0]); i++)
	{
		ret.push_back(string(Config::Inst()->m_prefixes[i]));
	}
	return ret;
}

// Loads the configuration file into the class's state data
void Config::LoadConfig_Internal()
{
	Lock lock(&m_lock, WRITE_LOCK);
	string line;
	string prefix;
	int prefixIndex;

	bool isValid[sizeof(m_prefixes)/sizeof(m_prefixes[0])];

	ifstream config;
	config.open(m_configFilePath.c_str());

	if(config.is_open())
	{
		while(config.good())
		{
			getline(config, line);
			prefixIndex = 0;
			prefix = m_prefixes[prefixIndex];

			// INTERFACE
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				Trim(line, ' ');
				m_interfaceLine = line;

				m_interfaces.clear();
				boost::split(m_interfaces, m_interfaceLine, boost::is_any_of("\t "), boost::token_compress_on);

				if (m_interfaces.size())
				{
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
					m_loopbackIFString = line;
					m_loopbackIF = line;
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

			//SMTP_USEAUTH
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());

				if(line.size() > 0)
				{
					m_SMTPUseAuth = atoi(line.c_str());
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
							m_userPath = m_pathHome + "/config/haystack_honeyd.config";
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

			// MIN_PACKET_THRESHOLD
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_minPacketThreshold = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// CUSTOM_PCAP_FILTER
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_customPcapString = line;
					isValid[prefixIndex] = true;
				}
				else
				{
					m_customPcapString = "";
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// CUSTOM_PCAP_MODE
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_overridePcapString = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// TRAINING_SESSION
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_trainingSession = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// WEB_UI_PORT
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(atoi(line.c_str()) > 0)
				{
					m_webUIPort = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// CLEAR_AFTER_HOSTILE_EVENT
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_clearAfterHostile = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// CAPTURE_BUFFER_SIZE
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_captureBufferSize = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// MASTER_UI_IP
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_masterUIIP = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// MASTER_UI_RECONNECT_TIME
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_masterUIReconnectTime = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// MASTER_UI_CLIENT_ID
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_masterUIClientID = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// MASTER_UI_ENABLED
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_masterUIEnabled = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// MASTER_UI_PORT
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_masterUIPort = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// FEATURE_WEIGHTS
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{

					istringstream is(line);
					m_featureWeights.clear();
					double n;
					while (is >> n)
					{
						m_featureWeights.push_back(n);
					}

					if (m_featureWeights.size() == DIM)
					{
						isValid[prefixIndex] = true;
					}

				}
				continue;
			}

			// CLASSIFICATION_ENGINE
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_classificationType = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// THRESHOLD_HOSTILE_TRIGGERS
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_hostileThresholds.clear();

					vector<string> thresholds;
					boost::split(thresholds, line, boost::is_any_of("\t "));

					if (thresholds.size() != DIM)
					{
						cout << "ERROR: THRESHOLD_HOSTILE_TRIGGERS does not contain the correct number of entries" << endl;
						continue;
					}

					for (uint i = 0; i < thresholds.size(); i++)
					{
						HostileThreshold setting;
						setting.m_hasMaxValueTrigger = false;
						setting.m_hasMinValueTrigger = false;

						if (thresholds.at(i).at(0) == '-')
						{

						}
						else if (thresholds.at(i).at(0) == '>')
						{
							// Check if this has both a > and a < symbol
							vector<string> parts;
							boost::split(parts, thresholds.at(i), boost::is_any_of("<"));
							if (parts.size() == 2)
							{
								string maxValueString = parts.at(0).substr(1, string::npos);
								istringstream s1(maxValueString);
								if (!(s1 >> setting.m_maxValueTrigger))
								{
									LOG(ERROR, "Unable to parse max value for THRESHOLD_HOSTILE_TRIGGERS", "");
								}
								else
								{
									setting.m_hasMaxValueTrigger = true;
								}


								string minValueString = parts.at(1);
								istringstream s2(minValueString);
								if (!(s2 >> setting.m_minValueTrigger))
								{
									LOG(ERROR, "Unable to parse min value for THRESHOLD_HOSTILE_TRIGGERS", "");
								}
								else
								{
									setting.m_hasMinValueTrigger = true;
								}
							}
							else
							{
								istringstream s(thresholds.at(i).substr(1, string::npos));
								if (!(s >> setting.m_maxValueTrigger))
								{
									LOG(ERROR, "Unable to parse max value for THRESHOLD_HOSTILE_TRIGGERS", "");
								}
								else
								{
									setting.m_hasMaxValueTrigger = true;
								}
							}

						}
						else if (thresholds.at(i).at(0) == '<')
						{
							istringstream s(thresholds.at(i).substr(1, thresholds.at(i).npos));
							if (!(s >> setting.m_minValueTrigger))
							{
								LOG(ERROR, "Unable to parse min value for THRESHOLD_HOSTILE_TRIGGERS", "");
							}
							else
							{
								setting.m_hasMinValueTrigger = true;
							}
						}

						m_hostileThresholds.push_back(setting);
					}

					isValid[prefixIndex] = true;

					/*
					for (uint i = 0; i < m_hostileThresholds.size(); i++)
					{
						if (m_hostileThresholds.at(i).hasMaxValueTrigger)
						{
							cout << "Max value for feature " << i << " is " << m_hostileThresholds.at(i).maxValueTrigger << endl;
						}

						if (m_hostileThresholds.at(i).hasMinValueTrigger)
						{
							cout << "Min value for feature " << i << " is " << m_hostileThresholds.at(i).minValueTrigger << endl;
						}
					}
					*/
				}
				continue;
			}

			// ONLY_CLASSIFY_HONEYPOT_TRAFFIC
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_onlyClassifyHoneypotTraffic = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
			}

			// CURRENT_CONFIG
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_currentConfig = string(line.c_str());
					isValid[prefixIndex] = true;
				}
			}

			// EMAIL_ALERTS_ENABLED
			prefixIndex++;
			prefix = m_prefixes[prefixIndex];
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if(line.size() > 0)
				{
					m_emailAlertsEnabled = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
			}
		}
	}
	else
	{
		// Do not call LOG here, Config and Logger are not yet initialized
		cout << "CRITICAL ERROR: No configuration file found! Could not open: " << m_configFilePath << endl;
	}

	config.close();

	GetSMTPSettings_FromFile();

	for(uint i = 0; i < sizeof(m_prefixes)/sizeof(m_prefixes[0]); i++)
	{
		if(!isValid[i])
		{
			stringstream ss;
			ss << "ERROR File: " << __FILE__ << "at line: " << __LINE__ << "Configuration option '"
				<< m_prefixes[i] << "' is invalid.";
			::openlog("Nova", OPEN_SYSL, LOG_AUTHPRIV);
			syslog(ERROR, "%s %s", "ERROR", ss.str().c_str());
			closelog();
		}
	}
}

bool Config::SaveUserConfig()
{
	Lock lock(&m_lock, READ_LOCK);
	string line, prefix;

	//Rewrite the config file with the new settings
	string configurationBackup = m_userConfigFilePath + ".tmp";
	boost::filesystem::path from = m_userConfigFilePath;
	boost::filesystem::path to = configurationBackup;
	boost::filesystem::copy_file(from, to, boost::filesystem::copy_option::overwrite_if_exists);

	ifstream *in = new ifstream(configurationBackup.c_str());
	ofstream *out = new ofstream(m_userConfigFilePath.c_str());

	if(out->is_open() && in->is_open())
	{
		while(in->good())
		{
			if(!getline(*in, line))
			{
				continue;
			}

			prefix = "group";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << "group " << m_group << endl;
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

	boost::filesystem::remove(to);

	return true;
}

bool Config::LoadUserConfig()
{
	Lock lock(&m_lock, WRITE_LOCK);

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
	Lock lock(&m_lock, WRITE_LOCK);

	// Try getting the $HOME env var
	char *homePath = getenv("HOME");
	if (homePath != NULL)
	{
		m_pathHome = string(homePath);
	}
	// Resort to checking the passwd file
	else
	{
		struct passwd *pw = getpwuid(getuid());
		m_pathHome = string(pw->pw_dir);
	}

	m_pathHome += "/.config/nova";

	//Get locations of nova files
	ifstream *paths =  new ifstream(m_pathPrefix + Config::m_pathsFile);
	string prefix, line;

	if(paths->is_open())
	{
		while(paths->good())
		{
			getline(*paths,line);

			prefix = "NOVA_RD";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				m_pathReadFolder = m_pathPrefix + ResolvePathVars(line);
				continue;
			}

			prefix = "NOVA_ICON";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				m_pathIcon = m_pathPrefix + ResolvePathVars(line);
				continue;
			}
		}
	}
	paths->close();
	delete paths;
	return true;
}

void Config::LoadInterfaces()
{
	Lock lock(&m_lock, WRITE_LOCK);
	struct ifaddrs *devices = NULL;
	ifaddrs *curIf = NULL;
	stringstream ss;

	//Get a list of interfaces
	if(getifaddrs(&devices))
	{
		LOG(ERROR, string("Ethernet Interface Auto-Detection failed: ") + string(strerror(errno)), "");
	}

	//Choose the first loopback adapter in the default case
	if(!m_loopbackIFString.compare("default"))
	{
		m_loIsDefault = true;
		for(curIf = devices; curIf != NULL; curIf = curIf->ifa_next)
		{
			//If we find a valid loopback interface exit the loop early (curIf != NULL)
			if((curIf->ifa_flags & IFF_LOOPBACK) && ((int)curIf->ifa_addr->sa_family == AF_INET))
			{
				m_loopbackIF = string(curIf->ifa_name);
				break;
			}
		}
	}
	else
	{
		m_loIsDefault = false;
	}

	m_interfaces.clear();
	boost::split(m_interfaces, m_interfaceLine, boost::is_any_of("\t "), boost::token_compress_on);
	vector<string> interfaces = m_interfaces;
	//Use all valid devices
	if(!interfaces[0].compare("default"))
	{
		m_interfaces.clear();
		m_ifIsDefault = true;
		for(curIf = devices; curIf != NULL; curIf = curIf->ifa_next)
		{
			if(!(curIf->ifa_flags & IFF_LOOPBACK) && ((int)curIf->ifa_addr->sa_family == AF_INET))
			{
				m_interfaces.push_back(string(curIf->ifa_name));
			}
		}
	}
	else
	{
		m_ifIsDefault = false;
	}

	freeifaddrs(devices);
}

bool Config::LoadVersionFile()
{
	ifstream versionFile((m_pathHome + "/config/" + VERSION_FILE_NAME).c_str());
	string line;

	if(versionFile.is_open())
	{
		if(versionFile.good())
		{
			getline(versionFile, line);
			string temp = line.substr(line.find_first_of(".") + 1, string::npos);

			m_version.versionString = line;
			m_version.buildYear = atoi(line.substr(0, line.find_first_of(".")).c_str());
			m_version.buildMonth = atoi(temp.substr(0, temp.find_first_of(".")).c_str());

			if (temp.find_first_of(".") != string::npos)
			{
				m_version.minorVersion = atoi(temp.substr(temp.find_first_of(".") + 1, string::npos).c_str());
			}
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	return true;
}

version Config::GetVersion()
{
	return m_version;
}

string Config::GetCurrentConfig()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_currentConfig;
}

string Config::GetVersionString()
{
	return m_version.versionString;
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
	Lock lock(&m_lock, READ_LOCK);
	string line, prefix;

	//XXX we need to do some interface checking to determine if we are still using the default configuration

	//Rewrite the config file with the new settings
	string configurationBackup = m_configFilePath + ".tmp";
	boost::filesystem::path from = m_configFilePath;
	boost::filesystem::path to = configurationBackup;
	boost::filesystem::copy_file(from, to, boost::filesystem::copy_option::overwrite_if_exists);

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

			prefix = "INTERFACE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix;
				if(m_ifIsDefault)
				{
					*out << " default" << endl;
					continue;
				}

				if (m_interfaces.size())
				{
					*out << m_interfaces.at(0);
					for (uint i = 1; i < m_interfaces.size(); i++)
					{
						*out << " " << m_interfaces.at(i);
					}
				}
				*out << endl;
				continue;
			}

			prefix = "DATAFILE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << GetPathTrainingFile() << endl;
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

			prefix = "DOPPELGANGER_INTERFACE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix;
				if(m_loIsDefault)
				{
					*out << " default" << endl;
					continue;
				}
				*out << " " << m_loopbackIF << endl;
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

			prefix = "CAPTURE_BUFFER_SIZE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " <<  GetCaptureBufferSize() << endl;
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

	boost::filesystem::remove(to);

	return true;
}

//	Returns: True if(after the function) the user has all necessary nova config files
//		IE: Returns false only if the user doesn't have configs AND we weren't able to make them
bool Config::InitUserConfigs()
{
	bool returnValue = true;
	struct stat fileAttr;

	// Important note
	// This is called before the logger is initialized. Calling LOG here will likely result in a crash. Just use cout instead.

	//check for home folder
	if(!stat(m_pathHome.c_str(), &fileAttr ) == 0)
	{
		boost::filesystem::path fromPath = m_pathPrefix + "/usr/share/nova/userFiles/";
		boost::filesystem::path toPath = m_pathHome + "/";

		if(!RecursiveDirectoryCopy(fromPath, toPath, false))
		{
			cout << "Error copying files to user's HOME folder." << endl;
		}

		if(stat(m_pathHome.c_str(), &fileAttr) == 0)
		{
			return returnValue;
		}
		else
		{
			cout << "Was unable to create directory " << m_pathHome << endl;
			returnValue = false;
		}
	}

	return returnValue;
}

string Config::ToString()
{
	Lock lock(&m_lock, READ_LOCK);

	std::stringstream ss;
	ss << "GetConfigFilePath() " << GetConfigFilePath() << endl;
	ss << "GetDoppelInterface() " << GetDoppelInterface() << endl;
	ss << "GetDoppelIp() " << GetDoppelIp() << endl;
	ss << "GetEnabledFeatures() " << GetEnabledFeatures() << endl;
	ss << "GetInterfaces() :";
	vector<string> ifList = GetInterfaces();
	for(uint i = 0; i < ifList.size(); i++)
	{
		if(i) //If i != 0;
		{
			ss << ", ";
		}
		ss << ifList[i];
	}
	ss << "GetPathCESaveFile() " << GetPathCESaveFile() << endl;
	ss << "GetPathConfigHoneydDm() " << GetPathConfigHoneydUser() << endl;
	ss << "GetPathConfigHoneydHs() " << GetPathConfigHoneydHS() << endl;
	ss << "GetPathPcapFile() " << GetPathPcapFile() << endl;
	ss << "GetPathTrainingFile() " << GetPathTrainingFile() << endl;

	ss << "GetReadPcap() " << GetReadPcap() << endl;
	ss << "GetIsDmEnabled() " << GetIsDmEnabled() << endl;
	ss << "GetGotoLive() " << GetGotoLive() << endl;

	ss << "GetClassificationTimeout() " << GetClassificationTimeout() << endl;
	ss << "GetDataTTL() " << GetDataTTL() << endl;
	ss << "GetK() " << GetK() << endl;
	ss << "GetSaveFreq() " << GetSaveFreq() << endl;
	ss << "GetTcpCheckFreq() " << GetTcpCheckFreq() << endl;
	ss << "GetTcpTimout() " << GetTcpTimout() << endl;
	ss << "GetThinningDistance() " << GetThinningDistance() << endl;

	ss << "GetClassificationThreshold() " << GetClassificationThreshold() << endl;
	ss << "GetEps() " << GetEps() << endl;


	return ss.str();
}

std::string Config::ReadSetting(std::string key)
{
	Lock lock(&m_lock, WRITE_LOCK);

	string line;
	string value;

	ifstream config(m_configFilePath.c_str());

	if(config.is_open())
	{
		while(config.good())
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
	Lock lock(&m_lock, WRITE_LOCK);

	bool validKey = false;
	for (uint i = 0; i < sizeof(Config::Inst()->m_prefixes)/sizeof(Config::Inst()->m_prefixes[0]); i++)
	{
		if (!Config::Inst()->m_prefixes[i].compare(key))
		{
			validKey = true;
			break;
		}
	}

	if (!validKey)
	{
		LOG(WARNING, "WriteSetting was called with invalid setting key", "");
		return false;
	}

	string line;
	bool error = false;

	//Rewrite the config file with the new settings
	string configurationBackup = m_configFilePath + ".tmp";
	boost::filesystem::path from = m_configFilePath;
	boost::filesystem::path to = configurationBackup;
	string copyCommand = "cp -fp " + m_configFilePath + " " + configurationBackup;
	boost::filesystem::copy_file(from, to, boost::filesystem::copy_option::overwrite_if_exists);

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

	boost::filesystem::remove(to);

	if(error)
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
	Lock lock(&m_lock, READ_LOCK);
	return m_classificationThreshold;
}

int Config::GetClassificationTimeout()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_classificationTimeout;
}

string Config::GetConfigFilePath()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_configFilePath;
}

int Config::GetDataTTL()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_dataTTL;
}

string Config::GetDoppelInterface()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_loopbackIF;
}

string Config::GetDoppelIp()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_doppelIp;
}

string Config::GetEnabledFeatures()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_enabledFeatureMask;
}

uint Config::GetEnabledFeatureCount()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_enabledFeatureCount;
}

bool Config::IsFeatureEnabled(uint i)
{
	Lock lock(&m_lock, READ_LOCK);
	return m_isFeatureEnabled[i];
}

double Config::GetEps()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_eps;
}

bool Config::GetGotoLive()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_gotoLive;
}

bool Config::GetUseAllInterfaces()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_ifIsDefault;
}

bool Config::GetUseAnyLoopback()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_loIsDefault;
}

string Config::GetInterface(uint i)
{
	Lock lock(&m_lock, READ_LOCK);
	string interface = "";
	if(m_interfaces.size() > i)
	{
		interface = m_interfaces[i];
	}
	return interface;
}

vector<string> Config::GetInterfaces()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_interfaces;
}

vector<string> Config::GetIPv4HostInterfaceList()
{
	Lock lock(&m_lock, READ_LOCK);
	struct ifaddrs *devices = NULL;
	ifaddrs *curIf = NULL;
	vector<string> ret;
	//Get a list of interfaces
	if(getifaddrs(&devices))
	{
		LOG(ERROR, string("Ethernet Interface Auto-Detection failed: ") + string(strerror(errno)), "");
	}
	for(curIf = devices; curIf != NULL; curIf = curIf->ifa_next)
	{
		if(((int)curIf->ifa_addr->sa_family == AF_INET) && !(curIf->ifa_flags & IFF_LOOPBACK))
		{
			ret.push_back(curIf->ifa_name);
		}
	}
	return ret;
}

vector<string> Config::GetIPv4LoopbackInterfaceList()
{
	Lock lock(&m_lock, READ_LOCK);
	struct ifaddrs *devices = NULL;
	ifaddrs *curIf = NULL;
	vector<string> ret;
	//Get a list of interfaces
	if(getifaddrs(&devices))
	{
		LOG(ERROR, string("Ethernet Interface Auto-Detection failed: ") + string(strerror(errno)), "");
	}
	for(curIf = devices; curIf != NULL; curIf = curIf->ifa_next)
	{
		if(((int)curIf->ifa_addr->sa_family == AF_INET) && (curIf->ifa_flags & IFF_LOOPBACK))
		{
			ret.push_back(curIf->ifa_name);
		}
	}
	return ret;
}

uint Config::GetInterfaceCount()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_interfaces.size();
}

bool Config::GetIsDmEnabled()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_isDmEnabled;
}

int Config::GetK()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_k;
}

string Config::GetPathCESaveFile()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_pathCESaveFile;
}

string Config::GetPathConfigHoneydUser()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_pathConfigHoneydUser;
}

string Config::GetPathConfigHoneydHS()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_pathConfigHoneydHs;
}

string Config::GetPathPcapFile()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_pathPcapFile;
}

string Config::GetPathTrainingFile()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_pathTrainingFile;
}

string Config::GetPathWhitelistFile()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_pathHome + "/" + m_pathWhitelistFile;
}

bool Config::GetReadPcap()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_readPcap;
}

int Config::GetSaveFreq()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_saveFreq;
}

int Config::GetTcpCheckFreq()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_tcpCheckFreq;
}

int Config::GetTcpTimout()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_tcpTimout;
}

double Config::GetThinningDistance()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_thinningDistance;
}

string Config::GetKey()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_key;
}

vector<in_addr_t> Config::GetNeighbors()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_neighbors;
}

string Config::GetGroup()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_group;
}

bool Config::GetSMTPSettings_FromFile()
{
	string debugPath = m_pathHome + "/config/smtp.txt";

	ifstream ifs(m_pathHome + "/config/smtp.txt");

	if (!ifs.is_open())
	{
		// Note: This needs to be cout since the logger isn't initialized yet
		cout << "Unable to open SMTP settings file at " << debugPath << endl;
		return false;
	}

	getline(ifs, m_SMTPUser);
	getline(ifs, m_SMTPPass);

	ifs.close();
	return true;
}

bool Config::SaveSMTPSettings()
{
	string debugPath = m_pathHome + "/config/smtp.txt";
	ofstream out(debugPath.c_str());

	if (!out.is_open())
	{
		cout << "ERROR: Unable to open SMTP settings file at " << debugPath << endl;
		return false;
	}

	out << m_SMTPUser << endl;
	out << m_SMTPPass << endl;

	out.close();
	return true;
}

void Config::SetUseAllInterfaces(bool which)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_ifIsDefault = which;
}

bool Config::SetInterfaces(std::vector<std::string> newInterfaceList)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_interfaces = newInterfaceList;
	return true;
}

void Config::SetUseAnyLoopback(bool which)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_loIsDefault = which;
}

bool Config::SetUseAllInterfacesBinding(bool which)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_ifIsDefault = which;
	return true;
}

bool Config::SetUseAnyLoopbackBinding(bool which)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_loIsDefault = which;
	return true;
}

void Config::SetClassificationThreshold(double classificationThreshold)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_classificationThreshold = classificationThreshold;
}

void Config::SetClassificationTimeout(int classificationTimeout)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_classificationTimeout = classificationTimeout;
}

void Config::SetConfigFilePath(string configFilePath)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_configFilePath = configFilePath;
}

void Config::SetDataTTL(int dataTTL)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_dataTTL = dataTTL;
}

void Config::SetDoppelInterface(string doppelInterface)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_loopbackIF = doppelInterface;
}

void Config::SetDoppelIp(string doppelIp)
{
	Lock lock(&m_lock, WRITE_LOCK);
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
	Lock lock(&m_lock, WRITE_LOCK);
	SetEnabledFeatures_noLocking(enabledFeatureMask);
}

void Config::EnableAllFeatures()
{
	Lock lock (&m_lock, WRITE_LOCK);

	char mask[DIM];
	for(int i = 0; i < DIM; i++)
	{
		mask[i] = '1';
	}
	SetEnabledFeatures_noLocking(string(mask));
}

void Config::SetEps(double eps)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_eps = eps;
}

void Config::SetGotoLive(bool gotoLive)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_gotoLive = gotoLive;
}

void Config::AddInterface(string interface)
{
	Lock lock(&m_lock, WRITE_LOCK);
	for(uint i = 0; i < m_interfaces.size(); i++)
	{
		if(!m_interfaces[i].compare(interface))
		{
			return;
		}
	}
	m_interfaces.push_back(interface);
}

void Config::RemoveInterface(string interface)
{
	Lock lock(&m_lock, WRITE_LOCK);
	for(uint i = 0; i < m_interfaces.size(); i++)
	{
		if(!m_interfaces[i].compare(interface))
		{
			m_interfaces.erase(m_interfaces.begin()+i);
			break;
		}
	}
}

void Config::ClearInterfaces()
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_interfaces.clear();
}

std::vector<std::string> Config::ListInterfaces()
{
	Lock lock(&m_lock, WRITE_LOCK);
	struct ifaddrs * devices = NULL;
	ifaddrs *curIf = NULL;
	stringstream ss;

	//Get a list of interfaces
	if(getifaddrs(&devices))
	{
		LOG(ERROR, string("Ethernet Interface Auto-Detection failed: ") + string(strerror(errno)), "");
	}

	// ********** ETHERNET INTERFACES ************* //
	vector<string> interfaces;

	for(curIf = devices; curIf != NULL; curIf = curIf->ifa_next)
	{
		if(!(curIf->ifa_flags & IFF_LOOPBACK) && ((int)curIf->ifa_addr->sa_family == AF_PACKET))
		{
			interfaces.push_back(string(curIf->ifa_name));
		}
	}

	freeifaddrs(devices);
	return interfaces;
}

std::string Config::GetUseAllInterfacesBinding()
{
	if(m_ifIsDefault)
	{
		return "true";
	}
	else
	{
		return "false";
	}
}

std::vector<std::string> Config::ListLoopbacks()
{
	Lock lock(&m_lock, WRITE_LOCK);
	struct ifaddrs * devices = NULL;
	ifaddrs *curIf = NULL;
	stringstream ss;
	std::vector<std::string> ret;

	//Get a list of interfaces
	if(getifaddrs(&devices))
	{
		LOG(ERROR, string("Ethernet Interface Auto-Detection failed: ") + string(strerror(errno)), "");
	}

	for(curIf = devices; curIf != NULL; curIf = curIf->ifa_next)
	{
		//If we find a valid loopback interface exit the loop early (curIf != NULL)
		if((curIf->ifa_flags & IFF_LOOPBACK) && ((int)curIf->ifa_addr->sa_family == AF_INET))
		{
			ret.push_back(string(curIf->ifa_name));
		}
	}

	freeifaddrs(devices);
	return ret;
}

void Config::SetIsDmEnabled(bool isDmEnabled)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_isDmEnabled = isDmEnabled;
}

void Config::SetK(int k)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_k = k;
}

void Config::SetPathCESaveFile(string pathCESaveFile)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_pathCESaveFile = pathCESaveFile;
}

void Config::SetPathConfigHoneydUser(string pathConfigHoneydUser)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_pathConfigHoneydUser = pathConfigHoneydUser;
}

void Config::SetPathConfigHoneydHs(string pathConfigHoneydHs)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_pathConfigHoneydHs = pathConfigHoneydHs;
}

void Config::SetPathPcapFile(string pathPcapFile)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_pathPcapFile = pathPcapFile;
}

void Config::SetPathTrainingFile(string pathTrainingFile)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_pathTrainingFile = pathTrainingFile;
}

void Config::SetPathWhitelistFile(string pathWhitelistFile)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_pathWhitelistFile = pathWhitelistFile;
}

void Config::SetReadPcap(bool readPcap)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_readPcap = readPcap;
}

void Config::SetSaveFreq(int saveFreq)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_saveFreq = saveFreq;
}

void Config::SetTcpCheckFreq(int tcpCheckFreq)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_tcpCheckFreq = tcpCheckFreq;
}

void Config::SetTcpTimout(int tcpTimout)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_tcpTimout = tcpTimout;
}

void Config::SetThinningDistance(double thinningDistance)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_thinningDistance = thinningDistance;
}

void Config::SetKey(string key)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_key = key;
}

void Config::SetNeigbors(vector<in_addr_t> neighbors)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_neighbors = neighbors;
}

void Config::SetGroup(string group)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_group = group;
}

string Config::GetLoggerPreferences()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_loggerPreferences;
}

string Config::GetSMTPAddr()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_SMTPAddr;
}

string Config::GetSMTPDomain()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_SMTPDomain;
}

vector<string> Config::GetSMTPEmailRecipients()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_SMTPEmailRecipients;
}

in_port_t Config::GetSMTPPort()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_SMTPPort;
}

std::string Config::GetSMTPUser()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_SMTPUser;
}

std::string Config::GetSMTPPass()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_SMTPPass;
}

void Config::SetLoggerPreferences(string loggerPreferences)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_loggerPreferences = loggerPreferences;

}

bool Config::SetSMTPUser(std::string SMTPUser)
{
	Lock lock(&m_lock, READ_LOCK);
	m_SMTPUser = SMTPUser;
	return SaveSMTPSettings();
}

bool Config::SetSMTPPass(std::string SMTP_Pass)
{
	Lock lock(&m_lock, READ_LOCK);
	m_SMTPPass = SMTP_Pass;
	return SaveSMTPSettings();
}

void Config::SetSMTPAddr(string SMTPAddr)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_SMTPAddr = SMTPAddr;

}

void Config::SetSMTPDomain(string SMTPDomain)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_SMTPDomain = SMTPDomain;

}

void Config::SetSMTPEmailRecipients(vector<string> SMTPEmailRecipients)
{
	Lock lock(&m_lock, WRITE_LOCK);
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
	Lock lock(&m_lock, WRITE_LOCK);
	m_SMTPPort = SMTPPort;

}

string Config::GetPathReadFolder()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_pathReadFolder;
}

string Config::GetPathIcon()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_pathIcon;
}

string Config::GetPathHome()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_pathHome;
}

double Config::GetSqurtEnabledFeatures()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_squrtEnabledFeatures;
}

char Config::GetHaystackStorage()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_haystackStorage;
}

void Config::SetHaystackStorage(char haystackStorage)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_haystackStorage = haystackStorage;
}

string Config::GetUserPath()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_userPath;
}

void Config::SetUserPath(string userPath)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_userPath = userPath;
}

uint Config::GetMinPacketThreshold()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_minPacketThreshold;
}

void Config::SetMinPacketThreshold(uint packets)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_minPacketThreshold = packets;
}

string Config::GetCustomPcapString()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_customPcapString;
}

void Config::SetCustomPcapString(std::string customPcapString)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_customPcapString = customPcapString;
}

bool Config::GetOverridePcapString()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_overridePcapString;
}

void Config::SetOverridePcapString(bool overridePcapString)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_overridePcapString = overridePcapString;
}

vector <string> Config::GetIpAddresses(string ipListFile)
{
	ifstream ipListFileStream(ipListFile.c_str());
	vector<string> whitelistedAddresses;

	if(ipListFileStream.is_open())
	{
		while(ipListFileStream.good())
		{
			string line;
			getline(ipListFileStream,line);
			if(line != "" && line.at(0) != '#' )
			{
				whitelistedAddresses.push_back(line);
			}
		}
		ipListFileStream.close();
	}
	else
	{
		LOG(ERROR,"Unable to open file: " + ipListFile, "");
	}

	return whitelistedAddresses;
}

vector <string> Config::GetHoneydIpAddresses(string ipListFile)
{
	ifstream ipListFileStream(ipListFile.c_str());
	vector<string> whitelistedAddresses;

	if(ipListFileStream.is_open())
	{
		while(ipListFileStream.good())
		{
			string line;
			getline(ipListFileStream,line);
			if(line != "" && line.at(0) != '#' )
			{
				std::vector<std::string> strs;
				boost::split(strs, line, boost::is_any_of(", "));
				if (strs.size() > 0)
				{
					whitelistedAddresses.push_back(strs.at(0));
				}
			}
		}
		ipListFileStream.close();
	}
	else
	{
		LOG(ERROR,"Unable to open file: " + ipListFile, "");
	}

	return whitelistedAddresses;
}

vector <string> Config::GetHaystackAddresses(string honeyDConfigPath)
{
	//Path to the main log file
	ifstream honeydConfFile(honeyDConfigPath.c_str());
	vector<string> retAddresses;

	if( honeydConfFile == NULL)
	{
		LOG(ERROR, string("Error opening haystack file at ") + honeyDConfigPath + ". Does it exist?", "");
		exit(EXIT_FAILURE);
	}

	string LogInputLine;

	while(!honeydConfFile.eof())
	{
		stringstream LogInputLineStream;

		//Get the next line
		getline(honeydConfFile, LogInputLine);

		//Load the line into a stringstream for easier tokenizing
		LogInputLineStream << LogInputLine;
		string token;
		string honeydTemplate;

		//Is the first word "bind"?
		getline(LogInputLineStream, token, ' ');

		if(token.compare( "bind" ) != 0)
		{
			continue;
		}

		//The next token will be the IP address
		getline(LogInputLineStream, token, ' ');

		// Get the template
		getline(LogInputLineStream, honeydTemplate, ' ');

		if(honeydTemplate != "DoppelgangerReservedTemplate")
		{
			retAddresses.push_back(token);
		}
	}
	return retAddresses;
}

string Config::GetTrainingSession()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_trainingSession;
}

string Config::SetTrainingSession(string trainingSession)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_trainingSession = trainingSession;
	return m_trainingSession;
}

int Config::GetWebUIPort()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_webUIPort;
}

void Config::SetWebUIPort(int port)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_webUIPort = port;
}

bool Config::GetClearAfterHostile()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_clearAfterHostile;
}

void Config::SetClearAfterHostile(bool clearAfterHostile)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_clearAfterHostile = clearAfterHostile;
}

int Config::GetCaptureBufferSize()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_captureBufferSize;
}

void Config::SetCaptureBufferSize(int bufferSize)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_captureBufferSize = bufferSize;
}

void Config::SetSMTPUseAuth(bool useAuth)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_SMTPUseAuth = useAuth;
}

bool Config::GetSMTPUseAuth()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_SMTPUseAuth;
}

vector<double> Config::GetFeatureWeights()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_featureWeights;
}

string Config::GetClassificationEngineType()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_classificationType;
}

vector<HostileThreshold> Config::GetHostileThresholds()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_hostileThresholds;
}

bool Config::GetOnlyClassifyHoneypotTraffic()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_onlyClassifyHoneypotTraffic;
}

bool Config::SetCurrentConfig(string configName)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_currentConfig = configName;
	return true;
}

int Config::GetMasterUIPort()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_masterUIPort;
}

void Config::SetMasterUIPort(int newPort)
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_masterUIPort = newPort;
}

bool Config::GetAreEmailAlertsEnabled()
{
	Lock lock(&m_lock, READ_LOCK);
	return m_emailAlertsEnabled;
}

}
