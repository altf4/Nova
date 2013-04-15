//============================================================================
// Name        : ProtocolHandler.cpp
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
// Description : Manages the message sending protocol to and from the Nova UI
//============================================================================

#include "ProtocolHandler.h"
#include "Config.h"
#include "Logger.h"
#include "Control.h"
#include "Novad.h"
#include "messaging/MessageManager.h"
#include "SuspectTable.h"
#include "Lock.h"

#include <sstream>
#include "pthread.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cerrno>
#include <stdio.h>
#include <stdlib.h>

#define BOOST_FILESYSTEM_VERSION 2
#include <boost/filesystem.hpp>

using namespace Nova;
using namespace std;

int IPCParentSocket = -1;

extern SuspectTable suspects;
extern SuspectTable suspectsSinceLastSave;
extern time_t startTime;

struct sockaddr_un msgRemote, msgLocal;
int UIsocketSize;

namespace Nova
{

void HandleExitRequest(Message *incoming)
{
	LOG(NOTICE, "Quitting: Got an exit request from the UI. Goodbye!",
			"Got a CONTROL_EXIT_REQUEST, quitting.");
	SaveAndExit(0);
}

void HandleClearAllRequest(Message *incoming)
{
	suspects.EraseAllSuspects();
	suspectsSinceLastSave.EraseAllSuspects();
	boost::filesystem::path delString = Config::Inst()->GetPathCESaveFile();
	bool successResult = true;
	try
	{
		boost::filesystem::remove(delString);
	}
	catch(boost::filesystem::filesystem_error const& e)
	{
		LOG(ERROR, ("Unable to delete CE state file." + string(e.what())),"");
		successResult = false;
	}

	if(successResult)
	{
		LOG(DEBUG, "Cleared all suspects due to UI request",
				"Got a CONTROL_CLEAR_ALL_REQUEST, cleared all suspects.");

		//First, send the reply (wth message ID) to just the original sender
		Message updateMessage(UPDATE_ALL_SUSPECTS_CLEARED);
		if(incoming->m_contents.has_m_messageid())
		{
			updateMessage.m_contents.set_m_messageid(incoming->m_contents.m_messageid());
		}
		MessageManager::Instance().WriteMessage(&updateMessage, incoming->m_contents.m_sessionindex());

		//Now send a generic message to the rest of the clients
		updateMessage.m_contents.clear_m_messageid();
		MessageManager::Instance().WriteMessageExcept(&updateMessage, incoming->m_contents.m_sessionindex());
	}
}

void HandleClearSuspectRequest(Message *incoming)
{
	bool result = suspects.Erase(incoming->m_contents.m_suspectid());

	if(!result)
	{
		LOG(DEBUG, "Failed to Erase suspect from the main suspect table.", "");
	}

	result = suspectsSinceLastSave.Erase(incoming->m_contents.m_suspectid());
	if(!result)
	{
		LOG(DEBUG, "Failed to Erase suspect from the unsaved suspect table.", "");
		// Send failure notice
		Message updateMessage;
		updateMessage.m_contents.set_m_type(UPDATE_SUSPECT_CLEARED);
		updateMessage.m_contents.set_m_success(false);
		if(incoming->m_contents.has_m_messageid())
		{
			updateMessage.m_contents.set_m_messageid(incoming->m_contents.m_messageid());
		}
		updateMessage.m_contents.mutable_m_suspectid()->CopyFrom(incoming->m_contents.m_suspectid());
		MessageManager::Instance().WriteMessage(&updateMessage, incoming->m_contents.m_sessionindex());
		return;
	}

	struct in_addr suspectAddress;
	suspectAddress.s_addr = ntohl(incoming->m_contents.m_suspectid().m_ip());

	LOG(DEBUG, "Cleared a suspect due to UI request",
			"Got a CONTROL_CLEAR_SUSPECT_REQUEST, cleared suspect: "
			+string(inet_ntoa(suspectAddress))+ " on interface " + incoming->m_contents.m_suspectid().m_ifname() + ".");

	//First, send the reply (wth message ID) to just the original sender
	Message updateMessage;
	updateMessage.m_contents.set_m_type(UPDATE_SUSPECT_CLEARED);
	updateMessage.m_contents.set_m_success(true);
	if(incoming->m_contents.has_m_messageid())
	{
		updateMessage.m_contents.set_m_messageid(incoming->m_contents.m_messageid());
	}
	updateMessage.m_contents.mutable_m_suspectid()->CopyFrom(incoming->m_contents.m_suspectid());
	MessageManager::Instance().WriteMessage(&updateMessage, 0);

	//Now send a generic message to the rest of the clients
	updateMessage.m_contents.clear_m_messageid();
	MessageManager::Instance().WriteMessageExcept(&updateMessage, incoming->m_contents.m_sessionindex());
}

void HandleSaveSuspectsRequest(Message *incoming)
{
	if(incoming->m_contents.m_filepath().length() == 0)
	{
		suspects.SaveSuspectsToFile(string("save.txt"));
	}
	else
	{
		suspects.SaveSuspectsToFile(string(incoming->m_contents.m_filepath()));
	}

	LOG(DEBUG, "Saved suspects to file due to UI request",
		"Got a CONTROL_SAVE_SUSPECTS_REQUEST, saved all suspects.");
}

void HandleReclassifyAllRequest(Message *incoming)
{
	Reload();

	LOG(DEBUG, "Reclassified all suspects due to UI request",
		"Got a CONTROL_RECLASSIFY_ALL_REQUEST, reclassified all suspects.");
}

void HandleStartCaptureRequest(Message *incoming)
{
	StartCapture();
}

void HandleStopCaptureRequest(Message *incoming)
{
	StopCapture();
}

void HandleRequestSuspectList(Message *incoming)
{
	Message reply;
	reply.m_contents.set_m_type(REQUEST_SUSPECTLIST_REPLY);
	reply.m_contents.set_m_listtype(incoming->m_contents.m_listtype());
	if(incoming->m_contents.has_m_messageid())
	{
		reply.m_contents.set_m_messageid(incoming->m_contents.m_messageid());
	}

	switch(incoming->m_contents.m_listtype())
	{
		case SUSPECTLIST_ALL:
		{
			vector<SuspectID_pb> benign = suspects.GetKeys_of_BenignSuspects();
			for(uint i = 0; i < benign.size(); i++)
			{
				SuspectID_pb *temp = reply.m_contents.add_m_suspectids();
				temp->CopyFrom(benign.at(i));
			}

			vector<SuspectID_pb> hostile = suspects.GetKeys_of_HostileSuspects();
			for(uint i = 0; i < hostile.size(); i++)
			{
				SuspectID_pb *temp = reply.m_contents.add_m_suspectids();
				temp->CopyFrom(hostile.at(i));
			}
			break;
		}
		case SUSPECTLIST_HOSTILE:
		{
			vector<SuspectID_pb> hostile = suspects.GetKeys_of_HostileSuspects();
			for(uint i = 0; i < hostile.size(); i++)
			{
				SuspectID_pb *temp = reply.m_contents.add_m_suspectids();
				temp->CopyFrom(hostile.at(i));
			}
			break;
		}
		case SUSPECTLIST_BENIGN:
		{
			vector<SuspectID_pb> benign = suspects.GetKeys_of_BenignSuspects();
			for(uint i = 0; i < benign.size(); i++)
			{
				SuspectID_pb *temp = reply.m_contents.add_m_suspectids();
				temp->CopyFrom(benign.at(i));
			}
			break;
		}
		default:
		{
			LOG(DEBUG, "UI sent us an invalid message", "Got an unexpected RequestMessage type");
			break;
		}
	}

	MessageManager::Instance().WriteMessage(&reply, incoming->m_contents.m_sessionindex());
}

void HandleRequestSuspect(Message *incoming)
{
	Message reply;
	reply.m_contents.set_m_type(REQUEST_SUSPECT_REPLY);
	if(incoming->m_contents.has_m_messageid())
	{
		reply.m_contents.set_m_messageid(incoming->m_contents.m_messageid());
	}

	Suspect tempSuspect;

	//If there was no requested suspect, then return an empty suspect (failure)
	if(incoming->m_contents.has_m_suspectid())
	{
		if(reply.m_contents.m_featuremode() == NO_FEATURE_DATA)
		{
			tempSuspect = suspects.GetShallowSuspect(incoming->m_contents.m_suspectid());
		}
		else
		{
			tempSuspect = suspects.GetSuspect(incoming->m_contents.m_suspectid());
		}
	}

	Suspect test;

	if(tempSuspect != test)
	{
		reply.m_suspects.push_back(&tempSuspect);
	}
	else
	{
		reply.m_contents.set_m_success(false);
	}

	MessageManager::Instance().WriteMessage(&reply, incoming->m_contents.m_sessionindex());
}

void HandleRequestAllSuspects(Message *incoming)
{
	Message reply;
	reply.m_contents.set_m_type(REQUEST_ALL_SUSPECTS_REPLY);
	if(incoming->m_contents.has_m_messageid())
	{
		reply.m_contents.set_m_messageid(incoming->m_contents.m_messageid());
	}

	vector<SuspectID_pb> keys;

	switch(incoming->m_contents.m_listtype())
	{
		case SUSPECTLIST_ALL:
		{
			keys = suspects.GetAllKeys();
			break;
		}
		case SUSPECTLIST_HOSTILE:
		{
			keys = suspects.GetKeys_of_HostileSuspects();
			break;
		}
		case SUSPECTLIST_BENIGN:
		{
			keys = suspects.GetKeys_of_BenignSuspects();
			break;
		}
		default:
		{
			LOG(DEBUG, "UI sent us an invalid message", "Got an unexpected RequestMessage type");
			break;
		}
	}

	//Get copies of the suspects
	vector<Suspect> suspectCopies;
	for(uint i = 0; i < keys.size(); i++)
	{
		suspectCopies.push_back(suspects.GetSuspect(keys[i]));
	}

	//Populate the message
	for(uint i = 0; i < suspectCopies.size(); i++)
	{
		reply.m_suspects.push_back(&suspectCopies[i]);
	}

	MessageManager::Instance().WriteMessage(&reply, incoming->m_contents.m_sessionindex());
}

void HandleRequestUptime(Message *incoming)
{
	Message reply;
	reply.m_contents.set_m_type(REQUEST_UPTIME_REPLY);
	if(incoming->m_contents.has_m_messageid())
	{
		reply.m_contents.set_m_messageid(incoming->m_contents.m_messageid());
	}
	reply.m_contents.set_m_starttime(startTime);
	MessageManager::Instance().WriteMessage(&reply, incoming->m_contents.m_sessionindex());
}

void HandlePing(Message *incoming)
{
	Message pong;
	if(incoming->m_contents.has_m_messageid())
	{
		pong.m_contents.set_m_messageid(incoming->m_contents.m_messageid());
	}
	pong.m_contents.set_m_type(REQUEST_PONG);
	MessageManager::Instance().WriteMessage(&pong, incoming->m_contents.m_sessionindex());
}

}
