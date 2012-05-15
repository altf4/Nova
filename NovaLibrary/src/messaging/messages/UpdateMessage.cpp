//============================================================================
// Name        : UpdateMessage.cpp
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
// Description : Messages coming asynchronously from Novad to the UI, updating
//		the UI with some new piece of information
//============================================================================

#include "UpdateMessage.h"
#include <string.h>

namespace Nova
{

UpdateMessage::UpdateMessage(enum UpdateType updateType, enum ProtocolDirection direction)
{
	m_suspect = NULL;
	m_messageType = UPDATE_MESSAGE;
	m_updateType = updateType;
	m_protocolDirection = direction;
}

UpdateMessage::~UpdateMessage()
{

}

UpdateMessage::UpdateMessage(char *buffer, uint32_t length)
{
	if( length < UPDATE_MSG_MIN_SIZE )
	{
		m_serializeError = true;
		return;
	}

	m_serializeError = false;

	//Deserialize the Message header
	if(!DeserializeHeader(&buffer))
	{
		m_serializeError = true;
		return;
	}

	//Copy the control message type
	memcpy(&m_updateType, buffer, sizeof(m_updateType));
	buffer += sizeof(m_updateType);

	switch(m_updateType)
	{
		case UPDATE_SUSPECT:
		{
			//Uses: 1) Message Header
			//		2) ControlMessage Type
			//		3) Length of incoming serialized suspect
			//		3) Serialized suspect
			uint32_t expectedSize = MESSADE_HDR_SIZE + sizeof(m_updateType) + sizeof(m_suspectLength);
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
		case UPDATE_SUSPECT_ACK:
		{
			//Uses: 1) Message Header
			//		2) update Type
			uint32_t expectedSize = MESSADE_HDR_SIZE + sizeof(m_updateType);
			if(length != expectedSize)
			{
				m_serializeError = true;
				return;
			}

			break;
		}
		case UPDATE_ALL_SUSPECTS_CLEARED:
		{
			//Uses: 1) Message Header
			//		2) Update Type
			uint32_t expectedSize = MESSADE_HDR_SIZE + sizeof(m_updateType);
			if(length != expectedSize)
			{
				m_serializeError = true;
				return;
			}
			break;
		}
		case UPDATE_ALL_SUSPECTS_CLEARED_ACK:
		{
			//Uses: 1) Message Header
			//		2) Update Type
			uint32_t expectedSize = MESSADE_HDR_SIZE + sizeof(m_updateType);
			if(length != expectedSize)
			{
				m_serializeError = true;
				return;
			}
			break;
		}
		case UPDATE_SUSPECT_CLEARED:
		{
			//Uses: 1) Message Header
			//		2) Update Type
			//		3) Suspect IP
			uint32_t expectedSize = MESSADE_HDR_SIZE + sizeof(m_updateType) + sizeof(m_IPAddress);
			if(length != expectedSize)
			{
				m_serializeError = true;
				return;
			}

			memcpy(&m_IPAddress, buffer, sizeof(m_IPAddress));
			buffer += sizeof(m_IPAddress);

			break;
		}
		case UPDATE_SUSPECT_CLEARED_ACK:
		{
			//Uses: 1) Message Header
			//		2) Update Type
			uint32_t expectedSize = MESSADE_HDR_SIZE + sizeof(m_updateType);
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

char *UpdateMessage::Serialize(uint32_t *length)
{
	char *buffer, *originalBuffer;
	uint32_t messageSize;

	switch(m_updateType)
	{
		case UPDATE_SUSPECT:
		{
			//Uses: 1) Message Header
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

			messageSize = MESSADE_HDR_SIZE + sizeof(m_updateType) + sizeof(m_suspectLength) + m_suspectLength;
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			SerializeHeader(&buffer);
			//Put the update Message type in
			memcpy(buffer, &m_updateType, sizeof(m_updateType));
			buffer += sizeof(m_updateType);
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
		case UPDATE_SUSPECT_ACK:
		{
			//Uses: 1) Message Header
			//		2) update Message Type
			messageSize = MESSADE_HDR_SIZE + sizeof(m_updateType);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			SerializeHeader(&buffer);
			//Put the Control Message type in
			memcpy(buffer, &m_updateType, sizeof(m_updateType));
			buffer += sizeof(m_updateType);

			break;
		}
		case UPDATE_ALL_SUSPECTS_CLEARED:
		{
			//Uses: 1) Message Header
			//		2) update Message Type
			messageSize = MESSADE_HDR_SIZE + sizeof(m_updateType);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			SerializeHeader(&buffer);
			//Put the Control Message type in
			memcpy(buffer, &m_updateType, sizeof(m_updateType));
			buffer += sizeof(m_updateType);

			break;
		}
		case UPDATE_ALL_SUSPECTS_CLEARED_ACK:
		{
			//Uses: 1) Message Header
			//		2) update Message Type
			messageSize = MESSADE_HDR_SIZE + sizeof(m_updateType);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			SerializeHeader(&buffer);
			//Put the Control Message type in
			memcpy(buffer, &m_updateType, sizeof(m_updateType));
			buffer += sizeof(m_updateType);

			break;
		}
		case UPDATE_SUSPECT_CLEARED:
		{
			//Uses: 1) Message Header
			//		2) update Message Type
			messageSize = MESSADE_HDR_SIZE + sizeof(m_updateType) +  sizeof(m_IPAddress);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			SerializeHeader(&buffer);

			//Put the Control Message type in
			memcpy(buffer, &m_updateType, sizeof(m_updateType));
			buffer += sizeof(m_updateType);
			memcpy(buffer, &m_IPAddress, sizeof(m_IPAddress));
			buffer += sizeof(m_IPAddress);

			break;
		}
		case UPDATE_SUSPECT_CLEARED_ACK:
		{
			//Uses: 1) Message Header
			//		2) Update Message Type
			messageSize = MESSADE_HDR_SIZE + sizeof(m_updateType);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			SerializeHeader(&buffer);
			//Put the Control Message type in
			memcpy(buffer, &m_updateType, sizeof(m_updateType));
			buffer += sizeof(m_updateType);

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
