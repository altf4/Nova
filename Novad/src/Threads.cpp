//============================================================================
// Name        : Threads.cpp
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
// Description : Novad thread loops
//============================================================================

#include "WhitelistConfiguration.h"
#include "ClassificationEngine.h"
#include "ProtocolHandler.h"
#include "EvidenceTable.h"
#include "PacketCapture.h"
#include "Doppelganger.h"
#include "SuspectTable.h"
#include "FeatureSet.h"
#include "NovaUtil.h"
#include "Threads.h"
#include "Control.h"
#include "Logger.h"
#include "Config.h"
#include "Point.h"
#include "Novad.h"
#include "Lock.h"

#include <vector>
#include <math.h>
#include <time.h>
#include <string>
#include <errno.h>
#include <fstream>
#include <sstream>
#include <unistd.h>
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

extern time_t lastLoadTime;
extern time_t lastSaveTime;

//HS Vars
extern string dhcpListFile;
extern vector<string> haystackDhcpAddresses;
extern vector<string> whitelistIpAddresses;
extern vector<string> whitelistIpRanges;
extern vector<PacketCapture*> packetCaptures;

extern int honeydDHCPNotifyFd;
extern int honeydDHCPWatch;

extern int whitelistNotifyFd;
extern int whitelistWatch;

extern pthread_mutex_t packetCapturesLock;

extern EvidenceTable suspectEvidence;

extern Doppelganger *doppel;

namespace Nova
{
void *ClassificationLoop(void *ptr)
{
	MaskKillSignals();

	//Classification Loop
	do
	{
		sleep(Config::Inst()->GetClassificationTimeout());
		CheckForDroppedPackets();

		//Calculate the "true" Feature Set for each Suspect
		vector<SuspectIdentifier> updateKeys = suspects.GetKeys_of_ModifiedSuspects();
		for(uint i = 0; i < updateKeys.size(); i++)
		{
			UpdateAndClassify(updateKeys[i]);
		}
		doppel->UpdateDoppelganger();

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

void *UpdateIPFilter(void *ptr)
{
	MaskKillSignals();

	while(true)
	{
		if(honeydDHCPWatch > 0)
		{
			int BUF_LEN = (1024 *(sizeof(struct inotify_event)) + 16);
			char buf[BUF_LEN];

			// Blocking call, only moves on when the kernel notifies it that file has been changed
			int readLen = read(honeydDHCPNotifyFd, buf, BUF_LEN);
			if(readLen > 0)
			{
				honeydDHCPWatch = inotify_add_watch(honeydDHCPNotifyFd, dhcpListFile.c_str(),
						IN_CLOSE_WRITE | IN_MOVED_TO | IN_MODIFY | IN_DELETE);
				haystackDhcpAddresses = Config::GetHoneydIpAddresses(dhcpListFile);

				UpdateHaystackFeatures();

				{
					Lock lock(&packetCapturesLock);
					for(uint i = 0; i < packetCaptures.size(); i++)
					{
						try {
						string captureFilterString = ConstructFilterString(packetCaptures.at(i)->GetIdentifier());
							packetCaptures.at(i)->SetFilter(captureFilterString);
						}
						catch (Nova::PacketCaptureException &e)
						{
							LOG(ERROR, string("Unable to update capture filter: ") + e.what(), "");
						}
					}
				}
			}
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

	while(true)
	{
		if(whitelistWatch > 0)
		{
			int BUF_LEN = (1024 *(sizeof(struct inotify_event)) + 16);
			char buf[BUF_LEN];

			// Blocking call, only moves on when the kernel notifies it that file has been changed
			int readLen = read(whitelistNotifyFd, buf, BUF_LEN);
			if(readLen > 0)
			{
				whitelistWatch = inotify_add_watch(whitelistNotifyFd, Config::Inst()->GetPathWhitelistFile().c_str(),
						IN_CLOSE_WRITE | IN_MOVED_TO | IN_MODIFY | IN_DELETE);
				whitelistIpAddresses = WhitelistConfiguration::GetIps();
				whitelistIpRanges = WhitelistConfiguration::GetIpRanges();

				{
					Lock lock(&packetCapturesLock);
					for(uint i = 0; i < packetCaptures.size(); i++)
					{
						try {
							string captureFilterString = ConstructFilterString(packetCaptures.at(i)->GetIdentifier());
							packetCaptures.at(i)->SetFilter(captureFilterString);
						}
						catch (Nova::PacketCaptureException &e)
						{
							LOG(ERROR, string("Unable to update capture filter: ") + e.what(), "");
						}
					}
				}


				// Clear any suspects that were whitelisted from the GUIs
				/*
				 * TODO: Fix this. Disabled during switch to SuspectIdentifier objects instead of IPs for suspect references
				for(uint i = 0; i < whitelistIpAddresses.size(); i++)
				{
					if(suspects.Erase(inet_addr(whitelistIpAddresses.at(i).c_str())))
					{
						UpdateMessage *msg = new UpdateMessage(UPDATE_SUSPECT_CLEARED);
						msg->m_IPAddress = inet_addr(whitelistIpAddresses.at(i).c_str());
						NotifyUIs(msg,UPDATE_SUSPECT_CLEARED_ACK, -1);
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
