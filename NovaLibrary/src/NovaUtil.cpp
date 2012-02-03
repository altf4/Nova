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

	if(homePath == "")
	{
		exit(1);
	}
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

	if(readPath == "")
	{
		exit(1);
	}
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

	if(writePath == "")
	{
		exit(1);
	}
	return writePath;
}

string GetLocalIP(const char *dev)
{
	static struct ifreq ifreqs[20];
	struct ifconf ifconf;
	uint  nifaces, i;

	openlog(__FILE__, OPEN_SYSL, LOG_AUTHPRIV);

	memset(&ifconf,0,sizeof(ifconf));
	ifconf.ifc_buf = (char*) (ifreqs);
	ifconf.ifc_len = sizeof(ifreqs);

	int sock, rval;
	sock = socket(AF_INET,SOCK_STREAM,0);

	if(sock < 0)
	{
		syslog(SYSL_ERR, "Line: %d socket: %s", __LINE__, strerror(errno));
		close(sock);
		return NULL;
	}

	if((rval = ioctl(sock, SIOCGIFCONF , (char*) &ifconf)) < 0 )
	{
		syslog(SYSL_ERR, "Line: %d ioctl(SIOGIFCONF): %s", __LINE__, strerror(errno));
	}

	close(sock);
	nifaces =  ifconf.ifc_len/sizeof(struct ifreq);

	for(i = 0; i < nifaces; i++)
	{
		if( strcmp(ifreqs[i].ifr_name, dev) == 0 )
		{
			char ip_addr [ INET_ADDRSTRLEN ];
			struct sockaddr_in *b = (struct sockaddr_in *) &(ifreqs[i].ifr_addr);

			inet_ntop(AF_INET, &(b->sin_addr), ip_addr, INET_ADDRSTRLEN);
			return string(ip_addr);
		}
	}
	return string("");
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


uint GetSerializedAddr(u_char * buf)
{
	uint addr = 0;
	memcpy(&addr, buf, 4);
	return addr;
}


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

trainingDumpMap* ReadEngineDumpFile(string inputFile)
{
	trainingDumpMap* trainingTable = new trainingDumpMap();
	trainingTable->set_empty_key("");

	ifstream dataFile(inputFile.data());
	string line, ip, data;

	if (dataFile.is_open())
	{
		while (dataFile.good() && getline(dataFile,line))
		{
			ip = line.substr(0,line.find_first_of(' '));
			data = line.substr(line.find_first_of(' ') + 1, string::npos);
			data = "\t" + data;

			(*trainingTable)[ip] = new vector<string>();
			(*trainingTable)[ip]->push_back(data);
		}
	}
	dataFile.close();

	return trainingTable;
}

trainingSuspectMap* ReadClassificationDb(string inputFile)
{
	trainingSuspectMap* suspects = new trainingSuspectMap();
	suspects->set_empty_key("");

	string line;
	bool getHeader = true;

	trainingSuspect* suspect = new trainingSuspect();
	suspect->points = new vector<string>();

	ifstream stream(inputFile.data());
	if (stream.is_open())
	{
		while (stream.good() && getline(stream,line))
		{
			if (line.length() > 0)
			{
				if (getHeader)
				{
					suspect->uid = line.substr(0,line.find_first_of(' '));
					suspect->isHostile = atoi(line.substr(line.find_first_of(' ') + 1, 1).c_str());
					suspect->description = line.substr(line.find_first_of('"'), line.length());
					getHeader = false;
				}
				else {
					suspect->points->push_back(line);
				}
			}
			else
			{
				if (!getHeader)
				{
					(*suspects)[suspect->uid] = suspect;
					suspect = new trainingSuspect();
					suspect->points = new vector<string>();
					getHeader = true;
				}
			}
		}
	}
	stream.close();

	return suspects;
}

void MergeDumpIntoDb(string inputFile, string outputFile, trainingSuspectMap* headerMap)
{
	trainingDumpMap* trainingTable = ReadEngineDumpFile(inputFile);

	string line;
	bool getUID = true;
	int uid, max = 0;
	ifstream stream(outputFile.data());
	if (stream.is_open())
	{
		while (stream.good() && getline(stream,line))
		{
			if (line.length() > 0)
			{
				if (getUID)
					uid = atoi(line.substr(0,line.find_first_of(' ')).c_str());
				getUID = false;
			}
			else
			{
				getUID = true;
			}

			if (uid > max)
				max = uid;
		}
	}
	stream.close();


	uid = max + 1;
	ofstream out(outputFile.data());
	for (trainingDumpMap::iterator ipIt = trainingTable->begin(); ipIt != trainingTable->end(); ipIt++)
	{
		// TODO: make sure the input had all the IPs we're checking so this line doesn't make googlehashmap explode on error
		trainingSuspect* header = (*headerMap)[ipIt->first];

		if (header->isIncluded)
		{
			out << endl << 	uid << " " << header->isHostile << " \"" << header->description << "\"" << endl;
			for (vector<string>::iterator i = ipIt->second->begin(); i != ipIt->second->end(); i++)
			{
				out << *i << endl;
			}
			uid++;
		}
	}

	out.close();
}

string MakeCeFileFromDb(trainingSuspectMap& db)
{
	stringstream ss;

	for (trainingSuspectMap::iterator it = db.begin(); it != db.end(); it++)
	{
		if (it->second->isIncluded)
		{
			for (uint i = 0; i < it->second->points->size(); i++)
			{
				ss << it->second->points->at(i) << " " << it->second->isHostile << endl;
			}
		}
	}

	return ss.str();
}

}
