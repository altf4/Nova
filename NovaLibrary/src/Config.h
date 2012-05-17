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
    static bool InitUserConfigs(std::string homeNovaPath);


    // These are generic static getters/setters for the web interface
    // Use of these should be minimized. Instead, use the specific typesafe getter/setter
    // that you need.
    std::string ReadSetting(std::string key);
    bool WriteSetting(std::string key, std::string value);


    std::string ToString();

    // Getters
    std::string GetConfigFilePath();
    std::string GetDoppelInterface();
    std::string GetDoppelIp();
    std::string GetEnabledFeatures();
    bool IsFeatureEnabled(int i) ;
    uint GetEnabledFeatureCount();
    std::string GetInterface();
    std::string GetPathCESaveFile();
    std::string GetPathConfigHoneydUser();
    std::string GetPathConfigHoneydHS();
    std::string GetPathPcapFile();
    std::string GetPathTrainingCapFolder();
    std::string GetPathTrainingFile();
    std::string GetKey();
    std::vector<in_addr_t> GetNeighbors();

    bool GetReadPcap();
    bool GetUseTerminals();
    bool GetIsDmEnabled();
    bool GetIsTraining();
    bool GetGotoLive();

    int GetClassificationTimeout();
    int GetDataTTL();
    int GetK();
    int GetSaMaxAttempts();
    int GetSaPort();
    int GetSaveFreq();
    int GetTcpCheckFreq();
    int GetTcpTimout();
    double GetThinningDistance();

    double GetClassificationThreshold();
    double GetSaSleepDuration();
    double GetEps();

    std::string GetGroup();

    // Setters
    void SetClassificationThreshold(double classificationThreshold);
    void SetClassificationTimeout(int classificationTimeout);
    void SetConfigFilePath(std::string configFilePath);
    void SetDataTTL(int dataTTL);
    void SetDoppelInterface(std::string doppelInterface);
    void SetDoppelIp(std::string doppelIp);
    void SetEnabledFeatures(std::string enabledFeatureMask);
    void SetEps(double eps);
    void SetGotoLive(bool gotoLive);
    void SetInterface(std::string interface);
    void SetIsDmEnabled(bool isDmEnabled);
    void SetIsTraining(bool isTraining);
    void SetK(int k);
    void SetPathCESaveFile(std::string pathCESaveFile);
    void SetPathConfigHoneydUser(std::string pathConfigHoneydUser);
    void SetPathConfigHoneydHs(std::string pathConfigHoneydHs);
    void SetPathPcapFile(std::string pathPcapFile);
    void SetPathTrainingCapFolder(std::string pathTrainingCapFolder);
    void SetPathTrainingFile(std::string pathTrainingFile);
    void SetReadPcap(bool readPcap);
    void SetSaMaxAttempts(int saMaxAttempts);
    void SetSaPort(int saPort);
    void SetSaSleepDuration(double saSleepDuration);
    void SetSaveFreq(int saveFreq);
    void SetTcpCheckFreq(int tcpCheckFreq);
    void SetTcpTimout(int tcpTimout);
    void SetThinningDistance(double thinningDistance);
    void SetUseTerminals(bool useTerminals);
    void SetKey(std::string key);
    void SetNeigbors(std::vector<in_addr_t> neighbors);
    void SetGroup(std::string group);

    std::string GetLoggerPreferences();
    std::string GetSMTPAddr();
    std::string GetSMTPDomain();
    std::vector<std::string> GetSMTPEmailRecipients();
    in_port_t GetSMTPPort();

    void SetLoggerPreferences(std::string loggerPreferences);
    void SetSMTPAddr(std::string SMTPAddr);
    void SetSMTPDomain(std::string SMTPDomain);
	void SetSMTPPort(in_port_t SMTPPort);

	double GetSqurtEnabledFeatures();

    // Set with a vector of email addresses
    void SetSMTPEmailRecipients(std::vector<std::string> SMTPEmailRecipients);
    // Set with a CSV std::string from the config file
    void SetSMTPEmailRecipients(std::string SMTPEmailRecipients);

    // Getters for the paths stored in /etc
    std::string GetPathBinaries();
    std::string GetPathWriteFolder();
    std::string GetPathReadFolder();
    std::string GetPathHome();
    std::string GetPathIcon();

    char GetHaystackStorage();
    void SetHaystackStorage(char haystackStorage);

    std::string GetUserPath();
    void SetUserPath(std::string userPath);


protected:
	Config();

private:
	static Config *m_instance;

	__attribute__ ((visibility ("hidden"))) static std::string m_prefixes[];
	__attribute__ ((visibility ("hidden"))) static std::string m_requiredFiles[];

	std::string m_interface;
	std::string m_doppelIp;
	std::string m_doppelInterface;

	// Enabled feature stuff, we provide a few formats and helpers
	std::string m_enabledFeatureMask;
	bool m_isFeatureEnabled[DIM];
	uint m_enabledFeatureCount;
	double m_squrtEnabledFeatures;


	std::string m_pathConfigHoneydHs;
	std::string m_pathConfigHoneydUser;
	std::string m_pathPcapFile;
	std::string m_pathTrainingFile;
	std::string m_pathTrainingCapFolder;
	std::string m_pathCESaveFile;

	std::string m_group;

	int m_tcpTimout;
	int m_tcpCheckFreq;
	int m_classificationTimeout;
	int m_saPort;
	int m_k;
	double m_thinningDistance;
	int m_saveFreq;
	int m_dataTTL;
	int m_saMaxAttempts;

	double m_saSleepDuration;
	double m_eps;
	double m_classificationThreshold;

	bool m_readPcap;
	bool m_gotoLive;
	bool m_isTraining;
	bool m_isDmEnabled;

	// the SMTP server domain name for display purposes
	std::string m_SMTPDomain;
	// the email address that will be set as sender
	std::string m_SMTPAddr;
	// the port for SMTP send; normally 25 if I'm not mistaken, may take this out
	in_port_t m_SMTPPort;

	std::string m_loggerPreferences;
	// a vector containing the email recipients; may move this into the actual classes
	// as opposed to being in this struct
	std::vector<std::string> m_SMTPEmailRecipients;

	// User config options
	std::vector<in_addr_t> m_neighbors;
	std::string m_key;

	std::string m_configFilePath;
	std::string m_userConfigFilePath;


	// Options from the PATHS file (currently /etc/nova/paths)
	std::string m_pathBinaries;
	std::string m_pathWriteFolder;
	std::string m_pathReadFolder;
	std::string m_pathHome;
	std::string m_pathIcon;

	char m_haystackStorage;
	std::string m_userPath;

	pthread_rwlock_t m_lock;

	// Used for loading the nova path file, resolves paths with env vars to full paths
	static std::string ResolvePathVars(std::string path);

	// Non-locking versions of some functions for internal use
	void SetEnabledFeatures_noLocking(std::string enabledFeatureMask);

    // Set with a CSV std::string from the config file
    void SetSMTPEmailRecipients_noLocking(std::string SMTPEmailRecipients);

};
}

#endif /* NOVACONFIGURATION_H_ */
