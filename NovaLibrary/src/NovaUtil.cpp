//============================================================================
// Name        : NovaUtil.cpp
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Utility class for storing functions that are used by multiple
//				 processes but don't warrant their own class
//============================================================================/*

#include "NovaUtil.h"

using namespace std;

namespace Nova{

//Encrpyts/decrypts a char buffer of size 'size' depending on mode
void CryptBuffer(u_char * buf, uint size, bool mode)
{
	//TODO
}

// Reads the paths file and returns the homePath of nova
string GetHomePath()
{
	//Get locations of nova files
	ifstream *paths =  new ifstream(PATHS_FILE);
	string prefix, homePath, line;

	if(paths->is_open())
	{
		while(paths->good())
		{
			getline(*paths,line);

			prefix = "NOVA_HOME";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				homePath = line;
				break;
			}
		}
	}
	paths->close();
	delete paths;
	paths = NULL;

	//Resolves environment variables
	homePath = ResolvePathVars(homePath);

	if(homePath == "")
	{
		exit(1);
	}
	return homePath;
}
// Replaces any env vars in 'path' and returns the absolute path
string ResolvePathVars(string path)
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
uint GetSerializedAddr(u_char * buf)
{
	uint addr = 0;
	memcpy(&addr, buf, 4);
	return addr;
}

//Returns the number of bits used in the mask when given in in_addr_t form
int GetMaskBits(in_addr_t mask)
{
	mask = ~mask;
	int i = 32;
	while(mask != 0)
	{
		mask = mask/2;
		i--;
	}
	return i;
}

}
