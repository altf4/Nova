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

	// Loads the PATH file (usually in /etc)
	bool LoadPaths();

	// Loads default values for all variables
	void SetDefaults();

	// Checks to see if the current user has a ~/.nova directory, and creates it if not, along with default config files
	//	Returns: True if (after the function) the user has all necessary ~/.nova config files
	//		IE: Returns false only if the user doesn't have configs AND we weren't able to make them
    static bool InitUserConfigs(string homeNovaPath);
    static bool AddUserToGroup();

    string toString();

    // Getters
    string getConfigFilePath() ;
    string getDoppelInterface() ;
    string getDoppelIp() ;
    string getEnabledFeatures() ;
    bool isFeatureEnabled(int i) ;
    uint getEnabledFeatureCount() ;
    string getInterface() ;
    string getPathCESaveFile() ;
    string getPathConfigHoneydDm() ;
    string getPathConfigHoneydHs() ;
    string getPathPcapFile() ;
    string getPathTrainingCapFolder() ;
    string getPathTrainingFile() ;
    string getKey() ;
    vector<in_addr_t> getNeighbors() ;

    bool getReadPcap() ;
    bool getUseTerminals() ;
    bool getIsDmEnabled() ;
    bool getIsTraining() ;
    bool getGotoLive() ;

    int getClassificationTimeout() ;
    int getDataTTL() ;
    int getK() ;
    int getSaMaxAttempts() ;
    int getSaPort() ;
    int getSaveFreq() ;
    int getTcpCheckFreq() ;
    int getTcpTimout() ;
    int getThinningDistance() ;

    double getClassificationThreshold() ;
    double getSaSleepDuration() ;
    double getEps() ;

    string getGroup() ;

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
    string getLoggerPreferences() ;
    string getSMTPAddr() ;
    string getSMTPDomain() ;
    vector<string> getSMTPEmailRecipients() ;
    in_port_t getSMTPPort() ;
    void setLoggerPreferences(string loggerPreferences);
    void setSMTPAddr(string SMTPAddr);
    void setSMTPDomain(string SMTPDomain);
	void setSMTPPort(in_port_t SMTPPort);

	double getSqurtEnabledFeatures();

    // Set with a vector of email addresses
    void setSMTPEmailRecipients(vector<string> SMTPEmailRecipients);
    // Set with a CSV string from the config file
    void setSMTPEmailRecipients(string SMTPEmailRecipients);

    // Getters for the paths stored in /etc
    string getPathBinaries() ;
    string getPathWriteFolder() ;
    string getPathReadFolder() ;
    string getPathHome() ;

protected:
	Config();

private:
	static Config *m_instance;

	__attribute__ ((visibility ("hidden"))) static string m_prefixes[];
	__attribute__ ((visibility ("hidden"))) static string m_requiredFiles[];

	string m_interface;
	string m_doppelIp;
	string m_doppelInterface;

	// Enabled feature stuff, we provide a few formats and helpers
	string m_enabledFeatureMask;
	bool m_isFeatureEnabled[DIM];
	uint m_enabledFeatureCount;
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

	// the SMTP server domain name for display purposes
	string SMTPDomain;
	// the email address that will be set as sender
	string SMTPAddr;
	// the port for SMTP send; normally 25 if I'm not mistaken, may take this out
	in_port_t SMTPPort;

	string loggerPreferences;
	// a vector containing the email recipients; may move this into the actual classes
	// as opposed to being in this struct
	vector<string> SMTPEmailRecipients;

	// User config options
	vector<in_addr_t> m_neighbors;
	string m_key;

	string m_configFilePath;
	string m_userConfigFilePath;


	// Options from the PATHS file (currently /etc/nova/paths)
	string pathBinaries;
	string pathWriteFolder;
	string pathReadFolder;
	string pathHome;

	pthread_rwlock_t lock;

	// Used for loading the nova path file, resolves paths with env vars to full paths
	static string ResolvePathVars(string path);

	// Non-locking versions of some functions for internal use
	void setEnabledFeatures_noLocking(string enabledFeatureMask);

    // Set with a CSV string from the config file
    void setSMTPEmailRecipients_noLocking(string SMTPEmailRecipients);

};
}

#endif /* NOVACONFIGURATION_H_ */
