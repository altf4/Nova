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

#include "WhitelistConfiguration.h"
#include "ClassificationEngine.h"
#include "ProtocolHandler.h"
#include "EvidenceTable.h"
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
// Suspects not yet written to the state file
extern SuspectTable suspectsSinceLastSave;

extern vector<struct sockaddr_in> hostAddrs;

//** Silent Alarm **
extern struct sockaddr_in serv_addr;

extern time_t lastLoadTime;
extern time_t lastSaveTime;

//Commented out to suppress unused warnings.
//extern string trainingCapFile;
//extern ofstream trainingFileStream;

//HS Vars
extern string dhcpListFile;
extern vector<string> haystackDhcpAddresses;
extern vector<string> whitelistIpAddresses;
extern vector<string> whitelistIpRanges;
extern vector<pcap_t *> handles;

extern int honeydDHCPNotifyFd;
extern int honeydDHCPWatch;

extern int whitelistNotifyFd;
extern int whitelistWatch;

extern ClassificationEngine *engine;
extern EvidenceTable suspectEvidence;

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
		CheckForDroppedPackets();

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

		// Get list of Suspects that need a classification and feature update
		vector<uint64_t> updateKeys = suspects.GetKeys_of_ModifiedSuspects();
		suspects.UpdateAllSuspects();
		for(uint i = 0; i < updateKeys.size(); i++)
		{
			UpdateAndStore(updateKeys[i]);
		}
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

		for(uint i = 0; i < hostAddrs.size(); i++)
		{
			//If this is from ourselves, then drop it.
			if(hostAddrs[i].sin_addr.s_addr == sendaddr.sin_addr.s_addr)
			{
				close(connectionSocket);
				continue;
			}
		}

		CryptBuffer(buf, bytesRead, DECRYPT);

		in_addr_t addr = 0;
		memcpy(&addr, buf, 4);
		uint64_t key = addr;
		Suspect *newSuspect = new Suspect();
		if(newSuspect->Deserialize(buf, MAX_MSG_SIZE, MAIN_FEATURE_DATA) == 0)
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
		if(honeydDHCPWatch > 0)
		{
			int BUF_LEN = (1024 * (sizeof(struct inotify_event)) + 16);
			char buf[BUF_LEN];
			char errbuf[PCAP_ERRBUF_SIZE];
			char filter_exp[64];
			struct bpf_program *fp = new struct bpf_program();

			bpf_u_int32 maskp; /* subnet mask */
			bpf_u_int32 netp; /* ip          */

			// Blocking call, only moves on when the kernel notifies it that file has been changed
			int readLen = read(honeydDHCPNotifyFd, buf, BUF_LEN);
			if(readLen > 0)
			{
				honeydDHCPWatch = inotify_add_watch(honeydDHCPNotifyFd, dhcpListFile.c_str(),
						IN_CLOSE_WRITE | IN_MOVED_TO | IN_MODIFY | IN_DELETE);
				haystackDhcpAddresses = GetIpAddresses(dhcpListFile);
				string haystackAddresses_csv = ConstructFilterString();
				for(uint i = 0; i < handles.size(); i++)
				{
					/* ask pcap for the network address and mask of the device */
					int ret = pcap_lookupnet(Config::Inst()->GetInterface(i).c_str(), &netp, &maskp, errbuf);
					if(ret == -1)
					{
						LOG(ERROR, "Unable to start packet capture.",
							"Unable to get the network address and mask: "+string(strerror(errno)));
						exit(EXIT_FAILURE);
					}

					if(pcap_compile(handles[i], fp, haystackAddresses_csv.data(), 0, maskp) == -1)
					{
						LOG(ERROR, "Unable to enable packet capture.",
							"Couldn't parse pcap filter: "+ string(filter_exp) + " " + pcap_geterr(handles[i]));
					}
					if(pcap_setfilter(handles[i], fp) == -1)
					{
						LOG(ERROR, "Unable to enable packet capture.",
							"Couldn't install pcap filter: "+ string(filter_exp) + " " + pcap_geterr(handles[i]));
					}
					//Free the compiled filter program after assignment, it is no longer needed after set filter
					pcap_freecode(fp);
				}
			}
			delete fp;
		}
		else
		{
			// This is the case when there's no file to watch, just sleep and wait for it to
			// be created by honeyd when it starts up.
			sleep(2);
			honeydDHCPWatch = inotify_add_watch(honeydDHCPNotifyFd, dhcpListFile.c_str(),
					IN_CLOSE_WRITE | IN_MOVED_TO | IN_MODIFY | IN_DELETE);
		}
	}
	return NULL;
}

void *UpdateWhitelistIPFilter(void *ptr)
{
	MaskKillSignals();

	while (true)
	{
		if(whitelistWatch > 0)
		{
			int BUF_LEN = (1024 * (sizeof(struct inotify_event)) + 16);
			char buf[BUF_LEN];
			struct bpf_program fp; /* The compiled filter expression */
			char filter_exp[64];
			char errbuf[PCAP_ERRBUF_SIZE];

			bpf_u_int32 maskp; /* subnet mask */
			bpf_u_int32 netp; /* ip          */

			// Blocking call, only moves on when the kernel notifies it that file has been changed
			int readLen = read(whitelistNotifyFd, buf, BUF_LEN);
			if(readLen > 0)
			{
				whitelistWatch = inotify_add_watch(whitelistNotifyFd, Config::Inst()->GetPathWhitelistFile().c_str(),
						IN_CLOSE_WRITE | IN_MOVED_TO | IN_MODIFY | IN_DELETE);
				whitelistIpAddresses = WhitelistConfiguration::GetIps();
				whitelistIpRanges = WhitelistConfiguration::GetIpRanges();
				string filterString = ConstructFilterString();
				for(uint i = 0; i < handles.size(); i++)
				{

					/* ask pcap for the network address and mask of the device */
					int ret = pcap_lookupnet(Config::Inst()->GetInterface(i).c_str(), &netp, &maskp, errbuf);
					if(ret == -1)
					{
						LOG(ERROR, "Unable to start packet capture.",
							"Unable to get the network address and mask: "+string(strerror(errno)));
						exit(EXIT_FAILURE);
					}

					if(pcap_compile(handles[i], &fp, filterString.data(), 0, maskp) == -1)
					{
						LOG(ERROR, "Unable to enable packet capture.",
							"Couldn't parse pcap filter: "+ string(filter_exp) + " " + pcap_geterr(handles[i]));
					}
					if(pcap_setfilter(handles[i], &fp) == -1)
					{
						LOG(ERROR, "Unable to enable packet capture.",
							"Couldn't install pcap filter: "+ string(filter_exp) + " " + pcap_geterr(handles[i]));
					}
					pcap_freecode(&fp);

					// Clear any suspects that were whitelisted from the GUIs
					for (uint i = 0; i < whitelistIpAddresses.size(); i++)
					{
					if (suspects.Erase(inet_addr(whitelistIpAddresses.at(i).c_str())))
					{
						UpdateMessage *msg = new UpdateMessage(UPDATE_SUSPECT_CLEARED, DIRECTION_TO_UI);
						msg->m_IPAddress = inet_addr(whitelistIpAddresses.at(i).c_str());
						NotifyUIs(msg,UPDATE_SUSPECT_CLEARED_ACK, -1);
					}
				}

				}

				/*
				// TODO: Should we clear IP range whitelisted suspects? Could be a huge number of clears...
				// This doesn't work yet.
				for (uint i = 0; i < whitelistIpRanges.size(); i++)
				{
					uint32_t ip = htonl(inet_addr(WhitelistConfiguration::GetIp(whitelistIpRanges.at(i)).c_str()));

					string netmask = WhitelistConfiguration::GetSubnet(whitelistIpRanges.at(i));
					uint32_t mask;
					if (netmask != "")
					{
						mask = htonl(inet_addr(netmask.c_str()));
					}

					while (mask != ~0)
					{
						if (suspects.Erase(ip))
						{
							UpdateMessage *msg = new UpdateMessage(UPDATE_SUSPECT_CLEARED, DIRECTION_TO_UI);
							msg->m_IPAddress = ip;
							NotifyUIs(msg,UPDATE_SUSPECT_CLEARED_ACK, -1);

							cout << "erased" << endl;
						}

						in_addr foo;
						foo.s_addr = ntohl(ip);
						cout << "Attempted to erase " << inet_ntoa(foo) << endl;

						ip++;
						mask++;
					}

				}
				*/
			}
		}
		else
		{
			// This is the case when there's no file to watch, just sleep and wait for it to
			// be created by honeyd when it starts up.
			sleep(3);
			whitelistWatch = inotify_add_watch(whitelistNotifyFd, Config::Inst()->GetPathWhitelistFile().c_str(),
					IN_CLOSE_WRITE | IN_MOVED_TO | IN_MODIFY | IN_DELETE);
		}
	}

	return NULL;
}

void *StartPcapLoop(void *ptr)
{
	u_char * index = new u_char(*(u_char *)ptr);
	if((*index >= handles.size()) || (handles[*index] == NULL))
	{
		LOG(CRITICAL, "Invalid pcap handle provided, unable to start pcap loop!", "");
		exit(EXIT_FAILURE);
	}
	pthread_t consumer;
	pthread_create(&consumer, NULL, ConsumerLoop, NULL);
	pthread_detach(consumer);
	pcap_loop(handles[*index], -1, Packet_Handler, index);
	return NULL;
}

void *ConsumerLoop(void *ptr)
{
	while(true)
	{
		//Blocks on a mutex/condition if there's no evidence to process
		Evidence *cur = suspectEvidence.GetEvidence();

		//Do not deallocate evidence, we still need it
		suspectsSinceLastSave.ProcessEvidence(cur, true);

		//Consume evidence
		suspects.ProcessEvidence(cur, false);
	}
	return NULL;
}

}
