//============================================================================
// Name        : ClassificationEngine.cpp
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
// Description : Classifies suspects as either hostile or benign then takes appropriate action
//============================================================================

#include "ClassificationEngine.h"
#include "NOVAConfiguration.h"
#include "NovaUtil.h"
#include "Logger.h"
#include "Point.h"

#include <vector>
#include <string>
#include <errno.h>
#include <fstream>
#include <sstream>
#include <syslog.h>
#include <sys/un.h>
#include <signal.h>
#include <math.h>

using namespace std;
using namespace Nova;

//Used in classification algorithm. Store it here so we only need to calculate it once
double sqrtDIM;

// Maintains a list of suspects and information on network activity
SuspectHashTable suspects;
SuspectHashTable suspectsSinceLastSave;

// Vector of ip addresses that correspond to other nova instances
vector<in_addr_t> neighbors;

// Key used for port knocking
string key;

pthread_rwlock_t lock;

// Buffers
u_char buffer[MAX_MSG_SIZE];
u_char data[MAX_MSG_SIZE];
u_char GUIData[MAX_MSG_SIZE];

//NOT normalized
vector <Point*> dataPtsWithClass;
static struct sockaddr_in hostAddr;

//** Main (ReceiveSuspectData) **
struct sockaddr_un remote;
struct sockaddr* remoteSockAddr = (struct sockaddr *)&remote;

int CE_IPCsock;

//** Silent Alarm **
struct sockaddr_un alarmRemote;
struct sockaddr* alarmRemotePtr =(struct sockaddr *)&alarmRemote;
struct sockaddr_in serv_addr;
struct sockaddr* serv_addrPtr = (struct sockaddr *)&serv_addr;

int oldClassification;
int sockfd = 1;

//** ReceiveGUICommand **
int GUISocket;

//** SendToUI **
struct sockaddr_un GUISendRemote;
struct sockaddr* GUISendPtr = (struct sockaddr *)&GUISendRemote;
int GUISendSocket;
int GUILen;

//Universal Socket variables (constants that can be re-used)
int socketSize = sizeof(remote);
int inSocketSize = sizeof(serv_addr);
socklen_t * sockSizePtr = (socklen_t*)&socketSize;

// kdtree stuff
int nPts = 0;						//actual number of data points
ANNpointArray dataPts;				//data points
ANNpointArray normalizedDataPts;	//normalized data points
ANNkd_tree*	kdTree;					// search structure
string dataFile;					//input for data points

//Used to indicate if the kd tree needs to be reformed
bool updateKDTree = false;

// Used for disabling features
bool featureEnabled[DIM];
uint32_t featureMask;
int enabledFeatures;

// Misc
int len;
const char *outFile;				//output for data points during training
extern string userHomePath;
extern string novaConfigPath;

// Used for data normalization
double maxFeatureValues[DIM];
double minFeatureValues[DIM];
double meanFeatureValues[DIM];

// Nova Configuration Variables (read from config file)
bool isTraining;
string trainingFolder;
string trainingCapFile;
bool useTerminals;
in_port_t sAlarmPort;					//Silent Alarm destination port
int classificationTimeout;		//In seconds, how long to wait between classifications
int k;							//number of nearest neighbors
double eps;						//error bound
double classificationThreshold ; //value of classification to define split between hostile / benign
uint SA_Max_Attempts;			//The number of times to attempt to reconnect to a neighbor
double SA_Sleep_Duration;		//The time to sleep after a port knocking request and allow it to go through
Logger * loggerConf;
int saveFrequency;
int dataTTL;
string ceSaveFile;
string SMTP_addr;
string SMTP_domain;
in_port_t SMTP_port;
Nova::userMap service_pref;
vector<string> email_recipients;

// Normalization method to use on each feature
// TODO: Make this a configuration var somewhere in Novaconfig.txt?
normalizationType normalization[] = {
		LINEAR_SHIFT, // Don't normalize IP traffic distribution, already between 0 and 1
		LINEAR_SHIFT,
		LOGARITHMIC,
		LOGARITHMIC,
		LOGARITHMIC,
		LOGARITHMIC,
		LOGARITHMIC,
		LOGARITHMIC,
		LOGARITHMIC
};

// End configured variables

time_t lastLoadTime;
time_t lastSaveTime;

bool enableGUI = true;

void *Nova::ClassificationEngineMain(void *ptr)
{
	bzero(GUIData,MAX_MSG_SIZE);
	bzero(data,MAX_MSG_SIZE);
	bzero(buffer, MAX_MSG_SIZE);

	pthread_rwlock_init(&lock, NULL);

	signal(SIGINT, saveAndExit);

	lastLoadTime = time(NULL);
	if (lastLoadTime == ((time_t)-1))
		syslog(SYSL_ERR, "Line: %d Error unable to get timestamp", __LINE__);

	lastSaveTime = time(NULL);
	if (lastSaveTime == ((time_t)-1))
		syslog(SYSL_ERR, "Line: %d Error unable to get timestamp", __LINE__);

	suspects.set_empty_key(0);
	suspects.resize(INIT_SIZE_SMALL);

	suspectsSinceLastSave.set_empty_key(0);
	suspectsSinceLastSave.resize(INIT_SIZE_SMALL);

	struct sockaddr_un localIPCAddress;

	pthread_t classificationLoopThread;
	pthread_t trainingLoopThread;
	pthread_t silentAlarmListenThread;
	pthread_t GUIListenThread;

	loggerConf = new Logger(novaConfigPath.c_str(), true);

	if(chdir(userHomePath.c_str()) == -1)
	    loggerConf->Logging(INFO, "Failed to change directory to " + userHomePath);


	Reload();

	if(!useTerminals)
	{
		openlog("ClassificationEngine", NO_TERM_SYSL, LOG_AUTHPRIV);
	}

	else
	{
		openlog("ClassificationEngine", OPEN_SYSL, LOG_AUTHPRIV);
	}


	pthread_create(&GUIListenThread, NULL, CE_GUILoop, NULL);
	//Are we Training or Classifying?
	if(isTraining)
	{
		// We suffix the training capture files with the date/time
		time_t rawtime;
		time ( &rawtime );
		struct tm * timeinfo = localtime(&rawtime);

		char buffer [40];
		strftime (buffer,40,"%m-%d-%y_%H-%M-%S",timeinfo);
		trainingCapFile = userHomePath + "/" + trainingFolder + "/training" + buffer + ".dump";

		enabledFeatures = DIM;
		pthread_create(&trainingLoopThread,NULL,TrainingLoop,(void *)outFile);
	}
	else
	{
		pthread_create(&classificationLoopThread,NULL,ClassificationLoop, NULL);
		pthread_create(&silentAlarmListenThread,NULL,SilentAlarmLoop, NULL);
	}

	if((CE_IPCsock = socket(AF_UNIX,SOCK_STREAM,0)) == -1)
	{
		syslog(SYSL_ERR, "Line: %d socket: %s", __LINE__, strerror(errno));
		close(CE_IPCsock);
		exit(EXIT_FAILURE);
	}

	localIPCAddress.sun_family = AF_UNIX;

	//Builds the key path
	string key = KEY_FILENAME;
	key = userHomePath + key;

	strcpy(localIPCAddress.sun_path, key.c_str());
	unlink(localIPCAddress.sun_path);
	len = strlen(localIPCAddress.sun_path) + sizeof(localIPCAddress.sun_family);

    if(bind(CE_IPCsock,(struct sockaddr *)&localIPCAddress,len) == -1)
    {
    	syslog(SYSL_ERR, "Line: %d bind: %s", __LINE__, strerror(errno));
    	close(CE_IPCsock);
        exit(EXIT_FAILURE);
    }

    if(listen(CE_IPCsock, SOCKET_QUEUE_SIZE) == -1)
    {
    	syslog(SYSL_ERR, "Line: %d listen: %s", __LINE__, strerror(errno));
		close(CE_IPCsock);
        exit(EXIT_FAILURE);
    }


    // This is the structure that you'll use whenever you call Logging. The first argument
    // is the severity, can be found either in Logger.h's enum Levels, or you can
    // just use any of the syslog levels. All caps, always.
    // The second is the string to display in any of the mediums through which logging
    // will route based on severity level.
    loggerConf->Logging(INFO, "Classification Engine has begun the main loop...");


    //"Main Loop"
	while(true)
	{
		if (CEReceiveSuspectData())
		{
			if(!classificationTimeout)
			{
				if (!isTraining)
					ClassificationLoop(NULL);
				else
					TrainingLoop(NULL);
			}
		}
	}

	//Shouldn't get here!
	syslog(SYSL_ERR, "Line: %d Main thread ended. Shouldn't get here!!!", __LINE__);
	close(CE_IPCsock);
}

//Called when process receives a SIGINT, like if you press ctrl+c
void Nova::saveAndExit(int param)
{
	pthread_rwlock_wrlock(&lock);
	AppendToStateFile();
	pthread_rwlock_unlock(&lock);

	exit(EXIT_SUCCESS);
}

void Nova::AppendToStateFile()
{
	lastSaveTime = time(NULL);
	if (lastSaveTime == ((time_t)-1))
		syslog(SYSL_ERR, "Line: %d Error unable to get timestamp", __LINE__);

	// Don't bother saving if no new suspect data, just confuses deserialization
	if (suspectsSinceLastSave.size() <= 0)
		return;

	u_char tableBuffer[MAX_MSG_SIZE];
	uint32_t dataSize = 0;
	uint32_t index = 0;

	// Compute the total dataSize
	for (SuspectHashTable::iterator it = suspectsSinceLastSave.begin(); it != suspectsSinceLastSave.end(); it++)
	{
		if (!it->second->features.packetCount)
			continue;

		index = it->second->SerializeSuspect(tableBuffer);
		index += it->second->features.SerializeFeatureData(tableBuffer + index);

		dataSize += index;
	}

	// No suspects with packets to update
	if (dataSize == 0)
		return;

	ofstream out(ceSaveFile.data(), ofstream::binary | ofstream::app);
	if(!out.is_open())
	{
		syslog(SYSL_ERR, "Line: %d unbable open CE save file %s", __LINE__, ceSaveFile.c_str());
		return;
	}

	out.write((char*)&lastSaveTime, sizeof lastSaveTime);
	out.write((char*)&dataSize, sizeof dataSize);

	syslog(SYSL_INFO, "Line: %d Appending %d bytes to the CE state save file at %s", __LINE__, dataSize, ceSaveFile.c_str());

	// Serialize our suspect table
	for (SuspectHashTable::iterator it = suspectsSinceLastSave.begin(); it != suspectsSinceLastSave.end(); it++)
	{
		if (!it->second->features.packetCount)
			continue;

		index = it->second->SerializeSuspect(tableBuffer);
		index += it->second->features.SerializeFeatureData(tableBuffer + index);

		out.write((char*) tableBuffer, index);
	}

	out.close();

	// Clear out the unsaved data table (they're all saved now)
	for (SuspectHashTable::iterator it = suspectsSinceLastSave.begin(); it != suspectsSinceLastSave.end(); it++)
		delete it->second;
	suspectsSinceLastSave.clear();
}

void Nova::LoadStateFile()
{
	time_t timeStamp;
	uint32_t dataSize;

	lastLoadTime = time(NULL);
	if (lastLoadTime == ((time_t)-1))
		syslog(SYSL_ERR, "Line: %d Error unable to get timestamp", __LINE__);

	// Open input file
	ifstream in(ceSaveFile.data(), ios::binary | ios::in);
	if(!in.is_open())
	{
		syslog(SYSL_ERR, "Line: %d Attempted load last saved state but unable to open CE load file. This in normal for first run.", __LINE__);
		return;
	}

	// get length of input for error checking of partially written files
	in.seekg (0, ios::end);
	uint lengthLeft = in.tellg();
	in.seekg (0, ios::beg);

	while (in.is_open() && !in.eof() && lengthLeft)
	{
		// Bytes left, but not enough to make a header (timestamp + size)?
		if (lengthLeft < (sizeof timeStamp + sizeof dataSize))
		{
			syslog(SYSL_ERR, "Line %d Error: Data file should have another header entry, but contains only %d bytes left", __LINE__, lengthLeft);
			break;
		}

		in.read((char*) &timeStamp, sizeof timeStamp);
		lengthLeft -= sizeof timeStamp;

		in.read((char*) &dataSize, sizeof dataSize);
		lengthLeft -= sizeof dataSize;

		if (dataTTL && (timeStamp < lastLoadTime - dataTTL))
		{
			syslog(SYSL_INFO, "Line %d: Throwing out old CE save state with timestamp of %d", __LINE__, (int)timeStamp);
			in.seekg(dataSize, ifstream::cur);
			lengthLeft -= dataSize;
			continue;
		}

		// Not as many bytes left as the size of the entry?
		if (lengthLeft < dataSize)
		{
			syslog(SYSL_ERR, "Line %d Error: Data file should have another entry of size %d, but contains only %d bytes left", __LINE__, dataSize, lengthLeft);
			break;
		}

		u_char tableBuffer[dataSize];
		in.read((char*) tableBuffer, dataSize);
		lengthLeft -= dataSize;

		// Read each suspect
		uint32_t bytesSoFar = 0;
		while (bytesSoFar < dataSize)
		{
			Suspect* newSuspect = new Suspect();
			uint32_t suspectBytes = 0;
			suspectBytes += newSuspect->DeserializeSuspect(tableBuffer + bytesSoFar + suspectBytes);
			suspectBytes += newSuspect->features.DeserializeFeatureData(tableBuffer + bytesSoFar + suspectBytes);
			bytesSoFar += suspectBytes;

			if (suspects[newSuspect->IP_address.s_addr] == NULL)
			{
				suspects[newSuspect->IP_address.s_addr] = newSuspect;
				suspects[newSuspect->IP_address.s_addr]->needs_feature_update = true;
				suspects[newSuspect->IP_address.s_addr]->needs_classification_update = true;
			}
			else
			{
				suspects[newSuspect->IP_address.s_addr]->features += newSuspect->features;
				suspects[newSuspect->IP_address.s_addr]->needs_feature_update = true;
				suspects[newSuspect->IP_address.s_addr]->needs_classification_update = true;
				delete newSuspect;
			}
		}
	}

	in.close();
}

void Nova::RefreshStateFile()
{
	time_t timeStamp;
	uint32_t dataSize;
	vector<in_addr_t> deletedKeys;

	lastLoadTime = time(NULL);
	if (lastLoadTime == ((time_t)-1))
		syslog(SYSL_ERR, "Line: %d Error unable to get timestamp", __LINE__);

	// Open input file
	ifstream in(ceSaveFile.data(), ios::binary | ios::in);
	if(!in.is_open())
	{
		syslog(SYSL_ERR, "Line: %d Attempted load but unable to open CE load file", __LINE__);
		return;
	}

	// Open the tmp file
	string tmpFile = ceSaveFile + ".tmp";
	ofstream out(tmpFile.data(), ios::binary);
	if(!out.is_open())
	{
		syslog(SYSL_ERR, "Line: %d Attempted load but unable to open CE load tmp file", __LINE__);
		in.close();
		return;
	}

	// get length of input for error checking of partially written files
	in.seekg (0, ios::end);
	uint lengthLeft = in.tellg();
	in.seekg (0, ios::beg);

	while (in.is_open() && !in.eof() && lengthLeft)
	{
		// Bytes left, but not enough to make a header (timestamp + size)?
		if (lengthLeft < (sizeof timeStamp + sizeof dataSize))
		{
			syslog(SYSL_ERR, "Line %d Error: Data file should have another header entry, but contains only %d bytes left", __LINE__, lengthLeft);
			break;
		}

		in.read((char*) &timeStamp, sizeof timeStamp);
		lengthLeft -= sizeof timeStamp;

		in.read((char*) &dataSize, sizeof dataSize);
		lengthLeft -= sizeof dataSize;

		if (dataTTL && (timeStamp < lastLoadTime - dataTTL))
		{
			syslog(SYSL_INFO, "Line %d: Throwing out old CE save state with timestamp of %d", __LINE__, (int)timeStamp);
			in.seekg(dataSize, ifstream::cur);
			lengthLeft -= dataSize;
			continue;
		}

		// Not as many bytes left as the size of the entry?
		if (lengthLeft < dataSize)
		{
			syslog(SYSL_ERR, "Line %d Error: Data file should have another entry of size %d, but contains only %d bytes left", __LINE__, dataSize, lengthLeft);
			break;
		}

		u_char tableBuffer[dataSize];
		in.read((char*) tableBuffer, dataSize);
		lengthLeft -= dataSize;

		// Read each suspect
		uint32_t bytesSoFar = 0;
		while (bytesSoFar < dataSize)
		{
			Suspect* newSuspect = new Suspect();
			uint32_t suspectBytes = 0;
			suspectBytes += newSuspect->DeserializeSuspect(tableBuffer + bytesSoFar + suspectBytes);
			suspectBytes += newSuspect->features.DeserializeFeatureData(tableBuffer + bytesSoFar + suspectBytes);

			// Did a suspect get cleared? Not in suspects anymore, but still is suspectsSinceLastSave
			SuspectHashTable::iterator saveIter = suspectsSinceLastSave.find(newSuspect->IP_address.s_addr);
			SuspectHashTable::iterator normalIter = suspects.find(newSuspect->IP_address.s_addr);
			if (normalIter == suspects.end() && saveIter != suspectsSinceLastSave.end())
			{
				cout << "Deleting suspect" << endl;
				in_addr_t key = newSuspect->IP_address.s_addr;
				deletedKeys.push_back(key);
				Suspect *currentSuspect = suspectsSinceLastSave[key ];
				suspectsSinceLastSave.set_deleted_key(5);
				suspectsSinceLastSave.erase(key);
				suspectsSinceLastSave.clear_deleted_key();

				delete currentSuspect;
			}

			vector<in_addr_t>::iterator vIter = find (deletedKeys.begin(), deletedKeys.end(), newSuspect->IP_address.s_addr);
			if (vIter != deletedKeys.end())
			{
				// Shift the rest of the data over on top of our bad suspect
				memmove(tableBuffer+bytesSoFar,
						tableBuffer+bytesSoFar+suspectBytes,
						(dataSize-bytesSoFar-suspectBytes)*sizeof(tableBuffer[0]));
				dataSize -= suspectBytes;
			}
			else
			{
				bytesSoFar += suspectBytes;
			}

			delete newSuspect;
		}

		// If the entry is valid still, write it to the tmp file
		if (dataSize > 0)
		{
			out.write((char*) &timeStamp, sizeof timeStamp);
			out.write((char*) &dataSize, sizeof dataSize);
			out.write((char*) tableBuffer, dataSize);
		}
	}

	out.close();
	in.close();

	string copyCommand = "cp -f " + tmpFile + " " + ceSaveFile;
	if (system(copyCommand.c_str()) == -1)
		syslog(SYSL_ERR, "Line %d: Error: Unable to copy CE state tmp file to CE state file. System call to cp failed.", __LINE__);
}

void Nova::Reload()
{
	// Aquire a lock to stop the other threads from classifying till we're done
	pthread_rwlock_wrlock(&lock);

	// Clear the enabledFeatures count
	enabledFeatures = 0;

	// Clear max and min values
	for (int i = 0; i < DIM; i++)
		maxFeatureValues[i] = 0;

	for (int i = 0; i < DIM; i++)
		minFeatureValues[i] = -1;

	for (int i = 0; i < DIM; i++)
		meanFeatureValues[i] = 0;

	dataPtsWithClass.clear();

	// Reload the configuration file
	string novaConfig = userHomePath + "/Config/NOVAConfig.txt";
	CELoadConfig((char*)novaConfig.c_str());

	// Did our data file move?
	dataFile = userHomePath + "/" +dataFile;
	outFile = dataFile.c_str();

	// Reload the data file
	if (dataPts != NULL)
		annDeallocPts(dataPts);
	if (normalizedDataPts != NULL)
		annDeallocPts(normalizedDataPts);
	LoadDataPointsFromFile(dataFile);

	// Set everyone to be reclassified
	for (SuspectHashTable::iterator it = suspects.begin() ; it != suspects.end(); it++)
		it->second->needs_classification_update = true;

	pthread_rwlock_unlock(&lock);
}

//Infinite loop that recieves messages from the GUI
void *Nova::CE_GUILoop(void *ptr)
{
	struct sockaddr_un GUIAddress;
	int len;

	if((GUISocket = socket(AF_UNIX,SOCK_STREAM,0)) == -1)
	{
		syslog(SYSL_ERR, "Line: %d socket: %s", __LINE__, strerror(errno));
		close(GUISocket);
		exit(EXIT_FAILURE);
	}

	GUIAddress.sun_family = AF_UNIX;

	//Builds the key path
	string key = CE_GUI_FILENAME;
	key = userHomePath + key;

	strcpy(GUIAddress.sun_path, key.c_str());
	unlink(GUIAddress.sun_path);
	len = strlen(GUIAddress.sun_path) + sizeof(GUIAddress.sun_family);

	if(bind(GUISocket,(struct sockaddr *)&GUIAddress,len) == -1)
	{
		syslog(SYSL_ERR, "Line: %d bind: %s", __LINE__, strerror(errno));
		close(GUISocket);
		exit(EXIT_FAILURE);
	}

	if(listen(GUISocket, SOCKET_QUEUE_SIZE) == -1)
	{
		syslog(SYSL_ERR, "Line: %d listen: %s", __LINE__, strerror(errno));
		close(GUISocket);
		exit(EXIT_FAILURE);
	}
	while(true)
	{
		CEReceiveGUICommand();
	}
}

void *Nova::ClassificationLoop(void *ptr)
{
	//Builds the GUI address
	string GUIKey = userHomePath + CE_FILENAME;
	GUISendRemote.sun_family = AF_UNIX;
	strcpy(GUISendRemote.sun_path, GUIKey.c_str());
	GUILen = strlen(GUISendRemote.sun_path) + sizeof(GUISendRemote.sun_family);

	//Builds the Silent Alarm IPC address
	string key = userHomePath + KEY_ALARM_FILENAME;
	alarmRemote.sun_family = AF_UNIX;
	strcpy(alarmRemote.sun_path, key.c_str());
	len = strlen(alarmRemote.sun_path) + sizeof(alarmRemote.sun_family);

	//Builds the Silent Alarm Network address
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(sAlarmPort);

	LoadStateFile();

	//Classification Loop
	do
	{
		sleep(classificationTimeout);
		pthread_rwlock_wrlock(&lock);
		//Calculate the "true" Feature Set for each Suspect
		for (SuspectHashTable::iterator it = suspects.begin() ; it != suspects.end(); it++)
		{
			if(it->second->needs_feature_update)
			{
				it->second->CalculateFeatures(featureMask);
			}
		}
		//Calculate the normalized feature sets, actually used by ANN
		//	Writes into Suspect ANNPoints
		NormalizeDataPoints();


		pthread_rwlock_unlock(&lock);
		pthread_rwlock_rdlock(&lock);

		//Perform classification on each suspect
		for (SuspectHashTable::iterator it = suspects.begin() ; it != suspects.end(); it++)
		{
			if(it->second->needs_classification_update)
			{
				oldClassification = it->second->isHostile;
				Classify(it->second);

				//If suspect is hostile and this Nova instance has unique information
				// 			(not just from silent alarms)
				if(it->second->isHostile)
				{
					SilentAlarm(it->second);
				}
				SendToUI(it->second);
			}
		}
		pthread_rwlock_unlock(&lock);

		if (saveFrequency > 0)
			if ((time(NULL) - lastSaveTime) > saveFrequency)
			{
				pthread_rwlock_wrlock(&lock);
				AppendToStateFile();
				pthread_rwlock_unlock(&lock);
			}

		if (dataTTL > 0)
			if ((time(NULL) - lastLoadTime) > dataTTL)
			{
				pthread_rwlock_wrlock(&lock);
				AppendToStateFile();
				RefreshStateFile();

				for (SuspectHashTable::iterator it = suspects.begin(); it != suspects.end(); it++)
					delete it->second;
				suspects.clear();

				for (SuspectHashTable::iterator it = suspectsSinceLastSave.begin(); it != suspectsSinceLastSave.end(); it++)
					delete it->second;
				suspectsSinceLastSave.clear();

				LoadStateFile();
				pthread_rwlock_unlock(&lock);
			}

	} while(classificationTimeout);

	//Shouldn't get here!!
	if(classificationTimeout)
		syslog(SYSL_ERR, "Line: %d Main thread ended. Shouldn't get here!!!", __LINE__);
	return NULL;
}


void *Nova::TrainingLoop(void *ptr)
{
	//Builds the GUI address
	string GUIKey = userHomePath + CE_FILENAME;
	GUISendRemote.sun_family = AF_UNIX;
	strcpy(GUISendRemote.sun_path, GUIKey.c_str());
	GUILen = strlen(GUISendRemote.sun_path) + sizeof(GUISendRemote.sun_family);


	//Training Loop
	do
	{
		sleep(classificationTimeout);
		ofstream myfile (trainingCapFile.data(), ios::app);

		if (myfile.is_open())
		{
			pthread_rwlock_wrlock(&lock);
			//Calculate the "true" Feature Set for each Suspect
			for (SuspectHashTable::iterator it = suspects.begin() ; it != suspects.end(); it++)
			{

				if(it->second->needs_feature_update)
				{
					it->second->CalculateFeatures(featureMask);
					if(it->second->annPoint == NULL)
						it->second->annPoint = annAllocPt(DIM);


					for(int j=0; j < DIM; j++)
						it->second->annPoint[j] = it->second->features.features[j];

						myfile << string(inet_ntoa(it->second->IP_address)) << " ";

						for(int j=0; j < DIM; j++)
							myfile << it->second->annPoint[j] << " ";

						myfile << "\n";

					it->second->needs_feature_update = false;
					//cout << it->second->ToString(featureEnabled);
					SendToUI(it->second);
				}
			}
			pthread_rwlock_unlock(&lock);
		}
		else
		{
			syslog(SYSL_ERR, "Line: %d Unable to open file %s", __LINE__, trainingCapFile.data());
		}
		myfile.close();
	} while(classificationTimeout);

	//Shouldn't get here!
	if (classificationTimeout)
		syslog(SYSL_ERR, "Line: %d Training thread ended. Shouldn't get here!!!", __LINE__);

	return NULL;
}

void *Nova::SilentAlarmLoop(void *ptr)
{
	int sockfd;
	u_char buf[MAX_MSG_SIZE];
	struct sockaddr_in sendaddr;

	if((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1)
	{
		syslog(SYSL_ERR, "Line: %d socket: %s", __LINE__, strerror(errno));
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	sendaddr.sin_family = AF_INET;
	sendaddr.sin_port = htons(sAlarmPort);
	sendaddr.sin_addr.s_addr = INADDR_ANY;

	memset(sendaddr.sin_zero,'\0', sizeof sendaddr.sin_zero);
	struct sockaddr* sockaddrPtr = (struct sockaddr*) &sendaddr;
	socklen_t sendaddrSize = sizeof sendaddr;

	if(bind(sockfd,sockaddrPtr,sendaddrSize) == -1)
	{
		syslog(SYSL_ERR, "Line: %d bind: %s", __LINE__, strerror(errno));
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	stringstream ss;
	ss << "sudo iptables -A INPUT -p udp --dport " << sAlarmPort << " -j REJECT --reject-with icmp-port-unreachable";
	if(system(ss.str().c_str()) == -1)
	{
	    loggerConf->Logging(INFO, "Failed to update iptables.");
	}
	ss.str("");
	ss << "sudo iptables -A INPUT -p tcp --dport " << sAlarmPort << " -j REJECT --reject-with tcp-reset";
	if(system(ss.str().c_str()) == -1)
	{
	    loggerConf->Logging(INFO, "Failed to update iptables.");
	}

    if(listen(sockfd, SOCKET_QUEUE_SIZE) == -1)
    {
		syslog(SYSL_ERR, "Line: %d listen: %s", __LINE__, strerror(errno));
		close(sockfd);
        exit(EXIT_FAILURE);
    }

	int connectionSocket, bytesRead;

	//Accept incoming Silent Alarm TCP Connections
	while(1)
	{

		bzero(buf, MAX_MSG_SIZE);

		//Blocking call
		if((connectionSocket = accept(sockfd, sockaddrPtr, &sendaddrSize)) == -1)
		{
			syslog(SYSL_ERR, "Line: %d accept: %s", __LINE__, strerror(errno));
			close(connectionSocket);
			continue;
		}

		if((bytesRead = recv(connectionSocket, buf, MAX_MSG_SIZE, MSG_WAITALL)) == -1)
		{
			syslog(SYSL_ERR, "Line: %d recv: %s", __LINE__, strerror(errno));
			close(connectionSocket);
			continue;
		}

		//If this is from ourselves, then drop it.
		if(hostAddr.sin_addr.s_addr == sendaddr.sin_addr.s_addr)
		{
			close(connectionSocket);
			continue;
		}
		CryptBuffer(buf, bytesRead, DECRYPT);

		pthread_rwlock_wrlock(&lock);
		try
		{
			uint addr = GetSerializedAddr(buf);
			SuspectHashTable::iterator it = suspects.find(addr);

			//If this is a new suspect put it in the table
			if(it == suspects.end())
			{
				suspects[addr] = new Suspect();
				suspects[addr]->DeserializeSuspectWithData(buf, BROADCAST_DATA);
				//We set isHostile to false so that when we classify the first time
				// the suspect will go from benign to hostile and be sent to the doppelganger module
				suspects[addr]->isHostile = false;
			}
			//If this suspect exists, update the information
			else
			{
				//This function will overwrite everything except the information used to calculate the classification
				// a combined classification will be given next classification loop
				suspects[addr]->DeserializeSuspectWithData(buf, BROADCAST_DATA);
			}
			suspects[addr]->flaggedByAlarm = true;
			//We need to move host traffic data from broadcast into the bin for this host, and remove the old bin
			syslog(SYSL_INFO, "Line: %d Received Silent Alarm!\n %s", __LINE__, (suspects[addr]->ToString(featureEnabled)).c_str());
			if(!classificationTimeout)
				ClassificationLoop(NULL);
		}
		catch(std::exception e)
		{
			syslog(SYSL_ERR, "Line: %d Error interpreting received Silent Alarm: %s", __LINE__, string(e.what()).c_str());
		}
		close(connectionSocket);
		pthread_rwlock_unlock(&lock);
	}
	close(sockfd);
	syslog(SYSL_INFO, "Line: %d Silent Alarm thread ended. Shouldn't get here!!!", __LINE__);
	return NULL;
}


void Nova::FormKdTree()
{
	delete kdTree;
	//Normalize the data points
	//Foreach data point
	for(int j = 0;j < enabledFeatures;j++)
	{
		//Foreach feature within the data point
		for(int i=0;i < nPts;i++)
		{
			if(maxFeatureValues[j] != 0)
			{
				normalizedDataPts[i][j] = Normalize(normalization[j], dataPts[i][j], minFeatureValues[j], maxFeatureValues[j]);
			}
			else
			{
				syslog(SYSL_INFO, "Line: %d Max Feature Value for feature %d is 0!", __LINE__, (j + 1));
				break;
			}
		}
	}
	kdTree = new ANNkd_tree(					// build search structure
			normalizedDataPts,					// the data points
					nPts,						// number of points
					enabledFeatures);						// dimension of space
	updateKDTree = false;
}


void Nova::Classify(Suspect *suspect)
{
	ANNidxArray nnIdx = new ANNidx[k];			// allocate near neigh indices
	ANNdistArray dists = new ANNdist[k];		// allocate near neighbor dists

	kdTree->annkSearch(							// search
			suspect->annPoint,					// query point
			k,									// number of near neighbors
			nnIdx,								// nearest neighbors (returned)
			dists,								// distance (returned)
			eps);								// error bound

	for (int i = 0; i < DIM; i++)
		suspect->featureAccuracy[i] = 0;

	suspect->hostileNeighbors = 0;

	//Determine classification according to weight by distance
	//	.5 + E[(1-Dist) * Class] / 2k (Where Class is -1 or 1)
	//	This will make the classification range from 0 to 1
	double classifyCount = 0;

	for (int i = 0; i < k; i++)
	{
		dists[i] = sqrt(dists[i]);				// unsquare distance

		for (int j = 0; j < DIM; j++)
		{
			if (featureEnabled[j])
			{
				double distance = suspect->annPoint[j] - kdTree->thePoints()[nnIdx[i]][j];
				if (distance < 0)
					distance *= -1;

				suspect->featureAccuracy[j] += distance;
			}
		}

		if(nnIdx[i] == -1)
		{
			syslog(SYSL_ERR, "Line: %d Unable to find a nearest neighbor for Data point: %d\n Try decreasing the Error bound", __LINE__, i);
		}
		else
		{
			//If Hostile
			if(dataPtsWithClass[nnIdx[i]]->classification == 1)
			{
				classifyCount += (sqrtDIM - dists[i]);
				suspect->hostileNeighbors++;
			}
			//If benign
			else if(dataPtsWithClass[nnIdx[i]]->classification == 0)
			{
				classifyCount -= (sqrtDIM - dists[i]);
			}
			else
			{
				//error case; Data points must be 0 or 1
				syslog(SYSL_ERR, "Line: %d Data point: %d has an invalid classification of: %d. This must be either 0 (benign) or 1 (hostile)",
					   __LINE__, i, dataPtsWithClass[nnIdx[i]]->classification);
				suspect->classification = -1;
				delete [] nnIdx;							// clean things up
				delete [] dists;
				annClose();
				return;
			}
		}
	}

	for (int j = 0; j < DIM; j++)
				suspect->featureAccuracy[j] /= k;

	pthread_rwlock_unlock(&lock);
	pthread_rwlock_wrlock(&lock);

	suspect->classification = .5 + (classifyCount / ((2.0 * (double)k) * sqrtDIM ));

	// Fix for rounding errors caused by double's not being precise enough if DIM is something like 2
	if (suspect->classification < 0)
		suspect->classification = 0;
	else if (suspect->classification > 1)
		suspect->classification = 1;

	if( suspect->classification > classificationThreshold)
	{
		suspect->isHostile = true;
	}
	else
	{
		suspect->isHostile = false;
	}
	delete [] nnIdx;							// clean things up
    delete [] dists;

    annClose();
	suspect->needs_classification_update = false;
	pthread_rwlock_unlock(&lock);
	pthread_rwlock_rdlock(&lock);
}


void Nova::NormalizeDataPoints()
{
	for (SuspectHashTable::iterator it = suspects.begin();it != suspects.end();it++)
	{
		// Used for matching the 0->DIM index with the 0->enabledFeatures index
		int ai = 0;
		for(int i = 0;i < DIM;i++)
		{
			if (featureEnabled[i])
			{
				if(it->second->features.features[i] > maxFeatureValues[ai])
				{
					//For proper normalization the upper bound for a feature is the max value of the data.
					it->second->features.features[i] = maxFeatureValues[ai];
				}
				else if (it->second->features.features[i] > maxFeatureValues[ai])
				{
					it->second->features.features[i] = minFeatureValues[ai];
				}
				ai++;
			}

		}
	}

	//if(updateKDTree) FormKdTree();

	//Normalize the suspect points
	for (SuspectHashTable::iterator it = suspects.begin();it != suspects.end();it++)
	{
		if(it->second->needs_feature_update)
		{
			if(it->second->annPoint == NULL)
				it->second->annPoint = annAllocPt(enabledFeatures);

			int ai = 0;
			for(int i = 0;i < DIM;i++)
			{
				if (featureEnabled[i])
				{
					if(maxFeatureValues[ai] != 0)
						it->second->annPoint[ai] = Normalize(normalization[i], it->second->features.features[i], minFeatureValues[ai], maxFeatureValues[ai]);
					else
						syslog(SYSL_INFO, "Line: %d max Feature Value for feature %d is 0!", __LINE__, (ai + 1));
					ai++;
				}

			}
			it->second->needs_feature_update = false;
		}
	}
}


void Nova::PrintPt(ostream &out, ANNpoint p)
{
	out << "(" << p[0];
	for (int i = 1;i < enabledFeatures;i++)
	{
		out << ", " << p[i];
	}
	out << ")\n";
}


void Nova::LoadDataPointsFromFile(string inFilePath)
{
	ifstream myfile (inFilePath.data());
	string line;

	//string array to check whether a line in data.txt file has the right number of fields
	string fieldsCheck[DIM];
	bool valid = true;

	int i = 0;
	int k = 0;
	int badLines = 0;

	//Count the number of data points for allocation
	if (myfile.is_open())
	{
		while (!myfile.eof())
		{
			if(myfile.peek() == EOF)
			{
				break;
			}
			getline(myfile,line);
			i++;
		}
	}

	else
	{
		syslog(SYSL_ERR, "Line: %d Unable to open file.", __LINE__);
	}

	myfile.close();
	int maxPts = i;

	//Open the file again, allocate the number of points and assign
	myfile.open(inFilePath.data(), ifstream::in);
	dataPts = annAllocPts(maxPts, enabledFeatures); // allocate data points
	normalizedDataPts = annAllocPts(maxPts, enabledFeatures);


	if (myfile.is_open())
	{
		i = 0;

		while (!myfile.eof() && (i < maxPts))
		{
			k = 0;

			if(myfile.peek() == EOF)
			{
				break;
			}

			//initializes fieldsCheck to have all array indices contain the string "NotPresent"
			for(int j = 0; j < DIM; j++)
			{
				fieldsCheck[j] = "NotPresent";
			}

			//this will grab a line of values up to a newline or until DIM values have been taken in.
			while(myfile.peek() != '\n' && k < DIM)
			{
				getline(myfile, fieldsCheck[k], ' ');
				k++;
			}

			//starting from the end of fieldsCheck, if NotPresent is still inside the array, then
			//the line of the data.txt file is incorrect, set valid to false. Note that this
			//only works in regards to the 9 data points preceding the classification,
			//not the classification itself.
			for(int m = DIM - 1; m >= 0 && valid; m--)
			{
				if(!fieldsCheck[m].compare("NotPresent"))
				{
					valid = false;
				}
			}

			//if the next character is a newline after extracting as many data points as possible,
			//then the classification is not present. For now, I will merely discard the line;
			//there may be a more elegant way to do it. (i.e. pass the data to Classify or something)
			if(myfile.peek() == '\n' || myfile.peek() == ' ')
			{
				valid = false;
			}

			//if the line is valid, continue as normal
			if(valid)
			{
				dataPtsWithClass.push_back(new Point(enabledFeatures));

				// Used for matching the 0->DIM index with the 0->enabledFeatures index
				int actualDimension = 0;
				for(int defaultDimension = 0;defaultDimension < DIM;defaultDimension++)
				{
					double temp = strtod(fieldsCheck[defaultDimension].data(), NULL);

					if (featureEnabled[defaultDimension])
					{
						dataPtsWithClass[i]->annPoint[actualDimension] = temp;
						dataPts[i][actualDimension] = temp;

						//Set the max values of each feature. (Used later in normalization)
						if(temp > maxFeatureValues[actualDimension])
							maxFeatureValues[actualDimension] = temp;

						if(minFeatureValues[actualDimension] == -1 || temp < minFeatureValues[actualDimension])
							minFeatureValues[actualDimension] = temp;

						meanFeatureValues[actualDimension] += temp;

						actualDimension++;
					}
				}
				getline(myfile,line);
				dataPtsWithClass[i]->classification = atoi(line.data());
				i++;
			}
			//but if it isn't, just get to the next line without incrementing i.
			//this way every correct line will be inserted in sequence
			//without any gaps due to perhaps multiple line failures, etc.
			else
			{
				getline(myfile,line);
				badLines++;
			}
		}
		nPts = i;

		for (int j = 0; j < DIM; j++)
			meanFeatureValues[j] /= nPts;
	}
	else
	{
		syslog(SYSL_ERR, "Line: %d Unable to open file.", __LINE__);
	}
	myfile.close();

	syslog(SYSL_INFO, "Line: %d There were %d incomplete lines in the data file.", __LINE__, badLines);
	//Normalize the data points

	//Foreach feature within the data point
	for(int j = 0;j < enabledFeatures;j++)
	{
		//Foreach data point
		for(int i=0;i < nPts;i++)
		{
			normalizedDataPts[i][j] = Normalize(normalization[j], dataPts[i][j], minFeatureValues[j], maxFeatureValues[j]);
		}
	}

	kdTree = new ANNkd_tree(					// build search structure
			normalizedDataPts,					// the data points
					nPts,						// number of points
					enabledFeatures);						// dimension of space

	WriteDataPointsToFile("Data/normalizedPoints", kdTree);
}

double Nova::Normalize(normalizationType type, double value, double min, double max)
{
	switch (type)
	{
		case LINEAR:
		{
			return value / max;
		}
		case LINEAR_SHIFT:
		{
			return (value -min) / (max - min);
		}
		case LOGARITHMIC:
		{
			return (log(value - min + 1)) / (log(max - min + 1));
		}
		case NONORM:
		{
			return value;
		}
		default:
		{
			//loggerConf->Logging(ERROR, "Normalization failed: Normalization type unkown");
			return 0;
		}

		// TODO: A sigmoid normalization function could be very useful,
		// especially if we could somehow use it interactively to set the center and smoothing
		// while looking at the data visualizations to see what works best for a feature
	}
}


void Nova::WriteDataPointsToFile(string outFilePath, ANNkd_tree* tree)
{
	ofstream myfile (outFilePath.data());

	if (myfile.is_open())
	{
		for (int i = 0; i < tree->nPoints(); i++ )
		{
			for(int j=0; j < tree->theDim(); j++)
			{
				myfile << tree->thePoints()[i][j] << " ";
			}
			myfile << dataPtsWithClass[i]->classification;
			myfile << "\n";
		}
	}
	else
	{
		syslog(SYSL_ERR, "Line: %d Unable to open file.", __LINE__);
	}
	myfile.close();

}


void Nova::SilentAlarm(Suspect *suspect)
{
	int socketFD;

	uint dataLen = suspect->SerializeSuspect(data);

	//If the hostility hasn't changed don't bother the DM
	if(oldClassification != suspect->isHostile && suspect->isLive)
	{
		if ((socketFD = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		{
			syslog(SYSL_ERR, "Line: %d Unable to create socket to neighbor: %s", __LINE__, strerror(errno));
			close(socketFD);
			return;
		}

		if (connect(socketFD, alarmRemotePtr, len) == -1)
		{
			syslog(SYSL_ERR, "Line: %d Unable to connect to neigbor: %s", __LINE__, strerror(errno));
			close(socketFD);
			return;
		}

		if (send(socketFD, data, dataLen, 0) == -1)
		{
			syslog(SYSL_ERR, "Line: %d Unable to send to neigbor: %s", __LINE__, strerror(errno));
			close(socketFD);
			return;
		}
		close(socketFD);
	}
	if(suspect->features.unsentData->packetCount && suspect->isLive)
	{
		do
		{
			bzero(data, MAX_MSG_SIZE);
			dataLen = suspect->SerializeSuspect(data);

			// Serialize the unsent data
			dataLen += suspect->features.unsentData->SerializeFeatureData(data+dataLen);
			// Move the unsent data to the sent side
			suspect->features.UpdateFeatureData(true);
			// Clear the unsent data
			suspect->features.unsentData->clearFeatureData();

			//Update other Nova Instances with latest suspect Data
			for(uint i = 0; i < neighbors.size(); i++)
			{
				serv_addr.sin_addr.s_addr = neighbors[i];

				stringstream ss;
				string commandLine;

				ss << "sudo iptables -I INPUT -s " << string(inet_ntoa(serv_addr.sin_addr)) << " -p tcp -j ACCEPT";
				commandLine = ss.str();

				if(system(commandLine.c_str()) == -1)
				{
					loggerConf->Logging(INFO, "Failed to update iptables.");
				}


				uint i;
				for(i = 0; i < SA_Max_Attempts; i++)
				{
					if(CEKnockPort(OPEN))
					{
						//Send Silent Alarm to other Nova Instances with feature Data
						if ((sockfd = socket(AF_INET,SOCK_STREAM,6)) == -1)
						{
							syslog(SYSL_ERR, "Line: %d socket: %s", __LINE__, strerror(errno));
							close(sockfd);
							continue;
						}
						if (connect(sockfd, serv_addrPtr, inSocketSize) == -1)
						{
							//If the host isn't up we stop trying
							if(errno == EHOSTUNREACH)
							{
								//set to max attempts to hit the failed connect condition
								i = SA_Max_Attempts;
								syslog(SYSL_ERR, "Line: %d connect: %s", __LINE__, strerror(errno));
								break;
							}
							syslog(SYSL_ERR, "Line: %d connect: %s", __LINE__, strerror(errno));
							close(sockfd);
							continue;
						}
						break;
					}
				}
				//If connecting failed
				if(i == SA_Max_Attempts )
				{
					close(sockfd);
					ss.str("");
					ss << "sudo iptables -D INPUT -s " << string(inet_ntoa(serv_addr.sin_addr)) << " -p tcp -j ACCEPT";
					commandLine = ss.str();
					if(system(commandLine.c_str()) == -1)
					{
						loggerConf->Logging(INFO, "Failed to update iptables.");
					}
					continue;
				}

				if( send(sockfd,data,dataLen,0) == -1)
				{
					syslog(SYSL_ERR, "Line: %d Error in TCP Send: %s", __LINE__, strerror(errno));
					close(sockfd);
					ss.str("");
					ss << "sudo iptables -D INPUT -s " << string(inet_ntoa(serv_addr.sin_addr)) << " -p tcp -j ACCEPT";
					commandLine = ss.str();
					if(system(commandLine.c_str()) == -1)
					{
						loggerConf->Logging(INFO, "Failed to update iptables.");
					}
					continue;
				}
				close(sockfd);
				CEKnockPort(CLOSE);
				ss.str("");
				ss << "sudo iptables -D INPUT -s " << string(inet_ntoa(serv_addr.sin_addr)) << " -p tcp -j ACCEPT";
				commandLine = ss.str();
				if(system(commandLine.c_str()) == -1)
				{
					loggerConf->Logging(INFO, "Failed to update iptables.");
				}
			}
		}while(dataLen == MORE_DATA);
	}
}


bool Nova::CEKnockPort(bool mode)
{
	stringstream ss;
	ss << key;

	//mode == OPEN (true)
	if(mode)
		ss << "OPEN";

	//mode == CLOSE (false)
	else
		ss << "SHUT";

	uint keyDataLen = key.size() + 4;
	u_char keyBuf[1024];
	bzero(keyBuf, 1024);
	memcpy(keyBuf, ss.str().c_str(), ss.str().size());

	CryptBuffer(keyBuf, keyDataLen, ENCRYPT);

	//Send Port knock to other Nova Instances
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 17)) == -1)
	{
		syslog(SYSL_ERR, "Line: %d socket: %s", __LINE__, strerror(errno));
		close(sockfd);
		return false;
	}

	if( sendto(sockfd,keyBuf,keyDataLen, 0,serv_addrPtr, inSocketSize) == -1)
	{
		syslog(SYSL_ERR, "Line: %d Error in UDP Send: %s", __LINE__, strerror(errno));
		close(sockfd);
		return false;
	}

	close(sockfd);
	sleep(SA_Sleep_Duration);
	return true;
}


bool Nova::CEReceiveSuspectData()
{
	int bytesRead, connectionSocket;

    //Blocking call
    if ((connectionSocket = accept(CE_IPCsock, remoteSockAddr, sockSizePtr)) == -1)
    {
		syslog(SYSL_ERR, "Line: %d accept: %s", __LINE__, strerror(errno));
		close(connectionSocket);
        return false;
    }
    if((bytesRead = recv(connectionSocket, buffer, MAX_MSG_SIZE, MSG_WAITALL)) == -1)
    {
		syslog(SYSL_ERR, "Line: %d recv: %s", __LINE__, strerror(errno));
		close(connectionSocket);
        return false;
    }
	try
	{
		pthread_rwlock_wrlock(&lock);
		uint addr = GetSerializedAddr(buffer);

		SuspectHashTable::iterator it = suspects.find(addr);
		//If this is a new suspect make an entry in the table
		if(it == suspects.end())
			suspects[addr] = new Suspect();
		//Deserialize the data
		suspects[addr]->DeserializeSuspectWithData(buffer, LOCAL_DATA);


		it = suspectsSinceLastSave.find(addr);
		//If this is a new suspect make an entry in the table
		if(it == suspectsSinceLastSave.end())
			suspectsSinceLastSave[addr] = new Suspect();
		//Deserialize the data
		suspectsSinceLastSave[addr]->DeserializeSuspectWithData(buffer, BROADCAST_DATA);


		pthread_rwlock_unlock(&lock);
	}
	catch(std::exception e)
	{
		syslog(SYSL_ERR, "Line: %d Error in parsing received Suspect: %s", __LINE__, string(e.what()).c_str());
		close(connectionSocket);
		bzero(buffer, MAX_MSG_SIZE);
		pthread_rwlock_unlock(&lock);
		return false;
	}
	close(connectionSocket);
	return true;
}


void Nova::CEReceiveGUICommand()
{
    struct sockaddr_un msgRemote;
	int socketSize, msgSocket;
	int bytesRead;
	GUIMsg msg = GUIMsg();
	in_addr_t suspectKey = 0;
	u_char msgBuffer[MAX_GUIMSG_SIZE];

	bzero(msgBuffer, MAX_GUIMSG_SIZE);

	socketSize = sizeof(msgRemote);

	//Blocking call
	if ((msgSocket = accept(GUISocket, (struct sockaddr *)&msgRemote, (socklen_t*)&socketSize)) == -1)
	{
		syslog(SYSL_ERR, "Line: %d accept: %s", __LINE__, strerror(errno));
		close(msgSocket);
	}
	if((bytesRead = recv(msgSocket, msgBuffer, MAX_GUIMSG_SIZE, MSG_WAITALL )) == -1)
	{
		syslog(SYSL_ERR, "Line: %d recv: %s", __LINE__, strerror(errno));
		close(msgSocket);
	}

	msg.DeserializeMessage(msgBuffer);
	switch(msg.GetType())
	{
		case EXIT:
		{
			saveAndExit(0);
		}
		case CLEAR_ALL:
		{
			pthread_rwlock_wrlock(&lock);
			for (SuspectHashTable::iterator it = suspects.begin(); it != suspects.end(); it++)
				delete it->second;
			suspects.clear();

			for (SuspectHashTable::iterator it = suspectsSinceLastSave.begin(); it != suspectsSinceLastSave.end(); it++)
				delete it->second;
			suspectsSinceLastSave.clear();

			string delString = "rm -f " + ceSaveFile;
			if(system(delString.c_str()) == -1)
				syslog(SYSL_ERR, "Line: %d Unable to delete CE state file. System call to rm failed.", __LINE__);

			pthread_rwlock_unlock(&lock);
			break;
		}
		case CLEAR_SUSPECT:
		{
			suspectKey = inet_addr(msg.GetValue().c_str());
			pthread_rwlock_wrlock(&lock);
			suspectsSinceLastSave[suspectKey] = suspects[suspectKey];
			suspects.set_deleted_key(5);
			suspects.erase(suspectKey);
			suspects.clear_deleted_key();
			RefreshStateFile();
			pthread_rwlock_unlock(&lock);
			break;
		}
		case WRITE_SUSPECTS:
		{
			SaveSuspectsToFile(msg.GetValue());
			break;
		}
		case RELOAD:
		{
			Reload();
			break;
		}
		default:
		{
			break;
		}
	}
	close(msgSocket);
}


void Nova::SaveSuspectsToFile(string filename)
{
	syslog(SYSL_ERR, "Line: %d Got request to save file to %s", __LINE__, filename.c_str());
	dataPtsWithClass.push_back(new Point(enabledFeatures));

	ofstream out(filename.c_str());

	if(!out.is_open())
	{
		syslog(SYSL_ERR, "Line: %d Error: Unable to open file %s to save suspect data.", __LINE__, filename.c_str());
		return;
	}

	pthread_rwlock_rdlock(&lock);
	for (SuspectHashTable::iterator it = suspects.begin() ; it != suspects.end(); it++)
	{
		out << it->second->ToString(featureEnabled) << endl;
	}
	pthread_rwlock_unlock(&lock);

	out.close();
}


void Nova::SendToUI(Suspect *suspect)
{
	if (!enableGUI)
		return;

	uint GUIDataLen = suspect->SerializeSuspect(GUIData);

	if ((GUISendSocket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		syslog(SYSL_ERR, "Line: %d Unable to create GUI socket: %s", __LINE__, strerror(errno));
		close(GUISendSocket);
		return;
	}

	if (connect(GUISendSocket, GUISendPtr, GUILen) == -1)
	{
		syslog(SYSL_ERR, "Line: %d Unable to connect to GUI: %s", __LINE__, strerror(errno));
		close(GUISendSocket);
		return;
	}

	if (send(GUISendSocket, GUIData, GUIDataLen, 0) == -1)
	{
		syslog(SYSL_ERR, "Line: %d Unable to send to GUI: %s", __LINE__, strerror(errno));
		close(GUISendSocket);
		return;
	}
	close(GUISendSocket);
}


void Nova::CELoadConfig(char * configFilePath)
{
	string prefix, line;
	uint i = 0;
	int confCheck = 0;

	string settingsPath = userHomePath +"/settings";
	ifstream settings(settingsPath.c_str());
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
					}
					else if(nbr)
					{
						neighbors.push_back(nbr);
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
					key = line;
				else
					syslog(SYSL_ERR, "Line: %d Invalid Key parsed on line %d of the settings file.", __LINE__, i);
			}
		}
	}
	settings.close();

	NOVAConfiguration * NovaConfig = new NOVAConfiguration();
	NovaConfig->LoadConfig(configFilePath, userHomePath, __FILE__);

	confCheck = NovaConfig->SetDefaults();

	openlog("ClassificationEngine", OPEN_SYSL, LOG_AUTHPRIV);

	//Checks to make sure all values have been set.
	if(confCheck == 2)
	{
		syslog(SYSL_ERR, "Line: %d One or more values have not been set, and have no default.", __LINE__);

		exit(EXIT_FAILURE);
	}
	else if(confCheck == 1)
	{
		syslog(SYSL_INFO, "Line: %d INFO Config loaded successfully with defaults; some variables in NOVAConfig.txt were incorrectly set, not present, or not valid!", __LINE__);
	}
	else if (confCheck == 0)
	{
		syslog(SYSL_INFO, "Line: %d INFO Config loaded successfully.", __LINE__);
	}

	string hostAddrString = GetLocalIP(NovaConfig->options["INTERFACE"].data.c_str());

	if(hostAddrString.size() == 0)
	{
		syslog(SYSL_ERR, "Line: %d Bad interface, no IP's associated!", __LINE__);
		exit(EXIT_FAILURE);
	}

	closelog();

	inet_pton(AF_INET,hostAddrString.c_str(),&(hostAddr.sin_addr));


	useTerminals = atoi(NovaConfig->options["USE_TERMINALS"].data.c_str());
	sAlarmPort = atoi(NovaConfig->options["SILENT_ALARM_PORT"].data.c_str());
	k = atoi(NovaConfig->options["K"].data.c_str());
	eps =  atof(NovaConfig->options["EPS"].data.c_str());
	classificationTimeout = atoi(NovaConfig->options["CLASSIFICATION_TIMEOUT"].data.c_str());
	isTraining = atoi(NovaConfig->options["IS_TRAINING"].data.c_str());
	trainingFolder = NovaConfig->options["TRAINING_CAP_FOLDER"].data;
	classificationThreshold = atof(NovaConfig->options["CLASSIFICATION_THRESHOLD"].data.c_str());
	dataFile = NovaConfig->options["DATAFILE"].data;
	SA_Max_Attempts = atoi(NovaConfig->options["SA_MAX_ATTEMPTS"].data.c_str());
	SA_Sleep_Duration = atof(NovaConfig->options["SA_SLEEP_DURATION"].data.c_str());

	saveFrequency = atoi(NovaConfig->options["SAVE_FREQUENCY"].data.c_str());
	dataTTL = atoi(NovaConfig->options["DATA_TTL"].data.c_str());
	ceSaveFile = NovaConfig->options["CE_SAVE_FILE"].data;

	string enabledFeatureMask = NovaConfig->options["ENABLED_FEATURES"].data;

	featureMask = 0;
	for (uint i = 0; i < DIM; i++)
	{
		if ('1' == enabledFeatureMask.at(i))
		{
			featureEnabled[i] = true;
			featureMask += pow(2, i);
			enabledFeatures++;
		}
		else
		{
			featureEnabled[i] = false;
		}
	}

	sqrtDIM = sqrt(enabledFeatures);
}
