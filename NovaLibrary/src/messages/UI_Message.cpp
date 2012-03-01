//============================================================================
// Name        : UI_Message.cpp
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
// Description : Message object for GUI communication with Nova processes
//============================================================================/*

#include "UI_Message.h"
#include <sys/un.h>


using namespace std;

namespace Nova
{

UI_Message::UI_Message()
{
	m_type = INVALID;
	m_value = NONE;
}


UI_Message::UI_Message(UI_MessageType t)
{
	switch(t)
	{
		case EXIT:
		case CLEAR_ALL:
		case RELOAD:
			m_type = t;
			break;
		//If type is unrecognized or requires an argument set the type to INVALID
		default:
			m_type = INVALID;
			break;
	}
	m_value = NONE;
}


UI_Message::UI_Message(UI_MessageType t, string v)
{
	switch(t)
	{
		//Requires an argument, if it has none, set to INVALID.
		case CLEAR_SUSPECT:
		case WRITE_SUSPECTS:
			m_type = t;
			if(v.compare(NONE))
			{
				m_type = INVALID;
				m_value = NONE;
				break;
			}
			m_value = v;
			break;

		//These don't require an argument so we ignore it
		case EXIT:
		case CLEAR_ALL:
		case RELOAD:
			m_type = t;
			m_value = NONE;
			break;

		//If the message type is unrecognized
		default:
			m_type = INVALID;
			m_value = NONE;
			break;
	}
}


bool UI_Message::SetMessage(UI_MessageType t)
{
	//Only sets if the type is recognized and requires no argument
	switch(t)
	{
		case EXIT:
		case CLEAR_ALL:
		case RELOAD:
			m_type = t;
			m_value = NONE;
			return true;

		default:
			return false;
	}
}


bool UI_Message::SetMessage(UI_MessageType t, string v)
{
	switch(t)
	{
		//Requires an argument, invalid if it has none.
		case CLEAR_SUSPECT:
		case WRITE_SUSPECTS:
			if(v.compare("") == 0)
				return false;
			m_type = t;
			m_value = v;
			return true;

		//Doesn't take an argument, ignores it.
		case EXIT:
		case CLEAR_ALL:
		case RELOAD:
			m_type = t;
			m_value = NONE;
			return true;

		//Returns false if the type is not recognized
		default:
			return false;
	}
}


UI_MessageType UI_Message::GetType()
{
	return m_type;
}


string UI_Message::GetValue()
{
	return m_value;
}


uint UI_Message::SerialzeMessage(u_char * buf)
{
	//Only works if a valid message
	if(m_type != INVALID)
	{
		uint offset = 0;
		bzero(buf, 1+m_value.size()); //clear buffer for message
		memcpy(buf, &m_type, 1); //char is 1 byte
		offset++;
		switch(m_type)
		{
			//Only works if has an argument for types that need it
			case CLEAR_SUSPECT:
			case WRITE_SUSPECTS:
				if(m_value.compare(NONE))
				{
					strcpy((char*)buf+offset, m_value.c_str());
					offset += m_value.size();
					return offset;
				}
				bzero(buf, 1);
				return 0;
			default:
				return offset;
		}
	}
	return 0;
}


uint UI_Message::DeserializeMessage(u_char * buf)
{
	uint offset = 0;
	char c[MAX_VAL_SIZE];

	memcpy(&m_type, buf, 1); //char is 1 byte
	offset++;
	switch(m_type)
	{
		//If message that has an argument.
		case CLEAR_SUSPECT:
		case WRITE_SUSPECTS:
			strncpy(c, (char *)buf+offset, MAX_VAL_SIZE);
			m_value = c;

			//If argument is present
			if(m_value.compare(NONE))
			{
				offset += m_value.size();
				return offset;
			}

			//else no argument
			bzero(buf, 1);
			m_type = INVALID;
			return 0;

		//If message with no argument
		case EXIT:
		case CLEAR_ALL:
		case RELOAD:
			return offset;

		//if unrecognized msg
		default:
			bzero(buf, 1);
			return 0;
	}
}
}
