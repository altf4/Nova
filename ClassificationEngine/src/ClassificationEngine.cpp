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
#include <sys/ioctl.h>
#include <signal.h>
#include <net/if.h>
#include <sys/un.h>
#include <log4cxx/xml/domconfigurator.h>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

using namespace log4cxx;
using namespace log4cxx::xml;
using namespace std;
using namespace Nova;
using namespace ClassificationEngine;

static SuspectHashTable suspects;
pthread_rwlock_t lock;

//NOT normalized
vector <Point*> dataPtsWithClass;
bool isTraining = false;
static string hostAddrString;
static struct sockaddr_in hostAddr;

//Global variables related to Classification

//Configured in NOVAConfig_CE or specified config file.
string broadcastAddr;			//Silent Alarm destination IP address
int sAlarmPort;					//Silent Alarm destination port
int classificationTimeout;		//In seconds, how long to wait between classifications
int trainingTimeout;			//In seconds, how long to wait between training calculations
int maxFeatureVal;				//The value to normalize feature values to.
									//Higher value makes for more precision, but more cycles.

int k;							//number of nearest neighbors
double eps;						//error bound
int maxPts;						//maximum number of data points
double classificationThreshold = .5; //value of classification to define split between hostile / benign
//End configured variables
const int dim = DIM;					//dimension

int nPts = 0;						//actual number of data points
ANNpointArray dataPts;				//data points
ANNpointArray normalizedDataPts;	//normalized data points
string dataFile;					//input for data points
istream* queryIn = NULL;			//input for query points

const char *outFile;				//output for data points during training

void *outPtr;

LoggerPtr m_logger(Logger::getLogger("main"));

int maxFeatureValues[dim];


void siginthandler(int param)
{
	//Append suspect data to the datafile before exit
	//WriteDataPointsToFile((char*)outPtr);
	exit(1);
}

int main(int argc,char *argv[])
{
	int c, IPCsock, len;

	pthread_rwlock_init(&lock, NULL);

	//Path name variable for config file, set to a default
	char* nConfig = (char*)"Config/NOVAConfig_CE.txt";
	string line; //used for input checking

	struct sockaddr_un localIPCAddress;

	pthread_t classificationLoopThread;
	pthread_t trainingLoopThread;
	pthread_t silentAlarmListenThread;

	while((c = getopt (argc,argv,":n:l:")) != -1)
	{
		switch(c)
		{

			//"NOVA Config"
			case 'n':
				if(optarg != NULL)
				{
					line = string(optarg);
					if(line.size() > 4 && !line.substr(line.size()-4, line.size()).compare(".txt"))
					{
						nConfig = (char *)optarg;
					}
				}
				else
				{
					cerr << "Bad Input File Path" << endl;
					cout << Usage();
					exit(1);
				}
				break;

			//Log config file
			case 'l':
				if(optarg != NULL)
				{
					line = string(optarg);
					if(line.size() > 4 && !line.substr(line.size()-4, line.size()).compare(".xml"))
					{
						DOMConfigurator::configure(optarg);
					}
				}
				else
				{
					cerr << "Bad Output File Path" << endl;
					cout << Usage();
					exit(1);
				}
				break;

			case '?':
				cerr << "You entered an unrecognized input flag: " << (char)optopt << endl;
				cout << Usage();
				exit(1);
				break;

			case ':':
				cerr << "You're missing an argument after the flag: " << (char)optopt << endl;
				cout << Usage();
				exit(1);
				break;

			default:
			{
				cerr << "Sorry, I didn't recognize the option: " << (char)c << endl;
				cout << Usage();
				exit(1);
			}
		}
	}

	//Runs the configuration loader
	LoadConfig(nConfig);
	outPtr = (void *)outFile;

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
	ofstream key(KEY_FILENAME);
	key.close();
	strcpy(localIPCAddress.sun_path, KEY_FILENAME);
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
		TrafficEvent *event = new TrafficEvent();
		if( ReceiveTrafficEvent(IPCsock,TRAFFIC_EVENT_MTYPE,event) == false)
		{
			delete event;
			event = NULL;
			continue;
		}

		//If this is a new Suspect
		if(suspects.count(event->src_IP.s_addr) == 0)
		{
			pthread_rwlock_wrlock(&lock);
			suspects[event->src_IP.s_addr] = new Suspect(event);
			pthread_rwlock_unlock(&lock);
		}

		//A returning suspect
		else
		{
			pthread_rwlock_wrlock(&lock);
			suspects[event->src_IP.s_addr]->AddEvidence(event);
			pthread_rwlock_unlock(&lock);
		}
	}

	//Shouldn't get here!
	LOG4CXX_ERROR(m_logger,"Main thread ended. Shouldn't get here!!!");
	close(IPCsock);
	return 1;
}

//Separate thread which infinite loops, periodically updating all the classifications
//	for all the current suspects
void *Nova::ClassificationEngine::ClassificationLoop(void *ptr)
{
	int oldClassification;
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
		pthread_rwlock_unlock(&lock);
		pthread_rwlock_wrlock(&lock);
		//Calculate the normalized feature sets, actually used by ANN
		//	Writes into Suspect ANNPoints
		NormalizeDataPoints(maxFeatureVal);
		pthread_rwlock_unlock(&lock);
		pthread_rwlock_rdlock(&lock);
		//Perform classification on each suspect
		for (SuspectHashTable::iterator it = suspects.begin() ; it != suspects.end(); it++)
		{
			if(it->second->needs_classification_update)
			{
				pthread_rwlock_unlock(&lock);
				pthread_rwlock_wrlock(&lock);

				oldClassification = it->second->isHostile;

				Classify(it->second);
				cout << it->second->ToString();

				//If the hostility changed
				if( oldClassification != it->second->isHostile)
				{
					SilentAlarm(it->second);
				}

				SendToUI(it->second);


				pthread_rwlock_unlock(&lock);
				pthread_rwlock_rdlock(&lock);
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
	signal(SIGINT, siginthandler);

	//Training Loop
	while(true)
	{
		sleep(trainingTimeout);
		pthread_rwlock_wrlock(&lock);
		ofstream myfile (string(outFile).data(), ios::app);
		if (myfile.is_open())
		{
			//Calculate the "true" Feature Set for each Suspect
			for (SuspectHashTable::iterator it = suspects.begin() ; it != suspects.end(); it++)
			{
				if(it->second->needs_feature_update)
				{
					it->second->CalculateFeatures(isTraining);
					if( it->second->annPoint == NULL)
					{
						it->second->annPoint = annAllocPt(DIMENSION);
					}
					for(int j=0; j < dim; j++)
					{
						it->second->annPoint[j] = it->second->features->features[j];
						myfile << it->second->annPoint[j] << " ";
					}
					myfile << it->second->classification;
					myfile << "\n";
					cout << it->second->ToString();
					SendToUI(it->second);
				}
			}
			//	Writes into Suspect ANNPoints

			//WriteDataPointsToFile((char*)outPtr);
		}
		else LOG4CXX_INFO(m_logger, "Unable to open file.");
		myfile.close();
		pthread_rwlock_unlock(&lock);
	}
	//Shouldn't get here!
	LOG4CXX_ERROR(m_logger,"Training thread ended. Shouldn't get here!!!");
	return NULL;
}

//Thread for listening for Silent Alarms from other Nova instances
void *Nova::ClassificationEngine::SilentAlarmLoop(void *ptr)
{
	int sockfd;
	char buf[MAX_MSG_SIZE];
	struct sockaddr_in sendaddr;

	int numbytes;
	int addr_len;
	int broadcast=1;

	if((sockfd = socket(PF_INET,SOCK_DGRAM,0)) == -1)
	{
		LOG4CXX_ERROR(m_logger, "socket: " << strerror(errno));
		close(sockfd);
		exit(1);
	}

	if(setsockopt(sockfd,SOL_SOCKET,SO_BROADCAST,&broadcast,sizeof broadcast) == -1)
	{
		LOG4CXX_ERROR(m_logger, "setsockopt - SO_SOCKET: " << strerror(errno));
		close(sockfd);
		exit(1);
	}

	sendaddr.sin_family = AF_INET;
	sendaddr.sin_port = htons(sAlarmPort);
	sendaddr.sin_addr.s_addr = INADDR_ANY;
	memset(sendaddr.sin_zero,'\0', sizeof sendaddr.sin_zero);

	if(bind(sockfd,(struct sockaddr*) &sendaddr,sizeof sendaddr) == -1)
	{
		LOG4CXX_ERROR(m_logger, "bind: " << strerror(errno));
		close(sockfd);
		exit(1);
	}

	addr_len = sizeof sendaddr;

	while(1)
	{
		//Do the actual "listen"
		if ((numbytes = recvfrom(sockfd,buf,sizeof buf,0,(struct sockaddr *)&sendaddr,(socklen_t *)&addr_len)) == -1)
		{
			LOG4CXX_ERROR(m_logger, "recvfrom: " << strerror(errno));
			close(sockfd);
			exit(1);
		}

		//If this is from ourselves, then drop it.
		if(hostAddr.sin_addr.s_addr == sendaddr.sin_addr.s_addr)
		{
			continue;
		}

		Suspect *suspect = new Suspect();

		try
		{
			// create and open an archive for input
			stringstream ss;
			ss << buf;
			boost::archive::text_iarchive ia(ss);

			// read class state from archive
			ia >> suspect;
			LOG4CXX_INFO(m_logger,"Received a Silent Alarm!\n" << suspect->ToString());
			pthread_rwlock_rdlock(&lock);

			for (SuspectHashTable::iterator it = suspects.begin();it != suspects.end();it++)
			{
				if(it->second->IP_address.s_addr == suspect->IP_address.s_addr)
				{
					pthread_rwlock_unlock(&lock);
					pthread_rwlock_wrlock(&lock);
					it->second->flaggedByAlarm = true;
					pthread_rwlock_unlock(&lock);
					pthread_rwlock_rdlock(&lock);
					break;
				}
			}
			pthread_rwlock_unlock(&lock);
		}
		catch(boost::archive::archive_exception e)
		{
			close(sockfd);
			LOG4CXX_INFO(m_logger,"Error interpreting received Silent Alarm: " << string(e.what()));
		}
		delete suspect;
	}
	close(sockfd);
	LOG4CXX_INFO(m_logger,"Silent Alarm thread ended. Shouldn't get here!!!");
	return NULL;
}

//Performs classification on given suspect
//Where all the magic takes place
void Nova::ClassificationEngine::Classify(Suspect *suspect)
{
	ANNpoint			queryPt;				// query point
	ANNidxArray			nnIdx;					// near neighbor indices
	ANNdistArray		dists;					// near neighbor distances
	ANNkd_tree*			kdTree;					// search structure

	queryPt = suspect->annPoint;
	nnIdx = new ANNidx[k];						// allocate near neigh indices
	dists = new ANNdist[k];						// allocate near neighbor dists

	kdTree = new ANNkd_tree(					// build search structure
			normalizedDataPts,					// the data points
					nPts,						// number of points
					dim);						// dimension of space

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

		//If Hostile
		if( dataPtsWithClass[ nnIdx[i] ]->classification == 1)
		{
			classifyCount += (sqrtDIM - dists[i]);
		}
		//If benign
		else if( dataPtsWithClass[ nnIdx[i] ]->classification == 0)
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
		    delete kdTree;
		    annClose();
		    return;
		}
	}

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
    delete kdTree;

    annClose();
	suspect->needs_classification_update = false;
}

//Subroutine to copy the data points in 'suspects' to their respective ANN Points
void Nova::ClassificationEngine::CopyDataToAnnPoints()
{
	//Set the ANN point for each Suspect
	for (SuspectHashTable::iterator it = suspects.begin();it != suspects.end();it++)
	{
		//Create the ANN Point, if needed

	}
}

//Calculates normalized data points and stores into 'normalizedDataPts'
void Nova::ClassificationEngine::NormalizeDataPoints(int maxVal)
{
	//Find the max values for each feature
	for (SuspectHashTable::iterator it = suspects.begin();it != suspects.end();it++)
	{
		for(int i = 0;i < dim;i++)
		{
			if(it->second->features->features[i] > maxFeatureValues[i])
			{
				maxFeatureValues[i] = it->second->features->features[i];
			}
		}
	}
	//Normalize the suspect points
	for (SuspectHashTable::iterator it = suspects.begin();it != suspects.end();it++)
	{
		//Create the ANN Point, if needed
		if(it->second->annPoint == NULL)
		{
			it->second->annPoint = annAllocPt(DIMENSION);
		}

		//If the max is 0, then there's no need to normalize! (Plus it'd be a div by zero)
		for(int i = 0;i < dim;i++)
		{
			if(maxFeatureValues[0] != 0)
			{
				it->second->annPoint[i] = (double)(it->second->features->features[i] / maxFeatureValues[i]) * maxVal;
			}
			else
			{
				LOG4CXX_INFO(m_logger,"Max Feature Value for feature " << (i+1) << " is 0!");
			}
		}
	}

	//Normalize the data points
	//Foreach data point
	for(int j = 0;j < dim;j++)
	{
		//Foreach feature within the data point
		for(int i=0;i < nPts;i++)
		{
			if(maxFeatureValues[j] != 0)
			{
				normalizedDataPts[i][j] = (double)((dataPts[i][j] / maxFeatureValues[j]) * maxVal);
			}
			else
			{
				LOG4CXX_INFO(m_logger,"Max Feature Value for feature " << (i+1) << " is 0!");
				break;
			}
		}
	}
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

	dataPts = annAllocPts(maxPts, dim);			// allocate data points
	normalizedDataPts = annAllocPts(maxPts, dim);

	if (myfile.is_open())
	{
		int i = 0;

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
}

//Writes dataPtsWithClass out to a file specified by outFilePath
void Nova::ClassificationEngine::WriteDataPointsToFile(string outFilePath)
{
	ofstream myfile (outFilePath.data(), ios::app);

	if (myfile.is_open())
	{
		for ( SuspectHashTable::iterator it = suspects.begin();it != suspects.end();it++ )
		{
			for(int j=0; j < dim; j++)
			{
				myfile << it->second->annPoint[j] << " ";
			}
			myfile << it->second->classification;
			myfile << "\n";
		}
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
	usageString += "\t-n: Path to NOVA config txt file. (Config/NOVAConfig_CE.txt by default)\n";
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
    stringstream ss;
	boost::archive::text_oarchive oa(ss);

	int socketFD, len;
	struct sockaddr_un remote;

	//Serialize the data into a simple char buffer
	oa << suspect;
	string temp = ss.str();

	const char* data = temp.c_str();
	int dataLen = temp.size();

	if ((socketFD = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		LOG4CXX_ERROR(m_logger, "socket: " << strerror(errno));
		close(socketFD);
		return;
	}

	remote.sun_family = AF_UNIX;
	ofstream key(KEY_ALARM_FILENAME);
	key.close();
	strcpy(remote.sun_path, KEY_ALARM_FILENAME);
	len = strlen(remote.sun_path) + sizeof(remote.sun_family);

	if (connect(socketFD, (struct sockaddr *)&remote, len) == -1)
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

	//Send Silent Alarm to other Nova Instances
	int sockfd, broadcast = 1;
	struct sockaddr_in serv_addr;

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(sAlarmPort);
	serv_addr.sin_addr.s_addr = INADDR_BROADCAST;

	sockfd = socket(AF_INET,SOCK_DGRAM,0);

	if (sockfd < 0)
	{
		LOG4CXX_INFO(m_logger,"ERROR opening socket: " << strerror(errno));
		LOG4CXX_ERROR(m_logger, "socket: " << strerror(errno));
		close(sockfd);
		return;
	}

	if((setsockopt(sockfd,SOL_SOCKET,SO_BROADCAST,&broadcast,sizeof broadcast)) == -1)
	{
		LOG4CXX_ERROR(m_logger, "setsockopt - SO_SOCKET: " << strerror(errno));
		close(sockfd);
		return;
	}

	if( sendto(sockfd,data,dataLen,0,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) == -1)
	{
		LOG4CXX_ERROR(m_logger,"Error in UDP Send: " << strerror(errno));
		close(sockfd);
		return;
	}
}

///Receive a TrafficEvent from another local component.
/// This is a blocking function. If nothing is received, then wait on this thread for an answer
bool ClassificationEngine::ReceiveTrafficEvent(int socket, long msg_type, TrafficEvent *event)
{
	struct sockaddr_un remote;
    int socketSize, connectionSocket;
    int bytesRead;
    char buffer[MAX_MSG_SIZE];

    socketSize = sizeof(remote);

    //Blocking call
    if ((connectionSocket = accept(socket, (struct sockaddr *)&remote, (socklen_t*)&socketSize)) == -1)
    {
		LOG4CXX_ERROR(m_logger,"accept: " << strerror(errno));
		close(connectionSocket);
        return false;
    }
    if((bytesRead = recv(connectionSocket, buffer, MAX_MSG_SIZE, 0 )) == -1)
    {
		LOG4CXX_ERROR(m_logger,"recv: " << strerror(errno));
		close(connectionSocket);
        return NULL;
    }

	stringstream ss;
	ss << buffer;

	boost::archive::text_iarchive ia(ss);
	// Read into a temp object then copy. We do this b/c the ">>" operator for
	//	boost serialization creates a new object. So if we try to apply it to
	//	the "event" variable, it just clobbers the pointer and doesn't get
	//	seen

	TrafficEvent *tempEvent;

	try
	{
		ia >> tempEvent;
	}
	catch(boost::archive::archive_exception e)
	{
		LOG4CXX_ERROR(m_logger,"Error in parsing received TrafficEvent: " << string(e.what()));
		close(connectionSocket);
		return false;
	}
	tempEvent->copyTo(event);
	close(connectionSocket);
	return true;
}

//Send a silent alarm about the argument suspect
void Nova::ClassificationEngine::SendToUI(Suspect *suspect)
{

    stringstream ss;
	boost::archive::text_oarchive oa(ss);

	int socketFD, len;
	struct sockaddr_un remote;

	//Serialize the data into a simple char buffer
	oa << suspect;
	string temp = ss.str();

	const char* data = temp.c_str();
	int dataLen = temp.size();

	if ((socketFD = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		LOG4CXX_ERROR(m_logger,"socket: " << strerror(errno));
		close(socketFD);
		return;
	}

	remote.sun_family = AF_UNIX;
	strcpy(remote.sun_path, CE_FILENAME);
	len = strlen(remote.sun_path) + sizeof(remote.sun_family);

	if (connect(socketFD, (struct sockaddr *)&remote, len) == -1)
	{
		LOG4CXX_ERROR(m_logger,"connect: " << strerror(errno));
		close(socketFD);
		return;
	}

	if (send(socketFD, data, dataLen, 0) == -1)
	{
		LOG4CXX_ERROR(m_logger,"send: " << strerror(errno));
		close(socketFD);
		return;
	}
	close(socketFD);
}


void ClassificationEngine::LoadConfig(char* input)
{
	//Used to verify all values have been loaded
	bool verify[CONFIG_FILE_LINE_COUNT];
	for(uint i = 0; i < CONFIG_FILE_LINE_COUNT; i++)
		verify[i] = false;

	string line;
	string prefix;
	ifstream config(input);

	if(config.is_open())
	{
		while(config.good())
		{
			getline(config,line);

			prefix = "INTERFACE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				if(line.size() > 0)
				{
					hostAddrString = getLocalIP(line.c_str());
					if(hostAddrString.size() == 0)
					{
						LOG4CXX_ERROR(m_logger, "Bad interface, no IP's associated!");
						exit(1);
					}

					inet_pton(AF_INET,hostAddrString.c_str(),&(hostAddr.sin_addr));
					verify[0]=true;
				}
				continue;

			}

			prefix = "DATAFILE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				if(line.size() > 4 && !line.substr(line.size()-4, line.size()).compare(".txt"))
				{
					dataFile = line;
					outFile = line.c_str();
					verify[1]=true;
				}
				continue;
			}

			prefix = "BROADCAST_ADDR";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				if(line.size() > 6 && line.size() <  16)
				{
					broadcastAddr = line;
					verify[3]=true;
				}
				continue;
			}

			prefix = "SILENT_ALARM_PORT";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				if(atoi(line.c_str()) > 0)
				{
					sAlarmPort = atoi(line.c_str());
					verify[4]=true;
				}
				continue;
			}

			prefix = "K";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				if(atoi(line.c_str()) > 0)
				{
					k = atoi(line.c_str());
					verify[5]=true;
				}
				continue;
			}

			prefix = "EPS";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				if(atof(line.c_str()) >= 0)
				{
					eps = atof(line.c_str());
					verify[6]=true;
				}
				continue;
			}

			prefix = "MAX_PTS";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				if(atoi(line.c_str()) > 0)
				{
					maxPts = atoi(line.c_str());
					verify[7]=true;
				}
				continue;
			}

			prefix = "CLASSIFICATION_TIMEOUT";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				if(atoi(line.c_str()) > 0)
				{
					classificationTimeout = atoi(line.c_str());
					verify[8]=true;
				}
				continue;
			}

			prefix = "TRAINING_TIMEOUT";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				if(atoi(line.c_str()) > 0)
				{
					trainingTimeout = atoi(line.c_str());
					verify[9]=true;
				}
				continue;
			}

			prefix = "MAX_FEATURE_VALUE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				if(atoi(line.c_str()) > 0)
				{
					maxFeatureVal = atoi(line.c_str());
					verify[10]=true;
				}
				continue;
			}

			prefix = "IS_TRAINING";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				if(atoi(line.c_str()) == 0 || atoi(line.c_str()) == 1)
				{
					isTraining = atoi(line.c_str());
					verify[11]=true;
				}
				continue;
			}

			prefix = "CLASSIFICATION_THRESHOLD";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				if(atof(line.c_str()) >= 0)
				{
					classificationThreshold = atof(line.c_str());
					verify[12]=true;
				}
				continue;
			}


			prefix = "#";
			if(line.substr(0,prefix.size()).compare(prefix) && line.compare(""))
			{
				LOG4CXX_INFO(m_logger, "Unexpected entry in NOVA configuration file.");
				continue;
			}
		}

		//Checks to make sure all values have been set.
		bool v = true;
		for(uint i = 0; i < CONFIG_FILE_LINE_COUNT; i++)
		{
			v &= verify[i];
		}

		if(v == false)
		{
			LOG4CXX_ERROR(m_logger, "One or more values have not been set.");
			exit(1);
		}
		else
		{
			LOG4CXX_INFO(m_logger, "Config loaded successfully.");
		}
	}
	else
	{
		LOG4CXX_INFO(m_logger, "No configuration file detected.");
		exit(1);
	}
	config.close();
}
