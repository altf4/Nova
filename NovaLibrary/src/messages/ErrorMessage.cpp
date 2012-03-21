//============================================================================
// Name        : ErrorMessage.cpp
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
// Description : Message describing some error condition
//============================================================================

#include "ErrorMessage.h"
#include <string.h>

namespace Nova
{

ErrorMessage::ErrorMessage()
{
	m_messageType = ERROR_MESSAGE;
}

ErrorMessage::~ErrorMessage()
{

}

ErrorMessage::ErrorMessage(char *buffer, uint32_t length)
{
	if( length < ERROR_MSG_MIN_SIZE )
	{
		m_serializeError = true;
		return;
	}

	m_serializeError = false;

	//Copy the message type
	memcpy(&m_messageType, buffer, sizeof(m_messageType));
	buffer += sizeof(m_messageType);

	//Copy the control message type
	memcpy(&m_errorType, buffer, sizeof(m_errorType));
	buffer += sizeof(m_errorType);

	switch(m_errorType)
	{
		case ERROR_PROTOCOL_MISTAKE:
		{
			//Uses: 1) UI_Message Type
			//		2) Callback Type
			uint32_t expectedSize = sizeof(m_messageType) + sizeof(m_errorType);
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

char *ErrorMessage::Serialize(uint32_t *length)
{
	char *buffer, *originalBuffer;
	uint32_t messageSize;

	switch(m_errorType)
	{
		case ERROR_PROTOCOL_MISTAKE:
		{
			//Uses: 1) UI_Message Type
			//		2) ErrorMessage Type

			messageSize = sizeof(m_messageType) + sizeof(m_errorType);
			buffer = (char*)malloc(messageSize);
			originalBuffer = buffer;

			//Put the UI Message type in
			memcpy(buffer, &m_messageType, sizeof(m_messageType));
			buffer += sizeof(m_messageType);
			//Put the Control Message type in
			memcpy(buffer, &m_errorType, sizeof(m_errorType));
			buffer += sizeof(m_errorType);

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


