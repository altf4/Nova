//============================================================================
// Name        : ProtocolHandler.cpp
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
#include "SuspectTable.h"
#include "Lock.h"

#include "pthread.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <cerrno>
#include <stdio.h>
#include <stdlib.h>

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
//Launches a UI Handling thread, and returns
bool Spawn_UI_Handler()
{
	int len;
	string inKeyPath = Config::Inst()->GetPathHome() + "/keys" + NOVAD_LISTEN_FILENAME;
	string outKeyPath = Config::Inst()->GetPathHome() + "/keys" + UI_LISTEN_FILENAME;

    if((IPCParentSocket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
		LOG(ERROR, "Failed to connect to UI", "socket: "+string(strerror(errno)));
    	return false;
    }

    msgLocal.sun_family = AF_UNIX;
    strncpy(msgLocal.sun_path, inKeyPath.c_str(), inKeyPath.length());
    unlink(msgLocal.sun_path);
    len = strlen(msgLocal.sun_path) + sizeof(msgLocal.sun_family);

    if(::bind(IPCParentSocket, (struct sockaddr *)&msgLocal, len) == -1)
    {
		LOG(ERROR, "Failed to connect to UI", "bind: "+string(strerror(errno)));
    	close(IPCParentSocket);
    	return false;
    }

    if(listen(IPCParentSocket, SOMAXCONN) == -1)
    {
		LOG(ERROR, "Failed to connect to UI", "listen: "+string(strerror(errno)));
		close(IPCParentSocket);
    	return false;
    }

    pthread_t helperThread;
    pthread_create(&helperThread, NULL, Handle_UI_Helper, NULL);
    pthread_detach(helperThread);
    return true;
}

void *Handle_UI_Helper(void *ptr)
{
    while(true)
    {
    	int msgSocketFD;

    	//Blocking call
		if((msgSocketFD = accept(IPCParentSocket, (struct sockaddr *)&msgRemote, (socklen_t*)&UIsocketSize)) == -1)
		{
			LOG(ERROR, "Failed to connect to UI", "accept: " + string(strerror(errno)));
			close(IPCParentSocket);
			return false;
		}
		else
		{
			pthread_t UI_thread;
			int *msgSocketPtr = new int;
			*msgSocketPtr = msgSocketFD;
			pthread_create(&UI_thread, NULL, Handle_UI_Thread, (void*)msgSocketPtr);
			pthread_detach(UI_thread);
		}
    }

    return NULL;
}

void *Handle_UI_Thread(void *socketVoidPtr)
{
	//Get the argument out, put it on the stack, free it from the heap so we don't forget
	int *socketIntPtr = (int*)socketVoidPtr;
	int controlSocket = *socketIntPtr;
	delete socketIntPtr;

	MessageManager::Instance().StartSocket(controlSocket);

	bool keepLooping = true;

	while(keepLooping)
	{
		//Wait for a callback to occur
		//If register comes back false, then the socket was closed. So exit the thread
		if(!MessageManager::Instance().RegisterCallback(controlSocket))
		{
			keepLooping = false;
			continue;
		}

		Message *message = Message::ReadMessage(controlSocket, DIRECTION_TO_NOVAD, REPLY_TIMEOUT);
		switch(message->m_messageType)
		{
			case CONTROL_MESSAGE:
			{
				ControlMessage *controlMessage = (ControlMessage*)message;
				HandleControlMessage(*controlMessage, controlSocket);
				delete controlMessage;
				break;
			}
			case REQUEST_MESSAGE:
			{
				RequestMessage *msg = (RequestMessage*)message;
				HandleRequestMessage(*msg, controlSocket);
				delete msg;
				break;
			}
			case ERROR_MESSAGE:
			{
				ErrorMessage *errorMessage = (ErrorMessage*)message;
				switch(errorMessage->m_errorType)
				{
					case ERROR_SOCKET_CLOSED:
					{
						LOG(DEBUG, "The UI hung up","UI socket closed uncleanly, exiting this thread");
						keepLooping = false;
						break;;
					}
					case ERROR_MALFORMED_MESSAGE:
					{
						LOG(NOTICE, "There was an error reading a message from the UI", "Got a message but it was not deserialized correctly");
						break;
					}
					case ERROR_UNKNOWN_MESSAGE_TYPE:
					{
						LOG(NOTICE, "There was an error reading a message from the UI", "Received an unknown message type.");
						break;
					}
					case ERROR_PROTOCOL_MISTAKE:
					{
						LOG(NOTICE, "We sent a bad message to the UI", "Received an ERROR_PROTOCOL_MISTAKE.");
						break;
					}
					default:
					{
						LOG(NOTICE, "There was an error reading a message from the UI", "Unknown error type. Should see this!");
						break;
					}
				}
				delete errorMessage;
				break;

			}
			default:
			{
				//There was an error reading this message
				LOG(DEBUG, "There was an error reading a message from the UI", "Invalid message type");
				delete message;
				break;
			}
		}
	}

	return NULL;
}

void HandleControlMessage(ControlMessage &controlMessage, int socketFD)
{
	switch(controlMessage.m_controlType)
	{
		case CONTROL_EXIT_REQUEST:
		{
			//TODO: Check for any reason why might not want to exit
			ControlMessage exitReply(CONTROL_EXIT_REPLY, DIRECTION_TO_NOVAD);
			exitReply.m_success = true;

			Message::WriteMessage(&exitReply, socketFD);

			LOG(NOTICE, "Quitting: Got an exit request from the UI. Goodbye!",
					"Got a CONTROL_EXIT_REQUEST, quitting.");
			SaveAndExit(0);

			break;
		}
		case CONTROL_CLEAR_ALL_REQUEST:
		{
			suspects.EraseAllSuspects();
			suspectsSinceLastSave.EraseAllSuspects();
			string delString = "rm -f " + Config::Inst()->GetPathCESaveFile();
			bool successResult = true;
			if(system(delString.c_str()) == -1)
			{
				LOG(ERROR, "Unable to delete CE state file. System call to rm failed.","");
				successResult = false;
			}

			ControlMessage clearAllSuspectsReply(CONTROL_CLEAR_ALL_REPLY, DIRECTION_TO_NOVAD);
			clearAllSuspectsReply.m_success = successResult;
			Message::WriteMessage(&clearAllSuspectsReply, socketFD);

			if(successResult)
			{
				LOG(DEBUG, "Cleared all suspects due to UI request",
						"Got a CONTROL_CLEAR_ALL_REQUEST, cleared all suspects.");

				UpdateMessage *updateMessage = new UpdateMessage(UPDATE_ALL_SUSPECTS_CLEARED, DIRECTION_TO_UI);
				NotifyUIs(updateMessage, UPDATE_ALL_SUSPECTS_CLEARED_ACK, socketFD);
			}

			break;
		}
		case CONTROL_CLEAR_SUSPECT_REQUEST:
		{
			suspects.Erase(controlMessage.m_suspectAddress);
			suspectsSinceLastSave.Erase(controlMessage.m_suspectAddress);

			RefreshStateFile();

			//TODO: Should check for errors here and return result
			ControlMessage clearSuspectReply(CONTROL_CLEAR_SUSPECT_REPLY, DIRECTION_TO_NOVAD);
			clearSuspectReply.m_success = true;
			Message::WriteMessage(&clearSuspectReply, socketFD);

			struct in_addr suspectAddress;
			suspectAddress.s_addr = controlMessage.m_suspectAddress;

			LOG(DEBUG, "Cleared a suspect due to UI request",
					"Got a CONTROL_CLEAR_SUSPECT_REQUEST, cleared suspect: "+string(inet_ntoa(suspectAddress))+".");

			UpdateMessage *updateMessage = new UpdateMessage(UPDATE_SUSPECT_CLEARED, DIRECTION_TO_UI);
			updateMessage->m_IPAddress = controlMessage.m_suspectAddress;
			NotifyUIs(updateMessage, UPDATE_SUSPECT_CLEARED_ACK, socketFD);

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

			ControlMessage saveSuspectsReply(CONTROL_SAVE_SUSPECTS_REPLY, DIRECTION_TO_NOVAD);
			saveSuspectsReply.m_success = true;
			Message::WriteMessage(&saveSuspectsReply, socketFD);

			LOG(DEBUG, "Saved suspects to file due to UI request",
				"Got a CONTROL_SAVE_SUSPECTS_REQUEST, saved all suspects.");
			break;
		}
		case CONTROL_RECLASSIFY_ALL_REQUEST:
		{
			Reload(); //TODO: Should check for errors here and return result

			ControlMessage reclassifyAllReply(CONTROL_RECLASSIFY_ALL_REPLY, DIRECTION_TO_NOVAD);
			reclassifyAllReply.m_success = true;
			Message::WriteMessage(&reclassifyAllReply, socketFD);

			LOG(DEBUG, "Reclassified all suspects due to UI request",
				"Got a CONTROL_RECLASSIFY_ALL_REQUEST, reclassified all suspects.");
			break;
		}
		case CONTROL_CONNECT_REQUEST:
		{
			ControlMessage connectReply(CONTROL_CONNECT_REPLY, DIRECTION_TO_NOVAD);
			connectReply.m_success = true;
			Message::WriteMessage(&connectReply, socketFD);
			break;
		}
		case CONTROL_DISCONNECT_NOTICE:
		{
			ControlMessage disconnectReply(CONTROL_DISCONNECT_ACK, DIRECTION_TO_NOVAD);
			Message::WriteMessage(&disconnectReply, socketFD);

			MessageManager::Instance().CloseSocket(socketFD);

			LOG(NOTICE, "The UI hung up", "Got a CONTROL_DISCONNECT_NOTICE, closed down socket.");

			break;
		}
		default:
		{
			LOG(DEBUG, "UI sent us an invalid message","Got an unexpected ControlMessage type");
			break;
		}
	}
}

void HandleRequestMessage(RequestMessage &msg, int socketFD)
{
	switch(msg.m_requestType)
	{
		case REQUEST_SUSPECTLIST:
		{
			RequestMessage reply(REQUEST_SUSPECTLIST_REPLY, DIRECTION_TO_NOVAD);
			reply.m_listType = msg.m_listType;

			switch (msg.m_listType)
			{
				case SUSPECTLIST_ALL:
				{
					vector<uint64_t> benign = suspects.GetKeys_of_BenignSuspects();
					for (uint i = 0; i < benign.size(); i++)
					{
						reply.m_suspectList.push_back((in_addr_t)benign.at(i));
					}

					vector<uint64_t> hostile = suspects.GetKeys_of_HostileSuspects();
					for (uint i = 0; i < hostile.size(); i++)
					{
						reply.m_suspectList.push_back((in_addr_t)hostile.at(i));
					}
					break;
				}
				case SUSPECTLIST_HOSTILE:
				{
					vector<uint64_t> hostile = suspects.GetKeys_of_HostileSuspects();
					for (uint i = 0; i < hostile.size(); i++)
					{
						reply.m_suspectList.push_back((in_addr_t)hostile.at(i));
					}
					break;
				}
				case SUSPECTLIST_BENIGN:
				{
					vector<uint64_t> benign = suspects.GetKeys_of_BenignSuspects();
					for (uint i = 0; i < benign.size(); i++)
					{
						reply.m_suspectList.push_back((in_addr_t)benign.at(i));
					}
					break;
				}
				default:
				{
					LOG(DEBUG, "UI sent us an invalid message", "Got an unexpected RequestMessage type");
					break;
				}
			}


			Message::WriteMessage(&reply, socketFD);
			break;
		}
		case REQUEST_SUSPECT:
		{
			RequestMessage reply(REQUEST_SUSPECT_REPLY, DIRECTION_TO_NOVAD);
			Suspect tempSuspect = suspects.GetSuspect(msg.m_suspectAddress);
			reply.m_suspect = &tempSuspect;
			Message::WriteMessage(&reply, socketFD);

			break;
		}
		case REQUEST_UPTIME:
		{
			RequestMessage reply(REQUEST_UPTIME_REPLY, DIRECTION_TO_NOVAD);
			reply.m_uptime = time(NULL) - startTime;
			Message::WriteMessage(&reply, socketFD);

			break;
		}
		case REQUEST_PING:
		{
			RequestMessage connectReply(REQUEST_PONG, DIRECTION_TO_NOVAD);
			Message::WriteMessage(&connectReply, socketFD);

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
		Lock lock = MessageManager::Instance().UseSocket(sockets[i]);

		UpdateMessage suspectUpdate(UPDATE_SUSPECT, DIRECTION_TO_UI);
		suspectUpdate.m_suspect = suspect;
		if(!Message::WriteMessage(&suspectUpdate, sockets[i]))
		{
			LOG(DEBUG, string("Failed to send a suspect to the UI: ")+ inet_ntoa(suspect->GetInAddr()), "");
			continue;
		}

		Message *suspectReply = Message::ReadMessage(sockets[i], DIRECTION_TO_UI, REPLY_TIMEOUT);
		if(suspectReply->m_messageType != UPDATE_MESSAGE)
		{
			delete suspectReply;
			LOG(DEBUG, string("Failed to send a suspect to the UI: ")+ inet_ntoa(suspect->GetInAddr()), "");
			continue;
		}
		UpdateMessage *suspectCallback = (UpdateMessage*)suspectReply;
		if(suspectCallback->m_updateType != UPDATE_SUSPECT_ACK)
		{
			delete suspectCallback;
			LOG(DEBUG, string("Failed to send a suspect to the UI: ")+ inet_ntoa(suspect->GetInAddr()), "");
			continue;
		}
		delete suspectCallback;
		LOG(DEBUG, string("Sent a suspect to the UI: ")+ inet_ntoa(suspect->GetInAddr()), "");
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

		if(!Message::WriteMessage(arguments->m_updateMessage, sockets[i]))
		{
			LOG(NOTICE, "Failed to send message to UI", "Failed to send a Clear All Suspects message to a UI");
			continue;
		}

		Message *suspectReply = Message::ReadMessage(sockets[i], DIRECTION_TO_UI, REPLY_TIMEOUT);
		if(suspectReply->m_messageType != UPDATE_MESSAGE)
		{
			delete suspectReply;
			LOG(NOTICE, "Failed to send message to UI", "Got the wrong message type in response after sending a Clear All Suspects Notify");
			continue;
		}
		UpdateMessage *suspectCallback = (UpdateMessage*)suspectReply;
		if(suspectCallback->m_updateType != arguments->m_ackType)
		{
			delete suspectCallback;
			LOG(NOTICE, "Failed to send message to UI", "Got the wrong UpdateMessage subtype in response after sending a Clear All Suspects Notify");
			continue;
		}
		delete suspectCallback;
	}

	delete arguments->m_updateMessage;
	delete arguments;
	return NULL;
}

}
