//============================================================================
// Name        : ClassificationEngine.cpp
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Classifies suspects as either hostile or benign then takes appropriate action
//============================================================================

#include "ClassificationEngine.h"
#include <TrafficEvent.h>
#include <errno.h>
#include <fstream>
#include "Point.h"
#include <arpa/inet.h>
#include <GUIMsg.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <net/if.h>
#include <sys/un.h>
#include <log4cxx/xml/domconfigurator.h>
#include "NOVAConfiguration.h"

using namespace log4cxx;
using namespace log4cxx::xml;
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

//NOT normalized
vector <Point*> dataPtsWithClass;
bool isTraining = false;
bool useTerminals;
static string hostAddrString;
static struct sockaddr_in hostAddr;

//Global variables related to Classification

//Global memory assignments to improve performance of IPC loops

//** Main (ReceiveSuspectData) **
struct sockaddr_un remote;
struct sockaddr* remoteSockAddr = (struct sockaddr *)&remote;
int bytesRead;
int connectionSocket;
u_char buffer[MAX_MSG_SIZE];
TrafficEvent *tempEvent, *event;
Suspect * suspect;
int IPCsock;

//** Silent Alarm **
struct sockaddr_un alarmRemote;
struct sockaddr* alarmRemotePtr =(struct sockaddr *)&alarmRemote;
struct sockaddr_in serv_addr;
struct sockaddr* serv_addrPtr = (struct sockaddr *)&serv_addr;
int len;
int oldClassification;

u_char data[MAX_MSG_SIZE];
uint dataLen;

int numBytesRead;
int socketFD;
int sockfd, broadcast = 1;
int * bdPtr = &broadcast;
int bdsize = sizeof broadcast;
char alarmBuf[MAX_MSG_SIZE];


//** ReceiveGUICommand **
int GUISocket;

//** SendToUI **
struct sockaddr_un GUISendRemote;
struct sockaddr* GUISendPtr = (struct sockaddr *)&GUISendRemote;
int GUISendSocket;
int GUILen;
u_char GUIData[MAX_MSG_SIZE];
uint GUIDataLen;


//Universal Socket variables (constants that can be re-used)
int socketSize = sizeof(remote);
int inSocketSize = sizeof(serv_addr);
socklen_t * sockSizePtr = (socklen_t*)&socketSize;




//Configured in NOVAConfig_CE or specified config file.
string broadcastAddr;			//Silent Alarm destination IP address
int sAlarmPort;					//Silent Alarm destination port
int classificationTimeout;		//In seconds, how long to wait between classifications
int maxFeatureVal;				//The value to normalize feature values to.
									//Higher value makes for more precision, but more cycles.

int k;							//number of nearest neighbors
double eps;						//error bound
int maxPts;						//maximum number of data points
double classificationThreshold = .5; //value of classification to define split between hostile / benign
uint SA_Max_Attempts = 3;			//The number of times to attempt to reconnect to a neighbor
double SA_Sleep_Duration = 0.5;		//The time to sleep after a port knocking request and allow it to go through
//End configured variables
const int dim = DIM;					//dimension

int nPts = 0;						//actual number of data points
ANNpointArray dataPts;				//data points
ANNpointArray normalizedDataPts;	//normalized data points
ANNkd_tree*	kdTree;					// search structure
string dataFile;					//input for data points
istream* queryIn = NULL;			//input for query points

const char *outFile;				//output for data points during training

char * pathsFile = (char*)"/etc/nova/paths";
string homePath;

void *outPtr;

LoggerPtr m_logger(Logger::getLogger("main"));

int maxFeatureValues[dim];

//Used to indicate if the kd tree needs to be reformed
bool updateKDTree = false;

int main(int argc,char *argv[])
{
	bzero(GUIData,MAX_MSG_SIZE);
	bzero(data,MAX_MSG_SIZE);
	bzero(buffer, MAX_MSG_SIZE);
	pthread_rwlock_init(&lock, NULL);

	suspects.set_empty_key(NULL);
	suspects.resize(INITIAL_TABLESIZE);

	int len;
	struct sockaddr_un localIPCAddress;

	pthread_t classificationLoopThread;
	pthread_t trainingLoopThread;
	pthread_t silentAlarmListenThread;
	pthread_t GUIListenThread;

	string novaConfig, logConfig;
	string line, prefix; //used for input checking

	//Get locations of nova files
	ifstream *paths =  new ifstream(pathsFile);

	if(paths->is_open())
	{
		while(paths->good())
		{
			getline(*paths,line);

			prefix = "NOVA_HOME";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				homePath = line;
				break;
			}
		}
	}
	paths->close();
	delete paths;
	paths = NULL;

	//Resolves environment variables
	int start = 0;
	int end = 0;
	string var;

	while((start = homePath.find("$",end)) != -1)
	{
		end = homePath.find("/", start);
		//If no path after environment var
		if(end == -1)
		{

			var = homePath.substr(start+1, homePath.size());
			var = getenv(var.c_str());
			homePath = homePath.substr(0,start) + var;
		}
		else
		{
			var = homePath.substr(start+1, end-1);
			var = getenv(var.c_str());
			var = var + homePath.substr(end, homePath.size());
			if(start > 0)
			{
				homePath = homePath.substr(0,start)+var;
			}
			else
			{
				homePath = var;
			}
		}
	}

	if(homePath == "")
	{
		exit(1);
	}

	novaConfig = homePath + "/Config/NOVAConfig.txt";
	logConfig = homePath + "/Config/Log4cxxConfig_Console.xml";

	DOMConfigurator::configure(logConfig.c_str());

	//Runs the configuration loader
	LoadConfig((char*)novaConfig.c_str());

	if(!useTerminals)
	{
		logConfig = homePath +"/Config/Log4cxxConfig.xml";
		DOMConfigurator::configure(logConfig.c_str());
	}

	dataFile = homePath + "/" +dataFile;
	outFile = dataFile.c_str();
	outPtr = (void *)outFile;

	pthread_create(&GUIListenThread, NULL, GUILoop, NULL);
	//Are we Training or Classifying?
	if(isTraining)
	{
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
		LOG4CXX_ERROR(m_logger, "socket: " << strerror(errno));
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
    	LOG4CXX_ERROR(m_logger, "bind: " << strerror(errno));
    	close(IPCsock);
        exit(1);
    }

    if(listen(IPCsock, SOCKET_QUEUE_SIZE) == -1)
    {
    	LOG4CXX_ERROR(m_logger, "listen: " << strerror(errno));
		close(IPCsock);
        exit(1);
    }

	//"Main Loop"
	while(true)
	{
		ReceiveSuspectData();
	}

	//Shouldn't get here!
	LOG4CXX_ERROR(m_logger,"Main thread ended. Shouldn't get here!!!");
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
		LOG4CXX_ERROR(m_logger, "socket: " << strerror(errno));
		close(GUISocket);
		exit(1);
	}

	GUIAddress.sun_family = AF_UNIX;

	//Builds the key path
	string key = GUI_FILENAME;
	key = homePath + key;

	strcpy(GUIAddress.sun_path, key.c_str());
	unlink(GUIAddress.sun_path);
	len = strlen(GUIAddress.sun_path) + sizeof(GUIAddress.sun_family);

	if(bind(GUISocket,(struct sockaddr *)&GUIAddress,len) == -1)
	{
		LOG4CXX_ERROR(m_logger, "bind: " << strerror(errno));
		close(GUISocket);
		exit(1);
	}

	if(listen(GUISocket, SOCKET_QUEUE_SIZE) == -1)
	{
		LOG4CXX_ERROR(m_logger, "listen: " << strerror(errno));
		close(GUISocket);
		exit(1);
	}
	while(true)
	{
		ReceiveGUICommand();
	}
}

//Separate thread which infinite loops, periodically updating all the classifications
//	for all the current suspects
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
	while(true)
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
				it->second->CalculateFeatures(isTraining);
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
				cout << it->second->ToString();
				//If suspect is hostile and this Nova instance has unique information
				// 			(not just from silent alarms)
				if(it->second->isHostile)
					SilentAlarm(it->second);
				SendToUI(it->second);
			}
		}
		pthread_rwlock_unlock(&lock);
	}
	//Shouldn't get here!
	LOG4CXX_ERROR(m_logger,"Main thread ended. Shouldn't get here!!!");
	return NULL;
}


//Thread for calculating training data, and writing to file.
void *Nova::ClassificationEngine::TrainingLoop(void *ptr)
{
	string GUIKey = homePath + CE_FILENAME;
	strcpy(GUISendRemote.sun_path, GUIKey.c_str());

	GUISendRemote.sun_family = AF_UNIX;
	GUILen = strlen(GUISendRemote.sun_path) + sizeof(GUISendRemote.sun_family);

	//Training Loop
	while(true)
	{
		sleep(classificationTimeout);
		ofstream myfile (string(outFile).data(), ios::app);
		if (myfile.is_open())
		{
			pthread_rwlock_wrlock(&lock);
			//Calculate the "true" Feature Set for each Suspect
			for (SuspectHashTable::iterator it = suspects.begin() ; it != suspects.end(); it++)
			{
				if(it->second->needs_feature_update)
				{
					it->second->CalculateFeatures(isTraining);
					if(it->second->annPoint == NULL)
					{
						it->second->annPoint = annAllocPt(DIMENSION);
					}
					for(int j=0; j < dim; j++)
					{
						it->second->annPoint[j] = it->second->features.features[j];
						myfile << it->second->annPoint[j] << " ";
					}
					it->second->needs_feature_update = false;
					myfile << it->second->classification;
					myfile << "\n";
					cout << it->second->ToString();
					SendToUI(it->second);
				}
			}
			pthread_rwlock_unlock(&lock);
		}
		else LOG4CXX_INFO(m_logger, "Unable to open file.");
		myfile.close();
	}
	//Shouldn't get here!
	LOG4CXX_ERROR(m_logger,"Training thread ended. Shouldn't get here!!!");
	return NULL;
}

//Thread for listening for Silent Alarms from other Nova instances
void *Nova::ClassificationEngine::SilentAlarmLoop(void *ptr)
{
	int sockfd;
	u_char buf[MAX_MSG_SIZE];
	struct sockaddr_in sendaddr;

	if((sockfd = socket(AF_INET,SOCK_STREAM,6)) == -1)
	{
		LOG4CXX_ERROR(m_logger, "socket: " << strerror(errno));
		close(sockfd);
		exit(1);
	}

	sendaddr.sin_family = AF_INET;
	sendaddr.sin_port = htons(sAlarmPort);
	sendaddr.sin_addr.s_addr = hostAddr.sin_addr.s_addr;

	memset(sendaddr.sin_zero,'\0', sizeof sendaddr.sin_zero);
	struct sockaddr* sockaddrPtr = (struct sockaddr*) &sendaddr;
	socklen_t sendaddrSize = sizeof sendaddr;

	if(bind(sockfd,sockaddrPtr,sendaddrSize) == -1)
	{
		LOG4CXX_ERROR(m_logger, "bind: " << strerror(errno));
		close(sockfd);
		exit(1);
	}

    if(listen(sockfd, SOCKET_QUEUE_SIZE) == -1)
    {
		LOG4CXX_ERROR(m_logger,"listen: " << strerror(errno));
		close(sockfd);
        exit(1);
    }

	int connectionSocket, bytesRead;

	//Accept incoming Silent Alarm TCP Connections
	while(1)
	{

		//Blocking call
		if((connectionSocket = accept(sockfd, sockaddrPtr, &sendaddrSize)) == -1)
		{
			LOG4CXX_ERROR(m_logger,"accept: " << strerror(errno));
			close(connectionSocket);
			continue;
		}
		bzero(buf, MAX_MSG_SIZE);

		if((bytesRead = recv(connectionSocket, buf, MAX_MSG_SIZE, MSG_WAITALL)) == -1)
		{
			LOG4CXX_ERROR(m_logger,"recv: " << strerror(errno));
			close(connectionSocket);
			continue;
		}

		//If this is from ourselves, then drop it.
		if(hostAddr.sin_addr.s_addr == sendaddr.sin_addr.s_addr)
		{
			close(connectionSocket);
			continue;
		}

		crpytBuffer(buf, bytesRead, DECRYPT);

		string keyCheck = string((char*)buf);
		keyCheck = keyCheck.substr(0, key.size());

		//If the first packets are the key this is a knock request (closing) and should be ignored.
		if(!keyCheck.compare(key))
		{
			close(connectionSocket);
			continue;
		}

		pthread_rwlock_wrlock(&lock);
		try
		{
			uint addr = getSerializedAddr(buf);
			SuspectHashTable::iterator it = suspects.find(addr);

			//If this is a new suspect put it in the table
			if(it == suspects.end())
			{
				suspects[addr] = new Suspect();
				suspects[addr]->deserializeSuspectWithData(buf, BROADCAST_DATA);
				//We set isHostile to false so that when we classify the first time
				// the suspect will go from benign to hostile and be sent to the doppelganger module
				suspects[addr]->isHostile = false;
			}
			//If this suspect exists, update the information
			else
			{
				//This function will overwrite everything except the information used to calculate the classification
				// a combined classification will be given next classification loop
				suspects[addr]->deserializeSuspectWithData(buf, BROADCAST_DATA);
			}
			suspects[addr]->flaggedByAlarm = true;

			LOG4CXX_INFO(m_logger, "Received Silent Alarm!\n" << suspects[addr]->ToString());
		}
		catch(std::exception e)
		{
			LOG4CXX_ERROR(m_logger, "Error interpreting received Silent Alarm: " << string(e.what()));
			delete suspect;
			suspect = NULL;
		}
		close(connectionSocket);
		pthread_rwlock_unlock(&lock);
	}
	close(sockfd);
	LOG4CXX_INFO(m_logger,"Silent Alarm thread ended. Shouldn't get here!!!");
	return NULL;
}

//Forms the normalized kd tree, done once on start up
//will be called again if the a suspect's max value for a feature exceeds the current maximum
void Nova::ClassificationEngine::FormKdTree()
{
	delete kdTree;
	//Normalize the data points
	//Foreach data point
	for(int j = 0;j < dim;j++)
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
				LOG4CXX_INFO(m_logger,"Max Feature Value for feature " << (i+1) << " is 0!");
				break;
			}
		}
	}
	kdTree = new ANNkd_tree(					// build search structure
			normalizedDataPts,					// the data points
					nPts,						// number of points
					dim);						// dimension of space
	updateKDTree = false;
}

//Performs classification on given suspect
//Where all the magic takes place
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
			LOG4CXX_ERROR(m_logger, "Unable to find a nearest neighbor for Data point: " << i << "\nTry decreasing the Error bound");
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
				LOG4CXX_ERROR(m_logger,"Data point: " << i << " has an invalid classification of: " <<
						dataPtsWithClass[ nnIdx[i] ]->classification << ". This must be either 0 (benign) or 1 (hostile).");
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


//Calculates normalized data points for suspects
void Nova::ClassificationEngine::NormalizeDataPoints()
{
	//Find the max values for each feature
	for (SuspectHashTable::iterator it = suspects.begin();it != suspects.end();it++)
	{
		for(int i = 0;i < dim;i++)
		{
			if(it->second->features.features[i] > maxFeatureValues[i])
			{
				//For proper normalization the upper bound for a feature is the max value of the data.
				maxFeatureValues[i] = it->second->features.features[i];
				updateKDTree = true;
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
				it->second->annPoint = annAllocPt(DIMENSION);

			//If the max is 0, then there's no need to normalize! (Plus it'd be a div by zero)
			for(int i = 0;i < dim;i++)
			{
				if(maxFeatureValues[0] != 0)
					it->second->annPoint[i] = (double)(it->second->features.features[i] / maxFeatureValues[i]);
				else
					LOG4CXX_INFO(m_logger,"Max Feature Value for feature " << (i+1) << " is 0!");
			}
			it->second->needs_feature_update = false;
		}
	}
	pthread_rwlock_unlock(&lock);
	pthread_rwlock_rdlock(&lock);
}

//Prints a single ANN point, p, to stream, out
void Nova::ClassificationEngine::printPt(ostream &out, ANNpoint p)
{
	out << "(" << p[0];
	for (int i = 1;i < dim;i++)
	{
		out << ", " << p[i];
	}
	out << ")\n";
}

//Reads into the list of suspects from a file specified by inFilePath
void Nova::ClassificationEngine::LoadDataPointsFromFile(string inFilePath)
{
	ifstream myfile (inFilePath.data());
	string line;
	int i = 0;
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
	else LOG4CXX_ERROR(m_logger, "Unable to open file.");
	myfile.close();
	maxPts = i;

	//Open the file again, allocate the number of points and assign
	myfile.open(inFilePath.data(), ifstream::in);
	dataPts = annAllocPts(maxPts, dim);			// allocate data points
	normalizedDataPts = annAllocPts(maxPts, dim);

	if (myfile.is_open())
	{
		i = 0;

		while (!myfile.eof() && (i < maxPts))
		{
			if(myfile.peek() == EOF)
			{
				break;
			}

			dataPtsWithClass.push_back(new Point());

			for(int j = 0;j < dim;j++)
			{
				getline(myfile,line,' ');
				double temp = strtod(line.data(), NULL);

				dataPtsWithClass[i]->annPoint[j] = temp;
				dataPts[i][j] = temp;

				//Set the max values of each feature. (Used later in normalization)
				if(temp > maxFeatureValues[j])
				{
					maxFeatureValues[j] = temp;
				}
			}
			getline(myfile,line);
			dataPtsWithClass[i]->classification = atoi(line.data());
			i++;
		}
		nPts = i;
	}
	else LOG4CXX_ERROR(m_logger,"Unable to open file.");
	myfile.close();

	//Normalize the data points
	//Foreach data point
	for(int j = 0;j < dim;j++)
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
				LOG4CXX_INFO(m_logger,"Max Feature Value for feature " << (i+1) << " is 0!");
				break;
			}
		}
	}

	kdTree = new ANNkd_tree(					// build search structure
			normalizedDataPts,					// the data points
					nPts,						// number of points
					dim);						// dimension of space
}

//Writes dataPtsWithClass out to a file specified by outFilePath
void Nova::ClassificationEngine::WriteDataPointsToFile(string outFilePath)
{

	ofstream myfile (outFilePath.data(), ios::app);

	if (myfile.is_open())
	{
		pthread_rwlock_rdlock(&lock);
		for ( SuspectHashTable::iterator it = suspects.begin();it != suspects.end();it++ )
		{
			for(int j=0; j < dim; j++)
			{
				myfile << it->second->annPoint[j] << " ";
			}
			myfile << it->second->classification;
			myfile << "\n";
		}
		pthread_rwlock_unlock(&lock);
	}
	else LOG4CXX_ERROR(m_logger, "Unable to open file.");
	myfile.close();

}

//Returns usage tips
string Nova::ClassificationEngine::Usage()
{
	string usageString = "Nova Classification Engine!\n";
	usageString += "\tUsage: ClassificationEngine -l LogConfigPath -n NOVAConfigPath \n";
	usageString += "\t-l: Path to LOG4CXX config xml file.\n";
	usageString += "\t-n: Path to NOVA config txt file.\n";
	return usageString;
}



//Returns a string representation of the specified device's IP address
string Nova::ClassificationEngine::getLocalIP(const char *dev)
{
	static struct ifreq ifreqs[20];
	struct ifconf ifconf;

	uint  nifaces, i;
	memset(&ifconf,0,sizeof(ifconf));

	ifconf.ifc_buf = (char*)(ifreqs);
	ifconf.ifc_len = sizeof(ifreqs);

	int sock, rval;
	sock = socket(AF_INET,SOCK_STREAM,0);

	if(sock < 0)
	{
		LOG4CXX_ERROR(m_logger, "socket: " << strerror(errno));
		close(sock);
		return NULL;
	}

	if((rval = ioctl(sock,SIOCGIFCONF,(char*)&ifconf)) < 0 )
	{
		LOG4CXX_ERROR(m_logger, "ioctl(SIOGIFCONF): " << strerror(errno));
	}

	close(sock);
	nifaces =  ifconf.ifc_len/sizeof(struct ifreq);

	for(i = 0;i < nifaces;i++)
	{
		if( strcmp(ifreqs[i].ifr_name, dev) == 0 )
		{
			char ip_addr [INET_ADDRSTRLEN];
			struct sockaddr_in *b = (struct sockaddr_in *) &(ifreqs[i].ifr_addr);

			inet_ntop(AF_INET,&(b->sin_addr),ip_addr,INET_ADDRSTRLEN);
			return string(ip_addr);
		}
	}
	return string("");
}

//Send a silent alarm about the argument suspect
void Nova::ClassificationEngine::SilentAlarm(Suspect *suspect)
{
	bzero(data, MAX_MSG_SIZE);
	dataLen = suspect->serializeSuspect(data);
	uint featureData = 0;

	//If the hostility hasn't changed don't bother the DM
	if(oldClassification != suspect->isHostile && suspect->isLive)
	{
		if ((socketFD = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		{
			LOG4CXX_ERROR(m_logger, "socket: " << strerror(errno));
			close(socketFD);
			return;
		}

		if (connect(socketFD, alarmRemotePtr, len) == -1)
		{
			LOG4CXX_ERROR(m_logger, "connect: " << strerror(errno));
			close(socketFD);
			return;
		}

		if (send(socketFD, data, dataLen, 0) == -1)
		{
			LOG4CXX_ERROR(m_logger, "send: " << strerror(errno));
			close(socketFD);
			return;
		}
		close(socketFD);
	}
	if(suspect->features.packetCount.first && suspect->isLive)
	{
		do
		{

			//Update other Nova Instances with latest suspect Data
			for(uint i = 0; i < neighbors.size(); i++)
			{
				serv_addr.sin_addr.s_addr = neighbors[i];

				stringstream ss;
				string commandLine;

				ss << "iptables -I INPUT 1 -s " << string(inet_ntoa(serv_addr.sin_addr)) << " -p tcp -j ACCEPT";
				commandLine = ss.str();
				system(commandLine.c_str());

				if(knockPort(OPEN))
				{
					bzero(data,MAX_MSG_SIZE);
					dataLen = suspect->serializeSuspect(data);
					featureData = suspect->features.serializeFeatureDataBroadcast(data+dataLen);
					uint i;
					for(i = 0; i < SA_Max_Attempts; i++)
					{
						//Send Silent Alarm to other Nova Instances with feature Data
						if ((sockfd = socket(AF_INET,SOCK_STREAM,6)) == -1)
						{
							LOG4CXX_ERROR(m_logger, "socket: " << strerror(errno));
							close(sockfd);
							knockPort(OPEN);
							continue;
						}

						if (connect(sockfd, serv_addrPtr, inSocketSize) == -1)
						{
							LOG4CXX_INFO(m_logger, "connect: " << strerror(errno));
							close(sockfd);
							knockPort(OPEN);
							continue;
						}
						break;
					}
					if(i == SA_Max_Attempts)
					{
						close(sockfd);
						continue;
					}

					if( send(sockfd,data,dataLen+featureData,0) == -1)
					{
						LOG4CXX_ERROR(m_logger,"Error in TCP Send: " << strerror(errno));
						close(sockfd);
						continue;
					}
					close(sockfd);
					knockPort(CLOSE);
				}
				commandLine = "iptables -D INPUT 1";
				system(commandLine.c_str());
			}
			bzero(data,MAX_MSG_SIZE);
		}while(featureData == MORE_DATA);
	}
}

bool ClassificationEngine::knockPort(bool mode)
{
	bzero(data, MAX_MSG_SIZE);
	stringstream ss;
	ss << key;
	//mode == OPEN (true)
	if(mode)
	{
		ss << "OPEN";
		dataLen = key.size() + 4;
	}
	//mode == CLOSE (false)
	else
	{
		ss << "SHUT";
		dataLen = key.size() + 4;
	}
	memcpy(data, ss.str().c_str(), ss.str().size());

	crpytBuffer(data, dataLen, ENCRYPT);

	//Send Port knock to other Nova Instances
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 17)) == -1)
	{
		LOG4CXX_ERROR(m_logger, "socket: " << strerror(errno));
		close(sockfd);
		return false;
	}

	if( sendto(sockfd,data,dataLen, MSG_DONTWAIT,serv_addrPtr, inSocketSize) == -1)
	{
		LOG4CXX_ERROR(m_logger,"Error in UDP Send: " << strerror(errno));
		close(sockfd);
		return false;
	}

	close(sockfd);
	sleep(SA_Sleep_Duration);
	return true;
}

///Receive a TrafficEvent from another local component.
/// This is a blocking function. If nothing is received, then wait on this thread for an answer
bool ClassificationEngine::ReceiveSuspectData()
{
    //Blocking call
    if ((connectionSocket = accept(IPCsock, remoteSockAddr, sockSizePtr)) == -1)
    {
		LOG4CXX_ERROR(m_logger,"accept: " << strerror(errno));
		close(connectionSocket);
        return false;
    }
    if((bytesRead = recv(connectionSocket, buffer, MAX_MSG_SIZE, MSG_WAITALL)) == -1)
    {
		LOG4CXX_ERROR(m_logger,"recv: " << strerror(errno));
		close(connectionSocket);
        return false;
    }
	try
	{
		pthread_rwlock_wrlock(&lock);
		uint addr = getSerializedAddr(buffer);
		SuspectHashTable::iterator it = suspects.find(addr);

		//If this is a new suspect make an entry in the table
		if(it == suspects.end())
			suspects[addr] = new Suspect();
		//Deserialize the data
		suspects[addr]->deserializeSuspectWithData(buffer, LOCAL_DATA);
		pthread_rwlock_unlock(&lock);
	}
	catch(std::exception e)
	{
		LOG4CXX_ERROR(m_logger,"Error in parsing received TrafficEvent: " << string(e.what()));
		close(connectionSocket);
		bzero(buffer, MAX_MSG_SIZE);
		return false;
	}
	close(connectionSocket);
	return true;
}

/// This is a blocking function. If nothing is received, then wait on this thread for an answer
void ClassificationEngine::ReceiveGUICommand()
{
    struct sockaddr_un msgRemote;
	int socketSize, msgSocket;
	int bytesRead;
	GUIMsg msg = GUIMsg();
	u_char msgBuffer[MAX_GUIMSG_SIZE];

	socketSize = sizeof(msgRemote);

	//Blocking call
	if ((msgSocket = accept(GUISocket, (struct sockaddr *)&msgRemote, (socklen_t*)&socketSize)) == -1)
	{
		LOG4CXX_ERROR(m_logger,"accept: " << strerror(errno));
		close(msgSocket);
	}
	if((bytesRead = recv(msgSocket, msgBuffer, MAX_GUIMSG_SIZE, MSG_WAITALL )) == -1)
	{
		LOG4CXX_ERROR(m_logger,"recv: " << strerror(errno));
		close(msgSocket);
	}

	msg.deserializeMessage(msgBuffer);
	switch(msg.getType())
	{
		case EXIT:
			exit(1);
		case CLEAR_ALL:
			pthread_rwlock_wrlock(&lock);
			suspects.clear();
			pthread_rwlock_unlock(&lock);
			break;
		case CLEAR_SUSPECT:
			//TODO still no functionality for this yet
			break;
		default:
			break;
	}
	close(msgSocket);
}

//Send a silent alarm about the argument suspect
void Nova::ClassificationEngine::SendToUI(Suspect *suspect)
{
	GUIDataLen = suspect->serializeSuspect(GUIData);

	if ((GUISendSocket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		LOG4CXX_ERROR(m_logger,"socket: " << strerror(errno));
		close(GUISendSocket);
		return;
	}

	if (connect(GUISendSocket, GUISendPtr, GUILen) == -1)
	{
		LOG4CXX_ERROR(m_logger,"connect: " << strerror(errno));
		close(GUISendSocket);
		return;
	}

	if (send(GUISendSocket, GUIData, GUIDataLen, 0) == -1)
	{
		LOG4CXX_ERROR(m_logger,"send: " << strerror(errno));
		close(GUISendSocket);
		return;
	}
	close(GUISendSocket);
}

//Encrpyts/decrypts a char buffer of size 'size' depending on mode
void ClassificationEngine::crpytBuffer(u_char * buf, uint size, bool mode)
{
	//TODO
}

void ClassificationEngine::LoadConfig(char * input)
{
	string prefix, line;
	uint i = 0;
	bool v = true;

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
						LOG4CXX_ERROR(m_logger, "Invalid IP address parsed on line " << i << " of the settings file.");
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
					LOG4CXX_ERROR(m_logger, "Invalid Key parsed on line " << i << " of the settings file.");
			}
		}
	}
	settings.close();


	NOVAConfiguration * NovaConfig = new NOVAConfiguration();
	NovaConfig->LoadConfig(input, homePath);

	const string prefixes[] = {"INTERFACE","USE_TERMINALS",
	"BROADCAST_ADDR","SILENT_ALARM_PORT",
	"K", "EPS",
	"CLASSIFICATION_TIMEOUT","IS_TRAINING",
	"CLASSIFICATION_THRESHOLD","DATAFILE", "SA_MAX_ATTEMPTS", "SA_SLEEP_DURATION"};

	for (i = 0; i < 12; i++) {
		prefix = prefixes[i];

		NovaConfig->options[prefix];
		if (!NovaConfig->options[prefix].isValid) {
			LOG4CXX_ERROR(m_logger, i + " The configuration variable # " + prefixes[i] + " was not set in configuration file " + input);
			v = false;
		}
	}

	//Checks to make sure all values have been set.
	if(v == false)
	{
		LOG4CXX_ERROR(m_logger,"One or more values have not been set.");
		exit(1);
	}
	else
	{
		LOG4CXX_INFO(m_logger,"Config loaded successfully.");
	}

	hostAddrString = getLocalIP(NovaConfig->options["INTERFACE"].data.c_str());
	if(hostAddrString.size() == 0)
	{
		LOG4CXX_ERROR(m_logger, "Bad interface, no IP's associated!");
		exit(1);
	}

	inet_pton(AF_INET,hostAddrString.c_str(),&(hostAddr.sin_addr));


	useTerminals = atoi(NovaConfig->options["USE_TERMINALS"].data.c_str());
	broadcastAddr = NovaConfig->options["BROADCAST_ADDR"].data;
	sAlarmPort = atoi(NovaConfig->options["SILENT_ALARM_PORT"].data.c_str());
	k = atoi(NovaConfig->options["K"].data.c_str());
	eps =  atof(NovaConfig->options["EPS"].data.c_str());
	classificationTimeout = atoi(NovaConfig->options["CLASSIFICATION_TIMEOUT"].data.c_str());
	isTraining = atoi(NovaConfig->options["IS_TRAINING"].data.c_str());
	classificationThreshold = atof(NovaConfig->options["CLASSIFICATION_THRESHOLD"].data.c_str());
	dataFile = NovaConfig->options["DATAFILE"].data;
	SA_Max_Attempts = atoi(NovaConfig->options["SA_MAX_ATTEMPTS"].data.c_str());
	SA_Sleep_Duration = atof(NovaConfig->options["SA_SLEEP_DURATION"].data.c_str());
}
