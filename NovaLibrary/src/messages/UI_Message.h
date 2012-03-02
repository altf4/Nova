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
// Description : Parent message class for GUI communication with Nova processes
//============================================================================

#ifndef UI_Message_H_
#define UI_Message_H_

#include <stdlib.h>
#include "stdint.h"

#define MESSAGE_MIN_SIZE 1

namespace Nova
{

enum UI_MessageType: char
{
	CONTROL_MESSAGE = 0,
};

class UI_Message
{
public:

	//Empty constructor
	UI_Message();

	//Reads a UI_Message from the given socket
	//	NOTE: Blocking call, will wait until message appears
	//	connectFD - A valid socket
	// Returns - Pointer to newly allocated UI_Message object
	//				returns NULL on error
	//	NOTE: The caller must manually delete the returned object when finished with it
	static UI_Message *ReadMessage(int connectFD);

	//Writes a given UI_Message to the provided socket
	//	message - A pointer to the message object to send
	//	connectFD - a valid socket to send the message to
	// Returns - true on successfully sending the object, false on error
	static bool WriteMessage(UI_Message *message, int connectFD);

	enum UI_MessageType m_messageType;

protected:

	//Serializes the UI_Message object into a char array
	//	*length - Return parameter, specifies the length of the serialized array returned
	// Returns - A pointer to the serialized array
	//	NOTE: The caller must manually free() the returned buffer after use
	virtual char *Serialize(uint32_t *length);

	//Used to indicate serialization error in constructors
	//	(Since constructors can't return NULL)
	//Not ever sent.
	bool m_serializeError;

private:

	//Creates a new UI_Message from a given buffer. Calls the appropriate child constructor
	//	buffer - char pointer to a buffer in memory where the serialized message is at
	//	length - length of the buffer
	// Returns - Pointer to newly allocated UI_Message object
	//				returns NULL on error
	//	NOTE: The caller must manually delete the returned object when finished with it
	static UI_Message *Deserialize(char *buffer, uint32_t length);

};

}

#endif /* UI_Message_H_ */
