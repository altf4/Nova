//============================================================================
// Name        : ControlMessage.h
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

#ifndef CONTROLMESSAGE_H_
#define CONTROLMESSAGE_H_

#include <string>
#include <sys/types.h>
#include <arpa/inet.h>

#include "UI_Message.h"

//Maximum number of characters an argument can have.
#define MAX_PATH_SIZE 255
#define CONTROL_MSG_MIN_SIZE 2

//The different message types
enum ControlType: char
{
	CONTROL_EXIT_REQUEST = 0,		//Request for Novad to exit
	CONTROL_EXIT_REPLY,				//Reply from Novad with success
	CONTROL_CLEAR_ALL_REQUEST,		//Request for Novad to clear all suspects
	CONTROL_CLEAR_ALL_REPLY,		//Reply from Novad with success
	CONTROL_CLEAR_SUSPECT_REQUEST,	//Request for Novad to clear specified suspect
	CONTROL_CLEAR_SUSPECT_REPLY,	//Reply from Novad with success
	CONTROL_INVALID,
	CONTROL_WRITE_SUSPECTS,
	CONTROL_RELOAD,
};

using namespace std;

namespace Nova{

class ControlMessage : public UI_Message
{

public:

	//Empty constructor
	ControlMessage();

	//Deserialization constructor
	//	buffer - pointer to array in memory where serialized ControlMessage resides
	//	length - the length of this array
	//	On error, sets m_serializeError to true, on success sets it to false
	ControlMessage(char *buffer, uint32_t length);

	//Serializes the UI_Message object into a char array
	//	*length - Return parameter, specifies the length of the serialized array returned
	// Returns - A pointer to the serialized array
	//	NOTE: The caller must manually free() the returned buffer after use
	char *Serialze(uint32_t *length);

	enum ControlType controlType;

private:

	//********************
	//* Member Variables *
	//********************

	// The message type
	ControlType m_controlType;

	// The argument, if applicable.
	char m_filePath[MAX_PATH_SIZE];

	in_addr_t m_suspectAddress;
};
}

#endif /* CONTROLMESSAGE_H_ */
