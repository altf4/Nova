//============================================================================
// Name        : UpdateMessage.h
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
// Description : Messages coming asynchronously from Novad to the UI, updating
//		the UI with some new piece of information
//============================================================================

#ifndef UPDATEMESSAGE_H_
#define UPDATEMESSAGE_H_

#include "Message.h"
#include "../../Suspect.h"
#include "../../protobuf/marshalled_classes.pb.h"

#define UPDATE_MSG_MIN_SIZE 2

namespace Nova
{

class UpdateMessage : public Message
{

public:

	UpdateMessage(enum UpdateType updateType);
	~UpdateMessage();
	virtual void DeleteContents();

	//Deserialization constructor
	//	buffer - pointer to array in memory where serialized UpdateMessage resides
	//	length - the length of this array
	//	On error, sets m_serializeError to true, on success sets it to false
	UpdateMessage(char *buffer, uint32_t length);

	//Serializes the Message object into a char array
	//	*length - Return parameter, specifies the length of the serialized array returned
	// Returns - A pointer to the serialized array
	//	NOTE: The caller must manually free() the returned buffer after use
	char *Serialize(uint32_t *length);

	Suspect *m_suspect;
	UpdateMessage_pb m_contents;
};

}

#endif /* UPDATEMESSAGE_H_ */
