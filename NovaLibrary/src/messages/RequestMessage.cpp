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

RequestMessage::RequestMessage()
{
	m_messageType = REQUEST_MESSAGE;
	m_suspectListLength = 0;
}

RequestMessage::~RequestMessage()
{

}

RequestMessage::RequestMessage(char *buffer, uint32_t length)
{
	if( length < REQUEST_MSG_MIN_SIZE )
	{
		return;
	}

	m_serializeError = false;

	//Copy the message type
	memcpy(&m_messageType, buffer, sizeof(m_messageType));
	buffer += sizeof(m_messageType);

	//Copy the request message type
	memcpy(&m_requestType, buffer, sizeof(m_requestType));
	buffer += sizeof(m_requestType);

	//Copy the request list type
	memcpy(&m_listType, buffer, sizeof(m_listType));
	buffer += sizeof(m_listType);

	switch(m_requestType)
	{
		case RequestType::REQUEST_SUSPECTLIST_REPLY:
		{
			uint32_t expectedSize = sizeof(m_messageType) + sizeof(m_requestType) + sizeof(m_suspectListLength) + sizeof(m_listType);
			if(length < expectedSize)
			{
				m_serializeError = true;
				return;
			}

			memcpy(&m_suspectListLength, buffer, sizeof(m_suspectListLength));
			buffer += sizeof(m_suspectListLength);

			expectedSize += m_suspectListLength;
			if(expectedSize != length)
			{
				m_serializeError = true;
				return;
			}

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
		case RequestType::REQUEST_SUSPECTLIST:
		{
			uint32_t expectedSize = sizeof(m_messageType) + sizeof(m_requestType) + sizeof(m_listType);
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
			char suspectTempBuffer[MAX_MSG_SIZE];
			char *buffer = &suspectTempBuffer[0];
			m_suspectListLength = 0;

			for (uint i = 0; i < m_suspectList.size(); i++)
			{
				in_addr_t address = m_suspectList.at(i);
				memcpy(buffer, &address, sizeof(in_addr_t));
				buffer += sizeof(in_addr_t);

				m_suspectListLength += sizeof(in_addr_t);
			}

			messageSize = sizeof(m_messageType) + sizeof(m_requestType) + sizeof(m_suspectListLength) + m_suspectListLength + sizeof(m_listType);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;
			buffer += SerializeHeader(buffer);

			//Length of list suspect buffer
			memcpy(buffer, &m_listType, sizeof(m_listType));
			buffer += sizeof(m_listType);

			memcpy(buffer, &m_suspectListLength, sizeof(m_suspectListLength));
			buffer += sizeof(m_suspectListLength);

			//Suspect list buffer itself
			memcpy(buffer, suspectTempBuffer, m_suspectListLength);
			buffer += m_suspectListLength;

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
			buffer += SerializeHeader(buffer);

			// Puth the type of list we're requesting
			memcpy(buffer, &m_listType, sizeof(m_listType));
			buffer += sizeof(m_listType);


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
