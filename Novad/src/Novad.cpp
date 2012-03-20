//============================================================================
// Name        : Novad.cpp
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
// Description : Nova Daemon to perform network anti-reconnaissance
//============================================================================

#include "Config.h"
#include "SuspectTable.h"
#include "NovaUtil.h"
#include "Logger.h"
#include "Point.h"
#include "Novad.h"
#include "Control.h"
#include "ClassificationEngine.h"
#include "ProtocolHandler.h"
#include "FeatureSet.h"
#include "Threads.h"

#include <vector>
#include <math.h>
#include <string>
#include <errno.h>
#include <fstream>
#include <sstream>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <signal.h>
#include <sys/inotify.h>
#include <netinet/if_ether.h>
#include <iostream>
#include <time.h>

#include <boost/format.hpp>

using namespace std;
using namespace Nova;
using boost::format;


// Maintains a list of suspects and information on network activity
SuspectTable suspects;

// Suspects not yet written to the state file
SuspectTable suspectsSinceLastSave;
pthread_mutex_t suspectsSinceLastSaveLock;

// TCP session tracking table
TCPSessionHashTable SessionTable;
pthread_rwlock_t sessionLock;

//** Silent Alarm **
struct sockaddr_in serv_addr;
struct sockaddr* serv_addrPtr = (struct sockaddr *) &serv_addr;
struct sockaddr_in hostAddr;

// Timestamps for the CE state file exiration of data
time_t lastLoadTime;
time_t lastSaveTime;

//HS Vars
string dhcpListFile = "/var/log/honeyd/ipList";
vector<string> haystackAddresses;
vector<string> haystackDhcpAddresses;
pcap_t *handle;
bpf_u_int32 maskp; /* subnet mask */
bpf_u_int32 netp; /* ip          */

int notifyFd;
int watch;


ClassificationEngine *engine;

pthread_t classificationLoopThread;
pthread_t trainingLoopThread;
pthread_t silentAlarmListenThread;
pthread_t ipUpdateThread;
pthread_t TCP_timeout_thread;

int Nova::RunNovaD()
{
	suspects.Resize(INIT_SIZE_SMALL);
	suspectsSinceLastSave.Resize(INIT_SIZE_SMALL);

	SessionTable.set_empty_key("");
	SessionTable.resize(INIT_SIZE_HUGE);

	pthread_mutex_init(&suspectsSinceLastSaveLock, NULL);
	pthread_rwlock_init(&sessionLock, NULL);

	// Let both of these initialize before we have multiple threads going
	Config::Inst();
	Logger::Inst();

	// Change our working folder into the config folder so our relative paths are correct
	if(chdir(Config::Inst()->getPathHome().c_str()) == -1)
		LOG(INFO, "Failed to change directory to " + Config::Inst()->getPathHome(), "Failed to change directory to " + Config::Inst()->getPathHome());

	// Set up our signal handlers
	signal(SIGKILL, SaveAndExit);
	signal(SIGINT, SaveAndExit);
	signal(SIGTERM, SaveAndExit);
	signal(SIGPIPE, SIG_IGN);

	lastLoadTime = time(NULL);
	if (lastLoadTime == ((time_t)-1))
		LOG(ERROR, (format("File %1% at line %2%: Unable to get system time with time()")%__FILE__%__LINE__).str());

	lastSaveTime = time(NULL);
	if (lastSaveTime == ((time_t)-1))
		LOG(ERROR, (format("File %1% at line %2%: Unable to get system time with time()")%__FILE__%__LINE__).str());


	engine = new ClassificationEngine(suspects);

	Spawn_UI_Handler();

	Reload();

	//Are we Training or Classifying?
	if(Config::Inst()->getIsTraining())
	{
		pthread_create(&trainingLoopThread,NULL,TrainingLoop,NULL);
	}
	else
	{
		pthread_create(&classificationLoopThread,NULL,ClassificationLoop, NULL);
		pthread_create(&silentAlarmListenThread,NULL,SilentAlarmLoop, NULL);
	}

	notifyFd = inotify_init ();

	if (notifyFd > 0)
	{
		watch = inotify_add_watch (notifyFd, dhcpListFile.c_str(), IN_CLOSE_WRITE | IN_MOVED_TO | IN_MODIFY | IN_DELETE);
		pthread_create(&ipUpdateThread, NULL, UpdateIPFilter,NULL);
	}
	else
	{
		LOG(ERROR, (format("File %1% at line %2%: Unable to set up file watcher for the honeyd IP"
				" list file. DHCP addresse in honeyd will not be read")%__FILE__%__LINE__).str());
	}

	Start_Packet_Handler();

	//Shouldn't get here!
	LOG(CRITICAL, (format("File %1% at line %2%: Main thread ended. This should never happen,"
			" something went very wrong.")%__FILE__%__LINE__).str());

	return EXIT_FAILURE;
}

void Nova::MaskKillSignals()
{
	sigset_t x;
	sigemptyset (&x);
	sigaddset(&x, SIGKILL);
	sigaddset(&x, SIGTERM);
	sigaddset(&x, SIGINT);
	sigaddset(&x, SIGPIPE);
	sigprocmask(SIG_BLOCK, &x, NULL);
}

void Nova::AppendToStateFile()
{
	pthread_mutex_lock(&suspectsSinceLastSaveLock);
	lastSaveTime = time(NULL);
	if (lastSaveTime == ((time_t)-1))
		LOG(ERROR, (format("File %1% at line %2%: Unable to get timestamp, call to time()"
				" failed")%__FILE__%__LINE__).str());

	// Don't bother saving if no new suspect data, just confuses deserialization
	if (suspectsSinceLastSave.Size() <= 0)
	{
		pthread_mutex_unlock(&suspectsSinceLastSaveLock);
		return;
	}

	u_char tableBuffer[MAX_MSG_SIZE];
	uint32_t dataSize = 0;

	// Compute the total dataSize
	for(SuspectTableIterator it = suspectsSinceLastSave.Begin(); it.GetIndex() <  suspectsSinceLastSave.Size(); ++it)
	{
		Suspect currentSuspect = suspectsSinceLastSave.CheckOut(it.GetKey());
		currentSuspect.UpdateEvidence();
		currentSuspect.UpdateFeatureData(true);
		currentSuspect.CalculateFeatures();
		suspectsSinceLastSave.CheckIn(&currentSuspect);
		if (!currentSuspect.GetFeatureSet().m_packetCount)
		{
			continue;
		}
		else
		{
			dataSize += currentSuspect.SerializeSuspectWithData(tableBuffer);
		}
	}
	// No suspects with packets to update
	if (dataSize == 0)
	{
		pthread_mutex_unlock(&suspectsSinceLastSaveLock);
		return;
	}

	ofstream out(Config::Inst()->getPathCESaveFile().data(), ofstream::binary | ofstream::app);
	if(!out.is_open())
	{
		LOG(ERROR, (format("File %1% at line %2%: Unable to open the CE state file %3%")
				%__FILE__%__LINE__%Config::Inst()->getPathCESaveFile()).str());
		pthread_mutex_unlock(&suspectsSinceLastSaveLock);
		return;
	}

	out.write((char*)&lastSaveTime, sizeof lastSaveTime);
	out.write((char*)&dataSize, sizeof dataSize);

	LOG(DEBUG, (format("File %1% at line %2%: Appending %3% bytes to the CE state file")
			%__FILE__%__LINE__%dataSize).str());
	Suspect suspectCopy;
	// Serialize our suspect table
	for(SuspectTableIterator it = suspectsSinceLastSave.Begin(); it.GetIndex() <  suspectsSinceLastSave.Size(); ++it)
	{
		suspectCopy = suspectsSinceLastSave.CheckOut(it.GetKey());

		if (!suspectCopy.GetFeatureSet().m_packetCount)
			continue;
		dataSize = suspectCopy.SerializeSuspectWithData(tableBuffer);
		suspectsSinceLastSave.CheckIn(&suspectCopy);
		out.write((char*) tableBuffer, dataSize);
	}
	out.close();
	suspectsSinceLastSave.Clear();

	pthread_mutex_unlock(&suspectsSinceLastSaveLock);
}

void Nova::LoadStateFile()
{
	time_t timeStamp;
	uint32_t dataSize;

	lastLoadTime = time(NULL);
	if (lastLoadTime == ((time_t)-1))
		LOG(ERROR, (format("File %1% at line %2%: Unable to get timestamp, call to time() failed")
				%__FILE__%__LINE__).str());

	// Open input file
	ifstream in(Config::Inst()->getPathCESaveFile().data(), ios::binary | ios::in);
	if(!in.is_open())
	{
		LOG(ERROR, (format("File %1% at line %2%: Unable to open CE state file. This is normal for "
				"the first run.")%__FILE__%__LINE__).str());
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
			LOG(ERROR, "The CE state file may be corrupt",(format("File %1% at line %2%: CE state file "
				"should have another entry, but only contains %3% more bytes")%__FILE__%__LINE__%lengthLeft).str());
			break;
		}

		in.read((char*) &timeStamp, sizeof timeStamp);
		lengthLeft -= sizeof timeStamp;

		in.read((char*) &dataSize, sizeof dataSize);
		lengthLeft -= sizeof dataSize;

		if (Config::Inst()->getDataTTL() && (timeStamp < lastLoadTime - Config::Inst()->getDataTTL()))
		{
			LOG(DEBUG, (format("File %1% at line %2%: Throwing out old CE state with timestamp of %3%")
					%__FILE__%__LINE__%(int)timeStamp).str());

			in.seekg(dataSize, ifstream::cur);
			lengthLeft -= dataSize;
			continue;
		}

		// Not as many bytes left as the size of the entry?
		if (lengthLeft < dataSize)
		{
			LOG(ERROR, "The CE state file may be corruput, unable to read all data from it",
				(format("File %1% at line %2%: CE state file should have another entry of size %3% "
				"but only has %4% bytes left")%__FILE__%__LINE__%dataSize%lengthLeft).str());

			break;
		}

		u_char tableBuffer[dataSize];
		in.read((char*) tableBuffer, dataSize);
		lengthLeft -= dataSize;

		// Read each suspect
		uint32_t bytesSoFar = 0;
		Suspect suspectCopy;
		while (bytesSoFar < dataSize)
		{
			Suspect* newSuspect = new Suspect();
			uint32_t suspectBytes = 0;
			suspectBytes += newSuspect->DeserializeSuspect(tableBuffer + bytesSoFar + suspectBytes);

			FeatureSet fs = newSuspect->GetFeatureSet();
			suspectBytes += fs.DeserializeFeatureData(tableBuffer + bytesSoFar + suspectBytes);
			newSuspect->SetFeatureSet(&fs);
			bytesSoFar += suspectBytes;

			// If our suspect has no evidence, throw it out
			if (newSuspect->GetFeatureSet().m_packetCount == 0)
			{
				LOG(WARNING,"CEState file contained a suspect with no packets for evidence. Throwing out this suspect.");
			}
			else
			{
				if(!suspects.IsValidKey(newSuspect->GetIpAddress()))
				{
					newSuspect->SetNeedsClassificationUpdate(true);
					suspects.AddNewSuspect(newSuspect);
				}
				else
				{
					suspectCopy = suspects.CheckOut(newSuspect->GetIpAddress());
					FeatureSet fs = newSuspect->GetFeatureSet();
					suspectCopy.AddFeatureSet(&fs);
					suspectCopy.SetNeedsClassificationUpdate(true);
					suspects.CheckIn(&suspectCopy);
					delete newSuspect;
				}
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
		LOG(ERROR, (format("File %1% at line %2%: Unable to get timestamp, call to time() failed")%__FILE__
				%__LINE__).str());

	// Open input file
	ifstream in(Config::Inst()->getPathCESaveFile().data(), ios::binary | ios::in);
	if(!in.is_open())
	{
		LOG(ERROR, (format("File %1% at line %2%: Unable to open the CE state file at %3%")%__FILE__
				%__LINE__%Config::Inst()->getPathCESaveFile()).str());
		return;
	}

	// Open the tmp file
	string tmpFile = Config::Inst()->getPathCESaveFile() + ".tmp";
	ofstream out(tmpFile.data(), ios::binary);
	if(!out.is_open())
	{
		LOG(ERROR, (format("File %1% at line %2%: Unable to open the temporary CE state file at %3%"
				)%__FILE__%__LINE__%tmpFile).str());
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
			LOG(ERROR, "The CE state file may be corrupt", (format("File %1% at line %2%: CE state file "
					"should have another entry, but only contains %3% more bytes")%__FILE__%__LINE__%lengthLeft).str());
			break;
		}

		in.read((char*) &timeStamp, sizeof timeStamp);
		lengthLeft -= sizeof timeStamp;

		in.read((char*) &dataSize, sizeof dataSize);
		lengthLeft -= sizeof dataSize;

		if (Config::Inst()->getDataTTL() && (timeStamp < lastLoadTime - Config::Inst()->getDataTTL()))
		{
			LOG(DEBUG, (format("File %1% at line %2%: Throwing out old CE state with timestamp of %3%")
					%__FILE__%__LINE__%(int)timeStamp).str());

			in.seekg(dataSize, ifstream::cur);
			lengthLeft -= dataSize;
			continue;
		}

		// Not as many bytes left as the size of the entry?
		if (lengthLeft < dataSize)
		{
			LOG(ERROR, "The CE state file may be corrupt",
				(format("File %1% at line %2%: Data file should have another entry of size %3%, "
				"but contains only %4% bytes left")%__FILE__%__LINE__%dataSize%lengthLeft).str());

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

			FeatureSet fs = newSuspect->GetFeatureSet();
			fs.DeserializeFeatureData(tableBuffer + bytesSoFar + suspectBytes);
			newSuspect->SetFeatureSet(&fs);

			if(!suspects.IsValidKey(newSuspect->GetIpAddress())
					&& suspectsSinceLastSave.IsValidKey(newSuspect->GetIpAddress()))
			{
				in_addr_t key = newSuspect->GetIpAddress();
				suspectsSinceLastSave.Erase(key);
				// Shift the rest of the data over on top of our bad suspect
				memmove(tableBuffer + bytesSoFar, tableBuffer + bytesSoFar + suspectBytes,
						(dataSize - bytesSoFar - suspectBytes) * sizeof(tableBuffer[0]));
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

	string copyCommand = "cp -f " + tmpFile + " " + Config::Inst()->getPathCESaveFile();
	if (system(copyCommand.c_str()) == -1)
		LOG(ERROR, "Failed to write to the CE state file. This may be a permission problem, or the folder may not exist.",
			(format("File %1% at line %2%: Unable to copy CE state tmp file to CE state file."
			" System call to '%3' failed")%__FILE__%__LINE__%copyCommand).str());
}

void Nova::Reload()
{
	LoadConfiguration();

	// Reload the configuration file
	Config::Inst()->LoadConfig();

	engine->LoadDataPointsFromFile(Config::Inst()->getPathTrainingFile());
	Suspect suspectCopy;
	// Set everyone to be reclassified
	for(SuspectTableIterator it = suspects.Begin(); it.GetIndex() < suspects.Size(); ++it)
	{
		suspectCopy = suspects.CheckOut(it.GetKey());
		suspectCopy.SetNeedsClassificationUpdate(true);
		suspects.CheckIn(&suspectCopy);
	}
}


void Nova::SilentAlarm(Suspect *suspect, int oldClassification)
{
	int sockfd;
	char suspectAddr[INET_ADDRSTRLEN];
	string commandLine;
	string hostAddrString = GetLocalIP(Config::Inst()->getInterface().c_str());
	u_char serializedBuffer[MAX_MSG_SIZE];

	uint dataLen = suspect->SerializeSuspect(serializedBuffer);

	//If the hostility hasn't changed don't bother the DM
	if(oldClassification != suspect->GetIsHostile())
	{
		if(suspect->GetIsHostile() && Config::Inst()->getIsDmEnabled())
		{
			in_addr temp = suspect->GetInAddr();
			inet_ntop(AF_INET, &(temp), suspectAddr, INET_ADDRSTRLEN);

			commandLine = "sudo iptables -t nat -A PREROUTING -d ";
			commandLine += hostAddrString;
			commandLine += " -s ";
			commandLine += suspectAddr;
			commandLine += " -j DNAT --to-destination ";
			commandLine += Config::Inst()->getDoppelIp();

			if(system(commandLine.c_str()) != 0)
			{
				LOG(ERROR, (format("File %1% at line %2%: System call: "
					"'%3%' has failed.")%__FILE__%__LINE__%commandLine.c_str()).str());
			}
		}
		else
		{
			in_addr temp = suspect->GetInAddr();
			inet_ntop(AF_INET, &(temp), suspectAddr, INET_ADDRSTRLEN);

			commandLine = "sudo iptables -t nat -D PREROUTING -d ";
			commandLine += hostAddrString;
			commandLine += " -s ";
			commandLine += suspectAddr;
			commandLine += " -j DNAT --to-destination ";
			commandLine += Config::Inst()->getDoppelIp();

			if(system(commandLine.c_str()) != 0)
			{
				LOG(ERROR, (format("File %1% at line %2%: System call: "
					"'%3%' has failed.")%__FILE__%__LINE__%commandLine.c_str()).str());
			}
		}
	}
	if(suspect->GetUnsentFeatureSet().m_packetCount)
	{
		do
		{
			dataLen = suspect->SerializeSuspectWithData(serializedBuffer);
			// Move the unsent data to the sent side
			suspect->UpdateFeatureData(INCLUDE);
			// Clear the unsent data
			suspect->ClearUnsentData();

			//Update other Nova Instances with latest suspect Data
			for(uint i = 0; i < Config::Inst()->getNeighbors().size(); i++)
			{
				serv_addr.sin_addr.s_addr = Config::Inst()->getNeighbors()[i];

				stringstream ss;
				string commandLine;

				ss << "sudo iptables -I INPUT -s " << string(inet_ntoa(serv_addr.sin_addr)) << " -p tcp -j ACCEPT";
				commandLine = ss.str();

				if(system(commandLine.c_str()) == -1)
				{
					LOG(ERROR, "Failed to update iptables.", "Failed to update iptables.");
				}


				int i;
				for(i = 0; i < Config::Inst()->getSaMaxAttempts(); i++)
				{
					if(KnockPort(OPEN))
					{
						//Send Silent Alarm to other Nova Instances with feature Data
						if ((sockfd = socket(AF_INET,SOCK_STREAM,6)) == -1)
						{
							LOG(ERROR, (format("File %1% at line %2%: Unable to open socket to send silent alarm."
									" Errno: %3%")%__FILE__%__LINE__%strerror(errno)).str());
							close(sockfd);
							continue;
						}
						if (connect(sockfd, serv_addrPtr, sizeof(serv_addr)) == -1)
						{
							//If the host isn't up we stop trying
							if(errno == EHOSTUNREACH)
							{
								//set to max attempts to hit the failed connect condition
								i = Config::Inst()->getSaMaxAttempts();
								LOG(ERROR, (format("File %1% at line %2%: Unable to connect to host to send silent alarm."
										" Errno: %3%")%__FILE__%__LINE__%strerror(errno)).str());
								break;
							}
							LOG(ERROR, (format("File %1% at line %2%: Unable to open socket to send silent alarm."
									" Errno: %3%")%__FILE__%__LINE__%strerror(errno)).str());
							close(sockfd);
							continue;
						}
						break;
					}
				}
				//If connecting failed
				if(i == Config::Inst()->getSaMaxAttempts() )
				{
					close(sockfd);
					ss.str("");
					ss << "sudo iptables -D INPUT -s " << string(inet_ntoa(serv_addr.sin_addr)) << " -p tcp -j ACCEPT";
					commandLine = ss.str();
					if(system(commandLine.c_str()) == -1)
					{
						LOG(ERROR, "Failed to update iptables.", "Failed to update iptables.");
					}
					continue;
				}

				if( send(sockfd, serializedBuffer, dataLen, 0) == -1)
				{
					LOG(ERROR, (format("File %1% at line %2%: Error in TCP Send of silent alarm."
							" Errno: %3%")%__FILE__%__LINE__%strerror(errno)).str());

					close(sockfd);
					ss.str("");
					ss << "sudo iptables -D INPUT -s " << string(inet_ntoa(serv_addr.sin_addr)) << " -p tcp -j ACCEPT";
					commandLine = ss.str();
					if(system(commandLine.c_str()) == -1)
					{
						LOG(ERROR, "Failed to update iptables.", "Failed to update iptables.");
					}
					continue;
				}
				close(sockfd);
				KnockPort(CLOSE);
				ss.str("");
				ss << "sudo iptables -D INPUT -s " << string(inet_ntoa(serv_addr.sin_addr)) << " -p tcp -j ACCEPT";
				commandLine = ss.str();
				if(system(commandLine.c_str()) == -1)
				{
					LOG(ERROR, "Failed to update iptables.", "Failed to update iptables.");
				}
			}
		}while(dataLen == MORE_DATA);
	}
}


bool Nova::KnockPort(bool mode)
{
	int sockfd;
	stringstream ss;
	ss << Config::Inst()->getKey();

	//mode == OPEN (true)
	if(mode)
		ss << "OPEN";

	//mode == CLOSE (false)
	else
		ss << "SHUT";

	uint keyDataLen = Config::Inst()->getKey().size() + 4;
	u_char keyBuf[1024];
	bzero(keyBuf, 1024);
	memcpy(keyBuf, ss.str().c_str(), ss.str().size());

	CryptBuffer(keyBuf, keyDataLen, ENCRYPT);

	//Send Port knock to other Nova Instances
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 17)) == -1)
	{
		LOG(ERROR, (format("File %1% at line %2%:  Error in port knocking. Can't create socket: %3%")
				%__FILE__%__LINE__%strerror(errno)).str());
		close(sockfd);
		return false;
	}

	if( sendto(sockfd,keyBuf,keyDataLen, 0,serv_addrPtr, sizeof(serv_addr)) == -1)
	{
		LOG(ERROR, (format("File %1% at line %2%:  Error in UDP Send for port knocking: %3%")
				%__FILE__%__LINE__%strerror(errno)).str());
		close(sockfd);
		return false;
	}

	close(sockfd);
	sleep(Config::Inst()->getSaSleepDuration());
	return true;
}


bool Nova::Start_Packet_Handler()
{
	char errbuf[PCAP_ERRBUF_SIZE];

	int ret;
	string haystackAddresses_csv = "";

	struct bpf_program fp;			/* The compiled filter expression */
	char filter_exp[64];


	haystackAddresses = GetHaystackAddresses(Config::Inst()->getPathConfigHoneydHs());
	haystackDhcpAddresses = GetHaystackDhcpAddresses(dhcpListFile);
	haystackAddresses_csv = ConstructFilterString();

	//If we're reading from a packet capture file
	if(Config::Inst()->getReadPcap())
	{
		sleep(1); //To allow time for other processes to open
		handle = pcap_open_offline(Config::Inst()->getPathPcapFile().c_str(), errbuf);

		if(handle == NULL)
		{
			LOG(CRITICAL, (format("File %1% at line %2%: Couldn't open pcapc file: %3%: %4%")
					%__FILE__%__LINE__%Config::Inst()->getPathPcapFile().c_str()%errbuf).str());
			exit(EXIT_FAILURE);
		}
		if (pcap_compile(handle, &fp, haystackAddresses_csv.data(), 0, maskp) == -1)
		//if (pcap_compile(handle, &fp, "dst net 192.168.10 && !dst host 192.168.10.255" , 0, maskp) == -1)
		{
			LOG(CRITICAL, (format("File %1% at line %2%: Couldn't parse pcap filter: %3%: %4%")
					%__LINE__%filter_exp%pcap_geterr(handle)).str());
			exit(EXIT_FAILURE);
		}

		if (pcap_setfilter(handle, &fp) == -1)
		{
			LOG(CRITICAL, (format("File %1% at line %2%: Couldn't install pcap filter: %3%: %4%")
					% __FILE__%__LINE__%filter_exp%pcap_geterr(handle)).str());
			exit(EXIT_FAILURE);
		}
		//First process any packets in the file then close all the sessions
		pcap_loop(handle, -1, Packet_Handler,NULL);

		TCPTimeout(NULL);

		if(Config::Inst()->getGotoLive()) Config::Inst()->setReadPcap(false); //If we are going to live capture set the flag.

		LOG(DEBUG, "Done processing PCAP file");
	}


	if(!Config::Inst()->getReadPcap())
	{
		LoadStateFile();

		//Open in non-promiscuous mode, since we only want traffic destined for the host machine
		handle = pcap_open_live(Config::Inst()->getInterface().c_str(), BUFSIZ, 0, 1000, errbuf);

		if(handle == NULL)
		{
			LOG(ERROR, (format("File %1% at line %2%:  Unable to open network interface %3% for live capture:"
					"%4%")% __FILE__%__LINE__%Config::Inst()->getInterface().c_str()%errbuf).str());
			exit(EXIT_FAILURE);
		}

		/* ask pcap for the network address and mask of the device */
		ret = pcap_lookupnet(Config::Inst()->getInterface().c_str(), &netp, &maskp, errbuf);

		if(ret == -1)
		{
			LOG(ERROR, (format("File %1% at line %2%: Unable to get the network address and mask "
					"of the interface. Error: %3%")% __FILE__%__LINE__%errbuf).str());
			exit(EXIT_FAILURE);
		}

		if (pcap_compile(handle, &fp, haystackAddresses_csv.data(), 0, maskp) == -1)
		{
			LOG(ERROR, (format("File %1% at line %2%:  Couldn't parse filter: %3% %4%")
					% __FILE__%__LINE__% filter_exp%pcap_geterr(handle)).str());
			exit(EXIT_FAILURE);
		}

		if (pcap_setfilter(handle, &fp) == -1)
		{
			LOG(ERROR, (format("File %1% at line %2%:  Couldn't install filter:%3% %4%")
					% __FILE__%__LINE__% filter_exp%pcap_geterr(handle)).str());
			exit(EXIT_FAILURE);
		}
		//"Main Loop"
		//Runs the function "Packet_Handler" every time a packet is received
		pthread_create(&TCP_timeout_thread, NULL, TCPTimeout, NULL);

	    pcap_loop(handle, -1, Packet_Handler, NULL);
	}
	return false;
}

void Nova::Packet_Handler(u_char *useless,const struct pcap_pkthdr* pkthdr,const u_char* packet)
{
	//Memory assignments moved outside packet handler to increase performance
	int dest_port;
	Packet packet_info;
	struct ether_header *ethernet;  	/* net/ethernet.h */
	struct ip *ip_hdr; 					/* The IP header */
	char tcp_socket[55];

	if(packet == NULL)
	{
		LOG(ERROR, (format("File %1% at line %2%:  Failed to capture packet!")% __FILE__%__LINE__).str());
		return;
	}


	/* let's start with the ether header... */
	ethernet = (struct ether_header *) packet;

	/* Do a couple of checks to see what packet type we have..*/
	if (ntohs (ethernet->ether_type) == ETHERTYPE_IP)
	{
		ip_hdr = (struct ip*)(packet + sizeof(struct ether_header));

		//Prepare Packet structure
		packet_info.ip_hdr = *ip_hdr;
		packet_info.pcap_header = *pkthdr;
		//If this is to the host
		if(packet_info.ip_hdr.ip_dst.s_addr == hostAddr.sin_addr.s_addr)
			packet_info.fromHaystack = FROM_LTM;
		else
			packet_info.fromHaystack = FROM_HAYSTACK_DP;

		//IF UDP or ICMP
		if(ip_hdr->ip_p == 17 )
		{
			packet_info.udp_hdr = *(struct udphdr*) ((char *)ip_hdr + sizeof(struct ip));
			UpdateSuspect(packet_info);
		}
		else if(ip_hdr->ip_p == 1)
		{
			packet_info.icmp_hdr = *(struct icmphdr*) ((char *)ip_hdr + sizeof(struct ip));
			UpdateSuspect(packet_info);
		}
		//If TCP...
		else if(ip_hdr->ip_p == 6)
		{
			packet_info.tcp_hdr = *(struct tcphdr*)((char*)ip_hdr + sizeof(struct ip));
			dest_port = ntohs(packet_info.tcp_hdr.dest);

			bzero(tcp_socket, 55);
			snprintf(tcp_socket, 55, "%d-%d-%d", ip_hdr->ip_dst.s_addr, ip_hdr->ip_src.s_addr, dest_port);

			pthread_rwlock_wrlock(&sessionLock);
			//If this is a new entry...
			if(SessionTable[tcp_socket].session.size() == 0)
			{
				//Insert packet into Hash Table
				SessionTable[tcp_socket].session.push_back(packet_info);
				SessionTable[tcp_socket].fin = false;
			}

			//If there is already a session in progress for the given LogEntry
			else
			{
				//If Session is ending
				//TODO: The session may continue a few packets after the FIN. Account for this case.
				//See ticket #15
				if(packet_info.tcp_hdr.fin)// Runs appendToStateFile before exiting
				{
					SessionTable[tcp_socket].session.push_back(packet_info);
					SessionTable[tcp_socket].fin = true;
				}
				else
				{
					//Add this new packet to the session vector
					SessionTable[tcp_socket].session.push_back(packet_info);
				}
			}
			pthread_rwlock_unlock(&sessionLock);
		}

		// Allow for continuous classification
		if(!Config::Inst()->getClassificationTimeout())
		{
			if (!Config::Inst()->getIsTraining())
				ClassificationLoop(NULL);
			else
				TrainingLoop(NULL);
		}
	}
	else if(ntohs(ethernet->ether_type) == ETHERTYPE_ARP)
	{
		return;
	}
	else
	{
		LOG(ERROR, (format("File %1% at line %2%:  Unknown Non-IP Packet Received. Nova is ignoring it.")
				% __FILE__%__LINE__).str());
		return;
	}
}

void Nova::LoadConfiguration()
{
	string hostAddrString = GetLocalIP(Config::Inst()->getInterface().c_str());

	if(hostAddrString.size() == 0)
	{
		LOG(ERROR, (format("File %1% at line %2%:  Bad ethernet interface, no IP's associated!")
				% __FILE__%__LINE__).str());
		exit(EXIT_FAILURE);
	}

	inet_pton(AF_INET,hostAddrString.c_str(),&(hostAddr.sin_addr));
}


string Nova::ConstructFilterString()
{
	//Flatten out the vectors into a csv string
	string filterString = "";

	for(uint i = 0; i < haystackAddresses.size(); i++)
	{
		filterString += "dst host ";
		filterString += haystackAddresses[i];

		if(i+1 != haystackAddresses.size())
			filterString += " || ";
	}

	if (!haystackDhcpAddresses.empty() && !haystackAddresses.empty())
		filterString += " || ";

	for(uint i = 0; i < haystackDhcpAddresses.size(); i++)
	{
		filterString += "dst host ";
		filterString += haystackDhcpAddresses[i];

		if(i+1 != haystackDhcpAddresses.size())
			filterString += " || ";
	}

	if (filterString == "")
	{
		filterString = "dst host 0.0.0.0";
	}

	LOG(DEBUG, "Pcap filter string is " + filterString);
	return filterString;
}


vector <string> Nova::GetHaystackDhcpAddresses(string dhcpListFile)
{
	ifstream dhcpFile(dhcpListFile.data());
	vector<string> haystackDhcpAddresses;

	if (dhcpFile.is_open())
	{
		while ( dhcpFile.good() )
		{
			string line;
			getline (dhcpFile,line);
			if (strcmp(line.c_str(), ""))
				haystackDhcpAddresses.push_back(line);
		}
		dhcpFile.close();
	}
	else cout << "Unable to open file";

	return haystackDhcpAddresses;
}

vector <string> Nova::GetHaystackAddresses(string honeyDConfigPath)
{
	//Path to the main log file
	ifstream honeydConfFile (honeyDConfigPath.c_str());
	vector <string> retAddresses;
	retAddresses.push_back(GetLocalIP(Config::Inst()->getInterface().c_str()));

	if( honeydConfFile == NULL)
	{
		LOG(ERROR, (format("File %1% at line %2%:  Error opening log file. Does it exist?")% __FILE__%__LINE__).str());
		exit(EXIT_FAILURE);
	}

	string LogInputLine;

	while(!honeydConfFile.eof())
	{
		stringstream LogInputLineStream;

		//Get the next line
		getline(honeydConfFile, LogInputLine);

		//Load the line into a stringstream for easier tokenizing
		LogInputLineStream << LogInputLine;
		string token;

		//Is the first word "bind"?
		getline(LogInputLineStream, token, ' ');

		if(token.compare( "bind" ) != 0)
		{
			continue;
		}

		//The next token will be the IP address
		getline(LogInputLineStream, token, ' ');
		retAddresses.push_back(token);
	}
	return retAddresses;
}

void Nova::UpdateSuspect(Packet packet)
{
	Suspect * newSuspect = new Suspect(packet);
	in_addr_t addr = newSuspect->GetIpAddress();
	//If our suspect is new
	if(!suspects.IsValidKey(addr))
	{
		newSuspect->SetNeedsClassificationUpdate(true);
		newSuspect->SetIsLive(Config::Inst()->getReadPcap());
		suspects.AddNewSuspect(newSuspect);

		// Make a copy to add to the other table
		Suspect *newSuspectCopy = new Suspect(*newSuspect);
		pthread_mutex_lock(&suspectsSinceLastSaveLock);
		suspectsSinceLastSave.AddNewSuspect(newSuspectCopy);
		pthread_mutex_unlock(&suspectsSinceLastSaveLock);
	}
	//Else our suspect exists
	else
	{
		Suspect suspectCopy = suspects.CheckOut(addr);
		suspectCopy.AddEvidence(packet);
		suspectCopy.SetIsLive(Config::Inst()->getReadPcap());
		suspects.CheckIn(&suspectCopy);

		pthread_mutex_lock(&suspectsSinceLastSaveLock);
		Suspect suspectCopy2 = suspectsSinceLastSave.CheckOut(addr);
		suspectCopy2.AddEvidence(packet);
		suspectCopy2.SetIsLive(Config::Inst()->getReadPcap());
		suspectsSinceLastSave.CheckIn(&suspectCopy2);
		pthread_mutex_unlock(&suspectsSinceLastSaveLock);

		delete newSuspect;
	}
}
string Nova::GetLocalIP(const char *dev)
{
	static struct ifreq ifreqs[20];
	struct ifconf ifconf;
	uint  nifaces, i;

	memset(&ifconf,0,sizeof(ifconf));
	ifconf.ifc_buf = (char*) (ifreqs);
	ifconf.ifc_len = sizeof(ifreqs);

	int sock, rval;
	sock = socket(AF_INET,SOCK_STREAM,0);

	if(sock < 0)
	{
		LOG(ERROR, "Unable to determine the local interface's IP address", (format("File %1% at line %2%:  Error creating socket to check interface IP: %3%")% __FILE__%__LINE__%strerror(errno)).str());
	}

	if((rval = ioctl(sock, SIOCGIFCONF , (char*) &ifconf)) < 0 )
	{
		LOG(ERROR, "Unable to determine the local interface's IP address", (format("File %1% at line %2%:  Error with getLocalIP socket ioctl(SIOGIFCONF): %3%")% __FILE__%__LINE__%strerror(errno)).str());
	}

	close(sock);
	nifaces =  ifconf.ifc_len/sizeof(struct ifreq);

	for(i = 0; i < nifaces; i++)
	{
		if( strcmp(ifreqs[i].ifr_name, dev) == 0 )
		{
			char ip_addr [ INET_ADDRSTRLEN ];
			struct sockaddr_in *b = (struct sockaddr_in *) &(ifreqs[i].ifr_addr);

			inet_ntop(AF_INET, &(b->sin_addr), ip_addr, INET_ADDRSTRLEN);
			return string(ip_addr);
		}
	}
	return string("");
}
