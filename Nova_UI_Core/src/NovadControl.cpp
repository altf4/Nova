//============================================================================
// Name        : NovadControl.cpp
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
// Description : Controls the Novad process itself, in terms of stopping and starting
//============================================================================

#include "messaging/MessageManager.h"
#include "messaging/messages/Message.h"
#include "messaging/messages/ControlMessage.h"
#include "messaging/messages/ErrorMessage.h"
#include "Commands.h"
#include "Logger.h"
#include "Lock.h"

#include <iostream>
#include <stdio.h>
#include <unistd.h>

using namespace Nova;
using namespace std;

extern int IPCSocketFD;

namespace Nova
{
bool StartNovad(bool blocking)
{
	if(IsNovadUp(false))
	{
		return true;
	}

	if (!blocking)
	{
		if(system("nohup novad > /dev/null&") != 0)
		{
			return false;
		}
		else
		{
			return true;
		}
	}
	else
	{
		if(system("novad") != 0)
		{
			return false;
		}
		else
		{
			return true;
		}

	}
}

bool StopNovad()
{
	bool retSuccess;
	//Keep the scope of the following Ticket out of the call to Disconnect
	{
		Ticket ticket = MessageManager::Instance().StartConversation(IPCSocketFD);

		ControlMessage killRequest(CONTROL_EXIT_REQUEST);
		if(!MessageManager::Instance().WriteMessage(ticket, &killRequest))
		{
			LOG(ERROR, "Error sending command to NOVAD (CONTROL_EXIT_REQUEST)", "");
			return false;
		}

		Message *reply = MessageManager::Instance().ReadMessage(ticket);
		if(reply->m_messageType == ERROR_MESSAGE && ((ErrorMessage*)reply)->m_errorType == ERROR_TIMEOUT)
		{
			LOG(ERROR, "Timeout error when waiting for message reply", "");
			delete ((ErrorMessage*)reply);
			return false;
		}
		if(reply->m_messageType != CONTROL_MESSAGE )
		{
			LOG(ERROR, "Received the wrong kind of reply message", "");
			reply->DeleteContents();
			delete reply;
			return false;
		}

		ControlMessage *killReply = (ControlMessage*)reply;
		if( killReply->m_controlType != CONTROL_EXIT_REPLY )
		{
			LOG(ERROR, "Received the wrong kind of control message", "");
			reply->DeleteContents();
			delete reply;
			return false;
		}
		retSuccess = killReply->m_success;
		delete killReply;

		LOG(DEBUG, "Call to StopNovad complete", "");
	}
	DisconnectFromNovad();

	return retSuccess;
}

bool HardStopNovad()
{
	// THIS METHOD SHOULD ONLY BE CALLED ON DEADLOCK FOR NOVAD
	if(system(string("killall novad").c_str()) != -1)
	{
		LOG(INFO, "Nova has experienced a hard stop", "");
		return true;
	}
	LOG(ERROR, "Something happened while trying to kill Novad", "");
	return false;
}

bool SaveAllSuspects(std::string file)
{
	Ticket ticket = MessageManager::Instance().StartConversation(IPCSocketFD);

	ControlMessage saveRequest(CONTROL_SAVE_SUSPECTS_REQUEST);
	strcpy(saveRequest.m_filePath, file.c_str());

	if(!MessageManager::Instance().WriteMessage(ticket, &saveRequest))
	{
		LOG(ERROR, "Error sending command to NOVAD (CONTROL_SAVE_SUSPECTS_REQUEST)", "");
		return false;
	}

	Message *reply = MessageManager::Instance().ReadMessage(ticket, IPCSocketFD);
	if(reply->m_messageType == ERROR_MESSAGE && ((ErrorMessage*)reply)->m_errorType == ERROR_TIMEOUT)
	{
		LOG(ERROR, "Timeout error when waiting for message reply", "");
		reply->DeleteContents();
		delete reply;
		return false;
	}
	if(reply->m_messageType != CONTROL_MESSAGE )
	{
		LOG(ERROR, "Received the wrong kind of reply message", "");
		reply->DeleteContents();
		delete reply;
		return false;
	}

	ControlMessage *saveReply = (ControlMessage*)reply;
	if( saveReply->m_controlType != CONTROL_SAVE_SUSPECTS_REPLY )
	{
		LOG(ERROR, "Received the wrong kind of control message", "");
		saveReply->DeleteContents();
		delete saveReply;
		return false;
	}
	bool retSuccess = saveReply->m_success;
	delete saveReply;
	return retSuccess;
}

bool ClearAllSuspects()
{
	Ticket ticket = MessageManager::Instance().StartConversation(IPCSocketFD);

	ControlMessage clearRequest(CONTROL_CLEAR_ALL_REQUEST);
	if(!MessageManager::Instance().WriteMessage(ticket, &clearRequest))
	{
		LOG(ERROR, "Error sending command to NOVAD (CONTROL_CLEAR_ALL_REQUEST)", "");
		return false;
	}

	Message *reply = MessageManager::Instance().ReadMessage(ticket);
	if(reply->m_messageType == ERROR_MESSAGE && ((ErrorMessage*)reply)->m_errorType == ERROR_TIMEOUT)
	{
		LOG(ERROR, "Timeout error when waiting for message reply", "");
		reply->DeleteContents();
		delete reply;
		return false;
	}
	if(reply->m_messageType != CONTROL_MESSAGE )
	{
		LOG(ERROR, "Received the wrong kind of reply message", "");
		reply->DeleteContents();
		delete reply;
		return false;
	}

	ControlMessage *clearReply = (ControlMessage*)reply;
	if( clearReply->m_controlType != CONTROL_CLEAR_ALL_REPLY )
	{
		LOG(ERROR, "Received the wrong kind of control message", "");
		reply->DeleteContents();
		delete reply;
		return false;
	}
	bool retSuccess = clearReply->m_success;
	delete clearReply;
	return retSuccess;
}

bool ClearSuspect(SuspectIdentifier suspectId)
{
	Ticket ticket = MessageManager::Instance().StartConversation(IPCSocketFD);

	ControlMessage clearRequest(CONTROL_CLEAR_SUSPECT_REQUEST);
	clearRequest.m_suspectAddress = suspectId;
	if(!MessageManager::Instance().WriteMessage(ticket, &clearRequest))
	{
		LOG(ERROR, "Unable to send CONTROL_CLEAR_SUSPECT_REQUEST to NOVAD" ,"");
		return false;
	}

	Message *reply = MessageManager::Instance().ReadMessage(ticket);
	if(reply->m_messageType == ERROR_MESSAGE && ((ErrorMessage*)reply)->m_errorType == ERROR_TIMEOUT)
	{
		LOG(ERROR, "Timeout error when waiting for message reply", "");
		reply->DeleteContents();
		delete reply;
		return false;
	}
	if(reply->m_messageType != CONTROL_MESSAGE )
	{
		LOG(ERROR, "Received the wrong kind of reply message", "");
		reply->DeleteContents();
		delete reply;
		return false;
	}

	ControlMessage *clearReply = (ControlMessage*)reply;
	if( clearReply->m_controlType != CONTROL_CLEAR_SUSPECT_REPLY )
	{
		LOG(ERROR, "Received the wrong kind of control message", "");
		reply->DeleteContents();
		delete reply;
		return false;
	}
	bool retSuccess = clearReply->m_success;
	delete clearReply;
	return retSuccess;
}

bool ReclassifyAllSuspects()
{
	Ticket ticket = MessageManager::Instance().StartConversation(IPCSocketFD);

	ControlMessage reclassifyRequest(CONTROL_RECLASSIFY_ALL_REQUEST);
	if(!MessageManager::Instance().WriteMessage(ticket, &reclassifyRequest))
	{
		LOG(ERROR, "Error sending command to NOVAD (CONTROL_RECLASSIFY_ALL_REQUEST)", "");
		return false;
	}

	Message *reply = MessageManager::Instance().ReadMessage(ticket);
	if(reply->m_messageType == ERROR_MESSAGE && ((ErrorMessage*)reply)->m_errorType == ERROR_TIMEOUT)
	{
		LOG(ERROR, "Timeout error when waiting for message reply", "");
		reply->DeleteContents();
		delete reply;
		return false;
	}

	if(reply->m_messageType != CONTROL_MESSAGE )
	{
		LOG(ERROR, "Received the wrong kind of reply message", "");
		reply->DeleteContents();
		delete reply;
		return false;
	}

	ControlMessage *reclassifyReply = (ControlMessage*)reply;
	if( reclassifyReply->m_controlType != CONTROL_RECLASSIFY_ALL_REPLY )
	{
		LOG(ERROR, "Received the wrong kind of control message", "");
		reply->DeleteContents();
		delete reply;
		return false;
	}
	bool retSuccess = reclassifyReply->m_success;
	delete reclassifyReply;
	return retSuccess;
}

bool StartPacketCapture()
{
	Ticket ticket = MessageManager::Instance().StartConversation(IPCSocketFD);

	ControlMessage request(CONTROL_START_CAPTURE);
	if(!MessageManager::Instance().WriteMessage(ticket, &request))
	{
		LOG(ERROR, "Error sending command to NOVAD (CONTROL_STOP_CAPTURE)", "");
		return false;
	}

	Message *reply = MessageManager::Instance().ReadMessage(ticket);
	if(reply->m_messageType == ERROR_MESSAGE && ((ErrorMessage*)reply)->m_errorType == ERROR_TIMEOUT)
	{
		LOG(ERROR, "Timeout error when waiting for message reply", "");
		delete ((ErrorMessage*)reply);
		return false;
	}

	ControlMessage *controlReply = dynamic_cast<ControlMessage*>(reply);
	if(controlReply == NULL || controlReply->m_controlType != CONTROL_START_CAPTURE_ACK )
	{
		LOG(ERROR, "Received the wrong kind of reply message", "");
		reply->DeleteContents();
		delete reply;
		return false;
	}

	delete reply;
	return true;
}

bool StopPacketCapture()
{
	Ticket ticket = MessageManager::Instance().StartConversation(IPCSocketFD);

	ControlMessage request(CONTROL_STOP_CAPTURE);
	if(!MessageManager::Instance().WriteMessage(ticket, &request))
	{
		LOG(ERROR, "Error sending command to NOVAD (CONTROL_STOP_CAPTURE)", "");
		return false;
	}

	Message *reply = MessageManager::Instance().ReadMessage(ticket);
	if(reply->m_messageType == ERROR_MESSAGE && ((ErrorMessage*)reply)->m_errorType == ERROR_TIMEOUT)
	{
		LOG(ERROR, "Timeout error when waiting for message reply", "");
		delete ((ErrorMessage*)reply);
		return false;
	}

	ControlMessage *controlReply = dynamic_cast<ControlMessage*>(reply);
	if(controlReply == NULL || controlReply->m_controlType != CONTROL_STOP_CAPTURE_ACK )
	{
		LOG(ERROR, "Received the wrong kind of reply message", "");
		reply->DeleteContents();
		delete reply;
		return false;
	}

	delete reply;
	return true;
}

}
