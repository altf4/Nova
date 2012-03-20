//============================================================================
// Name        : NovaCLI.h
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
// Description : Command line interface for Nova
//============================================================================

#include "NovaCLI.h"
#include "Connection.h"
#include "StatusQueries.h"


#include <iostream>
#include <stdlib.h>
#include "boost/program_options.hpp"


namespace po = boost::program_options;
using namespace std;
using namespace Nova;

int main(int argc, const char *argv[])
{
	if (!strcmp(argv[1], "list"))
	{
		if (!strcmp(argv[2], "all"))
		{
			PrintSuspectList(SUSPECTLIST_ALL);
		}
		else if (!strcmp(argv[2], "hostile"))
		{
			PrintSuspectList(SUSPECTLIST_HOSTILE);
		}
		else if (!strcmp(argv[2], "benign"))
		{
			PrintSuspectList(SUSPECTLIST_BENIGN);
		}
		else
		{
			cout << "Error: Unknown suspect list type. Should be: 'all', 'hostile', or 'benign'" << endl;
		}
	}
	else if (!strcmp(argv[1], "start"))
	{
		if (!strcmp(argv[2], "nova"))
		{
			StartNova();
		}
		else if (!strcmp(argv[2], "haystack"))
		{
			StartHaystack();
		}
	}
	else if (!strcmp(argv[1], "stop"))
	{
		if (!strcmp(argv[2], "nova"))
		{
			StopNova();
		}
		else if (!strcmp(argv[2], "haystack"))
		{
			StopHaystack();
		}
	}

	return EXIT_SUCCESS;
}


namespace Nova
{

void StartNova()
{

}

void StartHaystack()
{

}

void StopNova()
{

}

void StopHaystack()
{

}

void PrintSuspectList(enum SuspectListType listType)
{
	CloseNovadConnection();

	if (!ConnectToNovad())
	{
		cout << "Failed to connect to Nova" << endl;
		exit(EXIT_FAILURE);
	}

	vector<in_addr_t> *suspects;
	suspects = GetSuspectList(SUSPECTLIST_ALL);


	if (suspects == NULL)
	{
		cout << "Failed to get suspect list" << endl;
		exit(EXIT_FAILURE);
	}

	for (uint i = 0; i < suspects->size(); i++)
	{
		in_addr tmp;
		tmp.s_addr = suspects->at(i);
		char *address = inet_ntoa(tmp);
		cout << address << endl;
	}
}
}
