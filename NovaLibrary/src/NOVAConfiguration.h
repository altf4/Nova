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

#ifndef NOVACONFIGURATION_H_
#define NOVACONFIGURATION_H_

#include "HashMapStructs.h"

using namespace std;

namespace Nova {

class NOVAConfiguration {

public:
	NOVAConfiguration(string configFilePath);
	~NOVAConfiguration();

	// Loads and parses a NOVA configuration file
	//      module - added s.t. rsyslog  will output NovaConfig messages as the parent process that called LoadConfig
	void LoadConfig(string module);

	// Loads default values for all variables
	void SetDefaults();

	// Checks to see if the current user has a ~/.nova directory, and creates it if not, along with default config files
	//	Returns: True if (after the function) the user has all necessary ~/.nova config files
	//		IE: Returns false only if the user doesn't have configs AND we weren't able to make them
    static bool InitUserConfigs(string homePath);

    string toString();

    // Getters
    string getConfigFilePath() const;
    string getDoppelInterface() const;
    string getDoppelIp() const;
    string getEnabledFeatures() const;
    string getInterface() const;
    string getPathCESaveFile() const;
    string getPathConfigHoneydDm() const;
    string getPathConfigHoneydHs() const;
    string getPathPcapFile() const;
    string getPathTrainingCapFolder() const;
    string getPathTrainingFile() const;

    bool getReadPcap() const;
    bool getUseTerminals() const;
    bool getIsDmEnabled() const;
    bool getIsTraining() const;
    bool getGotoLive() const;

    int getClassificationTimeout() const;
    int getDataTTL() const;
    int getK() const;
    int getSaMaxAttempts() const;
    int getSaPort() const;
    int getSaveFreq() const;
    int getTcpCheckFreq() const;
    int getTcpTimout() const;
    int getThinningDistance() const;

    double getClassificationThreshold() const;
    double getSaSleepDuration() const;
    double getEps() const;


    // Setters
    void setClassificationThreshold(double classificationThreshold);
    void setClassificationTimeout(int classificationTimeout);
    void setConfigFilePath(string configFilePath);
    void setDataTTL(int dataTTL);
    void setDoppelInterface(string doppelInterface);
    void setDoppelIp(string doppelIp);
    void setEnabledFeatures(string enabledFeatures);
    void setEps(double eps);
    void setGotoLive(bool gotoLive);
    void setInterface(string interface);
    void setIsDmEnabled(bool isDmEnabled);
    void setIsTraining(bool isTraining);
    void setK(int k);
    void setPathCESaveFile(string pathCESaveFile);
    void setPathConfigHoneydDm(string pathConfigHoneydDm);
    void setPathConfigHoneydHs(string pathConfigHoneydHs);
    void setPathPcapFile(string pathPcapFile);
    void setPathTrainingCapFolder(string pathTrainingCapFolder);
    void setPathTrainingFile(string pathTrainingFile);
    void setReadPcap(bool readPcap);
    void setSaMaxAttempts(int saMaxAttempts);
    void setSaPort(int saPort);
    void setSaSleepDuration(double saSleepDuration);
    void setSaveFreq(int saveFreq);
    void setTcpCheckFreq(int tcpCheckFreq);
    void setTcpTimout(int tcpTimout);
    void setThinningDistance(int thinningDistance);
    void setUseTerminals(bool useTerminals);

private:
	static const string prefixes[];
	static const string requiredFiles[];

	string interface;
	string doppelIp;
	string doppelInterface;
	string enabledFeatures;

	string pathConfigHoneydHs;
	string pathConfigHoneydDm;
	string pathPcapFile;
	string pathTrainingFile;
	string pathTrainingCapFolder;
	string pathCESaveFile;

	int tcpTimout;
	int tcpCheckFreq;
	int classificationTimeout;
	int saPort;
	int k;
	int thinningDistance;
	int saveFreq;
	int dataTTL;
	int saMaxAttempts;

	double saSleepDuration;
	double eps;
	double classificationThreshold;

	bool readPcap;
	bool gotoLive;
	bool useTerminals;
	bool isTraining;
	bool isDmEnabled;

	string configFilePath;
};
}

#endif /* NOVACONFIGURATION_H_ */
