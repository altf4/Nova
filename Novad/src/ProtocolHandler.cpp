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
#include "NOVAConfiguration.h"
#include "Logger.h"
#include "Control.h"
#include "Novad.h"

#include "pthread.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <cerrno>
#include <stdio.h>
#include <stdlib.h>
#include <boost/format.hpp>

using namespace Nova;
using boost::format;

extern string userHomePath;
extern NOVAConfiguration *globalConfig;
extern Logger *logger;

//Launches a UI Handling thread, and returns
void Spawn_UI_Handler()
{
	struct sockaddr_un msgRemote, msgLocal;
	int socketSize, IPCSocket;
	int len;
	string inKeyPath = userHomePath + NOVAD_IN_FILENAME;
	string outKeyPath = userHomePath + NOVAD_OUT_FILENAME;

    if ((IPCSocket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
    	logger->Log(ERROR, "Failed to connect to UI", (format("File %1% at line %2%:  socket: %s")% __FILE__%__LINE__% strerror(errno)).str());
    	return;
    }

    msgLocal.sun_family = AF_UNIX;
    strncpy(msgLocal.sun_path, inKeyPath.c_str(), inKeyPath.length());
    unlink(msgLocal.sun_path);
    len = strlen(msgLocal.sun_path) + sizeof(msgLocal.sun_family);

    if (bind(IPCSocket, (struct sockaddr *)&msgLocal, len) == -1)
    {
    	logger->Log(ERROR, "Failed to connect to UI", (format("File %1% at line %2%:  bind: %s")% __FILE__%__LINE__% strerror(errno)).str());
    	close(IPCSocket);
    	return;
    }

    if (listen(IPCSocket, SOMAXCONN) == -1)
    {
    	logger->Log(ERROR, "Failed to connect to UI", (format("File %1% at line %2%:  listen: %s")% __FILE__%__LINE__% strerror(errno)).str());
    	close(IPCSocket);
    	return;
    }

    while(true)
    {
    	int *msgSocket = (int*)malloc(sizeof(int));

    	//Blocking call
		if ((*msgSocket = accept(IPCSocket, (struct sockaddr *)&msgRemote, (socklen_t*)&socketSize)) == -1)
		{
			logger->Log(ERROR, "Failed to connect to UI", (format("File %1% at line %2%:  listen: %s")% __FILE__%__LINE__% strerror(errno)).str());
			close(IPCSocket);
			return;
		}

		pthread_t UI_thread;
		pthread_create(&UI_thread, NULL, Handle_UI_Thread, (void*)msgSocket);
    }

}

void *Handle_UI_Thread(void *socketVoidPtr)
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
			logger->Log(DEBUG, "There was an error reading a message from the UI",
					(format("File %1% at line %2%: Deserialization error.")% __FILE__%__LINE__).str());
			delete message;
			continue;
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
			default:
			{
				//There was an error reading this message
				logger->Log(DEBUG, "There was an error reading a message from the UI",
						(format("File %1% at line %2%: Invalid message type")% __FILE__%__LINE__).str());
				delete message;
				continue;
			}
		}
	}

	//Should not ever get here. Return value only present to suppress compiler warning
	return NULL;
}

void HandleControlMessage(ControlMessage &controlMessage, int socketFD)
{
	switch(controlMessage.m_controlType)
	{
		case CONTROL_EXIT_REQUEST:
		{
			//TODO: Check for any reason why might not want to exit
			ControlMessage *exitReply = new ControlMessage();
			exitReply->m_controlType = CONTROL_EXIT_REPLY;
			exitReply->m_success = true;

			UI_Message::WriteMessage(exitReply, socketFD);
			delete exitReply;
			SaveAndExit(0);

			break;
		}
		case CONTROL_CLEAR_ALL_REQUEST:
		{
			//TODO: Replace with new suspect table class
			pthread_rwlock_wrlock(&suspectTableLock);
			for (SuspectHashTable::iterator it = suspects.begin(); it != suspects.end(); it++)
				delete it->second;
			suspects.clear();

			for (SuspectHashTable::iterator it = suspectsSinceLastSave.begin(); it != suspectsSinceLastSave.end(); it++)
				delete it->second;
			suspectsSinceLastSave.clear();

			string delString = "rm -f " + globalConfig->getPathCESaveFile();
			if(system(delString.c_str()) == -1)
				logger->Log(ERROR, (format("File %1% at line %2%:  Unable to delete CE state file. System call to rm failed.")% __FILE__%__LINE__).str());

			pthread_rwlock_unlock(&suspectTableLock);

			ControlMessage *clearAllSuspectsReply = new ControlMessage();
			clearAllSuspectsReply->m_controlType = CONTROL_CLEAR_ALL_REPLY;
			clearAllSuspectsReply->m_success = true;
			UI_Message::WriteMessage(clearAllSuspectsReply, socketFD);
			delete clearAllSuspectsReply;

			break;
		}
		case CONTROL_CLEAR_SUSPECT_REQUEST:
		{
			//TODO: Replace with new suspect table class
			pthread_rwlock_wrlock(&suspectTableLock);
			suspectsSinceLastSave[controlMessage.m_suspectAddress] = suspects[controlMessage.m_suspectAddress];
			suspects.set_deleted_key(5);
			suspects.erase(suspectKey);
			suspects.clear_deleted_key();
			RefreshStateFile();
			pthread_rwlock_unlock(&suspectTableLock);

			//TODO: Should check for errors here and return result
			ControlMessage *clearSuspectReply = new ControlMessage();
			clearSuspectReply->m_controlType = CONTROL_CLEAR_SUSPECT_REPLY;
			clearSuspectReply->m_success = true;
			UI_Message::WriteMessage(clearSuspectReply, socketFD);
			delete clearSuspectReply;

			break;
		}
		case CONTROL_SAVE_SUSPECTS_REQUEST:
		{
			SaveSuspectsToFile(globalConfig->getPathCESaveFile()); //TODO: Should check for errors here and return result

			ControlMessage *saveSuspectsReply = new ControlMessage();
			saveSuspectsReply->m_controlType = CONTROL_SAVE_SUSPECTS_REPLY;
			saveSuspectsReply->m_success = true;
			UI_Message::WriteMessage(saveSuspectsReply, socketFD);
			delete saveSuspectsReply;

			break;
		}
		case CONTROL_RECLASSIFY_ALL_REQUEST:
		{
			Reload(); //TODO: Should check for errors here and return result

			ControlMessage *reclassifyAllReply = new ControlMessage();
			reclassifyAllReply->m_controlType = CONTROL_RECLASSIFY_ALL_REPLY;
			reclassifyAllReply->m_success = true;
			UI_Message::WriteMessage(reclassifyAllReply, socketFD);
			delete reclassifyAllReply;

			break;
		}
		default:
		{
			logger->Log(DEBUG, "UI sent us an invalid message",
					(format("File %1% at line %2%: Got an unexpected ControlMessage type")% __FILE__%__LINE__).str());
		}
	}
}
