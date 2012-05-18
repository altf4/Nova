//============================================================================
// Name        : Message.cpp
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
// Description : Parent message class for all message subtypes. Suitable for any
//		communications over a stream socket
//============================================================================

#include "UpdateMessage.h"
#include "ControlMessage.h"
#include "RequestMessage.h"
#include "ErrorMessage.h"
#include "Message.h"
#include "../MessageManager.h"
#include "../../Logger.h"

#include <string>
#include <vector>
#include <errno.h>
#include <sys/socket.h>

using namespace std;
using namespace Nova;

Message::Message()
{

}

Message::~Message()
{

}

Message *Message::ReadMessage(int connectFD, enum ProtocolDirection direction, int timeout)
{
	return MessageManager::Instance().PopMessage(connectFD, direction, timeout);
}

bool Message::WriteMessage(Message *message, int connectFD)
{
	if (connectFD == -1)
	{
		return false;
	}

	message->m_serialNumber = MessageManager::Instance().GetSerialNumber(connectFD, message->m_protocolDirection);

	uint32_t length;
	char *buffer = message->Serialize(&length);
	
	// Total bytes of a write() call that need to be sent
	uint32_t bytesSoFar;

	// Return value of the write() call, actual bytes sent
	uint32_t bytesWritten;

	// Send the message length
	bytesSoFar = 0;
    while (bytesSoFar < sizeof(length))
	{
		bytesWritten = write(connectFD, &length, sizeof(length) - bytesSoFar);
		if (bytesWritten < 0)
		{
			free(buffer);
			return false;
		}
		else
		{
			bytesSoFar += bytesWritten;
		}
	}

	
	// Send the message
	bytesSoFar = 0;
	while (bytesSoFar < length)
	{
		bytesWritten = write(connectFD, buffer, length - bytesSoFar);
		if (bytesWritten < 0)
		{
			free(buffer);
			return false;
		}
		else
		{
			bytesSoFar += bytesWritten;
		}
	}

	free(buffer);
	return true;
}

bool Message::DeserializeHeader(char **buffer)
{
	if(buffer == NULL)
	{
		return false;
	}
	if(*buffer == NULL)
	{
		return false;
	}

	memcpy(&m_messageType, *buffer, sizeof(m_messageType));
	*buffer += sizeof(m_messageType);

	memcpy(&m_protocolDirection, *buffer, sizeof(m_protocolDirection));
	*buffer += sizeof(m_protocolDirection);

	memcpy(&m_serialNumber, *buffer, sizeof(m_serialNumber));
	*buffer += sizeof(m_serialNumber);

	if((m_protocolDirection != DIRECTION_TO_UI) && (m_protocolDirection != DIRECTION_TO_NOVAD))
	{
		return false;
	}

	return true;
}

void Message::SerializeHeader(char **buffer)
{
	memcpy(*buffer, &m_messageType, sizeof(m_messageType));
	*buffer += sizeof(m_messageType);

	memcpy(*buffer, &m_protocolDirection, sizeof(m_protocolDirection));
	*buffer += sizeof(m_protocolDirection);

	memcpy(*buffer, &m_serialNumber, sizeof(m_serialNumber));
	*buffer += sizeof(m_serialNumber);
}

Message *Message::Deserialize(char *buffer, uint32_t length, enum ProtocolDirection direction)
{
	if(length < MESSAGE_MIN_SIZE)
	{
		return new ErrorMessage(ERROR_MALFORMED_MESSAGE, direction);
	}

	enum MessageType thisType;
	memcpy(&thisType, buffer, MESSAGE_MIN_SIZE);

	Message *message;
	switch(thisType)
	{
		case CONTROL_MESSAGE:
		{
			message = new ControlMessage(buffer, length);
			break;
		}
		case UPDATE_MESSAGE:
		{
			message = new UpdateMessage(buffer, length);
			break;
		}
		case REQUEST_MESSAGE:
		{
			message = new RequestMessage(buffer, length);
			break;
		}
		case ERROR_MESSAGE:
		{
			message = new ErrorMessage(buffer, length);
			break;
		}
		default:
		{
			return new ErrorMessage(ERROR_UNKNOWN_MESSAGE_TYPE, direction);
		}
	}

	if(message->m_serializeError)
	{
		delete message;
		return new ErrorMessage(ERROR_MALFORMED_MESSAGE, direction);
	}
	return message;
}

char *Message::Serialize(uint32_t *length)
{
	//Doesn't actually get called
	return NULL;
}
