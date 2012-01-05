//============================================================================
// Name        : GUIMsg.h
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Message object for GUI communication with Nova processes
//============================================================================/*

#ifndef GUIMSG_H_
#define GUIMSG_H_

#include "NovaUtil.h"



//Maximum number of characters an argument can have.
#define MAX_VAL_SIZE 255

//Default GUIMsg construction val, doubles as error flag.
#define NONE ""

//The different message types
enum GUIMsgType
{
	EXIT = 'e',
	CLEAR_ALL = 'c',
	CLEAR_SUSPECT = 's',
	INVALID = '0'
};

using namespace std;

namespace Nova{

class GUIMsg
{
	public:

	//********************
	//* Member Functions *
	//********************

	//For instantiation only, use other constructors if message is known.
	GUIMsg();

	//Constructor for messages that have no arguments
	GUIMsg(GUIMsgType type);

	//Constructor for messages that have an argument
	GUIMsg(GUIMsgType type, string val);

	//Sets the message, returns true if successful
	bool setMessage(GUIMsgType type);

	//Sets the message, returns true if successful
	bool setMessage(GUIMsgType type, string val);

	//Returns the message type
	char getType();

	//Returns the message argument
	string getValue();

	//Serializes the message into given buffer for communication
	//Returns bytes used
	uint serialzeMessage(u_char * buf);

	//Deserializes the message from the given buffer for reading
	//Returns bytes read
	uint deserializeMessage(u_char * buf);

	private:

	//********************
	//* Member Variables *
	//********************

	//The message type
	GUIMsgType type;
	//The argument if applicable.
	string val;
};
}
#endif /* GUIMSG_H_ */
