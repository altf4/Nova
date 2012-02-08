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
#include "NovaMessageClient.h"
#include <iostream>
#include <time.h>

using namespace std;
using namespace Nova;
using namespace ClassificationEngine;

// Maintains a list of suspects and information on network activity
static SuspectHashTable suspects;

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
static string hostAddrString;
static struct sockaddr_in hostAddr;

//** Main (ReceiveSuspectData) **
struct sockaddr_un remote;
struct sockaddr* remoteSockAddr = (struct sockaddr *)&remote;

Suspect * suspect;
int IPCsock;

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
int enabledFeatures = 0;

// Misc
int len;
const char *outFile;				//output for data points during training
string homePath;
double maxFeatureValues[DIM];

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
// End configured variables

lastPointHash lastPoints;

int main(int argc,char *argv[])
{
	bzero(GUIData,MAX_MSG_SIZE);
	bzero(data,MAX_MSG_SIZE);
	bzero(buffer, MAX_MSG_SIZE);

	pthread_rwlock_init(&lock, NULL);

	suspects.set_empty_key(0);
	suspects.resize(INIT_SIZE_SMALL);

	lastPoints.set_empty_key(0);

	struct sockaddr_un localIPCAddress;

	pthread_t classificationLoopThread;
	pthread_t trainingLoopThread;
	pthread_t silentAlarmListenThread;
	pthread_t GUIListenThread;

	string novaConfig;
	string line, prefix; //used for input checking

	//Get locations of nova files
	homePath = GetHomePath();
	novaConfig = homePath + "/Config/NOVAConfig.txt";
	chdir(homePath.c_str());

	//Runs the configuration loader
	LoadConfig((char*)novaConfig.c_str());

	if(!useTerminals)
	{
		openlog("ClassificationEngine", NO_TERM_SYSL, LOG_AUTHPRIV);
	}

	else
	{
		openlog("ClassificationEngine", OPEN_SYSL, LOG_AUTHPRIV);
	}

	dataFile = homePath + "/" +dataFile;
	outFile = dataFile.c_str();

	pthread_create(&GUIListenThread, NULL, GUILoop, NULL);
	//Are we Training or Classifying?
	if(isTraining)
	{
		// We suffix the training capture files with the date/time
		time_t rawtime;
		time ( &rawtime );
		struct tm * timeinfo = localtime(&rawtime);

		char buffer [40];
		strftime (buffer,40,"%m-%d-%y_%H-%M-%S",timeinfo);
		trainingCapFile = homePath + "/" + trainingFolder + "/training" + buffer + ".dump";

		enabledFeatures = DIM;
		pthread_create(&trainingLoopThread,NULL,TrainingLoop,(void *)outFile);
	}
	else
	{
		LoadDataPointsFromFile(dataFile);

		pthread_create(&classificationLoopThread,NULL,ClassificationLoop, NULL);
		pthread_create(&silentAlarmListenThread,NULL,SilentAlarmLoop, NULL);
	}

	if((IPCsock = socket(AF_UNIX,SOCK_STREAM,0)) == -1)
	{
		syslog(SYSL_ERR, "Line: %d socket: %s", __LINE__, strerror(errno));
		close(IPCsock);
		exit(1);
	}

	localIPCAddress.sun_family = AF_UNIX;

	//Builds the key path
	string key = KEY_FILENAME;
	key = homePath + key;

	strcpy(localIPCAddress.sun_path, key.c_str());
	unlink(localIPCAddress.sun_path);
	len = strlen(localIPCAddress.sun_path) + sizeof(localIPCAddress.sun_family);

    if(bind(IPCsock,(struct sockaddr *)&localIPCAddress,len) == -1)
    {
    	syslog(SYSL_ERR, "Line: %d bind: %s", __LINE__, strerror(errno));
    	close(IPCsock);
        exit(1);
    }

    if(listen(IPCsock, SOCKET_QUEUE_SIZE) == -1)
    {
    	syslog(SYSL_ERR, "Line: %d listen: %s", __LINE__, strerror(errno));
		close(IPCsock);
        exit(1);
    }

	//"Main Loop"
	while(true)
	{
		if (ReceiveSuspectData())
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
	close(IPCsock);
	return 1;
}

//Infinite loop that recieves messages from the GUI
void *Nova::ClassificationEngine::GUILoop(void *ptr)
{
	struct sockaddr_un GUIAddress;
	int len;

	if((GUISocket = socket(AF_UNIX,SOCK_STREAM,0)) == -1)
	{
		syslog(SYSL_ERR, "Line: %d socket: %s", __LINE__, strerror(errno));
		close(GUISocket);
		exit(1);
	}

	GUIAddress.sun_family = AF_UNIX;

	//Builds the key path
	string key = CE_GUI_FILENAME;
	key = homePath + key;

	strcpy(GUIAddress.sun_path, key.c_str());
	unlink(GUIAddress.sun_path);
	len = strlen(GUIAddress.sun_path) + sizeof(GUIAddress.sun_family);

	if(bind(GUISocket,(struct sockaddr *)&GUIAddress,len) == -1)
	{
		syslog(SYSL_ERR, "Line: %d bind: %s", __LINE__, strerror(errno));
		close(GUISocket);
		exit(1);
	}

	if(listen(GUISocket, SOCKET_QUEUE_SIZE) == -1)
	{
		syslog(SYSL_ERR, "Line: %d listen: %s", __LINE__, strerror(errno));
		close(GUISocket);
		exit(1);
	}
	while(true)
	{
		ReceiveGUICommand();
	}
}

void *Nova::ClassificationEngine::ClassificationLoop(void *ptr)
{
	//Builds the GUI address
	string GUIKey = homePath + CE_FILENAME;
	GUISendRemote.sun_family = AF_UNIX;
	strcpy(GUISendRemote.sun_path, GUIKey.c_str());
	GUILen = strlen(GUISendRemote.sun_path) + sizeof(GUISendRemote.sun_family);

	//Builds the Silent Alarm IPC address
	string key = homePath + KEY_ALARM_FILENAME;
	alarmRemote.sun_family = AF_UNIX;
	strcpy(alarmRemote.sun_path, key.c_str());
	len = strlen(alarmRemote.sun_path) + sizeof(alarmRemote.sun_family);

	//Builds the Silent Alarm Network address
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(sAlarmPort);

	//Classification Loop
	do
	{
		sleep(classificationTimeout);
		pthread_rwlock_rdlock(&lock);
		//Calculate the "true" Feature Set for each Suspect
		for (SuspectHashTable::iterator it = suspects.begin() ; it != suspects.end(); it++)
		{
			if(it->second->needs_feature_update)
			{
				pthread_rwlock_unlock(&lock);
				pthread_rwlock_wrlock(&lock);
				it->second->CalculateFeatures(featureMask);
				pthread_rwlock_unlock(&lock);
				pthread_rwlock_rdlock(&lock);
			}
		}
		//Calculate the normalized feature sets, actually used by ANN
		//	Writes into Suspect ANNPoints
		NormalizeDataPoints();
		//Perform classification on each suspect
		for (SuspectHashTable::iterator it = suspects.begin() ; it != suspects.end(); it++)
		{
			if(it->second->needs_classification_update)
			{
				oldClassification = it->second->isHostile;
				Classify(it->second);
				cout << it->second->ToString(featureEnabled);
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
	} while(classificationTimeout);

	//Shouldn't get here!!
	if(classificationTimeout)
		syslog(SYSL_ERR, "Line: %d Main thread ended. Shouldn't get here!!!", __LINE__);
	return NULL;
}


void *Nova::ClassificationEngine::TrainingLoop(void *ptr)
{
	//Builds the GUI address
	string GUIKey = homePath + CE_FILENAME;
	GUISendRemote.sun_family = AF_UNIX;
	strcpy(GUISendRemote.sun_path, GUIKey.c_str());
	GUILen = strlen(GUISendRemote.sun_path) + sizeof(GUISendRemote.sun_family);


	//Training Loop
	do
	{
		sleep(classificationTimeout);
		ofstream myfile (trainingCapFile.data(), ios::app);
		//bool savePoint;
		//ANNdist distance;
		//int distanceThreshold = 0;

		if (myfile.is_open())
		{
			pthread_rwlock_wrlock(&lock);
			//Calculate the "true" Feature Set for each Suspect
			for (SuspectHashTable::iterator it = suspects.begin() ; it != suspects.end(); it++)
			{
				//savePoint = false;

				if(it->second->needs_feature_update)
				{
					it->second->CalculateFeatures(featureMask);
					if(it->second->annPoint == NULL)
						it->second->annPoint = annAllocPt(DIM);


					for(int j=0; j < DIM; j++)
						it->second->annPoint[j] = it->second->features.features[j];

					/*
					// Save the first point we get
					if (lastPoints[it->first] == NULL)
					{
						savePoint = true;
						lastPoints[it->first] = annCopyPt(DIM, it->second->annPoint);
					}
					else
					{
						// Compute squared distance from last point
						distance = 0;
						for(int j=0; j < DIM; j++)
							distance += annDist(j, lastPoints[it->first] , it->second->annPoint);

						cout << "Distance is " << distance << endl;
						if (distance >= distanceThreshold)
						{
							savePoint = true;
							annDeallocPt(lastPoints[it->first]);
							lastPoints[it->first] = annCopyPt(DIM, it->second->annPoint);
						}
					}
					*/

					//if (savePoint)
					//{
						myfile << string(inet_ntoa(it->second->IP_address)) << " ";

						for(int j=0; j < DIM; j++)
							myfile << it->second->annPoint[j] << " ";

						myfile << "\n";
					//}


					it->second->needs_feature_update = false;
					cout << it->second->ToString(featureEnabled);
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

void *Nova::ClassificationEngine::SilentAlarmLoop(void *ptr)
{
	int sockfd;
	u_char buf[MAX_MSG_SIZE];
	struct sockaddr_in sendaddr;

	if((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1)
	{
		syslog(SYSL_ERR, "Line: %d socket: %s", __LINE__, strerror(errno));
		close(sockfd);
		exit(1);
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
		exit(1);
	}

	stringstream ss;
	ss << "sudo iptables -A INPUT -p udp --dport " << sAlarmPort << " -j REJECT --reject-with icmp-port-unreachable";
	system(ss.str().c_str());
	ss.str("");
	ss << "sudo iptables -A INPUT -p tcp --dport " << sAlarmPort << " -j REJECT --reject-with tcp-reset";
	system(ss.str().c_str());

    if(listen(sockfd, SOCKET_QUEUE_SIZE) == -1)
    {
		syslog(SYSL_ERR, "Line: %d listen: %s", __LINE__, strerror(errno));
		close(sockfd);
        exit(1);
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
			delete suspect;
			suspect = NULL;
		}
		close(connectionSocket);
		pthread_rwlock_unlock(&lock);
	}
	close(sockfd);
	syslog(SYSL_INFO, "Line: %d Silent Alarm thread ended. Shouldn't get here!!!", __LINE__);
	return NULL;
}


void Nova::ClassificationEngine::FormKdTree()
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
				normalizedDataPts[i][j] = (double)((dataPts[i][j] / maxFeatureValues[j]));
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


void Nova::ClassificationEngine::Classify(Suspect *suspect)
{
	ANNpoint			queryPt;				// query point
	ANNidxArray			nnIdx;					// near neighbor indices
	ANNdistArray		dists;					// near neighbor distances

	queryPt = suspect->annPoint;

	nnIdx = new ANNidx[k];						// allocate near neigh indices
	dists = new ANNdist[k];						// allocate near neighbor dists

	kdTree->annkSearch(							// search
			queryPt,							// query point
			k,									// number of near neighbors
			nnIdx,								// nearest neighbors (returned)
			dists,								// distance (returned)
			eps);								// error bound

	//Determine classification according to weight by distance
	//	.5 + E[(1-Dist) * Class] / 2k (Where Class is -1 or 1)
	//	This will make the classification range from 0 to 1
	double classifyCount = 0;

	for (int i = 0; i < k; i++)
	{
		dists[i] = sqrt(dists[i]);				// unsquare distance

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
	pthread_rwlock_unlock(&lock);
	pthread_rwlock_wrlock(&lock);

	suspect->classification = .5 + (classifyCount / ((2.0 * (double)k) * sqrtDIM ));

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


void Nova::ClassificationEngine::NormalizeDataPoints()
{
	//Find the max values for each feature
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
					maxFeatureValues[ai] = it->second->features.features[i];
					updateKDTree = true;
				}
				ai++;
			}

		}
	}
	if(updateKDTree) FormKdTree();

	pthread_rwlock_unlock(&lock);
	pthread_rwlock_wrlock(&lock);
	//Normalize the suspect points
	for (SuspectHashTable::iterator it = suspects.begin();it != suspects.end();it++)
	{
		if(it->second->needs_feature_update)
		{
			if(it->second->annPoint == NULL)
				it->second->annPoint = annAllocPt(enabledFeatures);

			//If the max is 0, then there's no need to normalize! (Plus it'd be a div by zero)
			int ai = 0;
			for(int i = 0;i < DIM;i++)
			{
				if (featureEnabled[i])
				{
					if(maxFeatureValues[ai] != 0)
						it->second->annPoint[ai] = (double)(it->second->features.features[i] / maxFeatureValues[ai]);
					else
						syslog(SYSL_INFO, "Line: %d max Feature Value for feature %d is 0!", __LINE__, (i + 1));
					ai++;
				}

			}
			it->second->needs_feature_update = false;
		}
	}
	pthread_rwlock_unlock(&lock);
	pthread_rwlock_rdlock(&lock);
}


void Nova::ClassificationEngine::PrintPt(ostream &out, ANNpoint p)
{
	out << "(" << p[0];
	for (int i = 1;i < enabledFeatures;i++)
	{
		out << ", " << p[i];
	}
	out << ")\n";
}


void Nova::ClassificationEngine::LoadDataPointsFromFile(string inFilePath)
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
						{
							maxFeatureValues[actualDimension] = temp;
						}
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
			if(maxFeatureValues[j] != 0)
			{
				normalizedDataPts[i][j] = (double)((dataPts[i][j] / maxFeatureValues[j]));
			}
			else
			{
				syslog(SYSL_INFO, "Line: %d Max Feature Value for feature %d is 0!", __LINE__, (i + 1));
				break;
			}
		}
	}

	kdTree = new ANNkd_tree(					// build search structure
			normalizedDataPts,					// the data points
					nPts,						// number of points
					enabledFeatures);						// dimension of space
}


void Nova::ClassificationEngine::WriteDataPointsToFile(string outFilePath)
{

	ofstream myfile (outFilePath.data(), ios::app);

	if (myfile.is_open())
	{
		pthread_rwlock_rdlock(&lock);
		for ( SuspectHashTable::iterator it = suspects.begin();it != suspects.end();it++ )
		{
			for(int j=0; j < enabledFeatures; j++)
			{
				myfile << it->second->annPoint[j] << " ";
			}
			myfile << it->second->classification;
			myfile << "\n";
		}
		pthread_rwlock_unlock(&lock);
	}
	else
	{
		syslog(SYSL_ERR, "Line: %d Unable to open file.", __LINE__);
	}
	myfile.close();

}

//Returns usage tips
string Nova::ClassificationEngine::Usage()
{
	string usageString = "Nova Classification Engine!\n";
	usageString += "\tUsage: ClassificationEngine -l LogConfigPath -n NOVAConfigPath \n";
	usageString += "\t-n: Path to NOVA config txt file.\n";
	return usageString;
}


void Nova::ClassificationEngine::SilentAlarm(Suspect *suspect)
{
	int socketFD;

	uint dataLen = suspect->SerializeSuspect(data);

	//If the hostility hasn't changed don't bother the DM
	if(oldClassification != suspect->isHostile && suspect->isLive)
	{
		if ((socketFD = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		{
			syslog(SYSL_ERR, "Line: %d socket: %s", __LINE__, strerror(errno));
			close(socketFD);
			return;
		}

		if (connect(socketFD, alarmRemotePtr, len) == -1)
		{
			syslog(SYSL_ERR, "Line: %d connect: %s", __LINE__, strerror(errno));
			close(socketFD);
			return;
		}

		if (send(socketFD, data, dataLen, 0) == -1)
		{
			syslog(SYSL_ERR, "Line: %d send: %s", __LINE__, strerror(errno));
			close(socketFD);
			return;
		}
		close(socketFD);
	}
	if(suspect->features.packetCount.first && suspect->isLive)
	{
		do
		{
			bzero(data, MAX_MSG_SIZE);
			dataLen = suspect->SerializeSuspect(data);
			dataLen += suspect->features.SerializeFeatureDataBroadcast(data+dataLen);
			//Update other Nova Instances with latest suspect Data
			for(uint i = 0; i < neighbors.size(); i++)
			{
				serv_addr.sin_addr.s_addr = neighbors[i];

				stringstream ss;
				string commandLine;

				ss << "sudo iptables -I INPUT -s " << string(inet_ntoa(serv_addr.sin_addr)) << " -p tcp -j ACCEPT";
				commandLine = ss.str();
				system(commandLine.c_str());

				uint i;
				for(i = 0; i < SA_Max_Attempts; i++)
				{
					if(KnockPort(OPEN))
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
					system(commandLine.c_str());
					continue;
				}

				if( send(sockfd,data,dataLen,0) == -1)
				{
					syslog(SYSL_ERR, "Line: %d Error in TCP Send: %s", __LINE__, strerror(errno));
					close(sockfd);
					ss.str("");
					ss << "sudo iptables -D INPUT -s " << string(inet_ntoa(serv_addr.sin_addr)) << " -p tcp -j ACCEPT";
					commandLine = ss.str();
					system(commandLine.c_str());
					continue;
				}
				close(sockfd);
				KnockPort(CLOSE);
				ss.str("");
				ss << "sudo iptables -D INPUT -s " << string(inet_ntoa(serv_addr.sin_addr)) << " -p tcp -j ACCEPT";
				commandLine = ss.str();
				system(commandLine.c_str());
			}
		}while(dataLen == MORE_DATA);
	}
}


bool ClassificationEngine::KnockPort(bool mode)
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


bool ClassificationEngine::ReceiveSuspectData()
{
	int bytesRead, connectionSocket;

    //Blocking call
    if ((connectionSocket = accept(IPCsock, remoteSockAddr, sockSizePtr)) == -1)
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
		pthread_rwlock_unlock(&lock);
	}
	catch(std::exception e)
	{
		syslog(SYSL_ERR, "Line: %d Error in parsing received Suspect: %s", __LINE__, string(e.what()).c_str());
		close(connectionSocket);
		bzero(buffer, MAX_MSG_SIZE);
		return false;
	}
	close(connectionSocket);
	return true;
}


void ClassificationEngine::ReceiveGUICommand()
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
			exit(1);
		case CLEAR_ALL:
			pthread_rwlock_wrlock(&lock);
			suspects.clear();
			pthread_rwlock_unlock(&lock);
			break;
		case CLEAR_SUSPECT:
			suspectKey = inet_addr(msg.GetValue().c_str());
			pthread_rwlock_wrlock(&lock);
			suspects.set_deleted_key(5);
			suspects.erase(suspectKey);
			suspects.clear_deleted_key();
			pthread_rwlock_unlock(&lock);
			break;
		case WRITE_SUSPECTS:
			SaveSuspectsToFile(msg.GetValue());
			break;
		default:
			break;
	}
	close(msgSocket);
}


void ClassificationEngine::SaveSuspectsToFile(string filename)
{
	syslog(SYSL_ERR, "Line: %d Got request to save file to %s", __LINE__, filename.c_str());
	dataPtsWithClass.push_back(new Point(enabledFeatures));

	ofstream *out = new ofstream(filename.c_str());

	if(!out->is_open())
	{
		syslog(SYSL_ERR, "Line: %d Error: Unable to open file %s to save suspect data.", __LINE__, filename.c_str());
		return;
	}

	pthread_rwlock_rdlock(&lock);
	for (SuspectHashTable::iterator it = suspects.begin() ; it != suspects.end(); it++)
	{
		*out << it->second->ToString(featureEnabled) << endl;
	}
	pthread_rwlock_unlock(&lock);

	out->close();
}


void Nova::ClassificationEngine::SendToUI(Suspect *suspect)
{
	uint GUIDataLen = suspect->SerializeSuspect(GUIData);

	if ((GUISendSocket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		syslog(SYSL_ERR, "Line: %d socket: %s", __LINE__, strerror(errno));
		close(GUISendSocket);
		return;
	}

	if (connect(GUISendSocket, GUISendPtr, GUILen) == -1)
	{
		syslog(SYSL_ERR, "Line: %d connect: %s", __LINE__, strerror(errno));
		close(GUISendSocket);
		return;
	}

	if (send(GUISendSocket, GUIData, GUIDataLen, 0) == -1)
	{
		syslog(SYSL_ERR, "Line: %d send: %s", __LINE__, strerror(errno));
		close(GUISendSocket);
		return;
	}
	close(GUISendSocket);
}


void ClassificationEngine::LoadConfig(char * configFilePath)
{
	string prefix, line;
	uint i = 0;
	int confCheck = 0;

	string settingsPath = homePath +"/settings";
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
	NovaConfig->LoadConfig(configFilePath, homePath, __FILE__);
	NovaMessageClient * NovaMessageConf = new NovaMessageClient(__FILE__);
	if(!NovaMessageConf->LoadConfiguration(configFilePath))
	{
		syslog(SYSL_ERR, "Line: %d One or more of the messaging configuration values have not been set, and have no default.", __LINE__);
		exit(1);
	}

	confCheck = NovaConfig->SetDefaults();

	openlog("ClassificationEngine", OPEN_SYSL, LOG_AUTHPRIV);

	//Checks to make sure all values have been set.
	if(confCheck == 2)
	{
		syslog(SYSL_ERR, "Line: %d One or more values have not been set, and have no default.", __LINE__);
		exit(1);
	}
	else if(confCheck == 1)
	{
		syslog(SYSL_INFO, "Line: %d INFO Config loaded successfully with defaults; some variables in NOVAConfig.txt were incorrectly set, not present, or not valid!", __LINE__);
	}
	else if (confCheck == 0)
	{
		syslog(SYSL_INFO, "Line: %d INFO Config loaded successfully.", __LINE__);
	}

	hostAddrString = GetLocalIP(NovaConfig->options["INTERFACE"].data.c_str());
	if(hostAddrString.size() == 0)
	{
		syslog(SYSL_ERR, "Line: %d Bad interface, no IP's associated!", __LINE__);
		exit(1);
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
