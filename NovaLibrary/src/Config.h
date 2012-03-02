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
#include "Defines.h"

using namespace std;

namespace Nova {

class Config {

public:
	// This is a singleton class, use this to access it
	static Config* Inst();


	~Config();

	// Loads and parses a NOVA configuration file
	//      module - added s.t. rsyslog  will output NovaConfig messages as the parent process that called LoadConfig
	void LoadConfig();
	bool SaveConfig();

	bool LoadUserConfig();
	// TODO: SaveUserConfig();
	// We don't have any GUI stuff to edit this.. but we should

	// Loads default values for all variables
	void SetDefaults();

	// Checks to see if the current user has a ~/.nova directory, and creates it if not, along with default config files
	//	Returns: True if (after the function) the user has all necessary ~/.nova config files
	//		IE: Returns false only if the user doesn't have configs AND we weren't able to make them
    static bool InitUserConfigs(string homeNovaPath);
    static bool AddUserToGroup();

    string toString();

    // Getters
    string getConfigFilePath() const;
    string getDoppelInterface() const;
    string getDoppelIp() const;
    string getEnabledFeatures() const;
    bool isFeatureEnabled(int i) const;
    int getEnabledFeatureCount() const;
    string getInterface() const;
    string getPathCESaveFile() const;
    string getPathConfigHoneydDm() const;
    string getPathConfigHoneydHs() const;
    string getPathPcapFile() const;
    string getPathTrainingCapFolder() const;
    string getPathTrainingFile() const;
    string getKey() const;
    vector<in_addr_t> getNeighbors() const;

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

    string getGroup() const;

    // Setters
    void setClassificationThreshold(double classificationThreshold);
    void setClassificationTimeout(int classificationTimeout);
    void setConfigFilePath(string configFilePath);
    void setDataTTL(int dataTTL);
    void setDoppelInterface(string doppelInterface);
    void setDoppelIp(string doppelIp);
    void setEnabledFeatures(string enabledFeatureMask);
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
    void setKey(string key);
    void setNeigbors(vector<in_addr_t> neighbors);
    void setGroup(string group);

protected:
	Config();

private:
	static Config *m_instance;

	static const string m_prefixes[];
	static const string m_requiredFiles[];

	string m_interface;
	string m_doppelIp;
	string m_doppelInterface;

	// Enabled feature stuff, we provide a few formats and helpers
	string m_enabledFeatureMask;
	bool m_isFeatureEnabled[DIM];
	int m_enabledFeatureCount;
	double m_squrtEnabledFeatures;


	string m_pathConfigHoneydHs;
	string m_pathConfigHoneydDm;
	string m_pathPcapFile;
	string m_pathTrainingFile;
	string m_pathTrainingCapFolder;
	string m_pathCESaveFile;

	string m_group;

	int m_tcpTimout;
	int m_tcpCheckFreq;
	int m_classificationTimeout;
	int m_saPort;
	int m_k;
	int m_thinningDistance;
	int m_saveFreq;
	int m_dataTTL;
	int m_saMaxAttempts;

	double m_saSleepDuration;
	double m_eps;
	double m_classificationThreshold;

	bool m_readPcap;
	bool m_gotoLive;
	bool m_useTerminals;
	bool m_isTraining;
	bool m_isDmEnabled;

	// User config options
	vector<in_addr_t> m_neighbors;
	string m_key;

	string m_configFilePath;
	string m_userConfigFilePath;
};
}

#endif /* NOVACONFIGURATION_H_ */
