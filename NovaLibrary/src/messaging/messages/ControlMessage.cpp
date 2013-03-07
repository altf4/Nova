//============================================================================
// Name        : ControlMessage.cpp
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
// Description : Message objects sent to control Novad's operation
//	Novad should reply either with a success message or an ack. For more complicated
//	return messages, consider using RequestMessage instead.
//============================================================================

#include "ControlMessage.h"

#include <sys/un.h>
#include <iostream>

using namespace std;

namespace Nova
{

ControlMessage::ControlMessage(enum ControlType controlType)
{
	m_messageType = CONTROL_MESSAGE;
	m_contents.set_m_controltype(controlType);
}

ControlMessage::~ControlMessage()
{

}

ControlMessage::ControlMessage(char *buffer, uint32_t length)
{
	if( length < CONTROL_MSG_MIN_SIZE )
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

	length -= MESSAGE_HDR_SIZE;
	if(!m_contents.ParseFromArray(buffer, length))
	{
		m_serializeError = true;
		return;
	}
}
char *ControlMessage::Serialize(uint32_t *length)
{
	*length = MESSAGE_HDR_SIZE + sizeof(uint32_t) + m_contents.ByteSize();
	char *buffer = (char*)malloc(*length);
	char *originalbuffer = buffer;
	SerializeHeader(&buffer, *length);
	if(!m_contents.SerializeToArray(buffer, m_contents.ByteSize()))
	{
		free(originalbuffer);
		return NULL;
	}
	return originalbuffer;
}

}
