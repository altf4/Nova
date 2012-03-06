//============================================================================
// Name        : NovaUtil.cpp
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
// Description : Utility class for storing functions that are used by multiple
//				 processes but don't warrant their own class
//============================================================================/*

#include "NovaUtil.h"
#include "Defines.h"
#include <fstream>

using namespace std;

namespace Nova{

void CryptBuffer(u_char * buf, uint size, bool mode)
{
	//TODO
}

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

	return homePath;
}
string GetReadPath()
{
	//Get locations of nova files
	ifstream *paths =  new ifstream(PATHS_FILE);
	string prefix, readPath, line;

	if(paths->is_open())
	{
		while(paths->good())
		{
			getline(*paths,line);

			prefix = "NOVA_RD";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				readPath = line;
				break;
			}
		}
	}
	paths->close();
	delete paths;
	paths = NULL;

	//Resolves environment variables
	readPath = ResolvePathVars(readPath);

	return readPath;
}

string GetWritePath()
{
	//Get locations of nova files
	ifstream *paths =  new ifstream(PATHS_FILE);
	string prefix, writePath, line;

	if(paths->is_open())
	{
		while(paths->good())
		{
			getline(*paths,line);

			prefix = "NOVA_WR";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				writePath = line;
				break;
			}
		}
	}
	paths->close();
	delete paths;
	paths = NULL;

	//Resolves environment variables
	writePath = ResolvePathVars(writePath);

	return writePath;
}


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
	if(var.compare(""))
		return var;
	else return path;
}



}
