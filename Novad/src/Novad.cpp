//============================================================================
// Name        : Novad.cpp
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
// Description : Nova Daemon to perform network anti-reconnaissance
//============================================================================

#include "HoneydConfiguration/HoneydConfiguration.h"
#include "messaging/MessageManager.h"
#include "WhitelistConfiguration.h"
#include "InterfacePacketCapture.h"
#include "ClassificationAggregator.h"
#include "HaystackControl.h"
#include "FilePacketCapture.h"
#include "ProtocolHandler.h"
#include "PacketCapture.h"
#include "EvidenceTable.h"
#include "Doppelganger.h"
#include "SuspectTable.h"
#include "FeatureSet.h"
#include "NovaUtil.h"
#include "Database.h"
#include "Threads.h"
#include "Control.h"
#include "Config.h"
#include "Logger.h"
#include "Point.h"
#include "Novad.h"
#include "Lock.h"

#include <vector>
#include <math.h>
#include <time.h>
#include <errno.h>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/un.h>
#include <signal.h>
#include <iostream>
#include <ifaddrs.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include "event2/thread.h"
#include <netinet/if_ether.h>

#define BOOST_FILESYSTEM_VERSION 2
#include <boost/filesystem.hpp>

using namespace std;
using namespace Nova;

// Maintains a list of suspects and information on network activity
SuspectTable suspects;
// Suspects not yet written to the state file
SuspectTable suspectsSinceLastSave;
//Contains packet evidence yet to be included in a suspect
EvidenceTable suspectEvidence;

pthread_mutex_t packetCapturesLock;
vector<PacketCapture*> packetCaptures;
vector<int> dropCounts;

Doppelganger *doppel;

// Timestamps for the CE state file exiration of data
time_t lastLoadTime;
time_t lastSaveTime;

// Time novad started, used for uptime and pcap capture names
time_t startTime;

Database *db;


//HS Vars
string dhcpListFile;
vector<string> haystackAddresses;
vector<string> haystackDhcpAddresses;
vector<string> whitelistIpAddresses;
vector<string> whitelistIpRanges;

int honeydDHCPNotifyFd;
int honeydDHCPWatch;

int whitelistNotifyFd;
int whitelistWatch;

ClassificationEngine *engine = NULL;

pthread_t classificationLoopThread;
pthread_t ipUpdateThread;
pthread_t ipWhitelistUpdateThread;
pthread_t consumer;

pthread_mutex_t shutdownClassificationMutex;
bool shutdownClassification;
pthread_cond_t shutdownClassificationCond;

namespace Nova
{

void StartServer()
{
	int len;
	string inKeyPath = Config::Inst()->GetPathHome() + "/config/keys" + NOVAD_LISTEN_FILENAME;
	evutil_socket_t IPCParentSocket;
	struct event_base *base;
	struct event *listener_event;
	struct sockaddr_un msgLocal;

	evthread_use_pthreads();
	base = event_base_new();
	if (!base)
	{
		LOG(ERROR, "Failed to set up socket base", "");
		return;
	}

	if((IPCParentSocket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		LOG(ERROR, "Failed to create socket for accept()", "socket: "+string(strerror(errno)));
		return;
	}

	evutil_make_socket_nonblocking(IPCParentSocket);

	msgLocal.sun_family = AF_UNIX;
	memset(msgLocal.sun_path, '\0', sizeof(msgLocal.sun_path));
	strncpy(msgLocal.sun_path, inKeyPath.c_str(), inKeyPath.length());
	unlink(msgLocal.sun_path);
	len = strlen(msgLocal.sun_path) + sizeof(msgLocal.sun_family);

	if(::bind(IPCParentSocket, (struct sockaddr *)&msgLocal, len) == -1)
	{
		LOG(ERROR, "Failed to bind to socket", "bind: "+string(strerror(errno)));
		close(IPCParentSocket);
		return;
	}

	if(listen(IPCParentSocket, SOMAXCONN) == -1)
	{
		LOG(ERROR, "Failed to listen for UIs", "listen: "+string(strerror(errno)));
		close(IPCParentSocket);
		return;
	}

	listener_event = event_new(base, IPCParentSocket, EV_READ|EV_PERSIST, MessageManager::DoAccept, (void*)base);
	event_add(listener_event, NULL);

	//Start our worker threads
	for(int i = 0; i < Config::Inst()->GetNumMessageWorkerThreads(); i++)
	{
		pthread_t workerThread;
		pthread_create(&workerThread, NULL, MessageWorker, NULL);
		pthread_detach(workerThread);
	}

	event_base_dispatch(base);

	LOG(ERROR, "Main accept dispatcher returned. This should not occur.", "");
	return;
}

int RunNovaD()
{
	dhcpListFile = Config::Inst()->GetIpListPath();
	Logger::Inst();
	HoneydConfiguration::Inst();

	if(!LockNovad())
	{
		exit(EXIT_FAILURE);
	}

	// Change our working folder into the config folder so our relative paths are correct
	if(chdir(Config::Inst()->GetPathHome().c_str()) == -1)
	{
		LOG(INFO, "Failed to change directory to " + Config::Inst()->GetPathHome(),"");
	}

	pthread_mutex_init(&packetCapturesLock, NULL);

	db = new Database();
	try
	{
		db->Connect();
	} catch (Nova::DatabaseException &e)
	{
		LOG(ERROR, "Unable to connect to SQL database. " + string(e.what()), "");
	}

	lastLoadTime = lastSaveTime = startTime = time(NULL);
	if(lastLoadTime == ((time_t)-1))
	{
		LOG(ERROR, "Unable to get timestamp, call to time() failed", "");
	}

	//Need to load the configuration before making the Classification Engine for setting up the DM
	//Reload requires a CE object so we do a partial config load here.
	//Loads the configuration file
	Config::Inst()->LoadConfig();

	LOG(ALERT, "Starting NOVA version " + Config::Inst()->GetVersionString(), "");

	doppel = new Doppelganger(suspects);
	doppel->InitDoppelganger();

	engine = new ClassificationAggregator();

	// Set up our signal handlers
	signal(SIGKILL, SaveAndExit);
	signal(SIGINT, SaveAndExit);
	signal(SIGTERM, SaveAndExit);
	signal(SIGPIPE, SIG_IGN);

	haystackAddresses = Config::GetHaystackAddresses(Config::Inst()->GetPathConfigHoneydHS());
	haystackDhcpAddresses = Config::GetHoneydIpAddresses(dhcpListFile);
	whitelistIpAddresses = WhitelistConfiguration::GetIps();
	whitelistIpRanges = WhitelistConfiguration::GetIpRanges();
	UpdateHaystackFeatures();

	LoadStateFile();

	whitelistNotifyFd = inotify_init();
	if(whitelistNotifyFd > 0)
	{
		whitelistWatch = inotify_add_watch(whitelistNotifyFd, Config::Inst()->GetPathWhitelistFile().c_str(), IN_CLOSE_WRITE | IN_MOVED_TO | IN_MODIFY | IN_DELETE);
		pthread_create(&ipWhitelistUpdateThread, NULL, UpdateWhitelistIPFilter, NULL);
		pthread_detach(ipWhitelistUpdateThread);
	}
	else
	{
		LOG(ERROR, "Unable to set up file watcher for the Whitelist IP file.","");
	}

	// If we're not reading from a pcap, monitor for IP changes in the honeyd file
	if(!Config::Inst()->GetReadPcap())
	{
		honeydDHCPNotifyFd = inotify_init();

		if(honeydDHCPNotifyFd > 0)
		{
			honeydDHCPWatch = inotify_add_watch (honeydDHCPNotifyFd, dhcpListFile.c_str(), IN_CLOSE_WRITE | IN_MOVED_TO | IN_MODIFY | IN_DELETE);
			pthread_create(&ipUpdateThread, NULL, UpdateIPFilter,NULL);
			pthread_detach(ipUpdateThread);
		}
		else
		{
			LOG(ERROR, "Unable to set up file watcher for the honeyd IP list file.","");
		}
	}

	pthread_mutex_init(&shutdownClassificationMutex, NULL);
	shutdownClassification = false;
	pthread_cond_init(&shutdownClassificationCond, NULL);
	pthread_create(&classificationLoopThread,NULL,ClassificationLoop, NULL);
	pthread_detach(classificationLoopThread);

	// TODO: Figure out if having multiple Consumer Loops has a performance benefit
	pthread_create(&consumer, NULL, ConsumerLoop, NULL);
	pthread_detach(consumer);

	StartCapture();

	//Go into the main accept() loop
	StartServer();

	return EXIT_FAILURE;
}

bool LockNovad()
{
	int lockFile = open((Config::Inst()->GetPathHome() + "/data/novad.lock").data(), O_CREAT | O_RDWR, 0666);
	int rc = flock(lockFile, LOCK_EX | LOCK_NB);
	if(rc != 0)
	{
		if(errno == EAGAIN)
		{
			cerr << "ERROR: Novad is already running. Please close all other instances before continuing." << endl;
		}
		else
		{
			cerr << "ERROR: Unable to open the novad.lock file. This could be due to bad file permissions on it. Error was: " << strerror(errno) << endl;
		}
		return false;
	}
	else
	{
		// We have the file open, it will be released if the process dies for any reason
		return true;
	}
}

void MaskKillSignals()
{
	sigset_t x;
	sigemptyset (&x);
	sigaddset(&x, SIGKILL);
	sigaddset(&x, SIGTERM);
	sigaddset(&x, SIGINT);
	sigaddset(&x, SIGPIPE);
	sigprocmask(SIG_BLOCK, &x, NULL);
}

//Appends the evidence gathered on suspects since the last save to the statefile
void AppendToStateFile()
{
	//Store the time we started the save for ref later.
	lastSaveTime = time(NULL);
	//if time(NULL) failed
	if(lastSaveTime == ~0)
	{
		LOG(ERROR, "Problem with CE State File", "Unable to get timestamp, call to time() failed");
	}

	//If there are no suspects to save
	if(suspectsSinceLastSave.Size() <= 0)
	{
		return;
	}

	//Open the output file
	ofstream out(Config::Inst()->GetPathCESaveFile().data(), ofstream::binary | ofstream::app);

	//Update the feature set and dump the contents of the table to the output file
	uint32_t dataSize;
	dataSize = suspectsSinceLastSave.DumpContents(&out, lastSaveTime);

	//Check that the dump was successful, print the results as a debug message.
	stringstream ss;
	if(dataSize)
	{
		ss << "Appending " << dataSize << " bytes to the CE state file at " << Config::Inst()->GetPathCESaveFile();
	}
	else
	{
		ss << "Unable to write any Suspects to the state file. func: 'DumpContents' returned 0.";
	}
	LOG(DEBUG,ss.str(),"");
	out.close();
	//Clear the table;
	suspectsSinceLastSave.EraseAllSuspects();
}

//Loads the statefile by reading table state dumps that haven't expired yet.
void LoadStateFile()
{
	lastLoadTime = time(NULL);
	if(lastLoadTime == ((time_t)-1))
	{
		LOG(ERROR, "Problem with CE State File", "Unable to get timestamp, call to time() failed");
	}

	// Open input file
	ifstream in(Config::Inst()->GetPathCESaveFile().data(), ios::binary | ios::in);
	if(!in.is_open())
	{
		LOG(WARNING, "Unable to open CE state file.", "");
		return;
	}

	// get length of input for error checking of partially written files
	in.seekg (0, ios::end);
	uint lengthLeft = in.tellg();
	in.seekg (0, ios::beg);
	uint expirationTime = lastLoadTime - Config::Inst()->GetDataTTL();
	if(Config::Inst()->GetDataTTL() == 0)
	{
		expirationTime = 0;
	}
	uint numBytes = 0;
	while(in.is_open() && !in.eof() && lengthLeft)
	{
		numBytes = suspects.ReadContents(&in, expirationTime);
		if(numBytes == 0)
		{

			in.close();

			// Back up the possibly corrupt state file
			int suffix = 0;
			string stateFileBackup = Config::Inst()->GetPathCESaveFile() + ".Corrupt";
			struct stat st;

			// Find an unused filename to move it to
			string fileName;
			do {
				suffix++;
				stringstream ss;
				ss << stateFileBackup;
				ss << suffix;
				fileName = ss.str();
			} while(stat(fileName.c_str(),&st) != -1);

			LOG(WARNING, "Backing up possibly corrupt state file to file " + fileName, "");

			// Copy the file
			boost::filesystem::path CESaveFile = Config::Inst()->GetPathCESaveFile();
			boost::filesystem::path target = fileName;

			cout << "CESaveFile " << CESaveFile.string() << endl;
			cout << "filename " << target.string() << endl;

			try
			{
				boost::filesystem::rename(CESaveFile, target);
			}
			catch(boost::filesystem::filesystem_error const& e)
			{
				LOG(ERROR, "There was a problem when attempting to move the corrupt state file.", "");
			}

			// Recreate an empty file
			stringstream touchCommand;
			touchCommand << "touch \"" << Config::Inst()->GetPathCESaveFile() << "\"";
			if(system(touchCommand.str().c_str()) == -1) {
				LOG(ERROR, "There was a problem when attempting to recreate the state file. System call to 'touch' failed:" + touchCommand.str(), "");
			}

			break;
		}
		lengthLeft -= numBytes;
	}
	in.close();
}

//Refreshes the state file by parsing the contents and writing un-expired state dumps to the new state file
void RefreshStateFile()
{
	//Variable assignments
	uint32_t dataSize = 0;
	vector<in_addr_t> deletedKeys;
	uint bytesRead = 0, lengthLeft = 0;
	time_t saveTime = 0;

	string ceFile = Config::Inst()->GetPathCESaveFile();
	string tmpFile = Config::Inst()->GetPathCESaveFile() + ".tmp";

	boost::filesystem::path from = ceFile;
	boost::filesystem::path to = tmpFile;

	lastLoadTime = time(NULL);
	time_t timestamp = lastLoadTime - Config::Inst()->GetDataTTL();

	//Check that we have a valid timestamp for the last load time.
	if(lastLoadTime == (~0)) // == -1 in signed vals
	{
		LOG(ERROR, "Problem with CE State File", "Unable to get timestamp, call to time() failed");
	}
	try
	{
		boost::filesystem::copy_file(from, to, boost::filesystem::copy_option::overwrite_if_exists);
	}
	catch(boost::filesystem::filesystem_error const& e)
	{
		LOG(ERROR, "Unable to refresh CE State File.",
				string("Unable to refresh CE State File: " + string(e.what())));
		return;
	}
	// Open input file
	ifstream in(tmpFile.data(), ios::binary | ios::in);
	if(!in.is_open())
	{
		LOG(ERROR,"Problem with CE State File",
			"Unable to open the CE state file at "+Config::Inst()->GetPathCESaveFile());
		return;
	}

	ofstream out(ceFile.data(), ios::binary | ios::trunc);
	if(!out.is_open())
	{
		LOG(ERROR, "Problem with CE State File", "Unable to open the temporary CE state file.");
		in.close();
		return;
	}

	//Get some variables about the input size for error checking.
	in.seekg (0, ios::end);
	lengthLeft = in.tellg();
	in.seekg (0, ios::beg);

	while(in.is_open() && !in.eof() && lengthLeft)
	{
		//If we can get the timestamp and data size
		if(lengthLeft < (sizeof timestamp + sizeof dataSize))
		{
			LOG(ERROR, "The CE state file may be corruput", "");
			return;
		}

		//Read the timestamp and dataSize
		in.read((char*)&saveTime, sizeof saveTime);
		in.read((char*)&dataSize, sizeof dataSize);
		bytesRead = dataSize + sizeof dataSize + sizeof saveTime;

		//If the data has expired, skip past
		if(saveTime < timestamp)
		{
			//Print debug msg
			stringstream ss;
			ss << "Throwing out old CE state at time: " << saveTime << ".";
			LOG(DEBUG,"Throwing out old CE state.", ss.str());

			//Skip past expired data
			in.seekg(dataSize, ios::cur);
		}
		else
		{
			char buf[dataSize];
			in.read(buf, dataSize);
			out.write((char*)&saveTime, sizeof saveTime);
			out.write((char*)&dataSize, sizeof dataSize);
			out.write(buf, dataSize);
		}
		lengthLeft -= bytesRead;
	}
	in.close();
	out.close();
}

void Reload()
{
	// Reload the configuration file
	Config::Inst()->LoadConfig();
	engine->Reload();
	suspects.SetEveryoneNeedsClassificationUpdate();
}

void StartCapture()
{
	Lock lock(&packetCapturesLock);
	StopCapture_noLocking();

	Reload();

	//If we're reading from a packet capture file
	if(Config::Inst()->GetReadPcap())
	{
		try
		{
			LOG(DEBUG, "Loading pcap file", "");
			string pcapFilePath = Config::Inst()->GetPathPcapFile() + "/capture.pcap";
			string ipAddressFile = Config::Inst()->GetPathPcapFile() + "/localIps.txt";

			FilePacketCapture *cap = new FilePacketCapture(pcapFilePath.c_str());
			cap->Init();
			cap->SetPacketCb(&Packet_Handler);
			string captureFilterString = ConstructFilterString(cap->GetIdentifier());
			cap->SetFilter(captureFilterString);
			cap->SetIdIndex(packetCaptures.size());
			packetCaptures.push_back(cap);

			cap->StartCaptureBlocking();

			LOG(DEBUG, "Done reading pcap file. Processing...", "");
			ClassificationLoop(NULL);
			LOG(DEBUG, "Done processing pcap file.", "");
		}
		catch (Nova::PacketCaptureException &e)
		{
			LOG(CRITICAL, string("Unable to open pcap file for capture: ") + e.what(), "");
			exit(EXIT_FAILURE);
		}

		Config::Inst()->SetReadPcap(false); //If we are going to live capture set the flag.
	}

	if(!Config::Inst()->GetReadPcap())
	{
		vector<string> ifList = Config::Inst()->GetInterfaces();

		//trainingFileStream = pcap_dump_open(handles[0], trainingCapFile.c_str());

		stringstream temp;
		temp << ifList.size() << endl;

		for(uint i = 0; i < ifList.size(); i++)
		{
			dropCounts.push_back(0);
			InterfacePacketCapture *cap = new InterfacePacketCapture(ifList[i].c_str());

			try
			{
				cap->SetPacketCb(&Packet_Handler);
				cap->Init();
				string captureFilterString = ConstructFilterString(cap->GetIdentifier());
				cap->SetFilter(captureFilterString);
				cap->StartCapture();
				cap->SetIdIndex(packetCaptures.size());
				packetCaptures.push_back(cap);
			}
			catch (Nova::PacketCaptureException &e)
			{
				LOG(CRITICAL, string("Exception when starting packet capture on device " + ifList[i] + ": ") + e.what(), "");
				exit(EXIT_FAILURE);
			}
		}
	}
}

void StopCapture()
{
	Lock lock(&packetCapturesLock);
	StopCapture_noLocking();
}

void StopCapture_noLocking()
{
	for (uint i = 0; i < packetCaptures.size(); i++)
	{
		packetCaptures.at(i)->StopCapture();
		delete packetCaptures.at(i);
	}

	packetCaptures.clear();
}

void Packet_Handler(u_char *index,const struct pcap_pkthdr *pkthdr,const u_char *packet)
{
	if(packet == NULL)
	{
		LOG(ERROR, "Failed to capture packet!","");
		return;
	}

	switch(ntohs(*(uint16_t *)(packet+12)))
	{
		//IPv4, currently the only handled case
		case ETHERTYPE_IP:
		{

			//Prepare Packet structure
			Evidence *evidencePacket = new Evidence(packet + sizeof(struct ether_header), pkthdr);
			PacketCapture* cap = reinterpret_cast<PacketCapture*>(index);

			if (cap == NULL)
			{
				LOG(ERROR, "Packet capture object is NULL. Can't tell what interface this packet came from.", "");
				evidencePacket->m_evidencePacket.interface = "UNKNOWN";
			}
			else
			{
				evidencePacket->m_evidencePacket.interface = cap->GetIdentifier();
			}

			if (!Config::Inst()->GetReadPcap())
			{
				suspectEvidence.InsertEvidence(evidencePacket);
			}
			else
			{
				// If reading from pcap file no Consumer threads, so process the evidence right away
				suspectsSinceLastSave.ProcessEvidence(evidencePacket, true);
				suspects.ProcessEvidence(evidencePacket, false);
			}
			return;
		}
		//Ignore IPV6
		case ETHERTYPE_IPV6:
		{
			return;
		}
		//Ignore ARP
		case ETHERTYPE_ARP:
		{
			return;
		}
		default:
		{
			//stringstream ss;
			//ss << "Ignoring a packet with unhandled protocol #" << (uint16_t)(ntohs(((struct ether_header *)packet)->ether_type));
			//LOG(DEBUG, ss.str(), "");
			return;
		}
	}
}

//Convert monitored ip address into a csv string
string ConstructFilterString(string captureIdentifier)
{
	string filterString = "not src net 0.0.0.0";
	if(Config::Inst()->GetCustomPcapString() != "")
	{
		if(Config::Inst()->GetOverridePcapString())
		{
			filterString = Config::Inst()->GetCustomPcapString();
			LOG(DEBUG, "Pcap filter string is: '" + filterString + "'","");
			return filterString;
		}
		else
		{
			filterString += " && " + Config::Inst()->GetCustomPcapString();
		}
	}

	//Insert static haystack IP's
	vector<string> hsAddresses = haystackAddresses;
	while(hsAddresses.size())
	{
		//Remove and add the haystack host entry
		filterString += " && not src net ";
		filterString += hsAddresses.back();
		hsAddresses.pop_back();
	}

	// Whitelist the DHCP haystack node IP addresses
	hsAddresses = haystackDhcpAddresses;
	while(hsAddresses.size())
	{
		//Remove and add the haystack host entry
		filterString += " && not src net ";
		filterString += hsAddresses.back();
		hsAddresses.pop_back();
	}

	hsAddresses.clear();
	for (uint i = 0; i < whitelistIpAddresses.size(); i++)
	{
		if (WhitelistConfiguration::GetInterface(whitelistIpAddresses.at(i)) == captureIdentifier)
		{
			hsAddresses.push_back(whitelistIpAddresses.at(i));
		}
		else if (WhitelistConfiguration::GetInterface(whitelistIpAddresses.at(i)) == "All Interfaces")
		{
			hsAddresses.push_back(whitelistIpAddresses.at(i));
		}
	}
	while(hsAddresses.size())
	{
		//Remove and add the haystack host entry
		filterString += " && not src net ";
		filterString += WhitelistConfiguration::GetIp(hsAddresses.back());
		hsAddresses.pop_back();
	}

	hsAddresses.clear();
	for (uint i = 0; i < whitelistIpRanges.size(); i++)
	{
		if (WhitelistConfiguration::GetInterface(whitelistIpRanges.at(i)) == captureIdentifier)
		{
			hsAddresses.push_back(whitelistIpRanges.at(i));
		}
		else if (WhitelistConfiguration::GetInterface(whitelistIpRanges.at(i)) == "All Interfaces")
		{
			hsAddresses.push_back(whitelistIpRanges.at(i));
		}
	}
	while(hsAddresses.size())
	{
		filterString += " && not src net ";
		filterString += WhitelistConfiguration::GetIp(hsAddresses.back());
		filterString += " mask ";
		filterString += WhitelistConfiguration::GetSubnet(hsAddresses.back());
		hsAddresses.pop_back();
	}


	// Are we only classifying on honeypot traffic?
	if (Config::Inst()->GetOnlyClassifyHoneypotTraffic())
	{
		hsAddresses = haystackDhcpAddresses;
		hsAddresses.insert(hsAddresses.end(), haystackAddresses.begin(), haystackAddresses.end());
		filterString += " && (0 == 1";
		while(hsAddresses.size())
		{
			filterString += " || dst net ";
			filterString += hsAddresses.back();
			hsAddresses.pop_back();
		}
		filterString += ")";
	}

	LOG(DEBUG, "Pcap filter string is \"" + filterString + "\"","");
	return filterString;
}


void UpdateAndClassify(SuspectID_pb key)
{
	//Check for a valid suspect
	Suspect suspectCopy = suspects.GetShallowSuspect(key);
	if(suspects.IsEmptySuspect(&suspectCopy))
	{
		return;
	}

	//Get the old hostility bool
	bool oldIsHostile = suspectCopy.GetIsHostile();

	//Classify
	suspects.ClassifySuspect(key);

	//Check that we updated correctly
	suspectCopy = suspects.GetShallowSuspect(key);
	if(suspects.IsEmptySuspect(&suspectCopy))
	{
		return;
	}


	if(suspectCopy.GetIsHostile() && (!oldIsHostile || Config::Inst()->GetClearAfterHostile()))
	{
		try
		{
			db->InsertSuspectHostileAlert(&suspectCopy);
		} catch (Nova::DatabaseException &e)
		{
			LOG(ERROR, "Unable to insert hostile suspect event into database. " + string(e.what()), "");
		}

		if(Config::Inst()->GetClearAfterHostile())
		{
			stringstream ss;
			ss << "Main suspect Erase returned: " << suspects.Erase(key);
			LOG(DEBUG, ss.str(), "");


			stringstream ss2;
			ss2 << "LastSave suspect Erase returned: " << suspectsSinceLastSave.Erase(key);
			LOG(DEBUG, ss2.str(), "");

			Message *msg = new Message(UPDATE_SUSPECT_CLEARED);
			msg->m_contents.mutable_m_suspectid()->CopyFrom(suspectCopy.GetIdentifier());
			MessageManager::Instance().WriteMessage(msg, 0);
			delete msg;
		}
		else
		{
			//Send to UI
			Message suspectUpdate;
			suspectUpdate.m_contents.set_m_type(UPDATE_SUSPECT);
			suspectUpdate.m_suspects.push_back(&suspectCopy);
			MessageManager::Instance().WriteMessage(&suspectUpdate, 0);
		}

		LOG(ALERT, "Detected potentially hostile traffic from: " + suspectCopy.ToString(), "");
	}
	else
	{
		Message suspectUpdate;
		suspectUpdate.m_contents.set_m_type(UPDATE_SUSPECT);
		suspectUpdate.m_suspects.push_back(&suspectCopy);
		MessageManager::Instance().WriteMessage(&suspectUpdate, 0);
	}

}

void CheckForDroppedPackets()
{
	Lock lock(&packetCapturesLock);
	// Quick check for libpcap dropping packets
	for(uint i = 0; i < packetCaptures.size(); i++)
	{
		int dropped = packetCaptures.at(i)->GetDroppedPackets();
		if(dropped != -1 && dropped != dropCounts[i])
		{
			if(dropped > dropCounts[i])
			{
				stringstream ss;
				ss << "Libpcap has dropped " << dropped - dropCounts[i] << " packets. Try increasing the capture buffer." << endl;
				LOG(WARNING, ss.str(), "");
			}
			dropCounts[i] = dropped;
		}
	}
}

void UpdateHaystackFeatures()
{
	vector<uint32_t> haystackNodes;
	for(uint i = 0; i < haystackAddresses.size(); i++)
	{
		haystackNodes.push_back(htonl(inet_addr(haystackAddresses[i].c_str())));
	}
	stringstream ss;
	ss << "Currently monitoring " << haystackAddresses.size() << " static honeypot IP addresses";
	LOG(DEBUG, ss.str(), "");

	for(uint i = 0; i < haystackDhcpAddresses.size(); i++)
	{
		haystackNodes.push_back(htonl(inet_addr(haystackDhcpAddresses[i].c_str())));
	}
	stringstream ss2;
	ss2 << "Currently monitoring " << haystackDhcpAddresses.size() << " DHCP honeypot IP addresses";
	LOG(DEBUG, ss2.str(), "");

	suspects.SetHaystackNodes(haystackNodes);
}

}
