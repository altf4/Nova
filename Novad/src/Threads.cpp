//============================================================================
// Name        : Threads.cpp
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
// Description : Novad thread loops
//============================================================================

#include "ClassificationEngine.h"
#include "ProtocolHandler.h"
#include "SuspectTable.h"
#include "FeatureSet.h"
#include "NovaUtil.h"
#include "Threads.h"
#include "Control.h"
#include "Logger.h"
#include "Config.h"
#include "Point.h"
#include "Novad.h"

#include <vector>
#include <math.h>
#include <time.h>
#include <string>
#include <errno.h>
#include <fstream>
#include <sstream>
#include <sys/un.h>
#include <net/if.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/inotify.h>
#include <netinet/if_ether.h>

using namespace std;
using namespace Nova;

// Maintains a list of suspects and information on network activity
extern SuspectTable suspects;
extern TCPSessionHashTable SessionTable;

extern struct sockaddr_in hostAddr;

//** Silent Alarm **
extern struct sockaddr_in serv_addr;

extern time_t lastLoadTime;
extern time_t lastSaveTime;

extern string trainingCapFile;
extern ofstream trainingFileStream;

//HS Vars
extern string dhcpListFile;
extern vector<string> haystackDhcpAddresses;
extern pcap_t *handle;
extern bpf_u_int32 maskp; /* subnet mask */

extern int notifyFd;
extern int watch;


extern pthread_rwlock_t sessionLock;
extern ClassificationEngine *engine;

namespace Nova
{
void *ClassificationLoop(void *ptr)
{
	MaskKillSignals();

	//Builds the Silent Alarm Network address
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(Config::Inst()->GetSaPort());

	//Classification Loop
	do
	{
		sleep(Config::Inst()->GetClassificationTimeout());
		//Calculate the "true" Feature Set for each Suspect
		vector<uint64_t> updateKeys = suspects.GetKeys_of_ModifiedSuspects();
		for(uint i = 0; i < updateKeys.size(); i++)
		{
			UpdateAndClassify(updateKeys[i]);
		}
		engine->m_dopp->UpdateDoppelganger();

		if(Config::Inst()->GetSaveFreq() > 0)
		{
			if((time(NULL) - lastSaveTime) > Config::Inst()->GetSaveFreq())
			{
				AppendToStateFile();
			}
		}

		if(Config::Inst()->GetDataTTL() > 0)
		{
			if((time(NULL) - lastLoadTime) > Config::Inst()->GetDataTTL())
			{
				AppendToStateFile();
				suspects.EraseAllSuspects();
				RefreshStateFile();
				LoadStateFile();
			}
		}
	}while(Config::Inst()->GetClassificationTimeout() && !Config::Inst()->GetReadPcap());

	if(Config::Inst()->GetReadPcap())
	{
		return NULL;
	}

	//Shouldn't get here!!
	if(Config::Inst()->GetClassificationTimeout())
	{
		LOG(CRITICAL, "The code should never get here, something went very wrong.", "");
	}

	return NULL;
}

void *TrainingLoop(void *ptr)
{
	MaskKillSignals();

	//Training Loop
	do
	{
		sleep(Config::Inst()->GetClassificationTimeout());
		trainingFileStream.open(trainingCapFile.data(), ios::app);

		if(trainingFileStream.is_open())
		{
			// Get list of Suspects that need a classification and feature update
			vector<uint64_t> updateKeys = suspects.GetKeys_of_ModifiedSuspects();
			suspects.UpdateAllSuspects();
			for(uint i = 0; i < updateKeys.size(); i++)
			{
				UpdateAndStore(updateKeys[i]);
			}
		}
		else
		{
			LOG(CRITICAL, "Unable to open the training capture file.",
				"Unable to open training capture file at: "+trainingCapFile);
		}
		trainingFileStream.close();
	} while (Config::Inst()->GetClassificationTimeout());

	//Shouldn't get here!
	if(Config::Inst()->GetClassificationTimeout())
	{
		LOG(CRITICAL, "The code should never get here, something went very wrong.", "");
	}
	return NULL;
}

void *SilentAlarmLoop(void *ptr)
{
	MaskKillSignals();

	int sockfd;
	u_char buf[MAX_MSG_SIZE];
	struct sockaddr_in sendaddr;

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		LOG(CRITICAL, "Unable to create the silent alarm socket.",
				"Unable to create the silent alarm socket: "+string(strerror(errno)));
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	sendaddr.sin_family = AF_INET;
	sendaddr.sin_port = htons(Config::Inst()->GetSaPort());
	sendaddr.sin_addr.s_addr = INADDR_ANY;

	memset(sendaddr.sin_zero, '\0', sizeof sendaddr.sin_zero);
	struct sockaddr* sockaddrPtr = (struct sockaddr*) &sendaddr;
	socklen_t sendaddrSize = sizeof sendaddr;

	if(::bind(sockfd, sockaddrPtr, sendaddrSize) == -1)
	{
		LOG(CRITICAL, "Unable to bind to the silent alarm socket.",
			"Unable to bind to the silent alarm socket: "+string(strerror(errno)));
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	stringstream ss;
	ss << "sudo iptables -A INPUT -p udp --dport "
			<< Config::Inst()->GetSaPort() << " -j REJECT"
					" --reject-with icmp-port-unreachable";
	if(system(ss.str().c_str()) == -1)
	{
		LOG(ERROR, "Failed to update iptables.", "");
	}
	ss.str("");
	ss << "sudo iptables -A INPUT -p tcp --dport "
			<< Config::Inst()->GetSaPort()
			<< " -j REJECT --reject-with tcp-reset";
	if(system(ss.str().c_str()) == -1)
	{
		LOG(ERROR, "Failed to update iptables.", "");
	}

	if(listen(sockfd, SOCKET_QUEUE_SIZE) == -1)
	{
		LOG(CRITICAL, "Unable to listen on the silent alarm socket.",
			"Unable to listen on the silent alarm socket.: "+string(strerror(errno)));
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	int connectionSocket, bytesRead;
	Suspect suspectCopy;

	//Accept incoming Silent Alarm TCP Connections
	while (1)
	{

		bzero(buf, MAX_MSG_SIZE);

		//Blocking call
		if((connectionSocket = accept(sockfd, sockaddrPtr, &sendaddrSize)) == -1)
		{
			LOG(CRITICAL, "Problem while accepting incoming silent alarm connection.",
				"Problem while accepting incoming silent alarm connection: "+string(strerror(errno)));
			continue;
		}

		if((bytesRead = recv(connectionSocket, buf, MAX_MSG_SIZE, MSG_WAITALL))== -1)
		{
			LOG(CRITICAL, "Problem while receiving incoming silent alarm connection.",
				"Problem while receiving incoming silent alarm connection: "+string(strerror(errno)));
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

		in_addr_t addr = 0;
		memcpy(&addr, buf, 4);
		uint64_t key = addr;
		Suspect * newSuspect = new Suspect();
		if(newSuspect->Deserialize(buf, MAIN_FEATURE_DATA) == 0)
		{
			close(connectionSocket);
			continue;
		}
		//If this suspect exists, update the information
		if(suspects.IsValidKey(key))
		{
			suspectCopy = suspects.CheckOut(key);
			suspectCopy.SetFlaggedByAlarm(true);
			FeatureSet fs = newSuspect->GetFeatureSet(MAIN_FEATURES);
			suspectCopy.AddFeatureSet(&fs, MAIN_FEATURES);
			suspects.CheckIn(&suspectCopy);

			// TODO: This looks like it may be a memory leak of newSuspect
		}
		//If this is a new suspect put it in the table
		else
		{
			newSuspect->SetIsHostile(false);
			newSuspect->SetFlaggedByAlarm(true);
			//We set isHostile to false so that when we classify the first time
			// the suspect will go from benign to hostile and be sent to the doppelganger module
			suspects.AddNewSuspect(newSuspect);
		}

		LOG(CRITICAL, string("Got a silent alarm!. Suspect: "+ newSuspect->ToString()), "");
		if(!Config::Inst()->GetClassificationTimeout())
		{
			UpdateAndClassify(newSuspect->GetIpAddress());
		}

		close(connectionSocket);
	}
	close(sockfd);
	LOG(CRITICAL, "The code should never get here, something went very wrong.", "");
	return NULL;
}

void *UpdateIPFilter(void *ptr)
{
	MaskKillSignals();

	while (true)
	{
		if(watch > 0)
		{
			int BUF_LEN = (1024 * (sizeof(struct inotify_event)) + 16);
			char buf[BUF_LEN];
			struct bpf_program fp; /* The compiled filter expression */
			char filter_exp[64];

			// Blocking call, only moves on when the kernel notifies it that file has been changed
			int readLen = read(notifyFd, buf, BUF_LEN);
			if(readLen > 0)
			{
				watch = inotify_add_watch(notifyFd, dhcpListFile.c_str(),
						IN_CLOSE_WRITE | IN_MOVED_TO | IN_MODIFY | IN_DELETE);
				haystackDhcpAddresses = GetHaystackDhcpAddresses(dhcpListFile);
				string haystackAddresses_csv = ConstructFilterString();

				if(pcap_compile(handle, &fp, haystackAddresses_csv.data(), 0,maskp) == -1)
				{
					LOG(ERROR, "Unable to enable packet capture.",
						"Couldn't parse pcap filter: "+ string(filter_exp) + " " + pcap_geterr(handle));
				}
				if(pcap_setfilter(handle, &fp) == -1)
				{
					LOG(ERROR, "Unable to enable packet capture.",
						"Couldn't install pcap filter: "+ string(filter_exp) + " " + pcap_geterr(handle));
				}
			}
		}
		else
		{
			// This is the case when there's no file to watch, just sleep and wait for it to
			// be created by honeyd when it starts up.
			sleep(2);
			watch = inotify_add_watch(notifyFd, dhcpListFile.c_str(),
					IN_CLOSE_WRITE | IN_MOVED_TO | IN_MODIFY | IN_DELETE);
		}
	}

	return NULL;
}

void *TCPTimeout(void *ptr)
{
	MaskKillSignals();
	do
	{
		pthread_rwlock_wrlock(&sessionLock);
		time_t currentTime = time(NULL);
		time_t packetTime;

		for (TCPSessionHashTable::iterator it = SessionTable.begin(); it != SessionTable.end(); it++)
		{
			if(it->second.session.size() > 0)
			{
				packetTime = it->second.session.back().pcap_header.ts.tv_sec;
				//If were reading packets from a file, assume all packets have been loaded and go beyond
				// timeout threshhold
				if(Config::Inst()->GetReadPcap())
				{
					currentTime = packetTime + 3 + Config::Inst()->GetTcpTimout();
				}
				// If it exists
				if(packetTime + 2 < currentTime)
				{
					//If session has been finished for more than two seconds
					if(it->second.fin == true)
					{
						for(uint p = 0; p < (SessionTable[it->first].session).size(); p++)
						{
							UpdateSuspect(SessionTable[it->first].session[p]);
						}
						// Allow for continuous classification
						if(!Config::Inst()->GetClassificationTimeout())
						{
							if(it->second.session.size() > 0)
							{
								if(!Config::Inst()->GetIsTraining())
								{
									UpdateAndClassify(it->second.session.at(0).ip_hdr.ip_src.s_addr);
								}
								else
								{
									UpdateAndStore(it->second.session.at(0).ip_hdr.ip_src.s_addr);
								}
							}
						}
						SessionTable[it->first].session.clear();
						SessionTable[it->first].fin = false;
					}
					//If this session is timed out
					else if(packetTime + Config::Inst()->GetTcpTimout()	< currentTime)
					{
						for (uint p = 0; p < (SessionTable[it->first].session).size(); p++)
						{
							UpdateSuspect(SessionTable[it->first].session[p]);
						}
						// Allow for continuous classification
						if(!Config::Inst()->GetClassificationTimeout())
						{
							if(!Config::Inst()->GetIsTraining())
							{
								UpdateAndClassify(it->second.session.at(0).ip_hdr.ip_src.s_addr);
							}
							else
							{
								UpdateAndStore(it->second.session.at(0).ip_hdr.ip_src.s_addr);
							}
						}
						SessionTable[it->first].session.clear();
						SessionTable[it->first].fin = false;
					}
				}
			}
		}
		pthread_rwlock_unlock(&sessionLock);
		//Check only once every TCP_CHECK_FREQ seconds
		sleep(Config::Inst()->GetTcpCheckFreq());
	}while(!Config::Inst()->GetReadPcap());

	//After a pcap file is read we do one iteration of this function to clear out the sessions
	//This is return is to prevent an error being thrown when there isn't one.
	if(Config::Inst()->GetReadPcap())
	{
		return NULL;
	}
	LOG(CRITICAL, "The code should never get here, something went very wrong.", "");
	return NULL;
}
}
