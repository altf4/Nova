//============================================================================
// Name        : CallbackMessage.h
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

#ifndef CALLBACKMESSAGE_H_
#define CALLBACKMESSAGE_H_

#include "UI_Message.h"
#include "../../Suspect.h"

#define CALLBACK_MSG_MIN_SIZE 2

//The different message types
enum CallbackType: char
{
	CALLBACK_SUSPECT_UDPATE = 0,		//Request for Novad to exit
	CALLBACK_SUSPECT_UDPATE_ACK = 1,		//Reply from Novad with success
};

namespace Nova
{

class CallbackMessage : public UI_Message
{

public:

	CallbackMessage(enum CallbackType callbackType, enum ProtocolDirection direction);
	~CallbackMessage();

	//Deserialization constructor
	//	buffer - pointer to array in memory where serialized ControlMessage resides
	//	length - the length of this array
	//	On error, sets m_serializeError to true, on success sets it to false
	CallbackMessage(char *buffer, uint32_t length);


	enum CallbackType m_callbackType;
	uint32_t m_suspectLength;
	Suspect *m_suspect;

protected:
	//Serializes the UI_Message object into a char array
	//	*length - Return parameter, specifies the length of the serialized array returned
	// Returns - A pointer to the serialized array
	//	NOTE: The caller must manually free() the returned buffer after use
	char *Serialize(uint32_t *length);
};

}

#endif /* CALLBACKMESSAGE_H_ */
