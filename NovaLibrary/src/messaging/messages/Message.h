//============================================================================
// Name        : Message.h
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

#ifndef Message_H_
#define Message_H_

#include <stdlib.h>
#include "stdint.h"

#define MESSAGE_MIN_SIZE 1

#define REPLY_TIMEOUT 3

//Currently, size is 2
#define MESSADE_HDR_SIZE sizeof(m_protocolDirection) + sizeof(m_messageType)

namespace Nova
{

//The direction that this message's PROTOCOL (not individual message) is going. Who initiated the first message in the protocol
//	IE: Is this a callback for Novad or the UI?
//Value is used by MessageQueue to allow for dumb queueing of messages
//	Otherwise the queue would have to be aware of the protocol used to know the direction
enum ProtocolDirection: char
{
	DIRECTION_TO_UI = 0,
	DIRECTION_TO_NOVAD = 1
};

enum MessageType: char
{
	CONTROL_MESSAGE = 0,
	UPDATE_MESSAGE,
	REQUEST_MESSAGE,
	ERROR_MESSAGE
};

class Message
{
public:

	//Empty constructor
	Message();
	virtual ~Message();

	//Reads a Message from the given socket
	//	NOTE: Blocking call, will wait until message appears
	//	connectFD - A valid socket
	//	timeout - Number of seconds to before returning a timeout error if no message received
	// Returns - Pointer to newly allocated Message object
	//				returns an ErrorMessage object on error. Will never return NULL.
	//	NOTE: The caller must manually delete the returned object when finished with it
	static Message *ReadMessage(int connectFD, enum ProtocolDirection direction, int timeout = 0);

	//Writes a given Message to the provided socket
	//	message - A pointer to the message object to send
	//	connectFD - a valid socket to send the message to
	// Returns - true on successfully sending the object, false on error
	static bool WriteMessage(Message *message, int connectFD);

	//Creates a new Message from a given buffer. Calls the appropriate child constructor
	//	buffer - char pointer to a buffer in memory where the serialized message is at
	//	length - length of the buffer
	//	direction - protocol direction that we expect the message to be going. Used in error conditions when there is no valid message
	// Returns - Pointer to newly allocated Message object
	//				returns NULL on error
	//	NOTE: The caller must manually delete the returned object when finished with it
	static Message *Deserialize(char *buffer, uint32_t length, enum ProtocolDirection direction);

	enum ProtocolDirection m_protocolDirection;

	enum MessageType m_messageType;

protected:

	//Serializes the Message object into a char array
	//	*length - Return parameter, specifies the length of the serialized array returned
	// Returns - A pointer to the serialized array
	//	NOTE: The caller must manually free() the returned buffer after use
	virtual char *Serialize(uint32_t *length);

	//Deserialize just the Message header, and advance the buffer input variable
	//	buffer: A pointer to the array of serialized bytes representing a message
	//	returns - True if deserialize happened without error, false on error
	bool DeserializeHeader(char **buffer);

	//Serializes the Message header into the given array
	//	buffer: Pointer to the array where the serialized bytes will go
	//	NOTE: Assumes there is space in *buffer for the header
	void SerializeHeader(char **buffer);

	//Used to indicate serialization error in constructors
	//	(Since constructors can't return NULL)
	//Not ever sent.
	bool m_serializeError;

};

}

#endif /* Message_H_ */
