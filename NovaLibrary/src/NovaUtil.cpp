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

trainingDumpMap* ParseEngineCaptureFile(string captureFile)
{
	trainingDumpMap* trainingTable = new trainingDumpMap();
	trainingTable->set_empty_key("");

	ifstream dataFile(captureFile.data());
	string line, ip, data;

	if (dataFile.is_open())
	{
		while (dataFile.good() && getline(dataFile,line))
		{
			uint firstDelim = line.find_first_of(' ');

			if (firstDelim == string::npos)
			{
				syslog(SYSL_ERR, "Line: %d Error: Invalid or corrupt CE capture file.", __LINE__);
				return NULL;
			}

			ip = line.substr(0,firstDelim);
			data = line.substr(firstDelim + 1, string::npos);
			data = "\t" + data;

			if ((*trainingTable)[ip] == NULL)
				(*trainingTable)[ip] = new vector<string>();

			(*trainingTable)[ip]->push_back(data);
		}
	}
	else
	{
		syslog(SYSL_ERR, "Line: %d Error: unable to open CE capture file for reading.", __LINE__);
		return NULL;
	}
	dataFile.close();

	return trainingTable;
}

trainingSuspectMap* ParseTrainingDb(string dbPath)
{
	trainingSuspectMap* suspects = new trainingSuspectMap();
	suspects->set_empty_key("");

	string line;
	bool getHeader = true;
	uint delimIndex;

	trainingSuspect* suspect = new trainingSuspect();
	suspect->points = new vector<string>();

	ifstream stream(dbPath.data());
	if (stream.is_open())
	{
		while (stream.good() && getline(stream,line))
		{
			if (line.length() > 0)
			{
				if (getHeader)
				{
					delimIndex = line.find_first_of(' ');

					if (delimIndex == string::npos)
					{
						syslog(SYSL_ERR, "Line: %d Error: Invalid or corrupt DB training file", __LINE__);
						return NULL;
					}

					suspect->uid = line.substr(0,delimIndex);
					suspect->isHostile = atoi(line.substr(delimIndex + 1, 1).c_str());

					delimIndex = line.find_first_of('"');
					if (delimIndex == string::npos)
					{
						syslog(SYSL_ERR, "Line: %d Error: Invalid or corrupt DB training file", __LINE__);
						return NULL;
					}

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
	else
	{
		syslog(SYSL_ERR, "Line: %d Error: unable to open training DB file for reading.", __LINE__);
		return NULL;
	}
	stream.close();

	return suspects;
}

bool CaptureToTrainingDb(string dbFile, trainingSuspectMap* entries)
{
	trainingSuspectMap* db = ParseTrainingDb(dbFile);

	if (db == NULL)
		return false;

	int max = 0, uid = 0;
	for (trainingSuspectMap::iterator it = db->begin(); it != db->end(); it++)
	{
		uid = atoi(it->first.c_str());

		if (uid > max)
			max = uid;
	}

	uid = max + 1;
	ofstream out(dbFile.data(), ios::app);
	for (trainingSuspectMap::iterator header = entries->begin(); header != entries->end(); header++)
	{
		if (header->second->isIncluded)
		{
			out << uid << " " << header->second->isHostile << " \"" << header->second->description << "\"" << endl;
			for (vector<string>::iterator i = header->second->points->begin(); i != header->second->points->end(); i++)
				out << *i << endl;
			out << endl;

			uid++;
		}
	}

	out.close();

	return true;
}

string MakaDataFile(trainingSuspectMap& db)
{
	stringstream ss;

	for (trainingSuspectMap::iterator it = db.begin(); it != db.end(); it++)
	{
		if (it->second->isIncluded)
		{
			for (uint i = 0; i < it->second->points->size(); i++)
			{
				ss << it->second->points->at(i).substr(1, string::npos) << " " << it->second->isHostile << endl;
			}
		}
	}

	return ss.str();
}

void ThinTrainingPoints(trainingDumpMap* suspects, double distanceThreshhold)
{
	uint numThinned = 0, numTotal = 0;
	double maxValues[DIM];
	for (uint i = 0; i < DIM; i++)
		maxValues[i] = 0;

	// Parse out the max values for normalization
	for (trainingDumpMap::iterator it = suspects->begin(); it != suspects->end(); it++)
	{
		for (int p = it->second->size() - 1; p >= 0; p--)
		{
			numTotal++;
			stringstream ss(it->second->at(p));
			for (uint d = 0; d < DIM; d++)
			{
				string featureString;
				double feature;
				getline(ss, featureString, ' ');

				feature = atof(featureString.c_str());
				if (feature > maxValues[d])
					maxValues[d] = feature;
			}
		}
	}


	ANNpoint newerPoint = annAllocPt(DIM);
	ANNpoint olderPoint = annAllocPt(DIM);

	for (trainingDumpMap::iterator it = suspects->begin(); it != suspects->end(); it++)
	{
		// Can't trim points if there's only 1
		if (it->second->size() < 2)
			continue;

		stringstream ss(it->second->at(it->second->size() - 1));
		for (int d = 0; d < DIM; d++)
		{
			string feature;
			getline(ss, feature, ' ');
			newerPoint[d] = atof(feature.c_str()) / maxValues[d];
		}

		for (int p = it->second->size() - 2; p >= 0; p--)
		{
			double distance = 0;

			stringstream ss(it->second->at(p));
			for (uint d = 0; d < DIM; d++)
			{
				string feature;
				getline(ss, feature, ' ');
				olderPoint[d] = atof(feature.c_str()) / maxValues[d];
			}

			for(uint d=0; d < DIM; d++)
				distance += annDist(d, olderPoint,newerPoint);

			// Should we throw this point away?
			if (distance < distanceThreshhold)
			{
				it->second->erase(it->second->begin() + p);
				numThinned++;
			}
			else
			{
				for (uint d = 0; d < DIM; d++)
					newerPoint[d] = olderPoint[d];
			}
		}
	}

	cout << "Total points: " << numTotal << endl;
	cout << "Number Thinned: " << numThinned << endl;

	annDeallocPt(newerPoint);
	annDeallocPt(olderPoint);
}

}
