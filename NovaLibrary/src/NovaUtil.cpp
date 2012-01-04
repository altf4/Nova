//============================================================================
// Name        : NovaUtil.cpp
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Utility class for storing functions that are used by multiple
//				 processes but don't warrant their own class
//============================================================================/*

#include "NovaUtil.h"

using namespace std;

namespace Nova
{
namespace NovaUtil
{

//Encrpyts/decrypts a char buffer of size 'size' depending on mode
void cryptBuffer(u_char * buf, uint size, bool mode)
{
	//TODO
}

// Replaces any env vars in 'path' and returns the absolute path
string resolvePathVars(string path)
{
	int start = 0;
	int end = 0;
	string var = "";

	while((start = path.find("$",end)) != -1)
	{
		end = path.find("/", start);
		//If no path after environment var
		if(end == -1)
		{

			var = path.substr(start+1, path.size());
			var = getenv(var.c_str());
			path = path.substr(0,start) + var;
		}
		else
		{
			var = path.substr(start+1, end-1);
			var = getenv(var.c_str());
			var = var + path.substr(end, path.size());
			if(start > 0)
			{
				path = path.substr(0,start)+var;
			}
			else
			{
				path = var;
			}
		}
	}
	return var;
}

//Extracts and returns the IP Address from a serialized suspect located at buf
uint getSerializedAddr(u_char * buf)
{
	uint addr = 0;
	memcpy(&addr, buf, 4);
	return addr;
}

}
}
