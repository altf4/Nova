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
// Description : Utility functions that are used by multiple components of nova
//					but do not warrant an object
//============================================================================

#include "NovaUtil.h"
#include "Logger.h"
#include "Config.h"

#include <sys/ioctl.h>
#include <string.h>
#include <net/if.h>
#include <errno.h>
#include <sstream>
#include <math.h>
#include <unistd.h>

using namespace std;

namespace Nova{

void CryptBuffer(u_char *buf, uint size, bool mode)
{
	//TODO
}

std::string GetLocalIP(string dev)
{
	return GetLocalIP(dev.c_str());
}

std::string GetLocalIP(const char *dev)
{
	static struct ifreq ifreqs[20];
	struct ifconf ifconf;
	uint  nifaces, i;

	memset(&ifconf,0,sizeof(ifconf));
	ifconf.ifc_buf = (char*) (ifreqs);
	ifconf.ifc_len = sizeof(ifreqs);

	int sock, rval;
	sock = socket(AF_INET,SOCK_STREAM,0);

	if(sock < 0)
	{
		LOG(ERROR, "Unable to determine the local interface's IP address.",
			"Error creating socket to check interface IP: "+string(strerror(errno)));
	}

	if((rval = ioctl(sock, SIOCGIFCONF , (char*) &ifconf)) < 0 )
	{
		LOG(ERROR, "Unable to determine the local interface's IP address.",
			"Error with getLocalIP socket ioctl(SIOGIFCONF): "+string(strerror(errno)));
	}

	close(sock);
	nifaces =  ifconf.ifc_len/sizeof(struct ifreq);

	for(i = 0; i < nifaces; i++)
	{
		if(strcmp(ifreqs[i].ifr_name, dev) == 0 )
		{
			char ip_addr [ INET_ADDRSTRLEN ];
			struct sockaddr_in *b = (struct sockaddr_in *) &(ifreqs[i].ifr_addr);

			inet_ntop(AF_INET, &(b->sin_addr), ip_addr, INET_ADDRSTRLEN);
			return string(ip_addr);
		}
	}
	return string("");
}

//Removes any instance of the specified character from the front and back of the string
//		str - reference to the string you want to modify
// 		c - character you wish to remove (Whitespace by default)
// Note: this function will result in an empty string, if every character is == c
void Trim(std::string& str, char c)
{
	while((!str.empty()) && (str.at(0) == c))
	{
		str = str.substr(1, str.size());
	}
	while((!str.empty()) && (str.at(str.size()-1) == c))
	{
		str = str.substr(0, str.size()-1);
	}
}

//Replaces any instance of the specified character inside the string
//		str - reference to the string you want to modify
// 		searchChar - character you wish to replace
//		replaceVal - character to insert into the string
void ReplaceChar(std::string& str, char searchChar, char replaceVal)
{
	for(uint i = 0; i < str.length(); i++)
	{
		if(str.at(i) == searchChar)
		{
			str.at(i) = replaceVal;
		}
	}
}

void ReplaceString(std::string& str, const std::string& oldStr, const std::string& newStr)
{
  size_t pos = 0;
  while((pos = str.find(oldStr, pos)) != std::string::npos)
  {
     str.replace(pos, oldStr.length(), newStr);
     pos += newStr.length();
  }
}

vector<double> ShiftDistribution(vector<double> inputDoubles, double targetValue, uint targetIndex)
{
	//Initialize needed vars
	double maxVal = 0;
	vector<double> ret;
	ret.clear();
	//Check that the vector is == 100
	for(uint i = 0; i < inputDoubles.size(); i++)
	{
		maxVal += inputDoubles[i];
	}
	if(maxVal != 100)
	{
		for(uint i = 0; i < inputDoubles.size(); i++)
		{
			ret.push_back((inputDoubles[i]/maxVal)*100);
		}
	}
	else
	{
		ret = inputDoubles;
	}

	//shiftAmount is how much we need to assign to the remaining nodes (positive or negative) to get
	// a sum of 100 when inserting the target value at the index
	double shiftAmount = ret[targetIndex] - targetValue;

	//Reset max and insert targetValue
	maxVal = 0;
	ret[targetIndex] = targetValue;

	//Calculate the max of remaining doubles to get a ratio later, skipping the targetIndex
	for(uint i = 0; i < ret.size(); i++)
	{
		if(i != targetIndex)
		{
			maxVal += ret[i];
		}
	}
	//For each double that isn't shifted, take the positive or negative percentage total shift needed to get a sum of 100
	// and multiple that by their ratio to get shift amount for the individual value
	for(uint i = 0; i < ret.size(); i++)
	{
		if(i != targetIndex)
		{
			ret[i] += (ret[i]/maxVal)*shiftAmount;
		}
	}
	return ret;
}

vector<int> RoundDistributionToIntegers(vector<double> inputDoubles)
{
	//Init vars
	vector<int> ret;
	ret.clear();

	//Round the values to the nearest integer, carrying over the remainder to the next value
	//  ie. 33.3333, 33.3333, 33.33333 goes to 33,34,33 instead of 33,33,33;
	for(uint i = 0; i < (inputDoubles.size()); i++)
	{
		//If we can carry over the remainder
		if((i + 1) < inputDoubles.size())
		{
			//Push back the rounded value
			ret.push_back(floor(inputDoubles[i] + 0.5));

			//If we rounded down, add the remainder to the next value
			if(ret[i] == floor(inputDoubles[i]))
			{
				inputDoubles[i+1] += (((int)(inputDoubles[i] + 0.5)) % 1);
			}
			//If we rounded up, subtract the remainder from the next value
			else
			{
				inputDoubles[i+1] -= (((int)(inputDoubles[i] + 0.5)) % 1);
			}
		}
		//To assert a sum of 100, the last value is the remainder left to get 100
		else
		{
			int curSum = 0;
			for(uint j = 0; j < ret.size(); j++)
			{
				curSum += ret[j];
			}
			ret.push_back(100-curSum);
		}
	}
	return ret;
}

}
