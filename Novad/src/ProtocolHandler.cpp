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


#include "messages/CallbackMessage.h"
#include "messages/ControlMessage.h"
#include "messages/RequestMessage.h"
#include "messages/ErrorMessage.h"
#include "ProtocolHandler.h"
#include "SuspectTable.h"
#include "Control.h"
#include "Socket.h"
#include "Config.h"
#include "Logger.h"
#include "Novad.h"
#include "Lock.h"

#include <cerrno>
#include <stdio.h>
#include <sys/un.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>

using namespace Nova;
using namespace std;

Socket callbackSocket, IPCSocket;

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
	Lock lock(&IPCSocket.m_mutex);

	int len;
	string inKeyPath = Config::Inst()->GetPathHome() + "/keys" + NOVAD_LISTEN_FILENAME;
	string outKeyPath = Config::Inst()->GetPathHome() + "/keys" + UI_LISTEN_FILENAME;

    if((IPCSocket.m_socketFD = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
		LOG(ERROR, "Failed to connect to UI", "socket: "+string(strerror(errno)));
    	return false;
    }

    msgLocal.sun_family = AF_UNIX;
    strncpy(msgLocal.sun_path, inKeyPath.c_str(), inKeyPath.length());
    unlink(msgLocal.sun_path);
    len = strlen(msgLocal.sun_path) + sizeof(msgLocal.sun_family);

    if(::bind(IPCSocket.m_socketFD, (struct sockaddr *)&msgLocal, len) == -1)
    {
		LOG(ERROR, "Failed to connect to UI", "bind: "+string(strerror(errno)));
    	close(IPCSocket.m_socketFD);
    	return false;
    }

    if(listen(IPCSocket.m_socketFD, SOMAXCONN) == -1)
    {
		LOG(ERROR, "Failed to connect to UI", "listen: "+string(strerror(errno)));
		close(IPCSocket.m_socketFD);
    	return false;
    }

    pthread_t helperThread;
    pthread_create(&helperThread, NULL, Handle_UI_Helper, NULL);

    return true;
}

void *Handle_UI_Helper(void *ptr)
{
    while(true)
    {
    	int msgSocketFD;

    	//Blocking call
		if((msgSocketFD = accept(IPCSocket.m_socketFD, (struct sockaddr *)&msgRemote, (socklen_t*)&UIsocketSize)) == -1)
		{
			LOG(ERROR, "Failed to connect to UI", "listen: "+string(strerror(errno)));
			close(IPCSocket.m_socketFD);
			return false;
		}
		else
		{
			pthread_t UI_thread;
			Socket *msgSocket = new Socket();
			msgSocket->m_socketFD = msgSocketFD;
			pthread_create(&UI_thread, NULL, Handle_UI_Thread, (void*)msgSocket);
			pthread_detach(UI_thread);
		}
    }

    return NULL;
}

void *Handle_UI_Thread(void *socketVoidPtr)
{
	//Get the argument out, put it on the stack, free it from the heap so we don't forget
	Socket *controlSocket = (Socket*)socketVoidPtr;

	while(true)
	{
		Lock lock(&controlSocket->m_mutex);

		UI_Message *message = UI_Message::ReadMessage(controlSocket->m_socketFD);
		switch(message->m_messageType)
		{
			case CONTROL_MESSAGE:
			{
				ControlMessage *controlMessage = (ControlMessage*)message;
				HandleControlMessage(*controlMessage, controlSocket->m_socketFD);
				delete controlMessage;
				break;
			}
			case REQUEST_MESSAGE:
			{
				RequestMessage *msg = (RequestMessage*)message;
				HandleRequestMessage(*msg, controlSocket->m_socketFD);
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
						delete controlSocket;
						controlSocket = NULL;
						close(callbackSocket.m_socketFD);
						callbackSocket.m_socketFD = -1;
						return NULL;
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

	delete controlSocket;
	return NULL;
}

void HandleControlMessage(ControlMessage &controlMessage, int socketFD)
{
	switch(controlMessage.m_controlType)
	{
		case CONTROL_EXIT_REQUEST:
		{
			//TODO: Check for any reason why might not want to exit
			ControlMessage exitReply(CONTROL_EXIT_REPLY);
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

			suspects.EraseAllSuspects();
			suspectsSinceLastSave.EraseAllSuspects();
			string delString = "rm -f " + Config::Inst()->GetPathCESaveFile();
			bool successResult = true;
			if(system(delString.c_str()) == -1)
			{
				LOG(ERROR, "Unable to delete CE state file. System call to rm failed.","");
				successResult = false;
			}

			ControlMessage clearAllSuspectsReply(CONTROL_CLEAR_ALL_REPLY);
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
			ControlMessage clearSuspectReply(CONTROL_CLEAR_SUSPECT_REPLY);
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

			ControlMessage saveSuspectsReply(CONTROL_SAVE_SUSPECTS_REPLY);
			saveSuspectsReply.m_success = true;
			UI_Message::WriteMessage(&saveSuspectsReply, socketFD);

			LOG(DEBUG, "Saved suspects to file due to UI request",
				"Got a CONTROL_SAVE_SUSPECTS_REQUEST, saved all suspects.");
			break;
		}
		case CONTROL_RECLASSIFY_ALL_REQUEST:
		{
			Reload(); //TODO: Should check for errors here and return result

			ControlMessage reclassifyAllReply(CONTROL_RECLASSIFY_ALL_REPLY);
			reclassifyAllReply.m_success = true;
			UI_Message::WriteMessage(&reclassifyAllReply, socketFD);

			LOG(DEBUG, "Reclassified all suspects due to UI request",
				"Got a CONTROL_RECLASSIFY_ALL_REQUEST, reclassified all suspects.");
			break;
		}
		case CONTROL_CONNECT_REQUEST:
		{
			bool successResult = ConnectToUI();

			ControlMessage connectReply(CONTROL_CONNECT_REPLY);
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
			ControlMessage disconnectReply(CONTROL_DISCONNECT_ACK);
			UI_Message::WriteMessage(&disconnectReply, socketFD);

			close(socketFD);
			close(callbackSocket.m_socketFD);
			socketFD = -1;
			callbackSocket.m_socketFD = -1;

			LOG(NOTICE, "The UI hung up", "Got a CONTROL_DISCONNECT_NOTICE, closed down socket.");

			break;
		}
		case CONTROL_PING:
		{
			ControlMessage connectReply(CONTROL_PONG);
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

void HandleRequestMessage(RequestMessage &msg, int socketFD)
{
	switch(msg.m_requestType)
	{
		case REQUEST_SUSPECTLIST:
		{
			RequestMessage reply(REQUEST_SUSPECTLIST_REPLY);
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


			UI_Message::WriteMessage(&reply, socketFD);
			break;
		}

		case REQUEST_SUSPECT:
		{
			RequestMessage reply(REQUEST_SUSPECT_REPLY);
			reply.m_suspect = new Suspect();
			*reply.m_suspect = suspects.GetSuspect(msg.m_suspectAddress);
			UI_Message::WriteMessage(&reply, socketFD);

			break;
		}

		case REQUEST_UPTIME:
		{
			RequestMessage reply(REQUEST_UPTIME_REPLY);
			reply.m_uptime = time(NULL) - startTime;
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


bool ConnectToUI()
{
	Lock lock(&callbackSocket.m_mutex);

	//Builds the key path
	string homePath = Config::Inst()->GetPathHome();
	string key = homePath;
	key += "/keys";
	key += UI_LISTEN_FILENAME;

	struct sockaddr_un UIAddress;

	//Builds the address
	UIAddress.sun_family = AF_UNIX;
	strcpy(UIAddress.sun_path, key.c_str());

	if((callbackSocket.m_socketFD = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		LOG(WARNING, "Unable to connect to UI", "Unable to create UI socket: "+string(strerror(errno)));
		close(callbackSocket.m_socketFD);
		return false;
	}

	if(connect(callbackSocket.m_socketFD, (struct sockaddr *)&UIAddress, sizeof(UIAddress)) == -1)
	{
		LOG(WARNING, "Unable to connect to UI", "Unable to connect to UI: "+string(strerror(errno)));
		close(callbackSocket.m_socketFD);
		return false;
	}

	return true;
}


bool SendSuspectToUI(Suspect *suspect)
{
	Lock lock(&callbackSocket.m_mutex);

	CallbackMessage suspectUpdate(CALLBACK_SUSPECT_UDPATE);
	suspectUpdate.m_suspect = suspect;
	if(!UI_Message::WriteMessage(&suspectUpdate, callbackSocket.m_socketFD))
	{
		return false;
	}

	UI_Message *suspectReply = UI_Message::ReadMessage(callbackSocket.m_socketFD);
	if(suspectReply->m_messageType == ERROR_MESSAGE )
	{
		ErrorMessage *error = (ErrorMessage*)suspectReply;
		if(error->m_errorType == ERROR_SOCKET_CLOSED)
		{
			//Only bother closing the socket if it's not already closed
			if(callbackSocket.m_socketFD != -1)
			{
				close(callbackSocket.m_socketFD);
				callbackSocket.m_socketFD = -1;
			}
		}
		delete error;
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
}
