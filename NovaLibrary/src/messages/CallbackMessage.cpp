//============================================================================
// Name        : CallbackMessage.cpp
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
// Description : Messages coming asynchronously from Novad to the UI
//============================================================================

#include "CallbackMessage.h"
#include <string.h>

namespace Nova
{

CallbackMessage::CallbackMessage()
{
	m_suspect = NULL;
	m_messageType = CALLBACK_MESSAGE;
}

CallbackMessage::~CallbackMessage()
{
	if(m_suspect != NULL)
	{
		delete m_suspect;
	}
}

CallbackMessage::CallbackMessage(char *buffer, uint32_t length)
{
	if( length < CALLBACK_MSG_MIN_SIZE )
	{
		return;
	}

	m_serializeError = false;

	//Copy the message type
	memcpy(&m_messageType, buffer, sizeof(m_messageType));
	buffer += sizeof(m_messageType);

	//Copy the control message type
	memcpy(&m_callbackType, buffer, sizeof(m_callbackType));
	buffer += sizeof(m_callbackType);

	switch(m_callbackType)
	{
		case CALLBACK_SUSPECT_UDPATE:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type
			//		3) Length of incoming serialized suspect
			//		3) Serialized suspect
			uint32_t expectedSize = sizeof(m_messageType) + sizeof(m_callbackType) + sizeof(m_suspectLength);
			if(length <= expectedSize)
			{
				m_serializeError = true;
				return;
			}

			memcpy(&m_suspectLength, buffer, sizeof(m_suspectLength));
			buffer += sizeof(m_suspectLength);

			expectedSize += m_suspectLength;
			if(expectedSize != length)
			{
				m_serializeError = true;
				return;
			}

			m_suspect = new Suspect();
			m_suspect->DeserializeSuspect((u_char*)buffer);

			break;
		}
		case CALLBACK_SUSPECT_UDPATE_ACK:
		{
			//Uses: 1) UI_Message Type
			//		2) Callback Type
			uint32_t expectedSize = sizeof(m_messageType) + sizeof(m_callbackType);
			if(length != expectedSize)
			{
				m_serializeError = true;
				return;
			}

			break;
		}
		default:
		{
			m_serializeError = true;
			return;
		}
	}
}

char *CallbackMessage::Serialize(uint32_t *length)
{
	char *buffer, *originalBuffer;
	uint32_t messageSize;

	switch(m_callbackType)
	{
		case CALLBACK_SUSPECT_UDPATE:
		{
			//Uses: 1) UI_Message Type
			//		2) ControlMessage Type
			//		3) Length of incoming serialized suspect
			//		3) Serialized suspect
			if(m_suspect == NULL)
			{
				return NULL;
			}
			char *suspectTempBuffer[MAX_MSG_SIZE];
			m_suspectLength = m_suspect->SerializeSuspect((u_char*)suspectTempBuffer);
			if(m_suspectLength == 0)
			{
				return NULL;
			}

			messageSize = sizeof(m_messageType) + sizeof(m_callbackType) + sizeof(m_suspectLength) + m_suspectLength;
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			//Put the UI Message type in
			memcpy(buffer, &m_messageType, sizeof(m_messageType));
			buffer += sizeof(m_messageType);
			//Put the Callback Message type in
			memcpy(buffer, &m_callbackType, sizeof(m_callbackType));
			buffer += sizeof(m_callbackType);
			//Length of suspect buffer
			memcpy(buffer, &m_suspectLength, sizeof(m_suspectLength));
			buffer += sizeof(m_suspectLength);
			//Suspect buffer itself
			memcpy(buffer, suspectTempBuffer, m_suspectLength);
			buffer += m_suspectLength;

			break;
		}
		case CALLBACK_SUSPECT_UDPATE_ACK:
		{
			//Uses: 1) UI_Message Type
			//		2) Callback Message Type
			messageSize = sizeof(m_messageType) + sizeof(m_callbackType);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			//Put the UI Message type in
			memcpy(buffer, &m_messageType, sizeof(m_messageType));
			buffer += sizeof(m_messageType);
			//Put the Control Message type in
			memcpy(buffer, &m_callbackType, sizeof(m_callbackType));
			buffer += sizeof(m_callbackType);

			break;
		}
		default:
		{
			return NULL;
		}
	}
	*length = messageSize;
	return originalBuffer;
}

}
