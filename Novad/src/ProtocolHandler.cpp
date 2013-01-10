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
#include "messaging/messages/UpdateMessage.h"
#include "messaging/messages/ControlMessage.h"
#include "messaging/messages/RequestMessage.h"
#include "messaging/messages/ErrorMessage.h"
#include "messaging/MessageManager.h"
#include "messaging/Ticket.h"
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

void HandleControlMessage(ControlMessage &controlMessage, Ticket &ticket)
{
	switch(controlMessage.m_controlType)
	{
		case CONTROL_EXIT_REQUEST:
		{
			//TODO: Check for any reason why might not want to exit
			ControlMessage exitReply(CONTROL_EXIT_REPLY);
			exitReply.m_success = true;

			MessageManager::Instance().WriteMessage(ticket, &exitReply);

			LOG(NOTICE, "Quitting: Got an exit request from the UI. Goodbye!",
					"Got a CONTROL_EXIT_REQUEST, quitting.");
			SaveAndExit(0);

			break;
		}
		case CONTROL_CLEAR_ALL_REQUEST:
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

			ControlMessage clearAllSuspectsReply(CONTROL_CLEAR_ALL_REPLY);
			clearAllSuspectsReply.m_success = successResult;
			MessageManager::Instance().WriteMessage(ticket, &clearAllSuspectsReply);

			if(successResult)
			{
				LOG(DEBUG, "Cleared all suspects due to UI request",
						"Got a CONTROL_CLEAR_ALL_REQUEST, cleared all suspects.");

				UpdateMessage *updateMessage = new UpdateMessage(UPDATE_ALL_SUSPECTS_CLEARED);
				NotifyUIs(updateMessage, UPDATE_ALL_SUSPECTS_CLEARED_ACK, ticket.m_socketFD);
			}

			break;
		}
		case CONTROL_CLEAR_SUSPECT_REQUEST:
		{
			bool result;

			result = suspects.Erase(controlMessage.m_suspectAddress);

			if (!result)
			{
				LOG(DEBUG, "Failed to Erase suspect from the main suspect table.", "");
			}

			result = suspectsSinceLastSave.Erase(controlMessage.m_suspectAddress);
			if (!result)
			{
				LOG(DEBUG, "Failed to Erase suspect from the unsaved suspect table.", "");
			}


			//RefreshStateFile();

			//TODO: Should check for errors here and return result
			ControlMessage clearSuspectReply(CONTROL_CLEAR_SUSPECT_REPLY);
			clearSuspectReply.m_success = true;
			MessageManager::Instance().WriteMessage(ticket, &clearSuspectReply);

			struct in_addr suspectAddress;
			suspectAddress.s_addr = ntohl(controlMessage.m_suspectAddress.m_ip);

			LOG(DEBUG, "Cleared a suspect due to UI request",
					"Got a CONTROL_CLEAR_SUSPECT_REQUEST, cleared suspect: "+string(inet_ntoa(suspectAddress))+ "on interface " + controlMessage.m_suspectAddress.m_interface + ".");

			UpdateMessage *updateMessage = new UpdateMessage(UPDATE_SUSPECT_CLEARED);
			updateMessage->m_IPAddress = controlMessage.m_suspectAddress;
			cout << "Sending UpdateMessage with suspect cleared on interface " << updateMessage->m_IPAddress.m_interface << endl;
			NotifyUIs(updateMessage, UPDATE_SUSPECT_CLEARED_ACK, ticket.m_socketFD);

			break;
		}
		case CONTROL_SAVE_SUSPECTS_REQUEST:
		{
			if(strlen(controlMessage.m_filePath) == 0)
			{
				//TODO: possibly make a logger call here for incorrect file name, probably need to check name in a more
				// comprehensive way. This may not be needed, as I can't see a way for execution to get here,
				// but better safe than sorry
				suspects.SaveSuspectsToFile(string("save.txt")); //TODO: Should check for errors here and return results
			}
			else
			{
				suspects.SaveSuspectsToFile(string(controlMessage.m_filePath));
				//TODO: Should check for errors here and return result
			}

			ControlMessage saveSuspectsReply(CONTROL_SAVE_SUSPECTS_REPLY);
			saveSuspectsReply.m_success = true;
			MessageManager::Instance().WriteMessage(ticket, &saveSuspectsReply);

			LOG(DEBUG, "Saved suspects to file due to UI request",
				"Got a CONTROL_SAVE_SUSPECTS_REQUEST, saved all suspects.");
			break;
		}
		case CONTROL_RECLASSIFY_ALL_REQUEST:
		{
			Reload(); //TODO: Should check for errors here and return result

			ControlMessage reclassifyAllReply(CONTROL_RECLASSIFY_ALL_REPLY);
			reclassifyAllReply.m_success = true;
			MessageManager::Instance().WriteMessage(ticket, &reclassifyAllReply);

			LOG(DEBUG, "Reclassified all suspects due to UI request",
				"Got a CONTROL_RECLASSIFY_ALL_REQUEST, reclassified all suspects.");
			break;
		}
		case CONTROL_CONNECT_REQUEST:
		{
			ControlMessage connectReply(CONTROL_CONNECT_REPLY);
			connectReply.m_success = true;
			MessageManager::Instance().WriteMessage(ticket, &connectReply);
			break;
		}
		case CONTROL_DISCONNECT_NOTICE:
		{
			ControlMessage disconnectReply(CONTROL_DISCONNECT_ACK);
			MessageManager::Instance().WriteMessage(ticket, &disconnectReply);

			//TODO: Actually shut down the connection?

			LOG(DEBUG, "The UI hung up", "Got a CONTROL_DISCONNECT_NOTICE, closed down socket.");

			break;
		}
		case CONTROL_START_CAPTURE:
		{
			ControlMessage ack(CONTROL_START_CAPTURE_ACK);
			MessageManager::Instance().WriteMessage(ticket, &ack);

			StartCapture();
			break;
		}
		case CONTROL_STOP_CAPTURE:
		{
			ControlMessage ack(CONTROL_STOP_CAPTURE_ACK);
			MessageManager::Instance().WriteMessage(ticket, &ack);

			StopCapture();

			break;
		}
		default:
		{
			LOG(DEBUG, "UI sent us an invalid message","Got an unexpected ControlMessage type");
			break;
		}
	}
}

void HandleRequestMessage(RequestMessage &msg, Ticket &ticket)
{
	switch(msg.m_requestType)
	{
		case REQUEST_SUSPECTLIST:
		{
			RequestMessage reply(REQUEST_SUSPECTLIST_REPLY);
			reply.m_listType = msg.m_listType;

			switch(msg.m_listType)
			{
				case SUSPECTLIST_ALL:
				{
					vector<SuspectIdentifier> benign = suspects.GetKeys_of_BenignSuspects();
					for(uint i = 0; i < benign.size(); i++)
					{
						reply.m_suspectList.push_back(benign.at(i));
					}

					vector<SuspectIdentifier> hostile = suspects.GetKeys_of_HostileSuspects();
					for(uint i = 0; i < hostile.size(); i++)
					{
						reply.m_suspectList.push_back(hostile.at(i));
					}
					break;
				}
				case SUSPECTLIST_HOSTILE:
				{
					vector<SuspectIdentifier> hostile = suspects.GetKeys_of_HostileSuspects();
					for(uint i = 0; i < hostile.size(); i++)
					{
						reply.m_suspectList.push_back(hostile.at(i));
					}
					break;
				}
				case SUSPECTLIST_BENIGN:
				{
					vector<SuspectIdentifier> benign = suspects.GetKeys_of_BenignSuspects();
					for(uint i = 0; i < benign.size(); i++)
					{
						reply.m_suspectList.push_back(benign.at(i));
					}
					break;
				}
				default:
				{
					LOG(DEBUG, "UI sent us an invalid message", "Got an unexpected RequestMessage type");
					break;
				}
			}


			MessageManager::Instance().WriteMessage(ticket, &reply);
			break;
		}
		case REQUEST_SUSPECT:
		{
			RequestMessage reply(REQUEST_SUSPECT_REPLY);
			Suspect tempSuspect = suspects.GetShallowSuspect(msg.m_suspectAddress);
			reply.m_suspect = &tempSuspect;
			MessageManager::Instance().WriteMessage(ticket, &reply);

			break;
		}
		case REQUEST_SUSPECT_WITHDATA:
		{
			RequestMessage reply(REQUEST_SUSPECT_WITHDATA_REPLY);
			Suspect tempSuspect = suspects.GetSuspect(msg.m_suspectAddress);
			reply.m_suspect = &tempSuspect;
			MessageManager::Instance().WriteMessage(ticket, &reply);

			break;
		}
		case REQUEST_UPTIME:
		{
			RequestMessage reply(REQUEST_UPTIME_REPLY);
			reply.m_startTime = startTime;
			MessageManager::Instance().WriteMessage(ticket, &reply);

			break;
		}
		case REQUEST_PING:
		{
			RequestMessage connectReply(REQUEST_PONG);
			MessageManager::Instance().WriteMessage(ticket, &connectReply);

			//TODO: This was too noisy. Even at the debug level. So it's ignored. Maybe bring it back?
			//LOG(DEBUG, "Got a Ping from UI. We're alive!",
			//	"Got a CONTROL_PING, sent a PONG.");

			break;
		}
		default:
		{
			LOG(DEBUG, "UI sent us an invalid message", "Got an unexpected RequestMessage type");
			break;
		}

	}
}

void SendSuspectToUIs(Suspect *suspect)
{
	vector<int> sockets = MessageManager::Instance().GetSocketList();

	for(uint i = 0; i < sockets.size(); ++i)
	{
		Ticket ticket = MessageManager::Instance().StartConversation(sockets[i]);

		//If the ticket came back bad, then ignore this one. The endpoint must have hung up before we got to it.
		if(ticket.m_socketFD == -1)
		{
			continue;
		}

		UpdateMessage suspectUpdate(UPDATE_SUSPECT);
		suspectUpdate.m_suspect = suspect;
		if(!MessageManager::Instance().WriteMessage(ticket, &suspectUpdate))
		{
			LOG(DEBUG, string("Failed to send a suspect to the UI: ")+ suspect->GetIpString(), "");
			continue;
		}

		Message *suspectReply = MessageManager::Instance().ReadMessage(ticket);
		if(suspectReply->m_messageType != UPDATE_MESSAGE)
		{
			suspectReply->DeleteContents();
			delete suspectReply;
			LOG(DEBUG, string("Failed to send a suspect to the UI: ")+ suspect->GetIpString(), "");
			continue;
		}
		UpdateMessage *suspectCallback = (UpdateMessage*)suspectReply;
		if(suspectCallback->m_updateType != UPDATE_SUSPECT_ACK)
		{
			suspectReply->DeleteContents();
			delete suspectReply;
			LOG(DEBUG, string("Failed to send a suspect to the UI: ")+ suspect->GetIpString(), "");
			continue;
		}
		delete suspectCallback;
		//LOG(DEBUG, string("Sent a suspect to the UI: ")+suspect->GetIpString(), "");
	}
}

void NotifyUIs(UpdateMessage *updateMessage, enum UpdateType ackType, int socketFD_sender)
{
	if(updateMessage == NULL)
	{
		return;
	}

	struct UI_NotificationPackage *args = new UI_NotificationPackage();
	args->m_updateMessage = updateMessage;
	args->m_ackType = ackType;
	args->m_socketFD_sender = socketFD_sender;

	pthread_t notifyThread;
	int errorCode = pthread_create(&notifyThread, NULL, NotifyUIsHelper, args);
	if(errorCode != 0 )
	{
		if(errorCode == EAGAIN)
		{
			LOG(WARNING, "pthread error: Could not make a new thread!", "pthread errno: EAGAIN. PTHREAD_THREADS_MAX would have been exceeded or "
					"not enough system resources.");
		}
		if(errorCode == EPERM)
		{
			LOG(WARNING, "pthread error: Insufficient permissions", "pthread errno: EPERM. Insufficient permissions");
		}
	}
	pthread_detach(notifyThread);
}

void *NotifyUIsHelper(void *ptr)
{
	struct UI_NotificationPackage *arguments = (struct UI_NotificationPackage*)ptr;

	//Notify all of the UIs
	vector<int> sockets = MessageManager::Instance().GetSocketList();
	for(uint i = 0; i < sockets.size(); ++i)
	{
		//Don't send an update to the UI that gave us the request
		if(sockets[i] == arguments->m_socketFD_sender)
		{
			continue;
		}

		Ticket ticket = MessageManager::Instance().StartConversation(sockets[i]);

		if(!MessageManager::Instance().WriteMessage(ticket, arguments->m_updateMessage))
		{
			LOG(DEBUG, "Failed to send message to UI", "Failed to send a Clear All Suspects message to a UI");
			continue;
		}

		Message *suspectReply = MessageManager::Instance().ReadMessage(ticket);
		if(suspectReply->m_messageType != UPDATE_MESSAGE)
		{
			suspectReply->DeleteContents();
			delete suspectReply;
			LOG(DEBUG, "Failed to send message to UI", "Got the wrong message type in response after sending a Clear All Suspects Notify");
			continue;
		}
		UpdateMessage *suspectCallback = (UpdateMessage*)suspectReply;
		if(suspectCallback->m_updateType != arguments->m_ackType)
		{
			suspectReply->DeleteContents();
			delete suspectReply;
			LOG(DEBUG, "Failed to send message to UI", "Got the wrong UpdateMessage subtype in response after sending a Clear All Suspects Notify");
			continue;
		}
		delete suspectCallback;
	}

	arguments->m_updateMessage->DeleteContents();
	delete arguments->m_updateMessage;
	delete arguments;
	return NULL;
}

}
