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
#include "messages/CallbackMessage.h"
#include "messages/ControlMessage.h"
#include "messages/RequestMessage.h"
#include "SuspectTable.h"
#include "SuspectTableIterator.h"


#include "pthread.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <cerrno>
#include <stdio.h>
#include <stdlib.h>

using namespace Nova;
using namespace std;

int callbackSocket = -1, IPCSocket = -1;


extern SuspectTable suspects;
extern SuspectTable suspectsSinceLastSave;
extern pthread_mutex_t suspectsSinceLastSaveLock;

struct sockaddr_un msgRemote, msgLocal;
int UIsocketSize;

//Launches a UI Handling thread, and returns
bool Nova::Spawn_UI_Handler()
{

	int len;
	string inKeyPath = Config::Inst()->GetPathHome() + "/keys" + NOVAD_LISTEN_FILENAME;
	string outKeyPath = Config::Inst()->GetPathHome() + "/keys" + UI_LISTEN_FILENAME;

    if((IPCSocket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
		LOG(ERROR, "Failed to connect to UI", "socket: "+string(strerror(errno)));
    	return false;
    }

    msgLocal.sun_family = AF_UNIX;
    strncpy(msgLocal.sun_path, inKeyPath.c_str(), inKeyPath.length());
    unlink(msgLocal.sun_path);
    len = strlen(msgLocal.sun_path) + sizeof(msgLocal.sun_family);

    if(::bind(IPCSocket, (struct sockaddr *)&msgLocal, len) == -1)
    {
		LOG(ERROR, "Failed to connect to UI", "bind: "+string(strerror(errno)));
    	close(IPCSocket);
    	return false;
    }

    if(listen(IPCSocket, SOMAXCONN) == -1)
    {
		LOG(ERROR, "Failed to connect to UI", "listen: "+string(strerror(errno)));
		close(IPCSocket);
    	return false;
    }

    pthread_t helperThread;
    pthread_create(&helperThread, NULL, Handle_UI_Helper, NULL);

    return true;
}

void *Nova::Handle_UI_Helper(void *ptr)
{
    while(true)
    {
    	int *msgSocket = (int*)malloc(sizeof(int));

    	//Blocking call
		if((*msgSocket = accept(IPCSocket, (struct sockaddr *)&msgRemote, (socklen_t*)&UIsocketSize)) == -1)
		{
			LOG(ERROR, "Failed to connect to UI", "listen: "+string(strerror(errno)));
			close(IPCSocket);
			return false;
		}
		else
		{
			pthread_t UI_thread;
			pthread_create(&UI_thread, NULL, Handle_UI_Thread, (void*)msgSocket);
			pthread_detach(UI_thread);
		}
    }

    return NULL;
}

void *Nova::Handle_UI_Thread(void *socketVoidPtr)
{
	//Get the argument out, put it on the stack, free it from the heap so we don't forget
	int *socketPtr = (int*)socketVoidPtr;
	int socketFD = *socketPtr;
	free(socketPtr);

	while(true)
	{
		UI_Message *message = UI_Message::ReadMessage(socketFD);
		if( message == NULL )
		{
			//There was an error reading this message
			LOG(DEBUG, "The UI hung up","Deserialization error.");
			break;
		}
		switch(message->m_messageType)
		{
			case CONTROL_MESSAGE:
			{
				ControlMessage *controlMessage = (ControlMessage*)message;
				HandleControlMessage(*controlMessage, socketFD);
				delete controlMessage;
				break;
			}
			case REQUEST_MESSAGE:
			{
				RequestMessage *msg = (RequestMessage*)message;
				HandleRequestMessage(*msg, socketFD);
				delete msg;
				break;
			}
			default:
			{
				//There was an error reading this message
				LOG(DEBUG, "There was an error reading a message from the UI", "Invalid message type");
				delete message;
				continue;
			}
		}
	}

	//Should not ever get here. Return value only present to suppress compiler warning
	return NULL;
}

void Nova::HandleControlMessage(ControlMessage &controlMessage, int socketFD)
{
	switch(controlMessage.m_controlType)
	{
		case CONTROL_EXIT_REQUEST:
		{
			//TODO: Check for any reason why might not want to exit
			ControlMessage exitReply;
			exitReply.m_controlType = CONTROL_EXIT_REPLY;
			exitReply.m_success = true;

			UI_Message::WriteMessage(&exitReply, socketFD);

			LOG(NOTICE, "Quitting: Got an exit request from the UI. Goodbye!",
					"Got a CONTROL_EXIT_REQUEST, quitting.");
			SaveAndExit(0);

			break;
		}
		case CONTROL_CLEAR_ALL_REQUEST:
		{
			//TODO: Replace with new suspect table class

			suspects.Clear();
			pthread_mutex_lock(&suspectsSinceLastSaveLock);
			suspectsSinceLastSave.Clear();
			pthread_mutex_unlock(&suspectsSinceLastSaveLock);
			string delString = "rm -f " + Config::Inst()->GetPathCESaveFile();
			bool successResult = true;
			if(system(delString.c_str()) == -1)
			{
				LOG(ERROR, "Unable to delete CE state file. System call to rm failed.","");
				successResult = false;
			}

			ControlMessage clearAllSuspectsReply;
			clearAllSuspectsReply.m_controlType = CONTROL_CLEAR_ALL_REPLY;
			clearAllSuspectsReply.m_success = successResult;
			UI_Message::WriteMessage(&clearAllSuspectsReply, socketFD);

			if(successResult)
			{
				LOG(DEBUG, "Cleared all suspects due to UI request",
						"Got a CONTROL_CLEAR_ALL_REQUEST, cleared all suspects.");
			}

			break;
		}
		case CONTROL_CLEAR_SUSPECT_REQUEST:
		{
			suspects.Erase(controlMessage.m_suspectAddress);
			suspectsSinceLastSave.Erase(controlMessage.m_suspectAddress);

			RefreshStateFile();

			//TODO: Should check for errors here and return result
			ControlMessage clearSuspectReply;
			clearSuspectReply.m_controlType = CONTROL_CLEAR_SUSPECT_REPLY;
			clearSuspectReply.m_success = true;
			UI_Message::WriteMessage(&clearSuspectReply, socketFD);

			struct in_addr suspectAddress;
			suspectAddress.s_addr = controlMessage.m_suspectAddress;

			LOG(DEBUG, "Cleared a suspect due to UI request",
					"Got a CONTROL_CLEAR_SUSPECT_REQUEST, cleared suspect: "+string(inet_ntoa(suspectAddress))+".");
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

			ControlMessage saveSuspectsReply;
			saveSuspectsReply.m_controlType = CONTROL_SAVE_SUSPECTS_REPLY;
			saveSuspectsReply.m_success = true;
			UI_Message::WriteMessage(&saveSuspectsReply, socketFD);

			LOG(DEBUG, "Saved suspects to file due to UI request",
				"Got a CONTROL_SAVE_SUSPECTS_REQUEST, saved all suspects.");
			break;
		}
		case CONTROL_RECLASSIFY_ALL_REQUEST:
		{
			Reload(); //TODO: Should check for errors here and return result

			ControlMessage reclassifyAllReply;
			reclassifyAllReply.m_controlType = CONTROL_RECLASSIFY_ALL_REPLY;
			reclassifyAllReply.m_success = true;
			UI_Message::WriteMessage(&reclassifyAllReply, socketFD);

			LOG(DEBUG, "Reclassified all suspects due to UI request",
				"Got a CONTROL_RECLASSIFY_ALL_REQUEST, reclassified all suspects.");
			break;
		}
		case CONTROL_CONNECT_REQUEST:
		{
			bool successResult = ConnectToUI();

			ControlMessage connectReply;
			connectReply.m_controlType = CONTROL_CONNECT_REPLY;
			connectReply.m_success = successResult;
			UI_Message::WriteMessage(&connectReply, socketFD);

			if(successResult)
			{
				LOG(NOTICE, "Connected to UI!","Got a CONTROL_CONNECT_REQUEST, succeeded.");
			}
			else
			{
				LOG(WARNING, "Tried to connect to UI, but failed", "Got a CONTROL_CONNECT_REQUEST, failed to connect.");
			}

			break;
		}
		case CONTROL_DISCONNECT_NOTICE:
		{
			close(callbackSocket);

			ControlMessage disconnectReply;
			disconnectReply.m_controlType = CONTROL_DISCONNECT_ACK;
			UI_Message::WriteMessage(&disconnectReply, socketFD);

			LOG(NOTICE, "The UI hung up", "Got a CONTROL_DISCONNECT_NOTICE, closed down socket.");

			break;
		}
		case CONTROL_PING:
		{
			ControlMessage connectReply;
			connectReply.m_controlType = CONTROL_PONG;
			UI_Message::WriteMessage(&connectReply, socketFD);

			//TODO: This was too noisy. Even at the debug level. So it's ignored. Maybe bring it back?
			//LOG(DEBUG, "Got a Ping from UI. We're alive!",
			//	"Got a CONTROL_PING, sent a PONG.");

			break;
		}
		default:
		{
			LOG(DEBUG, "UI sent us an invalid message","Got an unexpected ControlMessage type");
			break;
		}

	}
}

void Nova::HandleRequestMessage(RequestMessage &msg, int socketFD)
{
	switch(msg.m_requestType)
	{
		case REQUEST_SUSPECTLIST:
		{
			RequestMessage reply;
			reply.m_requestType = REQUEST_SUSPECTLIST_REPLY;
			reply.m_listType = msg.m_listType;

			switch (msg.m_listType)
			{
			case SUSPECTLIST_ALL:
			{
				vector<uint64_t> benign = suspects.GetBenignSuspectKeys();
				for (uint i = 0; i < benign.size(); i++)
				{
					reply.m_suspectList.push_back((in_addr_t)benign.at(i));
				}

				vector<uint64_t> hostile = suspects.GetHostileSuspectKeys();
				for (uint i = 0; i < hostile.size(); i++)
				{
					reply.m_suspectList.push_back((in_addr_t)hostile.at(i));
				}
				break;
			}
			case SUSPECTLIST_HOSTILE:
			{
				vector<uint64_t> hostile = suspects.GetHostileSuspectKeys();
				for (uint i = 0; i < hostile.size(); i++)
				{
					reply.m_suspectList.push_back((in_addr_t)hostile.at(i));
				}
				break;
			}
			case SUSPECTLIST_BENIGN:
			{
				vector<uint64_t> benign = suspects.GetBenignSuspectKeys();
				for (uint i = 0; i < benign.size(); i++)
				{
					reply.m_suspectList.push_back((in_addr_t)benign.at(i));
				}
				break;
			}
			default:
				LOG(DEBUG, "UI sent us an invalid message", "Got an unexpected RequestMessage type");
				break;
			}


			UI_Message::WriteMessage(&reply, socketFD);
			break;
		}

		case REQUEST_SUSPECT:
		{
			RequestMessage reply;
			reply.m_requestType = REQUEST_SUSPECT_REPLY;
			reply.m_suspect = new Suspect();
			*reply.m_suspect = suspects.Peek(msg.m_suspectAddress);
			UI_Message::WriteMessage(&reply, socketFD);

			break;
		}

			default:
		{
			LOG(DEBUG, "UI sent us an invalid message", "Got an unexpected RequestMessage type");
			break;
		}

	}
}


bool Nova::ConnectToUI()
{
	//Builds the key path
	string homePath = Config::Inst()->GetPathHome();
	string key = homePath;
	key += "/keys";
	key += UI_LISTEN_FILENAME;

	struct sockaddr_un UIAddress;

	//Builds the address
	UIAddress.sun_family = AF_UNIX;
	strcpy(UIAddress.sun_path, key.c_str());

	if((callbackSocket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		LOG(WARNING, "Unable to connect to UI", "Unable to create UI socket: "+string(strerror(errno)));
		close(callbackSocket);
		return false;
	}

	if(connect(callbackSocket, (struct sockaddr *)&UIAddress, sizeof(UIAddress)) == -1)
	{
		LOG(WARNING, "Unable to connect to UI", "Unable to connect to UI: "+string(strerror(errno)));
		close(callbackSocket);
		return false;
	}

	return true;
}


bool Nova::SendSuspectToUI(Suspect *suspect)
{
	CallbackMessage suspectUpdate;
	suspectUpdate.m_suspect = suspect;
	suspectUpdate.m_callbackType = CALLBACK_SUSPECT_UDPATE;
	if(!UI_Message::WriteMessage(&suspectUpdate, callbackSocket))
	{
		return false;
	}

	UI_Message *suspectReply = UI_Message::ReadMessage(callbackSocket);
	if(suspectReply == NULL)
	{
		return false;
	}
	if(suspectReply->m_messageType != CALLBACK_MESSAGE)
	{
		delete suspectReply;
		return false;
	}
	CallbackMessage *suspectCallback = (CallbackMessage*)suspectReply;
	if(suspectCallback->m_callbackType != CALLBACK_SUSPECT_UDPATE_ACK)
	{
		delete suspectCallback;
		return false;
	}
	delete suspectCallback;

	return true;
}
