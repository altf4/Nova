//============================================================================
// Name        : UI_Message.h
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

#ifndef UI_Message_H_
#define UI_Message_H_

#include <string>
#include <sys/types.h>


//Maximum number of characters an argument can have.
#define MAX_VAL_SIZE 255

//Default UI_Message construction val, doubles as error flag.
#define NONE ""

//The different message types
enum UI_MessageType
{
	EXIT = 'e',
	CLEAR_ALL = 'c',
	CLEAR_SUSPECT = 's',
	INVALID = '0',
	WRITE_SUSPECTS = 'w',
	RELOAD = 'r'
};

using namespace std;

namespace Nova{

class UI_Message
{
	public:

	//********************
	//* Member Functions *
	//********************

	// For instantiation only, use other constructors if message is known.
	UI_Message();

	// Constructor for messages that have no arguments
	//		type - type of message to create
	UI_Message(UI_MessageType type);

	// Constructor for messages that have an argument
	//		type - type of message to create
	//		val - argument to send along with the message
	UI_Message(UI_MessageType type, string val);

	// Sets the message
	//		type - type of message to create
	// Returns: true if successful
	bool SetMessage(UI_MessageType type);

	// Sets the message
	//		type - type of message to create
	//		val - argument to send along with the message
	// Returns: true if successful
	bool SetMessage(UI_MessageType type, string val);

	// Returns the message type
	UI_MessageType GetType();

	// Returns the message argument
	string GetValue();

	// Serializes the message into given buffer for communication
	//		buf - Pointer to buffer to store serialized data
	// Returns: Number of bytes in buffer used
	uint SerialzeMessage(u_char * buf);

	// Deserializes the message from the given buffer for reading
	//		buf - Points to buffer to read serialized data from
	// Returns: Number of bytes read
	uint DeserializeMessage(u_char * buf);

	private:

	//********************
	//* Member Variables *
	//********************

	// The message type
	UI_MessageType m_type;

	// The argument if applicable.
	string m_value;
};
}
#endif /* UI_Message_H_ */
