//============================================================================
// Name        : ControlMessage.cpp
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
// Description : Message objects sent to control Novad's operation
//============================================================================

#include "ControlMessage.h"
#include <sys/un.h>

using namespace std;

namespace Nova
{

ControlMessage::ControlMessage()
{
}

ControlMessage::ControlMessage(char *buffer, uint32_t length)
{
	if( length < CONTROL_MSG_MIN_SIZE )
	{
		return;
	}

	m_serializeError = false;

	//Copy the message type
	memcpy(&messageType, buffer, sizeof(messageType));
	buffer += sizeof(messageType);

	//Copy the control message type
	memcpy(&m_controlType, buffer, sizeof(m_controlType));
	buffer += sizeof(m_controlType);

	switch(m_controlType)
	{
		case CONTROL_EXIT_REQUEST:
		{
			break;
		}
		case CONTROL_EXIT_REPLY:
		{
			break;
		}
		case CONTROL_CLEAR_ALL_REQUEST:
		{
			break;
		}
		case CONTROL_CLEAR_ALL_REPLY:
		{
			break;
		}
		case CONTROL_CLEAR_SUSPECT_REQUEST:
		{
			break;
		}
		case CONTROL_CLEAR_SUSPECT_REPLY:
		{
			break;
		}
		case CONTROL_INVALID:
		{
			break;
		}
		case CONTROL_WRITE_SUSPECTS:
		{
			break;
		}
		case CONTROL_RELOAD:
		{
			break;
		}
		default:
		{
			m_serializeError = true;
			return;
		}
	}
}


//ControlMessage::ControlMessage(ControlType t)
//{
//	switch(t)
//	{
//		case EXIT:
//		case CLEAR_ALL:
//		case RELOAD:
//			m_controlType = t;
//			break;
//		//If type is unrecognized or requires an argument set the type to INVALID
//		default:
//			m_controlType = INVALID;
//			break;
//	}
//	m_value = NONE;
//}


//ControlMessage::ControlMessage(ControlType t, string v)
//{
//	switch(t)
//	{
//		//Requires an argument, if it has none, set to INVALID.
//		case CLEAR_SUSPECT:
//		case WRITE_SUSPECTS:
//			m_controlType = t;
//			if(v.compare(NONE))
//			{
//				m_controlType = INVALID;
//				m_value = NONE;
//				break;
//			}
//			m_value = v;
//			break;
//
//		//These don't require an argument so we ignore it
//		case EXIT:
//		case CLEAR_ALL:
//		case RELOAD:
//			m_controlType = t;
//			m_value = NONE;
//			break;
//
//		//If the message type is unrecognized
//		default:
//			m_controlType = INVALID;
//			m_value = NONE;
//			break;
//	}
//}

//char *Serialze(uint32_t *length)
//{
//	//Only works if a valid message
//	if(m_controlType != INVALID)
//	{
//		uint offset = 0;
//		bzero(buf, 1+m_value.size()); //clear buffer for message
//		memcpy(buf, &m_controlType, 1); //char is 1 byte
//		offset++;
//		switch(m_controlType)
//		{
//			//Only works if has an argument for types that need it
//			case CLEAR_SUSPECT:
//			case WRITE_SUSPECTS:
//				if(m_value.compare(NONE))
//				{
//					strcpy((char*)buf+offset, m_value.c_str());
//					offset += m_value.size();
//					return offset;
//				}
//				bzero(buf, 1);
//				return 0;
//			default:
//				return offset;
//		}
//	}
//	return 0;
//}


//uint ControlMessage::DeserializeMessage(u_char * buf)
//{
//	uint offset = 0;
//	char c[MAX_VAL_SIZE];
//
//	memcpy(&m_controlType, buf, 1); //char is 1 byte
//	offset++;
//	switch(m_controlType)
//	{
//		//If message that has an argument.
//		case CLEAR_SUSPECT:
//		case WRITE_SUSPECTS:
//			strncpy(c, (char *)buf+offset, MAX_VAL_SIZE);
//			m_value = c;
//
//			//If argument is present
//			if(m_value.compare(NONE))
//			{
//				offset += m_value.size();
//				return offset;
//			}
//
//			//else no argument
//			bzero(buf, 1);
//			m_controlType = INVALID;
//			return 0;
//
//		//If message with no argument
//		case EXIT:
//		case CLEAR_ALL:
//		case RELOAD:
//			return offset;
//
//		//if unrecognized msg
//		default:
//			bzero(buf, 1);
//			return 0;
//	}
//}

}
