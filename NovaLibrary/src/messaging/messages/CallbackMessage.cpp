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

CallbackMessage::CallbackMessage(enum CallbackType callbackType, enum ProtocolDirection direction)
{
	m_suspect = NULL;
	m_messageType = CALLBACK_MESSAGE;
	m_callbackType = callbackType;
	m_protocolDirection = direction;
}

CallbackMessage::~CallbackMessage()
{

}

CallbackMessage::CallbackMessage(char *buffer, uint32_t length)
{
	if( length < CALLBACK_MSG_MIN_SIZE )
	{
		m_serializeError = true;
		return;
	}

	m_serializeError = false;

	//Deserialize the UI_Message header
	if(!DeserializeHeader(&buffer))
	{
		m_serializeError = true;
		return;
	}

	//Copy the control message type
	memcpy(&m_callbackType, buffer, sizeof(m_callbackType));
	buffer += sizeof(m_callbackType);

	switch(m_callbackType)
	{
		case CALLBACK_SUSPECT_UDPATE:
		{
			//Uses: 1) UI_Message Header
			//		2) ControlMessage Type
			//		3) Length of incoming serialized suspect
			//		3) Serialized suspect
			uint32_t expectedSize = MESSADE_HDR_SIZE + sizeof(m_callbackType) + sizeof(m_suspectLength);
			if(length <= expectedSize)
			{
				m_serializeError = true;
				return;
			}

			memcpy(&m_suspectLength, buffer, sizeof(m_suspectLength));
			buffer += sizeof(m_suspectLength);
			expectedSize += m_suspectLength;
			if((expectedSize != length) > SANITY_CHECK)
			{
				m_serializeError = true;
				return;
			}
			m_suspect = new Suspect();
			if(m_suspect->Deserialize((u_char*)buffer, NO_FEATURE_DATA) != m_suspectLength)
			{
				m_serializeError = true;
				return;
			}
			break;
		}
		case CALLBACK_SUSPECT_UDPATE_ACK:
		{
			//Uses: 1) UI_Message Header
			//		2) Callback Type
			uint32_t expectedSize = MESSADE_HDR_SIZE + sizeof(m_callbackType);
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
			//Uses: 1) UI_Message Header
			//		2) ControlMessage Type
			//		3) Length of incoming serialized suspect
			//		3) Serialized suspect
			if(m_suspect == NULL)
			{
				return NULL;
			}
			m_suspectLength = m_suspect->GetSerializeLength(NO_FEATURE_DATA);
			if(m_suspectLength == 0)
			{
				return NULL;
			}

			messageSize = MESSADE_HDR_SIZE + sizeof(m_callbackType) + sizeof(m_suspectLength) + m_suspectLength;
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			SerializeHeader(&buffer);
			//Put the Callback Message type in
			memcpy(buffer, &m_callbackType, sizeof(m_callbackType));
			buffer += sizeof(m_callbackType);
			//Length of suspect buffer
			memcpy(buffer, &m_suspectLength, sizeof(m_suspectLength));
			buffer += sizeof(m_suspectLength);
			if(m_suspect->Serialize((u_char*)buffer, NO_FEATURE_DATA) != m_suspectLength)
			{
				return NULL;
			}
			buffer += m_suspectLength;
			break;
		}
		case CALLBACK_SUSPECT_UDPATE_ACK:
		{
			//Uses: 1) UI_Message Header
			//		2) Callback Message Type
			messageSize = MESSADE_HDR_SIZE + sizeof(m_callbackType);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			SerializeHeader(&buffer);
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
