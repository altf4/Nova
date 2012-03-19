//============================================================================
// Name        : ControlMessage.cpp
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
// Description : Message objects sent to control Novad's operation
//============================================================================

#include "ControlMessage.h"
#include <sys/un.h>

using namespace std;

namespace Nova
{

ControlMessage::ControlMessage()
{
	m_messageType = CONTROL_MESSAGE;
}

ControlMessage::~ControlMessage()
{

}

ControlMessage::ControlMessage(char *buffer, uint32_t length)
{
	if( length < CONTROL_MSG_MIN_SIZE )
	{
		return;
	}

	m_serializeError = false;

	//Copy the message type
	memcpy(&m_messageType, buffer, sizeof(m_messageType));
	buffer += sizeof(m_messageType);

	//Copy the control message type
	memcpy(&m_controlType, buffer, sizeof(m_controlType));
	buffer += sizeof(m_controlType);

	switch(m_controlType)
	{
		case CONTROL_EXIT_REQUEST:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type

			uint32_t expectedSize = sizeof(m_messageType) + sizeof(m_controlType);
			if(length != expectedSize)
			{
				m_serializeError = true;
				return;
			}

			break;
		}
		case CONTROL_EXIT_REPLY:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type
			//		3) Boolean success

			uint32_t expectedSize = sizeof(m_messageType) + sizeof(m_controlType) + sizeof(m_success);
			if(length != expectedSize)
			{
				m_serializeError = true;
				return;
			}

			memcpy(&m_success, buffer, sizeof(m_success));
			buffer += sizeof(m_success);

			break;
		}
		case CONTROL_CLEAR_ALL_REQUEST:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type

			uint32_t expectedSize = sizeof(m_messageType) + sizeof(m_controlType);
			if(length != expectedSize)
			{
				m_serializeError = true;
				return;
			}

			break;
		}
		case CONTROL_CLEAR_ALL_REPLY:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type
			//		3) Boolean success

			uint32_t expectedSize = sizeof(m_messageType) + sizeof(m_controlType) + sizeof(m_success);
			if(length != expectedSize)
			{
				m_serializeError = true;
				return;
			}

			memcpy(&m_success, buffer, sizeof(m_success));
			buffer += sizeof(m_success);

			break;
		}
		case CONTROL_CLEAR_SUSPECT_REQUEST:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type
			//		3) Suspect IP to clear

			uint32_t expectedSize = sizeof(m_messageType) + sizeof(m_controlType) + sizeof(m_suspectAddress);
			if(length != expectedSize)
			{
				m_serializeError = true;
				return;
			}

			memcpy(&m_suspectAddress, buffer, sizeof(m_suspectAddress));
			buffer += sizeof(m_suspectAddress);

			break;
		}
		case CONTROL_CLEAR_SUSPECT_REPLY:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type
			//		3) Boolean success

			uint32_t expectedSize = sizeof(m_messageType) + sizeof(m_controlType) + sizeof(m_success);
			if(length != expectedSize)
			{
				m_serializeError = true;
				return;
			}

			memcpy(&m_success, buffer, sizeof(m_success));
			buffer += sizeof(m_success);

			break;
		}
		case CONTROL_SAVE_SUSPECTS_REQUEST:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type

			uint32_t expectedSize = sizeof(m_messageType) + sizeof(m_controlType);
			if(length != expectedSize)
			{
				m_serializeError = true;
				return;
			}

			break;
		}
		case CONTROL_SAVE_SUSPECTS_REPLY:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type
			//		3) Boolean success

			uint32_t expectedSize = sizeof(m_messageType) + sizeof(m_controlType) + sizeof(m_success);
			if(length != expectedSize)
			{
				m_serializeError = true;
				return;
			}

			memcpy(&m_success, buffer, sizeof(m_success));
			buffer += sizeof(m_success);

			break;
		}
		case CONTROL_RECLASSIFY_ALL_REQUEST:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type

			uint32_t expectedSize = sizeof(m_messageType) + sizeof(m_controlType);
			if(length != expectedSize)
			{
				m_serializeError = true;
				return;
			}

			break;
		}
		case CONTROL_RECLASSIFY_ALL_REPLY:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type
			//		3) Boolean success

			uint32_t expectedSize = sizeof(m_messageType) + sizeof(m_controlType) + sizeof(m_success);
			if(length != expectedSize)
			{
				m_serializeError = true;
				return;
			}

			memcpy(&m_success, buffer, sizeof(m_success));
			buffer += sizeof(m_success);

			break;
		}
		case CONTROL_CONNECT_REQUEST:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type

			uint32_t expectedSize = sizeof(m_messageType) + sizeof(m_controlType);
			if(length != expectedSize)
			{
				m_serializeError = true;
				return;
			}

			break;
		}
		case CONTROL_CONNECT_REPLY:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type
			//		3) Boolean success

			uint32_t expectedSize = sizeof(m_messageType) + sizeof(m_controlType) + sizeof(m_success);
			if(length != expectedSize)
			{
				m_serializeError = true;
				return;
			}

			memcpy(&m_success, buffer, sizeof(m_success));
			buffer += sizeof(m_success);

			break;
		}
		case CONTROL_DISCONNECT_NOTICE:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type

			uint32_t expectedSize = sizeof(m_messageType) + sizeof(m_controlType);
			if(length != expectedSize)
			{
				m_serializeError = true;
				return;
			}

			break;
		}
		case CONTROL_DISCONNECT_ACK:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type

			uint32_t expectedSize = sizeof(m_messageType) + sizeof(m_controlType);
			if(length != expectedSize)
			{
				m_serializeError = true;
				return;
			}

			break;
		}
		case CONTROL_PING:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type

			uint32_t expectedSize = sizeof(m_messageType) + sizeof(m_controlType);
			if(length != expectedSize)
			{
				m_serializeError = true;
				return;
			}

			break;
		}
		case CONTROL_PONG:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type

			uint32_t expectedSize = sizeof(m_messageType) + sizeof(m_controlType);
			if(length != expectedSize)
			{
				m_serializeError = true;
				return;
			}

			break;
		}
		case CONTROL_INVALID:
		{
			break;
		}
		default:
		{
			m_serializeError = true;
			return;
		}
	}
}
char *ControlMessage::Serialize(uint32_t *length)
{
	char *buffer = NULL, *originalBuffer = NULL;
	uint32_t messageSize = 0;

	switch(m_controlType)
	{
		case CONTROL_EXIT_REQUEST:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type
			messageSize = sizeof(m_messageType) + sizeof(m_controlType);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			//Put the UI Message type in
			memcpy(buffer, &m_messageType, sizeof(m_messageType));
			buffer += sizeof(m_messageType);
			//Put the Control Message type in
			memcpy(buffer, &m_controlType, sizeof(m_controlType));
			buffer += sizeof(m_controlType);

			break;
		}
		case CONTROL_EXIT_REPLY:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type
			//		3) Boolean success
			messageSize = sizeof(m_messageType) + sizeof(m_controlType) + sizeof(m_success);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			//Put the UI Message type in
			memcpy(buffer, &m_messageType, sizeof(m_messageType));
			buffer += sizeof(m_messageType);
			//Put the Control Message type in
			memcpy(buffer, &m_controlType, sizeof(m_controlType));
			buffer += sizeof(m_controlType);
			//Put the Control Message type in
			memcpy(buffer, &m_success, sizeof(m_success));
			buffer += sizeof(m_success);

			break;
		}
		case CONTROL_CLEAR_ALL_REQUEST:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type
			messageSize = sizeof(m_messageType) + sizeof(m_controlType);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			//Put the UI Message type in
			memcpy(buffer, &m_messageType, sizeof(m_messageType));
			buffer += sizeof(m_messageType);
			//Put the Control Message type in
			memcpy(buffer, &m_controlType, sizeof(m_controlType));
			buffer += sizeof(m_controlType);

			break;
		}
		case CONTROL_CLEAR_ALL_REPLY:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type
			//		3) Boolean success
			messageSize = sizeof(m_messageType) + sizeof(m_controlType) + sizeof(m_success);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			//Put the UI Message type in
			memcpy(buffer, &m_messageType, sizeof(m_messageType));
			buffer += sizeof(m_messageType);
			//Put the Control Message type in
			memcpy(buffer, &m_controlType, sizeof(m_controlType));
			buffer += sizeof(m_controlType);
			//Put the Control Message type in
			memcpy(buffer, &m_success, sizeof(m_success));
			buffer += sizeof(m_success);

			break;
		}
		case CONTROL_CLEAR_SUSPECT_REQUEST:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type
			//		3) Boolean success
			messageSize = sizeof(m_messageType) + sizeof(m_controlType) + sizeof(m_suspectAddress);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			//Put the UI Message type in
			memcpy(buffer, &m_messageType, sizeof(m_messageType));
			buffer += sizeof(m_messageType);
			//Put the Control Message type in
			memcpy(buffer, &m_controlType, sizeof(m_controlType));
			buffer += sizeof(m_controlType);
			//Put the Control Message type in
			memcpy(buffer, &m_suspectAddress, sizeof(m_suspectAddress));
			buffer += sizeof(m_suspectAddress);

			break;
		}
		case CONTROL_CLEAR_SUSPECT_REPLY:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type
			//		3) Boolean success
			messageSize = sizeof(m_messageType) + sizeof(m_controlType) + sizeof(m_success);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			//Put the UI Message type in
			memcpy(buffer, &m_messageType, sizeof(m_messageType));
			buffer += sizeof(m_messageType);
			//Put the Control Message type in
			memcpy(buffer, &m_controlType, sizeof(m_controlType));
			buffer += sizeof(m_controlType);
			//Put the Control Message type in
			memcpy(buffer, &m_success, sizeof(m_success));
			buffer += sizeof(m_success);

			break;
		}
		case CONTROL_SAVE_SUSPECTS_REQUEST:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type
			messageSize = sizeof(m_messageType) + sizeof(m_controlType);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			//Put the UI Message type in
			memcpy(buffer, &m_messageType, sizeof(m_messageType));
			buffer += sizeof(m_messageType);
			//Put the Control Message type in
			memcpy(buffer, &m_controlType, sizeof(m_controlType));
			buffer += sizeof(m_controlType);

			break;
		}
		case CONTROL_SAVE_SUSPECTS_REPLY:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type
			//		3) Boolean success
			messageSize = sizeof(m_messageType) + sizeof(m_controlType) + sizeof(m_success);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			//Put the UI Message type in
			memcpy(buffer, &m_messageType, sizeof(m_messageType));
			buffer += sizeof(m_messageType);
			//Put the Control Message type in
			memcpy(buffer, &m_controlType, sizeof(m_controlType));
			buffer += sizeof(m_controlType);
			//Put the Control Message type in
			memcpy(buffer, &m_success, sizeof(m_success));
			buffer += sizeof(m_success);

			break;
		}
		case CONTROL_RECLASSIFY_ALL_REQUEST:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type
			messageSize = sizeof(m_messageType) + sizeof(m_controlType);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			//Put the UI Message type in
			memcpy(buffer, &m_messageType, sizeof(m_messageType));
			buffer += sizeof(m_messageType);
			//Put the Control Message type in
			memcpy(buffer, &m_controlType, sizeof(m_controlType));
			buffer += sizeof(m_controlType);

			break;
		}
		case CONTROL_RECLASSIFY_ALL_REPLY:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type
			//		3) Boolean success
			messageSize = sizeof(m_messageType) + sizeof(m_controlType) + sizeof(m_success);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			//Put the UI Message type in
			memcpy(buffer, &m_messageType, sizeof(m_messageType));
			buffer += sizeof(m_messageType);
			//Put the Control Message type in
			memcpy(buffer, &m_controlType, sizeof(m_controlType));
			buffer += sizeof(m_controlType);
			//Put the Control Message type in
			memcpy(buffer, &m_success, sizeof(m_success));
			buffer += sizeof(m_success);

			break;
		}
		case CONTROL_CONNECT_REQUEST:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type
			messageSize = sizeof(m_messageType) + sizeof(m_controlType);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			//Put the UI Message type in
			memcpy(buffer, &m_messageType, sizeof(m_messageType));
			buffer += sizeof(m_messageType);
			//Put the Control Message type in
			memcpy(buffer, &m_controlType, sizeof(m_controlType));
			buffer += sizeof(m_controlType);

			break;
		}
		case CONTROL_CONNECT_REPLY:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type
			//		3) Boolean success
			messageSize = sizeof(m_messageType) + sizeof(m_controlType) + sizeof(m_success);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			//Put the UI Message type in
			memcpy(buffer, &m_messageType, sizeof(m_messageType));
			buffer += sizeof(m_messageType);
			//Put the Control Message type in
			memcpy(buffer, &m_controlType, sizeof(m_controlType));
			buffer += sizeof(m_controlType);
			//Put the Control Message type in
			memcpy(buffer, &m_success, sizeof(m_success));
			buffer += sizeof(m_success);

			break;
		}
		case CONTROL_DISCONNECT_NOTICE:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type
			messageSize = sizeof(m_messageType) + sizeof(m_controlType);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			//Put the UI Message type in
			memcpy(buffer, &m_messageType, sizeof(m_messageType));
			buffer += sizeof(m_messageType);
			//Put the Control Message type in
			memcpy(buffer, &m_controlType, sizeof(m_controlType));
			buffer += sizeof(m_controlType);

			break;
		}
		case CONTROL_DISCONNECT_ACK:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type
			messageSize = sizeof(m_messageType) + sizeof(m_controlType);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			//Put the UI Message type in
			memcpy(buffer, &m_messageType, sizeof(m_messageType));
			buffer += sizeof(m_messageType);
			//Put the Control Message type in
			memcpy(buffer, &m_controlType, sizeof(m_controlType));
			buffer += sizeof(m_controlType);

			break;
		}
		case CONTROL_PING:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type
			messageSize = sizeof(m_messageType) + sizeof(m_controlType);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			//Put the UI Message type in
			memcpy(buffer, &m_messageType, sizeof(m_messageType));
			buffer += sizeof(m_messageType);
			//Put the Control Message type in
			memcpy(buffer, &m_controlType, sizeof(m_controlType));
			buffer += sizeof(m_controlType);

			break;
		}
		case CONTROL_PONG:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type
			messageSize = sizeof(m_messageType) + sizeof(m_controlType);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			//Put the UI Message type in
			memcpy(buffer, &m_messageType, sizeof(m_messageType));
			buffer += sizeof(m_messageType);
			//Put the Control Message type in
			memcpy(buffer, &m_controlType, sizeof(m_controlType));
			buffer += sizeof(m_controlType);

			break;
		}
		case CONTROL_INVALID:
		{
			break;
		}
		default:
		{
			//Error
			return NULL;
		}
	}

	*length = messageSize;
	return originalBuffer;
}

}
