//============================================================================
// Name        : GUIMsg.h
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Message object for GUI communication with Nova processes
//============================================================================/*

#ifndef GUIMSG_H_
#define GUIMSG_H_

#include <string>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

//The different message types
#define EXIT 'e'
#define CLEAR_ALL 'c'
#define CLEAR_SUSPECT 's'

//Default construction, doubles as error flag.
#define INVALID '0'
#define NONE ""

//Maximum number of characters an argument can have.
#define MAX_VAL_SIZE 255
#define MAX_GUIMSG_SIZE 256

using namespace std;
namespace Nova
{
class GUIMsg
{
	public:

	//********************
	//* Member Functions *
	//********************

	//For instantiation only, use other constructors if message is known.
	GUIMsg();

	//Constructor for messages that have no arguments
	GUIMsg(char type);

	//Constructor for messages that have an argument
	GUIMsg(char type, string val);

	//Sets the message, returns true if successful
	bool setMessage(char type);

	//Sets the message, returns true if successful
	bool setMessage(char type, string val);

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
	char type;
	//The argument if applicable.
	string val;
};
}
#endif /* GUIMSG_H_ */
