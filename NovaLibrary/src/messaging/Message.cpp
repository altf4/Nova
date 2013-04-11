//============================================================================
// Name        : Message.cpp
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
// Description : Parent message class for all message subtypes. Suitable for any
//		communications over a stream socket
//============================================================================

#include "Message.h"
#include "../Logger.h"

#include <string>
#include <vector>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;
using namespace Nova;

#define OUR_SERIAL_OFFSET 9
#define THEIR_SERIAL_OFFSET 5

Message::Message(enum MessageType type)
{
	m_contents.set_m_type(type);
}

Message::~Message()
{

}

void Message::DeleteContents()
{
	for(uint i = 0; i < m_suspects.size(); i++)
	{
		delete m_suspects[i];
	}
	m_suspects.clear();
}

uint32_t Message::GetLength()
{
	uint32_t length = m_contents.ByteSize();
	length += sizeof(uint32_t);

	for(uint i = 0; i < m_suspects.size(); i++)
	{
		length += m_suspects[i]->GetSerializeLength(m_contents.m_featuremode());
	}
	return length;
}

bool Message::Serialize(char *buffer, uint32_t length)
{
	uint32_t protobufLength = m_contents.ByteSize();

	if(protobufLength > length)
	{
		return false;
	}

	//Copy in the length of the protobuf section
	memcpy(buffer, &protobufLength, sizeof(protobufLength));
	buffer += sizeof(protobufLength);

	//Serialize in the protobuf objects
	if(!m_contents.SerializeToArray(buffer, protobufLength))
	{
		return false;
	}
	buffer += protobufLength;

	uint32_t runningLength = protobufLength + sizeof(protobufLength);
	//Serialize in any Suspects
	for(uint i = 0; i < m_suspects.size(); i++)
	{
		// Serialize our suspect
		uint32_t suspectLength = m_suspects[i]->GetSerializeLength(m_contents.m_featuremode());
		if(m_suspects[i]->Serialize((u_char*)buffer, suspectLength, m_contents.m_featuremode()) != suspectLength)
		{
			return false;
		}
		buffer += suspectLength;
		runningLength += suspectLength;
		if(runningLength > length)
		{
			return false;
		}
	}

	return true;
}

bool Message::Deserialize(char *buffer, uint32_t length)
{
	uint32_t protobufLength;
	memcpy(&protobufLength, buffer, sizeof(protobufLength));
	buffer += sizeof(protobufLength);
	length -= sizeof(protobufLength);

	if(!m_contents.ParseFromArray(buffer, protobufLength))
	{
		return false;
	}
	buffer += protobufLength;

	uint32_t bytesToGo = length - protobufLength;
	while(bytesToGo > 0)
	{
		Suspect *suspect = new Suspect();
		try
		{
			uint32_t suspectSize = suspect->Deserialize((u_char*)buffer, bytesToGo, m_contents.m_featuremode());
			if(suspectSize == 0)
			{
				delete suspect;
				return false;
			}
			bytesToGo -= suspectSize;
			buffer += suspectSize;
		}
		catch(Nova::serializationException &e)
		{
			delete suspect;
			return false;
		}
		m_suspects.push_back(suspect);
	}

	return true;
}
