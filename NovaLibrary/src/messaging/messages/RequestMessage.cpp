//============================================================================
// Name        : RequestMessage.cpp
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
// Description : Requests from the UI to Novad to get current state information
//============================================================================

#include <string.h>

#include "RequestMessage.h"
#include "../../Defines.h"

namespace Nova
{

RequestMessage::RequestMessage(enum RequestType requestType, enum ProtocolDirection direction)
{
	m_messageType = REQUEST_MESSAGE;
	m_suspectListLength = 0;
	m_requestType = requestType;
	m_protocolDirection = direction;
}

RequestMessage::~RequestMessage()
{

}

RequestMessage::RequestMessage(char *buffer, uint32_t length)
{
	if( length < REQUEST_MSG_MIN_SIZE )
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

	// Deserialize the request message type
	memcpy(&m_requestType, buffer, sizeof(m_requestType));
	buffer += sizeof(m_requestType);

	switch(m_requestType)
	{
		case REQUEST_SUSPECTLIST_REPLY:
		{
			// Deserialize the request list type
			memcpy(&m_listType, buffer, sizeof(m_listType));
			buffer += sizeof(m_listType);

			uint32_t expectedSize = MESSADE_HDR_SIZE + sizeof(m_requestType) + sizeof(m_suspectListLength) + sizeof(m_listType);
			if(length < expectedSize)
			{
				m_serializeError = true;
				return;
			}

			// Deserialize the length of the suspect list
			memcpy(&m_suspectListLength, buffer, sizeof(m_suspectListLength));
			buffer += sizeof(m_suspectListLength);

			expectedSize += m_suspectListLength;
			if(expectedSize != length)
			{
				m_serializeError = true;
				return;
			}

			// Deserialize the list
			m_suspectList.clear();
			for (uint i = 0; i < m_suspectListLength; i += sizeof(in_addr_t))
			{
				in_addr_t address;

				memcpy(&address, buffer, sizeof(in_addr_t));
				buffer += sizeof(in_addr_t);

				m_suspectList.push_back(address);
			}

			break;
		}
		case REQUEST_SUSPECTLIST:
		{
			uint32_t expectedSize = MESSADE_HDR_SIZE + sizeof(m_requestType) + sizeof(m_listType);
			if(length != expectedSize)
			{
				m_serializeError = true;
				return;
			}


			// Deserialize the request list type
			memcpy(&m_listType, buffer, sizeof(m_listType));
			buffer += sizeof(m_listType);

			break;
		}

		case REQUEST_SUSPECT:
		{
			uint32_t expectedSize = MESSADE_HDR_SIZE + sizeof(m_requestType) + sizeof(m_suspectAddress);
			if(length != expectedSize)
			{
				m_serializeError = true;
				return;
			}

			// Deserialize the address of the suspect being requested
			memcpy(&m_suspectAddress, buffer, sizeof(m_suspectAddress));
			buffer += sizeof(m_suspectAddress);

			break;
		}

		case REQUEST_SUSPECT_REPLY:
		{
			uint32_t expectedSize = MESSADE_HDR_SIZE + sizeof(m_requestType) + sizeof(m_suspectLength);
			if(length < expectedSize)
			{
				m_serializeError = true;
				return;
			}

			// DeSerialize the size of the suspect
			memcpy(&m_suspectLength, buffer, sizeof(m_suspectLength));
			buffer += sizeof(m_suspectLength );

			expectedSize += m_suspectLength;
			if(expectedSize != length)
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

		case REQUEST_UPTIME:
		{
			uint32_t expectedSize = MESSADE_HDR_SIZE + sizeof(m_requestType);
			if(length != expectedSize)
			{
				m_serializeError = true;
				return;
			}

			break;
		}

		case REQUEST_UPTIME_REPLY:
		{
			uint32_t expectedSize = MESSADE_HDR_SIZE + sizeof(m_requestType) + sizeof(m_uptime);
			if(length != expectedSize)
			{
				m_serializeError = true;
				return;
			}

			// Deserialize the uptime
			memcpy(&m_uptime, buffer, sizeof(m_uptime));
			buffer += sizeof(m_uptime);

			break;
		}
		case REQUEST_PING:
		{
			//Uses: 1) UI_Message Header
			//		2) ControlMessage Type

			uint32_t expectedSize = MESSADE_HDR_SIZE + sizeof(m_requestType);
			if(length != expectedSize)
			{
				m_serializeError = true;
				return;
			}

			break;
		}
		case REQUEST_PONG:
		{
			//Uses: 1) UI_Message Header
			//		2) ControlMessage Type

			uint32_t expectedSize = MESSADE_HDR_SIZE + sizeof(m_requestType);
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

char *RequestMessage::Serialize(uint32_t *length)
{
	char *buffer, *originalBuffer;
	uint32_t messageSize;

	switch(m_requestType)
	{
		case REQUEST_SUSPECTLIST_REPLY:
		{
			//Uses: 1) UI_Message Header
			//		2) Request Message Type
			// 		3) Type of list being returned (all, hostile, benign)
			//		4) Size of list
			//		5) List of suspect IPs

			m_suspectListLength = sizeof(in_addr_t) * m_suspectList.size();
			messageSize = MESSADE_HDR_SIZE + sizeof(m_requestType) + sizeof(m_suspectListLength) + m_suspectListLength + sizeof(m_listType);

			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			SerializeHeader(&buffer);
			//Put the Request Message type in
			memcpy(buffer, &m_requestType, sizeof(m_requestType));
			buffer += sizeof(m_requestType);

			//Length of list suspect buffer
			memcpy(buffer, &m_listType, sizeof(m_listType));
			buffer += sizeof(m_listType);

			// Length of the incoming list
			memcpy(buffer, &m_suspectListLength, sizeof(m_suspectListLength));
			buffer += sizeof(m_suspectListLength);

			//Suspect list buffer itself
			for (uint i = 0; i < m_suspectList.size(); i++)
			{
				in_addr_t address = m_suspectList.at(i);
				memcpy(buffer, &address, sizeof(in_addr_t));
				buffer += sizeof(in_addr_t);
			}

			break;
		}
		case REQUEST_SUSPECTLIST:
		{
			//Uses: 1) UI_Message Header
			//		2) request Message Type
			// 		3) Type of list we want (all, hostile, benign)
			messageSize = MESSADE_HDR_SIZE + sizeof(m_requestType) + sizeof(m_listType);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			SerializeHeader(&buffer);
			//Put the Request Message type in
			memcpy(buffer, &m_requestType, sizeof(m_requestType));
			buffer += sizeof(m_requestType);

			// Put the type of list we're requesting
			memcpy(buffer, &m_listType, sizeof(m_listType));
			buffer += sizeof(m_listType);

			break;
		}

		case REQUEST_SUSPECT:
		{
			//Uses: 1) UI_Message Header
			//		2) Request Message Type
			// 		3) IP address of suspect being requested in in_addr_t format
			messageSize = MESSADE_HDR_SIZE + sizeof(m_requestType) + sizeof(m_suspectAddress);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			SerializeHeader(&buffer);
			//Put the Request Message type in
			memcpy(buffer, &m_requestType, sizeof(m_requestType));
			buffer += sizeof(m_requestType);

			// Serialize our IP
			memcpy(buffer, &m_suspectAddress, sizeof(m_suspectAddress));
			buffer += sizeof(m_suspectAddress);
			break;
		}
		case REQUEST_SUSPECT_REPLY:
		{
			//Uses: 1) UI_Message Header
			//		2) Request Message Type
			//		3) Size of the requested suspect
			// 		4) The requested suspect

			m_suspectLength = m_suspect->GetSerializeLength(NO_FEATURE_DATA);
			if(m_suspectLength == 0)
			{
				return NULL;
			}

			messageSize = MESSADE_HDR_SIZE + sizeof(m_requestType) + sizeof(m_suspectLength) + m_suspectLength;
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			SerializeHeader(&buffer);
			//Put the Request Message type in
			memcpy(buffer, &m_requestType, sizeof(m_requestType));
			buffer += sizeof(m_requestType);

			// Serialize the size of the suspect
			memcpy(buffer, &m_suspectLength, sizeof(m_suspectLength));
			buffer += sizeof(m_suspectLength );
			// Serialize our suspect
			if(m_suspect->Serialize((u_char*)buffer, NO_FEATURE_DATA) != m_suspectLength)
			{
				return NULL;
			}
			buffer += m_suspectLength;
			break;
		}

		case REQUEST_UPTIME:
		{
			//Uses: 1) UI_Message Header
			//		2) Request Message Type

			messageSize = MESSADE_HDR_SIZE + sizeof(m_requestType);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			SerializeHeader(&buffer);
			//Put the Request Message type in
			memcpy(buffer, &m_requestType, sizeof(m_requestType));
			buffer += sizeof(m_requestType);

			break;
		}

		case REQUEST_UPTIME_REPLY:
		{
			//Uses: 1) UI_Message Type
			//		2) Request Message Type
			//		3) The uptime

			messageSize = MESSADE_HDR_SIZE + sizeof(m_requestType) + sizeof(m_uptime);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			SerializeHeader(&buffer);
			//Put the Request Message type in
			memcpy(buffer, &m_requestType, sizeof(m_requestType));
			buffer += sizeof(m_requestType);

			// Serialize the uptime
			memcpy(buffer, &m_uptime, sizeof(m_uptime));
			buffer += sizeof(m_uptime );

			break;
		}
		case REQUEST_PING:
		{
			//Uses: 1) UI_Message Header
			//		2) ControlMessage Type
			messageSize = MESSADE_HDR_SIZE + sizeof(m_requestType);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			SerializeHeader(&buffer);
			//Put the Control Message type in
			memcpy(buffer, &m_requestType, sizeof(m_requestType));
			buffer += sizeof(m_requestType);

			break;
		}
		case REQUEST_PONG:
		{
			//Uses: 1) UI_Message Header
			//		2) ControlMessage Type
			messageSize = MESSADE_HDR_SIZE + sizeof(m_requestType);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			SerializeHeader(&buffer);
			//Put the Control Message type in
			memcpy(buffer, &m_requestType, sizeof(m_requestType));
			buffer += sizeof(m_requestType);

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
