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

const string NOVAConfiguration::prefixes[] = 	{ "INTERFACE", "HS_HONEYD_CONFIG",
		"TCP_TIMEOUT", "TCP_CHECK_FREQ", "READ_PCAP", "PCAP_FILE", "GO_TO_LIVE",
		"USE_TERMINALS", "CLASSIFICATION_TIMEOUT", "SILENT_ALARM_PORT", "K", "EPS",
		"IS_TRAINING", "CLASSIFICATION_THRESHOLD", "DATAFILE", "SA_MAX_ATTEMPTS",
		"SA_SLEEP_DURATION", "DM_HONEYD_CONFIG", "DOPPELGANGER_IP", "DOPPELGANGER_INTERFACE",
		"DM_ENABLED", "ENABLED_FEATURES","TRAINING_CAP_FOLDER", "THINNING_DISTANCE",
		"SAVE_FREQUENCY", "DATA_TTL", "CE_SAVE_FILE"};

// Loads the configuration file into the class's state data
void NOVAConfiguration::LoadConfig(string module)
{
	string line;
	string prefix;
	int prefixIndex;

	bool isValid[sizeof(prefixes)/sizeof(prefixes[0])];

	string use = module.substr(4, (module.length()));

	openlog(use.c_str(), LOG_CONS | LOG_PID | LOG_NDELAY | LOG_PERROR, LOG_AUTHPRIV);

	ifstream config(configFilePath.c_str());

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

											interface = column;
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
						interface = line;
						isValid[prefixIndex] = true;
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
					pathConfigHoneydHs  = line;
					isValid[prefixIndex] = true;
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
					tcpTimout = atoi(line.c_str());
					isValid[prefixIndex] = true;
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
					tcpCheckFreq = atoi(line.c_str());
					isValid[prefixIndex] = true;
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
					readPcap = atoi(line.c_str());
					isValid[prefixIndex] = true;
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
					pathPcapFile = line;
					isValid[prefixIndex] = true;
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
					gotoLive = line.c_str();
					isValid[prefixIndex] = true;
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
					useTerminals = line.c_str();
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// CLASSIFICATION_TIMEOUT
			prefixIndex++;
			prefix = prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (atoi(line.c_str()) >= 0)
				{
					classificationTimeout = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// SILENT_ALARM_PORT
			prefixIndex++;
			prefix = prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				if(line.size() == prefix.size())
				{
					line += " ";
				}

				line = line.substr(prefix.size() + 1, line.size());

				if (atoi(line.c_str()) > 0)
				{
					saPort = atoi(line.c_str());
					isValid[prefixIndex] = true;
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
					k = atoi(line.c_str());
					isValid[prefixIndex] = true;
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
					eps = atof(line.c_str());
					isValid[prefixIndex] = true;
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
					isTraining = atoi(line.c_str());
					isValid[prefixIndex] = true;
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
					classificationThreshold= atof(line.c_str());
					isValid[prefixIndex] = true;
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
					pathTrainingFile = line;
					isValid[prefixIndex] = true;
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
					saMaxAttempts = atoi(line.c_str());
					isValid[prefixIndex] = true;
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
					saSleepDuration = atof(line.c_str());
					isValid[prefixIndex] = true;
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
					pathConfigHoneydDm = line;
					isValid[prefixIndex] = true;
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
					doppelIp = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// DOPPELGANGER_INTERFACE
			prefixIndex++;
			prefix = prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (line.size() > 0)
				{
					doppelInterface = line;
					isValid[prefixIndex] = true;
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
					isDmEnabled = atoi(line.c_str());
					isValid[prefixIndex] = true;
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
					enabledFeatures = line;
					isValid[prefixIndex] = true;
				}
				continue;

			}

			// TRAINING_CAP_FOLDER
			prefixIndex++;
			prefix = prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (line.size() > 0)
				{
					pathTrainingCapFolder = line;
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// THINNING_DISTANCE
			prefixIndex++;
			prefix = prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (atof(line.c_str()) > 0)
				{
					thinningDistance = atof(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// SAVE_FREQUENCY
			prefixIndex++;
			prefix = prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (atoi(line.c_str()) > 0)
				{
					saveFreq = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// DATA_TTL
			prefixIndex++;
			prefix = prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (atoi(line.c_str()) >= 0)
				{
					dataTTL = atoi(line.c_str());
					isValid[prefixIndex] = true;
				}
				continue;
			}

			// CE_SAVE_FILE
			prefixIndex++;
			prefix = prefixes[prefixIndex];
			if (!line.substr(0, prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size() + 1, line.size());
				if (line.size() > 0)
				{
					pathCESaveFile = line;
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


	for (uint i = 0; i < sizeof(prefixes)/sizeof(prefixes[0]); i++)
	{
		if (!isValid[i])
		{
			// TODO: Make this say which config option is invalid again
			syslog(SYSL_INFO, "Line: %d Configuration option %s is invalid in the configuration file", __LINE__, prefixes[i].c_str());
		}
	}
	closelog();

}



void NOVAConfiguration::SetDefaults()
{
	openlog(__FUNCTION__, OPEN_SYSL, LOG_AUTHPRIV);

	interface = "default";
	pathConfigHoneydHs 	= "Config/haystack.config";
	pathPcapFile 		= "../pcapfile";
	pathTrainingFile 	= "Data/data.txt";
	pathConfigHoneydDm	= "Config/doppelganger.config";
	pathTrainingCapFolder = "Data";
	pathCESaveFile = "ceStateSave";

	tcpTimout = 7;
	tcpCheckFreq = 3;
	readPcap = false;
	gotoLive = true;
	useTerminals = false;
	isDmEnabled = true;

	classificationTimeout = 3;
	saPort = 12024;
	k = 3;
	eps = 0.01;
	isTraining = 0;
	classificationThreshold = .5;
	saMaxAttempts = 3;
	saSleepDuration = .5;
	doppelIp = "10.0.0.1";
	doppelInterface = "lo";
	enabledFeatures = "111111111";
	thinningDistance = 0;
	saveFreq = 1440;
	dataTTL = 0;
}

// Checks to see if the current user has a ~/.nova directory, and creates it if not, along with default config files
//	Returns: True if (after the function) the user has all necessary ~/.nova config files
//		IE: Returns false only if the user doesn't have configs AND we weren't able to make them
bool NOVAConfiguration::InitUserConfigs(string homePath)
{
	struct stat fileAttr;
	//TODO: Do a proper check to make sure all config files exist, not just the .nova dir
	if ( stat( homePath.c_str(), &fileAttr ) == 0)
	{
		return true;
	}
	else
	{
		if( system("gksudo --description 'Add your user to the privileged nova user group. "
				"(Required for Nova to run)'  \"usermod -a -G nova $USER\"") != 0)
		{
			syslog(SYSL_ERR, "File: %s Line: %d bind: %s", __FILE__, __LINE__, "Was not able to assign user root privileges");
			return false;
		}

		//TODO: Do this command programmatically. Not by calling system()
		if( system("cp -rf /etc/nova/.nova $HOME") == -1)
		{
			syslog(SYSL_ERR, "File: %s Line: %d bind: %s", __FILE__, __LINE__, "Was not able to create user $HOME/.nova directory");
			return false;
		}

		//Check the ~/.nova dir again
		if ( stat( homePath.c_str(), &fileAttr ) == 0)
		{
			return true;
		}
		else
		{
			syslog(SYSL_ERR, "File: %s Line: %d bind: %s", __FILE__, __LINE__, "Was not able to create user $HOME/.nova directory");
			return false;
		}
	}
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
	this->configFilePath = configFilePath;
	SetDefaults();
}

NOVAConfiguration::~NOVAConfiguration()
{
}

double NOVAConfiguration::getClassificationThreshold() const
{
	return classificationThreshold;
}

int NOVAConfiguration::getClassificationTimeout() const
{
	return classificationTimeout;
}

string NOVAConfiguration::getConfigFilePath() const
{
	return configFilePath;
}

int NOVAConfiguration::getDataTTL() const
{
	return dataTTL;
}

string NOVAConfiguration::getDoppelInterface() const
{
	return doppelInterface;
}

string NOVAConfiguration::getDoppelIp() const
{
	return doppelIp;
}

string NOVAConfiguration::getEnabledFeatures() const
{
	return enabledFeatures;
}

double NOVAConfiguration::getEps() const
{
	return eps;
}

bool NOVAConfiguration::getGotoLive() const
{
	return gotoLive;
}

string NOVAConfiguration::getInterface() const
{
	return interface;
}

bool NOVAConfiguration::getIsDmEnabled() const
{
	return isDmEnabled;
}

bool NOVAConfiguration::getIsTraining() const
{
	return isTraining;
}

int NOVAConfiguration::getK() const
{
	return k;
}

string NOVAConfiguration::getPathCESaveFile() const
{
	return pathCESaveFile;
}

string NOVAConfiguration::getPathConfigHoneydDm() const
{
	return pathConfigHoneydDm;
}

string NOVAConfiguration::getPathConfigHoneydHs() const
{
	return pathConfigHoneydHs;
}

string NOVAConfiguration::getPathPcapFile() const
{
	return pathPcapFile;
}

string NOVAConfiguration::getPathTrainingCapFolder() const
{
	return pathTrainingCapFolder;
}

string NOVAConfiguration::getPathTrainingFile() const
{
	return pathTrainingFile;
}

bool NOVAConfiguration::getReadPcap() const
{
	return readPcap;
}

int NOVAConfiguration::getSaMaxAttempts() const
{
	return saMaxAttempts;
}

int NOVAConfiguration::getSaPort() const
{
	return saPort;
}

double NOVAConfiguration::getSaSleepDuration() const
{
	return saSleepDuration;
}

int NOVAConfiguration::getSaveFreq() const
{
	return saveFreq;
}

int NOVAConfiguration::getTcpCheckFreq() const
{
	return tcpCheckFreq;
}

int NOVAConfiguration::getTcpTimout() const
{
	return tcpTimout;
}

int NOVAConfiguration::getThinningDistance() const
{
	return thinningDistance;
}

bool NOVAConfiguration::getUseTerminals() const
{
	return useTerminals;
}

void NOVAConfiguration::setClassificationThreshold(double classificationThreshold)
{
	this->classificationThreshold = classificationThreshold;
}

void NOVAConfiguration::setClassificationTimeout(int classificationTimeout)
{
	this->classificationTimeout = classificationTimeout;
}

void NOVAConfiguration::setConfigFilePath(string configFilePath)
{
	this->configFilePath = configFilePath;
}

void NOVAConfiguration::setDataTTL(int dataTTL)
{
	this->dataTTL = dataTTL;
}

void NOVAConfiguration::setDoppelInterface(string doppelInterface)
{
	this->doppelInterface = doppelInterface;
}

void NOVAConfiguration::setDoppelIp(string doppelIp)
{
	this->doppelIp = doppelIp;
}

void NOVAConfiguration::setEnabledFeatures(string enabledFeatures)
{
	this->enabledFeatures = enabledFeatures;
}

void NOVAConfiguration::setEps(double eps)
{
	this->eps = eps;
}

void NOVAConfiguration::setGotoLive(bool gotoLive)
{
	this->gotoLive = gotoLive;
}

void NOVAConfiguration::setInterface(string interface)
{
	this->interface = interface;
}

void NOVAConfiguration::setIsDmEnabled(bool isDmEnabled)
{
	this->isDmEnabled = isDmEnabled;
}

void NOVAConfiguration::setIsTraining(bool isTraining)
{
	this->isTraining = isTraining;
}

void NOVAConfiguration::setK(int k)
{
	this->k = k;
}

void NOVAConfiguration::setPathCESaveFile(string pathCESaveFile)
{
	this->pathCESaveFile = pathCESaveFile;
}

void NOVAConfiguration::setPathConfigHoneydDm(string pathConfigHoneydDm)
{
	this->pathConfigHoneydDm = pathConfigHoneydDm;
}

void NOVAConfiguration::setPathConfigHoneydHs(string pathConfigHoneydHs)
{
	this->pathConfigHoneydHs = pathConfigHoneydHs;
}

void NOVAConfiguration::setPathPcapFile(string pathPcapFile)
{
	this->pathPcapFile = pathPcapFile;
}

void NOVAConfiguration::setPathTrainingCapFolder(string pathTrainingCapFolder)
{
	this->pathTrainingCapFolder = pathTrainingCapFolder;
}

void NOVAConfiguration::setPathTrainingFile(string pathTrainingFile)
{
	this->pathTrainingFile = pathTrainingFile;
}

void NOVAConfiguration::setReadPcap(bool readPcap)
{
	this->readPcap = readPcap;
}

void NOVAConfiguration::setSaMaxAttempts(int saMaxAttempts)
{
	this->saMaxAttempts = saMaxAttempts;
}

void NOVAConfiguration::setSaPort(int saPort)
{
	this->saPort = saPort;
}

void NOVAConfiguration::setSaSleepDuration(double saSleepDuration)
{
	this->saSleepDuration = saSleepDuration;
}

void NOVAConfiguration::setSaveFreq(int saveFreq)
{
	this->saveFreq = saveFreq;
}

void NOVAConfiguration::setTcpCheckFreq(int tcpCheckFreq)
{
	this->tcpCheckFreq = tcpCheckFreq;
}

void NOVAConfiguration::setTcpTimout(int tcpTimout)
{
	this->tcpTimout = tcpTimout;
}

void NOVAConfiguration::setThinningDistance(int thinningDistance)
{
	this->thinningDistance = thinningDistance;
}

void NOVAConfiguration::setUseTerminals(bool useTerminals)
{
	this->useTerminals = useTerminals;
}

}

