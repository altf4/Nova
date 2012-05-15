//============================================================================
// Name        : ErrorMessage.h
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

#ifndef ERRORMESSAGE_H_
#define ERRORMESSAGE_H_

#include "Message.h"

#define ERROR_MSG_MIN_SIZE 2

//The different message types
enum ErrorType: char
{
	//Return code types
	//(IE: NOT meant to be remotely sent or received)
	ERROR_SOCKET_CLOSED = 0,		//Message could not be read because the socket was closed
	ERROR_MALFORMED_MESSAGE,		//Message was received, but could not deserialize
	ERROR_UNKNOWN_MESSAGE_TYPE,		//The primary message type is unknown
	ERROR_TIMEOUT, 					//The call to read() timed out

	//Error codes for actual remote messages
	ERROR_PROTOCOL_MISTAKE			//Reply from Novad with success
};

namespace Nova
{

class ErrorMessage : public Message
{

public:

	ErrorMessage(enum ErrorType errorType, enum ProtocolDirection direction);
	~ErrorMessage();

	//Deserialization constructor
	//	buffer - pointer to array in memory where serialized ControlMessage resides
	//	length - the length of this array
	//	On error, sets m_serializeError to true, on success sets it to false
	ErrorMessage(char *buffer, uint32_t length);

	enum ErrorType m_errorType;

protected:

	//Serializes the ErrorMessage object into a char array
	//	*length - Return parameter, specifies the length of the serialized array returned
	// Returns - A pointer to the serialized array
	//	NOTE: The caller must manually free() the returned buffer after use
	char *Serialize(uint32_t *length);
};

}

#endif /* ERRORMESSAGE_H_ */
