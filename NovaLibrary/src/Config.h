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
//============================================================================/*

#ifndef NOVACONFIGURATION_H_
#define NOVACONFIGURATION_H_

#include "HashMapStructs.h"
#include "ClassificationEngine.h"

namespace Nova
{

#define VERSION_FILE_NAME "version.txt"

struct version
{
	std::string versionString;
	int buildYear;
	int buildMonth;
	int minorVersion;

	version()
	{
		versionString = "VERSION NOT SET";
		buildYear = 0;
		buildMonth = 0;
		minorVersion = 0;
	}
};

enum CLASSIFIER_MODES {
	CLASSIFIER_WEIGHTED,
	CLASSIFIER_HOSTILE_OVERRIDE,
	CLASSIFIER_BENIGN_OVERRIDE
};

enum NormalizationType {
	NONORM, 		// Does no data normalization. Feature must already be in a range from 0 to 1
	LINEAR,			// Simple linear normalization by dividing data by the max
	LINEAR_SHIFT, 	// Shifts min value to 0 before doing linear normalization
	LOGARITHMIC		// Logarithmic normalization, larger outlier value will have less of an effect
};

class Config
{

public:
	// This is a singleton class, use this to access it
	static Config *Inst();

	~Config();

	// Loads and parses a NOVA configuration file
	//      module - added s.t. rsyslog  will output NovaConfig messages as the parent process that called LoadConfig
	void LoadConfig();
	void LoadCustomSettings(int argc,  char** argv);

	// Checks to see if the current user has a ~/.nova directory, and creates it if not, along with default config files
	//	Returns: True if(after the function) the user has all necessary ~/.nova config files
	//		IE: Returns false only if the user doesn't have configs AND we weren't able to make them
    bool InitUserConfigs();

    // These are generic static getters/setters for the web interface
    // Use of these should be minimized. Instead, use the specific typesafe getter/setter
    // that you need.
    std::string ReadSetting(std::string key);
    bool WriteSetting(std::string key, std::string value);

    // Getters
    std::string GetConfigFilePath();
    std::string GetDoppelInterface();
    std::string GetDoppelIp();
    std::string GetInterface(uint i);
    std::vector<std::string> GetInterfaces();
    std::vector<std::string> GetIPv4HostInterfaceList();
    std::vector<std::string> GetIPv4LoopbackInterfaceList();
    uint GetInterfaceCount();
    std::string GetPathCESaveFile();
    std::string GetPathConfigHoneydUser();
    std::string GetPathConfigHoneydHS();
    std::string GetPathPcapFile();
    std::string GetPathWhitelistFile();
    std::string GetKey();
    std::vector<in_addr_t> GetNeighbors();

    bool GetReadPcap();
    bool GetCustomReadPcap();
    bool GetUseTerminals();
    bool GetIsDmEnabled();
    bool GetGotoLive();

    bool GetUseAllInterfaces();
    bool GetUseAnyLoopback();
    std::string GetUseAllInterfacesBinding();

    int GetClassificationTimeout();
    int GetDataTTL();
    int GetK();
    int GetSaveFreq();
    double GetThinningDistance();

    double GetClassificationThreshold();
    double GetEps();

    std::string GetGroup();

    // Setters
    void AddInterface(std::string interface);
    void RemoveInterface(std::string interface);
    void ClearInterfaces();
    std::vector<std::string> ListInterfaces();
    std::vector<std::string> ListLoopbacks();
    bool SetInterfaces(std::vector<std::string> newInterfaceList);
    void SetUseAllInterfaces(bool which);
    void SetUseAnyLoopback(bool which);
    bool SetUseAllInterfacesBinding(bool which);
    bool SetUseAnyLoopbackBinding(bool which);

    void SetClassificationThreshold(double classificationThreshold);
    void SetClassificationTimeout(int classificationTimeout);
    void SetConfigFilePath(std::string configFilePath);
    void SetDataTTL(int dataTTL);
    void SetDoppelInterface(std::string doppelInterface);
    void SetDoppelIp(std::string doppelIp);
    void SetEps(double eps);
    void SetGotoLive(bool gotoLive);
    void SetIsDmEnabled(bool isDmEnabled);
    void SetK(int k);
    void SetPathCESaveFile(std::string pathCESaveFile);
    void SetPathConfigHoneydUser(std::string pathConfigHoneydUser);
    void SetPathConfigHoneydHs(std::string pathConfigHoneydHs);
    void SetPathPcapFile(std::string pathPcapFile);
    void SetPathWhitelistFile(std::string pathWhitelistFile);
    void SetReadPcap(bool readPcap);
    void SetReadCustomPcap(bool readCustomPcap);
    void SetSaveFreq(int saveFreq);
    void SetThinningDistance(double thinningDistance);
    void SetUseTerminals(bool useTerminals);
    void SetKey(std::string key);
    void SetNeigbors(std::vector<in_addr_t> neighbors);
    void SetGroup(std::string group);
    bool SetCurrentConfig(std::string configName);

    void SetIpListPath(std::string path);
    std::string GetIpListPath();

    std::string GetLoggerPreferences();
    std::string GetSMTPAddr();
    std::string GetSMTPDomain();
    std::vector<std::string> GetSMTPEmailRecipients();
    in_port_t GetSMTPPort();
    std::string GetSMTPUser();
    std::string GetSMTPPass();
    bool GetSMTPUseAuth();

    void SetLoggerPreferences(std::string loggerPreferences);
    void SetSMTPUseAuth(bool useAuth);
    void SetSMTPAddr(std::string SMTPAddr);
    void SetSMTPDomain(std::string SMTPDomain);
	void SetSMTPPort(in_port_t SMTPPort);
	bool SetSMTPUser(std::string SMTPUser);
	bool SetSMTPPass(std::string STMP_Pass);

	bool UseRsyslog();
	std::string GetRsyslogIP();
	std::string GetRsyslogPort();
	std::string GetRsyslogConnType();
	void SetUseRsyslog(bool useRsyslog);
	void SetRsyslogIP(std::string newIp);
	void SetRsyslogPort(std::string newPort);
	void SetRsyslogConnType(std::string newConnType);

	bool GetSMTPSettings_FromFile();
	bool SaveSMTPSettings();

    // Set with a vector of email addresses
    void SetSMTPEmailRecipients(std::vector<std::string> SMTPEmailRecipients);
    // Set with a CSV std::string from the config file
    void SetSMTPEmailRecipients(std::string SMTPEmailRecipients);

    // Getters for the paths stored in /etc
    std::string GetPathReadFolder();
    std::string GetPathHome();

    inline std::string GetPathShared() {
    	return m_pathPrefix + "/usr/share/nova/sharedFiles";
    }

    std::string GetPathIcon();

    char GetHaystackStorage();
    void SetHaystackStorage(char haystackStorage);

    std::string GetUserPath();
    void SetUserPath(std::string userPath);

    uint GetMinPacketThreshold();
    void SetMinPacketThreshold(uint packets);

	std::string GetCustomPcapString();
	void SetCustomPcapString(std::string customPcapString);

	bool GetOverridePcapString();
	void SetOverridePcapString(bool overridePcapString);

	std::string GetTrainingSession();
	std::string SetTrainingSession(std::string trainingSession);
	int GetWebUIPort();
	void SetWebUIPort(int port);

	std::string GetCurrentConfig();

	version GetVersion();
	std::string GetVersionString();

	static std::vector <std::string> GetHaystackAddresses(std::string honeyDConfigPath);
	static std::vector <std::string> GetIpAddresses(std::string ipListFile);
	static std::vector <std::string> GetHoneydIpAddresses(std::string ipListFile);

	std::vector<std::string> GetPrefixes();

	bool GetClearAfterHostile();
	void SetClearAfterHostile(bool clearAfterHostile);

	int GetCaptureBufferSize();
	void SetCaptureBufferSize(int bufferSize);

	std::vector<std::string> GetClassificationEngines();
	std::vector<std::string> GetClassifierConfigs();
	std::vector<CLASSIFIER_MODES> GetClassifierModes();
	std::vector<int> GetClassifierWeights();

	bool GetOnlyClassifyHoneypotTraffic();

	void SetMasterUIPort(int newPort);
	int GetMasterUIPort();

	//Attempts to detect and use intefaces returned by pcap_lookupdev
	void LoadInterfaces();

	bool GetAreEmailAlertsEnabled();
	std::string GetPathTrainingData();

	vector<string> GetSupportedEngines();


	std::string GetCommandStartNovad();
	std::string GetCommandStopNovad();
	std::string GetCommandStartHaystack();
	std::string GetCommandStopHaystack();

	vector<NormalizationType> GetNormalizationFunctions();

protected:
	Config();

private:
	static Config *m_instance;

	__attribute__ ((visibility ("hidden"))) static std::string m_prefixes[];

	bool m_rsyslogUse;
	std::string m_rsyslogIP;
	std::string m_rsyslogPort;
	std::string m_rsyslogConnType;

	std::string m_doppelIp;
	std::string m_loopbackIF;
	std::string m_loopbackIFString;
	bool m_loIsDefault;
	bool m_ifIsDefault;

	// List of currently used interfaces
	std::vector<std::string> m_interfaces;

	// What the actual config file contains
	std::string m_interfaceLine;

	std::string m_pathConfigHoneydHs;
	std::string m_pathConfigHoneydUser;
	std::string m_pathPcapFile;
	std::string m_pathWhitelistFile;
	std::string m_pathCESaveFile;
	std::string m_pathTrainingData;

	std::string m_customPcapString;

	std::string m_group;

	int m_tcpTimout;
	int m_tcpCheckFreq;
	int m_classificationTimeout;
	int m_k;
	double m_thinningDistance;
	int m_saveFreq;
	int m_dataTTL;
	uint m_minPacketThreshold;
	int m_webUIPort;
	std::string m_webUIIface;
	int m_masterUIPort;

	int m_captureBufferSize;

	double m_eps;
	double m_classificationThreshold;

	bool m_readPcap;
	bool m_readCustomPcap;
	bool m_gotoLive;
	bool m_isDmEnabled;

	bool m_overridePcapString;

	bool m_emailAlertsEnabled;

	version m_version;

	std::string m_iplistPath;

	static std::string m_pathsFile;

	vector<NormalizationType> m_normalization;

	// the SMTP server domain name for display purposes
	std::string m_SMTPDomain;
	// the email address that will be set as sender
	std::string m_SMTPAddr;
	// the port for SMTP send; normally 25 if I'm not mistaken, 465 for SSL and 5 hundred something for TLS
	in_port_t m_SMTPPort;

	// username:password combination for interacting with the SMTP account that acts
	// as the relay for Nova mail alerts
	// need to find some way to store the password in an encrypted fashion yet
	// still have it as a Config private attribute
	std::string m_SMTPUser;
	std::string m_SMTPPass;

	bool m_SMTPUseAuth;

	bool m_clearAfterHostile;

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
	std::string m_pathReadFolder;
	std::string m_pathHomeConfig;
	std::string m_pathHome;
	std::string m_pathIcon;

	std::string m_trainingSession;

	std::vector<std::string> m_classifierEngines;
	std::vector<std::string> m_classifierConfigs;
	std::vector<CLASSIFIER_MODES> m_classifierTypes;
	std::vector<int> m_classifierWeights;

	bool m_masterUIEnabled;
	int m_masterUIReconnectTime;
	std::string m_masterUIIP;
	std::string m_masterUIClientID;

	bool m_onlyClassifyHoneypotTraffic;

	std::string m_currentConfig;

	static std::string m_pathPrefix;

	std::string m_commandStartNovad;
	std::string m_commandStopNovad;

	std::string m_commandStartHaystack;
	std::string m_commandStopHaystack;

	pthread_rwlock_t m_lock;

	// Used for loading the nova path file, resolves paths with env vars to full paths
	static std::string ResolvePathVars(std::string path);

    // Set with a CSV std::string from the config file
    void SetSMTPEmailRecipients_noLocking(std::string SMTPEmailRecipients);

	// Loads the PATH file (usually in /etc)
	bool LoadPaths();

	// Loads the version file
	bool LoadVersionFile();


	bool LoadUserConfig();

	//Private version of LoadConfig so the public version can call LoadInterfaces()
	//	LoadInterfaces cannot be called until m_instance has been created, but needs to execute after every load
	//	However the constructor calls LoadConfig, so we use a private version instead that doesn't include
	//	LoadInterfaces() which is called elsewhere
	void LoadConfig_Internal();

};
}

#endif /* NOVACONFIGURATION_H_ */
