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

}

uint32_t Message::GetLength()
{
	uint32_t length = m_contents.ByteSize();
	length += sizeof(uint32_t);

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

	return true;
}
