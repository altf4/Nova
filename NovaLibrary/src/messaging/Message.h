//============================================================================
// Name        : Message.h
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
// Description : Parent message class for all message subtypes. Suitable for any
//		communications over a stream socket
//============================================================================

#ifndef Message_H_
#define Message_H_

#include <stdlib.h>
#include "stdint.h"

#include "Suspect.h"

#define MESSAGE_MIN_SIZE 1

namespace Nova
{

class Message
{
public:

	//Empty constructor
	Message(){};
	Message(enum MessageType type);
	~Message();

	// This is called to delete any contents that the message is wrapping.
	// It is assumed the destructor will leave the contents alone, so the Message
	// can be safely deleted while what it wraps can continue to be used.
	void DeleteContents();

	//Creates a new Message from a given buffer. Calls the appropriate child constructor
	//	buffer - char pointer to a buffer in memory where the serialized message is at
	//	length - length of the buffer
	// Returns - true on success, false on error
	bool Deserialize(char *buffer, uint32_t length);

	//Serializes the Message object into a char array
	//  buffer - Contiguous array of memory to serialize to
	//	length - Length of the given buffer, maximum number of bytes to write
	// Returns - true on success, false on error
	bool Serialize(char *buffer, uint32_t length);

	//Returns the length of the message, as it would be serialized
	uint32_t GetLength();

	Message_pb m_contents;
	std::vector <Suspect*> m_suspects;
};

}

#endif /* Message_H_ */
