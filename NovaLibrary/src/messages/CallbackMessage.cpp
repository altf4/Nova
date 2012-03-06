//============================================================================
// Name        : CallbackMessage.cpp
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
// Description : Messages coming asynchronously from Novad to the UI
//============================================================================

#include "CallbackMessage.h"
#include <string.h>

namespace Nova
{

CallbackMessage::CallbackMessage()
{
	m_messageType = CALLBACK_MESSAGE;
}

CallbackMessage::CallbackMessage(char *buffer, uint32_t length)
{
	if( length < CALLBACK_MSG_MIN_SIZE )
	{
		return;
	}

	m_serializeError = false;

	//Copy the message type
	memcpy(&m_messageType, buffer, sizeof(m_messageType));
	buffer += sizeof(m_messageType);

	//Copy the control message type
	memcpy(&m_callbackType, buffer, sizeof(m_callbackType));
	buffer += sizeof(m_callbackType);

	switch(m_callbackType)
	{
		case CALLBACK_SUSPECT_UDPATE:
		{

		}
		case CALLBACK_SUSPECT_UDPATE_ACK:
		{

		}
		default:
		{

		}
	}
}

char *CallbackMessage::Serialize(uint32_t *length)
{
	char *buffer, *originalBuffer;
	uint32_t messageSize;

	switch(m_callbackType)
	{
		case CALLBACK_SUSPECT_UDPATE:
		{

		}
		case CALLBACK_SUSPECT_UDPATE_ACK:
		{

		}
		default:
		{

		}
	}
}

}
