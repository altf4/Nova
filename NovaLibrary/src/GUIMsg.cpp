//============================================================================
// Name        : GUIMsg.cpp
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Message object for GUI communication with Nova processes
//============================================================================/*

#include "GUIMsg.h"


using namespace std;

namespace Nova
{

GUIMsg::GUIMsg()
{
	type = INVALID;
	val = NONE;
}


GUIMsg::GUIMsg(GUIMsgType t)
{
	switch(t)
	{
		case EXIT:
			type = t;
			break;
		case CLEAR_ALL:
			type = t;
			break;
		//If type is unrecognized or requires an argument set the type to INVALID
		default:
			type = INVALID;
			break;
	}
	val = NONE;
}


GUIMsg::GUIMsg(GUIMsgType t, string v)
{
	switch(t)
	{
		//Requires an argument, if it has none, set to INVALID.
		case CLEAR_SUSPECT:
			type = t;
			if(v.compare(NONE))
			{
				type = INVALID;
				val = NONE;
				break;
			}
			val = v;
			break;

		case WRITE_SUSPECTS:
			type = t;
			if(v.compare(NONE))
			{
				type = INVALID;
				val = NONE;
				break;
			}
			val = v;
			break;

		//These don't require an argument so we ignore it
		case EXIT:
			type = t;
			val = NONE;
			break;

		//These don't require an argument so we ignore it
		case CLEAR_ALL:
			type = t;
			val = NONE;
			break;

		//If the message type is unrecognized
		default:
			type = INVALID;
			val = NONE;
			break;
	}
}


bool GUIMsg::SetMessage(GUIMsgType t)
{
	//Only sets if the type is recognized and requires no argument
	switch(t)
	{
		case EXIT:
			type = t;
			val = NONE;
			return true;
		case CLEAR_ALL:
			type = t;
			val = NONE;
			return true;

		default:
			return false;
	}
}


bool GUIMsg::SetMessage(GUIMsgType t, string v)
{
	switch(t)
	{
		//Requires an argument, invalid if it has none.
		case CLEAR_SUSPECT:
			if(v.compare("") == 0)
				return false;
			type = t;
			val = v;
			return true;

		//Requires an argument, invalid if it has none.
		case WRITE_SUSPECTS:
			if(v.compare("") == 0)
				return false;
			type = t;
			val = v;
			return true;

		//Doesn't take an argument, ignores it.
		case EXIT:
			type = t;
			val = NONE;
			return true;

		//Doesn't take an argument, ignores it.
		case CLEAR_ALL:
			type = t;
			val = NONE;
			return true;

		//Returns false if the type is not recognized
		default:
			return false;
	}
}


GUIMsgType GUIMsg::GetType()
{
	return type;
}


string GUIMsg::GetValue()
{
	return val;
}


uint GUIMsg::SerialzeMessage(u_char * buf)
{
	//Only works if a valid message
	if(type != INVALID)
	{
		uint offset = 0;
		bzero(buf, 1+val.size()); //clear buffer for message
		memcpy(buf, &type, 1); //char is 1 byte
		offset++;
		switch(type)
		{
			//Only works if has an argument for types that need it
			case CLEAR_SUSPECT:
				if(val.compare(NONE))
				{
					strcpy((char*)buf+offset, val.c_str());
					offset += val.size();
					return offset;
				}
				bzero(buf, 1);
				return 0;
			case WRITE_SUSPECTS:
				if(val.compare(NONE))
				{
					strcpy((char*)buf+offset, val.c_str());
					offset += val.size();
					return offset;
				}
				bzero(buf, 1);
				return 0;
			default:
				return offset;
		}
	}
	return 0;
}


uint GUIMsg::DeserializeMessage(u_char * buf)
{
	uint offset = 0;
	char c[MAX_VAL_SIZE];

	memcpy(&type, buf, 1); //char is 1 byte
	offset++;
	switch(type)
	{
		//If message that has an argument.
		case CLEAR_SUSPECT:
			strncpy(c, (char *)buf+offset, MAX_VAL_SIZE);
			val = c;

			//If argument is present
			if(val.compare(NONE))
			{
				offset += val.size();
				return offset;
			}

			//else no argument
			bzero(buf, 1);
			type = INVALID;
			return 0;

		case WRITE_SUSPECTS:
			strncpy(c, (char *)buf+offset, MAX_VAL_SIZE);
			val = c;

			//If argument is present
			if(val.compare(NONE))
			{
				offset += val.size();
				return offset;
			}

			//else no argument
			bzero(buf, 1);
			type = INVALID;
			return 0;

		//If message with no argument
		case EXIT:
		case CLEAR_ALL:
			return offset;

		//if unrecognized msg
		default:
			bzero(buf, 1);
			return 0;
	}
}
}
