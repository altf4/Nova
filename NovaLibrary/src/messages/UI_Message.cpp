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
// Description : Parent message class for GUI communication with Nova processes
//============================================================================/*

#include "UI_Message.h"
#include "ControlMessage.h"
#include "CallbackMessage.h"

#include "string.h"
#include <vector>

using namespace Nova;

UI_Message::UI_Message()
{

}

UI_Message::~UI_Message()
{

}

UI_Message *UI_Message::ReadMessage(int connectFD)
{
	//perform read operations ...
	char buff[4096];
	int bytesRead = 4096;
	vector <char> input;

	while( bytesRead == 4096)
	{
		bytesRead = read(connectFD, buff, 4096);
		if( bytesRead >= 0 )
		{
			input.insert(input.end(), buff, buff + bytesRead);
		}
		else
		{
			return NULL;
		}
	}

	if(input.size() < MESSAGE_MIN_SIZE)
	{
		return NULL;
	}

	return UI_Message::Deserialize(input.data(), input.size());
}

bool UI_Message::WriteMessage(UI_Message *message, int connectFD)
{
	uint32_t length;
	char *buffer = message->Serialize(&length);

	if( write(connectFD, buffer, length) < 0 )
	{
		free(buffer);
		return false;
	}

	free(buffer);
	return true;
}

UI_Message *UI_Message::Deserialize(char *buffer, uint32_t length)
{
	if(length < MESSAGE_MIN_SIZE)
	{
		return NULL;
	}

	enum UI_MessageType thisType;
	memcpy(&thisType, buffer, MESSAGE_MIN_SIZE);

	switch(thisType)
	{
		case CONTROL_MESSAGE:
		{
			ControlMessage *message = new ControlMessage(buffer, length);
			if(message->m_serializeError)
			{
				delete message;
				return NULL;
			}
			return message;
		}
		case CALLBACK_MESSAGE:
		{
			CallbackMessage *message = new CallbackMessage(buffer, length);
			if(message->m_serializeError)
			{
				delete message;
				return NULL;
			}
			return message;
		}
		default:
		{
			return NULL;
		}
	}
}

char *UI_Message::Serialize(uint32_t *length)
{
	//Doesn't actually get called
}
