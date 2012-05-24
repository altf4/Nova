//============================================================================
// Name        : WhitelistConfiguration.cpp
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
// Description : Configuration for the whitelist
//============================================================================/*

#include "WhitelistConfiguration.h"
#include "Logger.h"

#include <fstream>
#include <sstream>
#include <iostream>

using namespace std;

namespace Nova
{

bool WhitelistConfiguration::AddIp(std::string ip)
{
	return WhitelistConfiguration::AddEntry(ip);
}

bool WhitelistConfiguration::AddIpRange(std::string ip, std::string netmask)
{
	return WhitelistConfiguration::AddEntry(ip + "/" + netmask);
}


bool WhitelistConfiguration::DeleteEntry(std::string entry)
{
	ifstream ipListFileStream(Config::Inst()->GetPathWhitelistFile());
	stringstream ipListNew;

	if(ipListFileStream.is_open())
	{
		while(ipListFileStream.good())
		{
			string line;
			if (getline (ipListFileStream,line) == 0)
			{
				continue;
			}
			if(line != entry)
			{
				ipListNew << line << endl;
			}
		}
		ipListFileStream.close();
	}
	else
	{
		LOG(ERROR,"Unable to open file: " + Config::Inst()->GetPathWhitelistFile(), "");
		return false;
	}

	ofstream whitelist(Config::Inst()->GetPathWhitelistFile());
	if (whitelist.is_open())
	{
		whitelist << ipListNew.str();
		whitelist.close();
	}
	else
	{
		LOG(ERROR,"Unable to open file: " + Config::Inst()->GetPathWhitelistFile(), "");
		return false;
	}

	return true;
}


std::vector<std::string> WhitelistConfiguration::GetIps()
{
	return WhitelistConfiguration::GetWhitelistedIps(false);
}

std::vector<std::string> WhitelistConfiguration::GetIpRanges()
{
	return WhitelistConfiguration::GetWhitelistedIps(true);
}

string WhitelistConfiguration::GetSubnet(string whitelistEntry)
{
	uint seperator = whitelistEntry.find("/");
	if (seperator != string::npos)
	{
		return (whitelistEntry.substr(seperator + 1, string::npos));
	}
	else
	{
		return "";
	}
}

string WhitelistConfiguration::GetIp(string whitelistEntry)
{
	uint seperator = whitelistEntry.find("/");
	if (seperator != string::npos)
	{
		return (whitelistEntry.substr(0, seperator));
	}
	else
	{
		return whitelistEntry;
	}

}

// Private helper functions

vector<string> WhitelistConfiguration::GetWhitelistedIps(bool getRanges)
{
	ifstream ipListFileStream(Config::Inst()->GetPathWhitelistFile());
	vector<string> whitelistedAddresses;

	if(ipListFileStream.is_open())
	{
		while(ipListFileStream.good())
		{
			string line;
			if (getline (ipListFileStream,line) == 0)
			{
				break;
			}
			if(strcmp(line.c_str(), "") && line.at(0) != '#' )
			{
				// Shouldn't have any spaces if it's just an IP
				// TODO: should trim whitespace at the end of the lines
				if (line.find("/") == string::npos && !getRanges)
				{
					whitelistedAddresses.push_back(line);
				}
				else if (line.find("/") != string::npos && getRanges)
				{
					whitelistedAddresses.push_back(line);
				}
			}
		}
		ipListFileStream.close();
	}
	else
	{
		LOG(ERROR,"Unable to open file: " + Config::Inst()->GetPathWhitelistFile(), "");
	}

	return whitelistedAddresses;
}

bool WhitelistConfiguration::AddEntry(std::string entry)
{
	ifstream ipListFileStream(Config::Inst()->GetPathWhitelistFile());
	stringstream ipListNew;
	bool alreadyExists = false;

	if(ipListFileStream.is_open())
	{
		while(ipListFileStream.good())
		{
			string line;
			if (getline (ipListFileStream,line) == 0)
			{
				continue;
			}
			if(line == entry)
			{
				alreadyExists = true;
				break;
			}
			else
			{
				ipListNew << line << endl;
			}
		}
		ipListFileStream.close();
	}
	else
	{
		LOG(ERROR,"Unable to open file: " + Config::Inst()->GetPathWhitelistFile(), "");
		return false;
	}

	if (!alreadyExists)
	{
		ofstream whitelist(Config::Inst()->GetPathWhitelistFile());
		if (whitelist.is_open())
		{
			whitelist << ipListNew.str();
			whitelist << entry;
			whitelist.close();
		}
		else
		{
			LOG(ERROR,"Unable to open file: " + Config::Inst()->GetPathWhitelistFile(), "");
			return false;
		}
	}

	return true;
}


} /* namespace Nova */
