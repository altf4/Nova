//============================================================================
// Name        : UpdateMessage.h
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
// Description : Messages coming asynchronously from Novad to the UI, updating
//		the UI with some new piece of information
//============================================================================

#ifndef UPDATEMESSAGE_H_
#define UPDATEMESSAGE_H_

#include "Message.h"
#include "../../Suspect.h"

#define UPDATE_MSG_MIN_SIZE 2

//The different message types
enum UpdateType: char
{
	UPDATE_SUSPECT = 0,					//A new or updated suspect being sent to a UI
	UPDATE_SUSPECT_ACK = 1,				//Reply from Novad with success
	UPDATE_ALL_SUSPECTS_CLEARED,		//A UI has cleared all suspect records
	UPDATE_ALL_SUSPECTS_CLEARED_ACK,	//Acknowledgment of UPDATE_ALL_SUSPECTS_CLEARED
	UPDATE_SUSPECT_CLEARED,				//A single suspect was cleared by a UI
	UPDATE_SUSPECT_CLEARED_ACK,			//Acknowledgment of UPDATE_ALL_SUSPECTS_CLEARED_ACK
};

namespace Nova
{

class UpdateMessage : public Message
{

public:

	UpdateMessage(enum UpdateType updateType, enum ProtocolDirection direction);
	~UpdateMessage();

	//Deserialization constructor
	//	buffer - pointer to array in memory where serialized UpdateMessage resides
	//	length - the length of this array
	//	On error, sets m_serializeError to true, on success sets it to false
	UpdateMessage(char *buffer, uint32_t length);


	enum UpdateType m_updateType;
	uint32_t m_suspectLength;
	Suspect *m_suspect;
	in_addr_t m_IPAddress;

protected:
	//Serializes the Message object into a char array
	//	*length - Return parameter, specifies the length of the serialized array returned
	// Returns - A pointer to the serialized array
	//	NOTE: The caller must manually free() the returned buffer after use
	char *Serialize(uint32_t *length);
};

}

#endif /* UPDATEMESSAGE_H_ */
