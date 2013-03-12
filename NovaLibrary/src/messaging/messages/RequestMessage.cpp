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
	m_messageType = REQUEST_MESSAGE;
	m_contents.set_m_requesttype(requestType);
}

RequestMessage::~RequestMessage()
{

}

void RequestMessage::DeleteContents()
{
	for(uint i = 0; i < m_suspects.size(); i++)
	{
		delete m_suspects[i];
	}
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

	uint32_t contentsSize;
	memcpy(&contentsSize, buffer, sizeof(contentsSize));
	buffer += sizeof(contentsSize);

	if(!m_contents.ParseFromArray(buffer, contentsSize))
	{
		m_serializeError = true;
		return;
	}
	buffer += contentsSize;

	//If more bytes to go...
	uint32_t bytesToGo = length - (MESSAGE_HDR_SIZE + sizeof(contentsSize) + contentsSize);
	while(bytesToGo > 0)
	{
		Suspect *suspect = new Suspect();
		try
		{
			uint32_t suspectSize = suspect->Deserialize((u_char*)buffer, bytesToGo, m_contents.m_featuremode());
			if(suspectSize == 0)
			{
				delete suspect;
				m_serializeError = true;
				return;
			}
			bytesToGo -= suspectSize;
			buffer += suspectSize;
		}
		catch(Nova::serializationException &e)
		{
			delete suspect;
			m_serializeError = true;
			return;
		}
		m_suspects.push_back(suspect);
	}
}

char *RequestMessage::Serialize(uint32_t *length)
{
	char *buffer, *originalBuffer;
	uint32_t messageSize;

	uint32_t suspectsLength = 0;

	for(uint i = 0; i < m_suspects.size(); i++)
	{
		suspectsLength += m_suspects[i]->GetSerializeLength(m_contents.m_featuremode());
	}

	messageSize = MESSAGE_HDR_SIZE +  sizeof(uint32_t) + sizeof(uint32_t) + m_contents.ByteSize() + suspectsLength;

	buffer = (char*)malloc(messageSize);
	originalBuffer = buffer;

	SerializeHeader(&buffer, messageSize);

	uint32_t contentLength = m_contents.ByteSize();
	memcpy(buffer, &contentLength, sizeof(contentLength));
	buffer += sizeof(contentLength);

	if(!m_contents.SerializeToArray(buffer, m_contents.ByteSize()))
	{
		return NULL;
	}
	buffer += m_contents.ByteSize();

	if(suspectsLength > 0)
	{
		for(uint i = 0; i < m_suspects.size(); i++)
		{
			// Serialize our suspect
			uint32_t length = m_suspects[i]->GetSerializeLength(m_contents.m_featuremode());
			if(m_suspects[i]->Serialize((u_char*)buffer, length, m_contents.m_featuremode()) != length)
			{
				return NULL;
			}
			buffer += length;
		}
	}

	*length = messageSize;
	return originalBuffer;
}

}
