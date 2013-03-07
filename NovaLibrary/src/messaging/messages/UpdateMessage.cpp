//============================================================================
// Name        : UpdateMessage.cpp
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
// Description : Messages coming asynchronously from Novad to the UI, updating
//		the UI with some new piece of information
//============================================================================

#include "../../SerializationHelper.h"
#include "../../FeatureSet.h"
#include "UpdateMessage.h"
#include "../../protobuf/marshalled_classes.pb.h"

#include <string.h>

namespace Nova
{

UpdateMessage::UpdateMessage(enum UpdateType updateType)
{
	m_suspect = NULL;
	m_messageType = UPDATE_MESSAGE;
	m_contents.set_m_updatetype(updateType);
}

UpdateMessage::~UpdateMessage()
{

}

void UpdateMessage::DeleteContents()
{
	delete m_suspect;
	m_suspect = NULL;
}

UpdateMessage::UpdateMessage(char *buffer, uint32_t length)
{
	m_suspect = NULL;

	if(length < UPDATE_MSG_MIN_SIZE)
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
	if(bytesToGo > 0)
	{
		try
		{
			m_suspect = new Suspect();
			if(m_suspect->Deserialize((u_char*)buffer, bytesToGo, NO_FEATURE_DATA) != bytesToGo)
			{
				delete m_suspect;
				m_suspect = NULL;
				m_serializeError = true;
				return;
			}
		}
		catch(Nova::serializationException &e)
		{
			delete m_suspect;
			m_suspect = NULL;
			m_serializeError = true;
			return;
		}
	}
}

char *UpdateMessage::Serialize(uint32_t *length)
{
	char *buffer, *originalBuffer;
	uint32_t messageSize;
	bool sendSuspect = false;

	if(m_contents.m_updatetype() == UPDATE_SUSPECT)
	{
		sendSuspect = true;
	}

	uint32_t suspectLength = 0;
	if((m_suspect != NULL) && sendSuspect)
	{
		suspectLength = m_suspect->GetSerializeLength(NO_FEATURE_DATA);
		if(suspectLength == 0)
		{
			return NULL;
		}
		messageSize = MESSAGE_HDR_SIZE +  sizeof(uint32_t) + sizeof(uint32_t) + m_contents.ByteSize() + suspectLength;
	}
	else
	{
		messageSize = MESSAGE_HDR_SIZE +  sizeof(uint32_t) + sizeof(uint32_t) + m_contents.ByteSize();
	}
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

	if((m_suspect != NULL) && sendSuspect)
	{
		// Serialize our suspect
		if(m_suspect->Serialize((u_char*)buffer, suspectLength, NO_FEATURE_DATA) != suspectLength)
		{
			return NULL;
		}
		buffer += suspectLength;
	}

	*length = messageSize;
	return originalBuffer;
}

}
