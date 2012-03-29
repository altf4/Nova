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
// Description : Requests from the GUI to Novad to get current state information
//============================================================================

#include <string.h>

#include "RequestMessage.h"
#include "../Defines.h"

namespace Nova
{

RequestMessage::RequestMessage(enum RequestType requestType)
{
	m_messageType = REQUEST_MESSAGE;
	m_suspectListLength = 0;
	m_requestType = requestType;
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

	// Deserialize the message type
	memcpy(&m_messageType, buffer, sizeof(m_messageType));
	buffer += sizeof(m_messageType);

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

			uint32_t expectedSize = sizeof(m_messageType) + sizeof(m_requestType) + sizeof(m_suspectListLength) + sizeof(m_listType);
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
			uint32_t expectedSize = sizeof(m_messageType) + sizeof(m_requestType) + sizeof(m_listType);
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
			uint32_t expectedSize = sizeof(m_messageType) + sizeof(m_requestType) + sizeof(m_suspectAddress);
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
			uint32_t expectedSize = sizeof(m_messageType) + sizeof(m_requestType) + sizeof(m_suspectLength);
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
			m_suspect->DeserializeSuspect((u_char*)buffer);

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
			//Uses: 1) UI_Message Type
			//		2) Request Message Type
			// 		3) Type of list being returned (all, hostile, benign)
			//		4) Size of list
			//		5) List of suspect IPs

			m_suspectListLength = sizeof(in_addr_t) * m_suspectList.size();
			messageSize = sizeof(m_messageType) + sizeof(m_requestType) + sizeof(m_suspectListLength) + m_suspectListLength + sizeof(m_listType);

			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;
			buffer += SerializeHeader(buffer);

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
			//Uses: 1) UI_Message Type
			//		2) request Message Type
			// 		3) Type of list we want (all, hostile, benign)
			messageSize = sizeof(m_messageType) + sizeof(m_requestType) + sizeof(m_listType);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;
			// Serialize 1) and 2)
			buffer += SerializeHeader(buffer);

			// Put the type of list we're requesting
			memcpy(buffer, &m_listType, sizeof(m_listType));
			buffer += sizeof(m_listType);

			break;
		}

		case REQUEST_SUSPECT:
		{
			//Uses: 1) UI_Message Type
			//		2) Request Message Type
			// 		3) IP address of suspect being requested in in_addr_t format
			messageSize = sizeof(m_messageType) + sizeof(m_requestType) + sizeof(m_suspectAddress);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			// Serialize 1) and 2)
			buffer += SerializeHeader(buffer);

			// Serialize our IP
			memcpy(buffer, &m_suspectAddress, sizeof(m_suspectAddress));
			buffer += sizeof(m_suspectAddress);
			break;
		}
		case REQUEST_SUSPECT_REPLY:
		{
			//Uses: 1) UI_Message Type
			//		2) Request Message Type
			//		3) Size of the requested suspect
			// 		4) The requested suspect

			char suspectTempBuffer[MAX_MSG_SIZE];
			m_suspectLength = m_suspect->SerializeSuspect((u_char*)suspectTempBuffer);

			messageSize = sizeof(m_messageType) + sizeof(m_requestType) + sizeof(m_suspectLength) + m_suspectLength ;
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			// Serialize 1) and 2)
			buffer += SerializeHeader(buffer);

			// Serialize the size of the suspect
			memcpy(buffer, &m_suspectLength, sizeof(m_suspectLength));
			buffer += sizeof(m_suspectLength );

			// Serialize our suspect
			memcpy(buffer, suspectTempBuffer, m_suspectLength);
			buffer += m_suspectLength;

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

int RequestMessage::SerializeHeader(char *buffer)
{
	//Serializes: 	1) UI_Message Type
	//				2) request Message Type
	int bytes = 0;
	//Put the UI Message type in
	memcpy(buffer, &m_messageType, sizeof(m_messageType));
	buffer += sizeof(m_messageType);
	bytes += sizeof(m_messageType);

	//Put the Request Message type in
	memcpy(buffer, &m_requestType, sizeof(m_requestType));
	buffer += sizeof(m_requestType);
	bytes += sizeof(m_messageType);

	return bytes;
}
}
