//============================================================================
// Name        : RequestMessage.cpp
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
// Description : Requests from the UI to Novad to get current state information
//============================================================================

#include <string.h>

#include "../../SerializationHelper.h"
#include "RequestMessage.h"

namespace Nova
{

RequestMessage::RequestMessage(enum RequestType requestType)
{
	m_suspect = NULL;
	m_messageType = REQUEST_MESSAGE;
	m_suspectListLength = 0;
	m_requestType = requestType;
}

RequestMessage::~RequestMessage()
{

}

void RequestMessage::DeleteContents()
{
	delete m_suspect;
	m_suspect = NULL;
}

RequestMessage::RequestMessage(char *buffer, uint32_t length)
{
	m_suspect = NULL;

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

			// Deserialize the length of the suspect list
			memcpy(&m_suspectListLength, buffer, sizeof(m_suspectListLength));
			buffer += sizeof(m_suspectListLength);

			// Deserialize the list
			m_suspectList.clear();
			for(uint i = 0; i < m_suspectListLength; i++)
			{
				SuspectIdentifier address;
				buffer += address.Deserialize(reinterpret_cast<u_char*>(buffer), length);

				m_suspectList.push_back(address);
			}

			break;
		}
		case REQUEST_SUSPECTLIST:
		{
			uint32_t expectedSize = MESSAGE_HDR_SIZE + sizeof(m_requestType) + sizeof(m_listType);
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
			// Deserialize the address of the suspect being requested
			buffer += m_suspectAddress.Deserialize(reinterpret_cast<u_char*>(buffer), length);

			break;
		}

		case REQUEST_SUSPECT_REPLY:
		{
			uint32_t expectedSize = MESSAGE_HDR_SIZE + sizeof(m_requestType) + sizeof(m_suspectLength);
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
			try
			{
				if(m_suspect->Deserialize((u_char*)buffer, length, NO_FEATURE_DATA) != m_suspectLength)
				{
					m_serializeError = true;
					return;
				}
			} catch(Nova::serializationException &e) {
				m_serializeError = true;
				return;
			}
			break;
		}


		case REQUEST_SUSPECT_WITHDATA:
		{
			// Deserialize the address of the suspect being requested
			buffer += m_suspectAddress.Deserialize(reinterpret_cast<u_char*>(buffer), length);

			break;
		}

		case REQUEST_SUSPECT_WITHDATA_REPLY:
		{
			uint32_t expectedSize = MESSAGE_HDR_SIZE + sizeof(m_requestType) + sizeof(m_suspectLength);
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
			try
			{
				if(m_suspect->Deserialize((u_char*)buffer, length, MAIN_FEATURE_DATA) != m_suspectLength)
				{
					m_serializeError = true;
					return;
				}
			} catch(Nova::serializationException &e) {
				m_serializeError = true;
				return;
			}
			break;
		}

		case REQUEST_UPTIME_REPLY:
		{
			uint32_t expectedSize = MESSAGE_HDR_SIZE + sizeof(m_requestType) + sizeof(m_startTime);
			if(length != expectedSize)
			{
				m_serializeError = true;
				return;
			}

			// Deserialize the uptime
			memcpy(&m_startTime, buffer, sizeof(m_startTime));
			buffer += sizeof(m_startTime);

			break;
		}

		case REQUEST_UPTIME:
		case REQUEST_PING:
		case REQUEST_PONG:
		{
			//Uses: 1) UI_Message Header
			//		2) ControlMessage Type

			uint32_t expectedSize = MESSAGE_HDR_SIZE + sizeof(m_requestType);
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

			m_suspectListLength = m_suspectList.size();
			messageSize = MESSAGE_HDR_SIZE + sizeof(m_requestType) + sizeof(m_suspectListLength) + sizeof(m_listType) + sizeof(messageSize);
			for (uint i = 0; i < m_suspectList.size(); i++)
			{
				messageSize += m_suspectList.at(i).GetSerializationLength();
			}

			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			SerializeHeader(&buffer, messageSize);
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
			for(uint i = 0; i < m_suspectList.size(); i++)
			{
				buffer += m_suspectList.at(i).Serialize(reinterpret_cast<u_char*>(buffer), messageSize);
			}

			break;
		}
		case REQUEST_SUSPECTLIST:
		{
			//Uses: 1) UI_Message Header
			//		2) request Message Type
			// 		3) Type of list we want (all, hostile, benign)
			messageSize = MESSAGE_HDR_SIZE + sizeof(m_requestType) + sizeof(m_listType)+ sizeof(messageSize);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			SerializeHeader(&buffer, messageSize);
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
			// 		3) Suspect ID
			messageSize = MESSAGE_HDR_SIZE + sizeof(m_requestType) + m_suspectAddress.GetSerializationLength() + sizeof(messageSize);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			SerializeHeader(&buffer, messageSize);
			//Put the Request Message type in
			memcpy(buffer, &m_requestType, sizeof(m_requestType));
			buffer += sizeof(m_requestType);

			// Serialize our IP
			buffer += m_suspectAddress.Serialize(reinterpret_cast<u_char*>(buffer), messageSize);
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

			messageSize = MESSAGE_HDR_SIZE + sizeof(m_requestType) + sizeof(m_suspectLength) + m_suspectLength + sizeof(messageSize);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			SerializeHeader(&buffer, messageSize);
			//Put the Request Message type in
			memcpy(buffer, &m_requestType, sizeof(m_requestType));
			buffer += sizeof(m_requestType);

			// Serialize the size of the suspect
			memcpy(buffer, &m_suspectLength, sizeof(m_suspectLength));
			buffer += sizeof(m_suspectLength );
			// Serialize our suspect
			if(m_suspect->Serialize((u_char*)buffer, messageSize, NO_FEATURE_DATA) != m_suspectLength)
			{
				return NULL;
			}
			buffer += m_suspectLength;
			break;
		}

		case REQUEST_SUSPECT_WITHDATA:
		{
			//Uses: 1) UI_Message Header
			//		2) Request Message Type
			// 		3) Suspect ID
			messageSize = MESSAGE_HDR_SIZE + sizeof(m_requestType) + m_suspectAddress.GetSerializationLength() + sizeof(messageSize);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			SerializeHeader(&buffer, messageSize);
			//Put the Request Message type in
			memcpy(buffer, &m_requestType, sizeof(m_requestType));
			buffer += sizeof(m_requestType);

			// Serialize our IP
			buffer += m_suspectAddress.Serialize(reinterpret_cast<u_char*>(buffer), messageSize);
			break;
		}
		case REQUEST_SUSPECT_WITHDATA_REPLY:
		{
			//Uses: 1) UI_Message Header
			//		2) Request Message Type
			//		3) Size of the requested suspect
			// 		4) The requested suspect

			m_suspectLength = m_suspect->GetSerializeLength(MAIN_FEATURE_DATA);
			if(m_suspectLength == 0)
			{
				return NULL;
			}

			messageSize = MESSAGE_HDR_SIZE + sizeof(m_requestType) + sizeof(m_suspectLength) + m_suspectLength + sizeof(messageSize);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			SerializeHeader(&buffer, messageSize);
			//Put the Request Message type in
			memcpy(buffer, &m_requestType, sizeof(m_requestType));
			buffer += sizeof(m_requestType);

			// Serialize the size of the suspect
			memcpy(buffer, &m_suspectLength, sizeof(m_suspectLength));
			buffer += sizeof(m_suspectLength );
			// Serialize our suspect
			if(m_suspect->Serialize((u_char*)buffer, messageSize, MAIN_FEATURE_DATA) != m_suspectLength)
			{
				return NULL;
			}
			buffer += m_suspectLength;
			break;
		}

		case REQUEST_UPTIME_REPLY:
		{
			//Uses: 1) UI_Message Type
			//		2) Request Message Type
			//		3) The uptime

			messageSize = MESSAGE_HDR_SIZE + sizeof(m_requestType) + sizeof(m_startTime) + sizeof(messageSize);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			SerializeHeader(&buffer, messageSize);
			//Put the Request Message type in
			memcpy(buffer, &m_requestType, sizeof(m_requestType));
			buffer += sizeof(m_requestType);

			// Serialize the uptime
			memcpy(buffer, &m_startTime, sizeof(m_startTime));
			buffer += sizeof(m_startTime );

			break;
		}

		case REQUEST_UPTIME:
		case REQUEST_PING:
		case REQUEST_PONG:
		{
			//Uses: 1) UI_Message Header
			//		2) ControlMessage Type
			messageSize = MESSAGE_HDR_SIZE + sizeof(m_requestType) + sizeof(messageSize);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			SerializeHeader(&buffer, messageSize);
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
