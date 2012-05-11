//============================================================================
// Name        : Haystack.cpp
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
// Description : Controls the Honeyd Haystack and Doppelganger processes
//============================================================================

#include "Commands.h"
#include "Config.h"

using namespace std;
namespace Nova
{
bool StartHaystack()
{
	string executeString = "nohup sudo honeyd -i " + Config::Inst()->GetInterface() + " -i "
		+ Config::Inst()->GetDoppelInterface()+" -f ";
	executeString += Config::Inst()->GetPathHome() + '/';
	switch(Config::Inst()->GetHaystackStorage())
	{
		case 'I':
		{
			executeString += Config::Inst()->GetPathConfigHoneydHS();
			break;
		}
		case 'M':
		{
			executeString += Config::Inst()->GetPathConfigHoneydUser();
			break;
		}
		default:
		{
			return false;
		}
	}

	executeString += " -p " + Config::Inst()->GetPathReadFolder() + "/nmap-os-db -s "
		"/var/log/honeyd/honeydHaystackservice.log -t /var/log/honeyd/ipList > /dev/null &";
	cout << executeString << endl;
	if(system(executeString.c_str()) != 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool StopHaystack()
{
	// Kill honeyd processes
	FILE *out = popen("pidof honeyd","r");
	bool retSuccess = false;
	if(out != NULL)
	{
		char buffer[1024];
		char *line = fgets(buffer, sizeof(buffer), out);

		if (line != NULL)
		{
			string cmd = "sudo kill " + string(line);
			if(cmd.size() > 5)
			{
				if(system(cmd.c_str()) == EXIT_SUCCESS)
				{
					retSuccess = true;
				}
			}
		}
		else
		{
			//Haystack was already down
			retSuccess = true;
		}
	}
	pclose(out);
	return retSuccess;
}

bool IsHaystackUp()
{
	FILE *out = popen("pidof honeyd","r");
	bool retSuccess = false;
	if(out != NULL)
	{
		char buffer[1024];
		char *line = fgets(buffer, sizeof(buffer), out);

		if (line != NULL)
		{
			retSuccess = true;
		}
		else
		{
			retSuccess = false;
		}
	}
	pclose(out);
	return retSuccess;
}
}
