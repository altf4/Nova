//============================================================================
// Name        : NovaCLI.cpp
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
#include "nova_ui_core.h"
#include "Logger.h"


#include <iostream>
#include <stdlib.h>
#include "inttypes.h"
#include "boost/program_options.hpp"

namespace po = boost::program_options;
using namespace std;
using namespace Nova;
using namespace NovaCLI;

int main(int argc, const char *argv[])
{
	// Fail if no arguments
	if(argc < 2)
	{
		PrintUsage();
	}

	Config::Inst();

	// Disable notifications and email in the CLI
	Logger::Inst()->SetUserLogPreferences(EMAIL, EMERGENCY, '+');

	// We parse the input arguments here,
	// but refer to other functions to do any
	// actual work.

	// Listing suspect IP addresses
	if(!strcmp(argv[1], "list"))
	{
		if(argc < 3)
		{
			PrintUsage();
		}

		if(!strcmp(argv[2], "all"))
		{
			PrintSuspectList(SUSPECTLIST_ALL);
		}
		else if(!strcmp(argv[2], "hostile"))
		{
			PrintSuspectList(SUSPECTLIST_HOSTILE);
		}
		else if(!strcmp(argv[2], "benign"))
		{
			PrintSuspectList(SUSPECTLIST_BENIGN);
		}
		else
		{
			PrintUsage();
		}
	}

	else if (!strcmp(argv[1], "monitor"))
	{
		MonitorCallback();
	}

	else if (!strcmp(argv[1], "reclassify"))
	{
		ReclassifySuspects();
	}

	// Checking status of components
	else if(!strcmp(argv[1], "status"))
	{
		if(argc < 3)
		{
			PrintUsage();
		}

		if(!strcmp(argv[2], "nova"))
		{
			StatusNovaWrapper();
		}
		else if(!strcmp(argv[2], "haystack"))
		{
			StatusHaystackWrapper();
		}
		else if(!strcmp(argv[2], "quasar"))
		{
			StatusQuasar();
		}
		else
		{
			PrintUsage();
		}
	}

	// Starting components
	else if(!strcmp(argv[1], "start"))
	{
		if(argc < 3)
		{
			PrintUsage();
		}

		if(!strcmp(argv[2], "nova"))
		{
			if (argc > 3)
			{
				if (!strcmp(argv[3], "debug"))
				{
					StartNovaWrapper(true);
				}
				else
				{
					PrintUsage();
				}
			}
			else
			{
				StartNovaWrapper(false);
			}
		}
		else if(!strcmp(argv[2], "haystack"))
		{
			if (argc > 3)
			{
				if (!strcmp(argv[3], "debug"))
				{
					StartHaystackWrapper(true);
				}
				else
				{
					PrintUsage();
				}
			}
			else
			{
				StartHaystackWrapper(false);
			}

		}
		else if(!strcmp(argv[2], "quasar"))
		{
			if (argc > 3)
			{
				if (!strcmp(argv[3], "debug"))
				{
					StartQuasarWrapper(true);
				}
				else
				{
					PrintUsage();
				}
			}
			else
			{
				StartQuasarWrapper(false);
			}

		}
		else if (!strcmp(argv[2], "capture"))
		{
			StartCaptureWrapper();
		}
		else
		{
			PrintUsage();
		}
	}

	// Stopping components
	else if(!strcmp(argv[1], "stop"))
	{
		if(argc < 3)
		{
			PrintUsage();
		}

		if(!strcmp(argv[2], "nova"))
		{
			StopNovaWrapper();
		}
		else if(!strcmp(argv[2], "haystack"))
		{
			StopHaystackWrapper();
		}
		else if(!strcmp(argv[2], "quasar"))
		{
			StopQuasarWrapper();
		}
		else if (!strcmp(argv[2], "capture"))
		{
			StopCaptureWrapper();
		}
		else
		{
			PrintUsage();
		}
	}

	// Getting suspect information
	else if(!strcmp(argv[1], "get"))
	{
		if(argc < 3)
		{
			PrintUsage();
		}

		if(!strcmp(argv[2], "all"))
		{
			if(argc == 4 && !strcmp(argv[3], "csv"))
			{
				PrintAllSuspects(SUSPECTLIST_ALL, true);
			}
			else
			{
				PrintAllSuspects(SUSPECTLIST_ALL, false);
			}
		}
		else if(!strcmp(argv[2], "hostile"))
		{
			if(argc == 4 && !strcmp(argv[3], "csv"))
			{
				PrintAllSuspects(SUSPECTLIST_HOSTILE, true);
			}
			else
			{
				PrintAllSuspects(SUSPECTLIST_HOSTILE, false);
			}
		}
		else if(!strcmp(argv[2], "benign"))
		{
			if(argc == 4 && !strcmp(argv[3], "csv"))
			{
				PrintAllSuspects(SUSPECTLIST_BENIGN, true);
			}
			else
			{
				PrintAllSuspects(SUSPECTLIST_BENIGN, false);
			}
		}
		else if (!strcmp(argv[2], "data"))
		{
			if (argc != 5)
			{
				PrintUsage();
			}

			in_addr_t address;
			if(inet_pton(AF_INET, argv[4], &address) != 1)
			{
				cout << "Error: Unable to convert to IP address" << endl;
				exit(EXIT_FAILURE);
			}

			PrintSuspectData(address, string(argv[3]));
		}
		else
		{
			if (argc != 4)
			{
				PrintUsage();
			}

			// Some early error checking for the
			in_addr_t address;
			if(inet_pton(AF_INET, argv[3], &address) != 1)
			{
				cout << "Error: Unable to convert to IP address" << endl;
				exit(EXIT_FAILURE);
			}

			PrintSuspect(address, string(argv[2]));
		}
	}

	// Clearing suspect information
	else if(!strcmp(argv[1], "clear"))
	{
		if(argc < 3)
		{
			PrintUsage();
		}

		if(!strcmp(argv[2], "all"))
		{
			ClearAllSuspectsWrapper();
		}
		else
		{
			if(argc < 4)
			{
				PrintUsage();
			}

			// Some early error checking for the
			in_addr_t address;
			if(inet_pton(AF_INET, argv[3], &address) != 1)
			{
				cout << "Error: Unable to convert to IP address" << endl;
				exit(EXIT_FAILURE);
			}

			ClearSuspectWrapper(address, string(argv[2]));
		}
	}

	// Checking status of components
	else if(!strcmp(argv[1], "uptime"))
	{
		PrintUptime();
	}


	else if(!strcmp(argv[1], "readsetting"))
	{
		if(argc < 3)
		{
			PrintUsage();
		}

		cout << Config::Inst()->ReadSetting(string(argv[2])) << endl;
	}

	else if(!strcmp(argv[1], "writesetting"))
	{
		if(argc < 4)
		{
			PrintUsage();
		}

		if(Config::Inst()->WriteSetting(string(argv[2]), string(argv[3])))
		{
			LOG(DEBUG, "Finished writing setting to configuration file", "");
		}
		else
		{
			LOG(ERROR, "Unable to write setting to configuration file.", "");
		}
	}

	else if(!strcmp(argv[1], "listsettings"))
	{
		vector<string> settings = Config::Inst()->GetPrefixes();
		for (uint i = 0; i < settings.size(); i++)
		{
			cout << settings[i] << endl;
		}
	}

	else
	{
		PrintUsage();
	}

	return EXIT_SUCCESS;
}

namespace NovaCLI
{

void PrintUsage()
{
	cout << "Usage:" << endl;
	cout << "  " << EXECUTABLE_NAME << " status nova|haystack|quasar" << endl;
	cout << "    Outputs if the nova or haystack process is running and responding" << endl;
	cout << endl;
	cout << "  " << EXECUTABLE_NAME << " start nova|capture|haystack|quasar [debug]" << endl;
	cout << "    Starts the nova or haystack process, or starts capture on existing process. 'debug' will run in a blocking and verbose mode." << endl;
	cout << endl;
	cout << "  " << EXECUTABLE_NAME << " stop nova|capture|haystack|quasar" << endl;
	cout << "    Stops the nova, haystack process, or live packet capture" << endl;
	cout << endl;
	cout << "  " << EXECUTABLE_NAME << " list all|hostile|benign" << endl;
	cout << "    Outputs the current list of suspect IP addresses of a given type" << endl;
	cout << endl;
	cout << "  " << EXECUTABLE_NAME << " get all|hostile|benign [csv]" << endl;
	cout << "    Outputs the details for all suspects of a type (all, hostile only, or benign only). Optionally can be output in CSV format." << endl;
	cout << endl;
	cout << "  " << EXECUTABLE_NAME << " get interface xxx.xxx.xxx.xxx" << endl;
	cout << "    Outputs the details of a specific suspect with IP address xxx.xxx.xxx.xxx" << endl;
	cout << endl;
	cout << "  " << EXECUTABLE_NAME << " get data interface xxx.xxx.xxx.xxx" << endl;
	cout << "    Outputs the details of a specific suspect with IP address xxx.xxx.xxx.xxx, including low level data" << endl;
	cout << endl;
	cout << "  " << EXECUTABLE_NAME << " clear all" << endl;
	cout << "    Clears all saved data for suspects" << endl;
	cout << endl;
	cout << "  " << EXECUTABLE_NAME << " clear interface xxx.xxx.xxx.xxx" << endl;
	cout << "    Clears all saved data for a specific suspect" << endl;
	cout << endl;
	cout << "  " << EXECUTABLE_NAME << " writesetting SETTING VALUE" << endl;
	cout << "    Writes setting to configuration file" << endl;
	cout << endl;
	cout << "  " << EXECUTABLE_NAME << " readsetting SETTING" << endl;
	cout << "    Reads setting from configuration file" << endl;
	cout << endl;
	cout << "  " << EXECUTABLE_NAME << " listsettings" << endl;
	cout << "    Lists settings that can be set in the configuration file" << endl;
	cout << endl;
	cout << "  " << EXECUTABLE_NAME << " monitor" << endl;
	cout << "    Monitors live output from novad (mainly for debugging)" << endl;

	exit(EXIT_FAILURE);
}

void StatusNovaWrapper()
{
	if(!ConnectToNovad())
	{
		cout << "Novad Status: Not running" << endl;
	}
	else
	{
		if(IsNovadUp(false))
		{
			cout << "Novad Status: Running and responding" << endl;
		}
		else
		{
			cout << "Novad Status: Not responding" << endl;
		}

		CloseNovadConnection();
	}
}

void StatusHaystackWrapper()
{
	if(IsHaystackUp())
	{
		cout << "Haystack Status: Running" << endl;
	}
	else
	{
		cout << "Haystack Status: Not running" << endl;
	}
}

bool StatusQuasar()
{
	return system("forever list");
}

void StartNovaWrapper(bool debug)
{
	if(!ConnectToNovad())
	{
		if(StartNovad(debug))
		{
			cout << "Started Novad" << endl;
		}
		else
		{
			cout << "Failed to start Novad" << endl;
		}
	}
	else
	{
		cout << "Novad is already running" << endl;
		CloseNovadConnection();
	}
}

void StartHaystackWrapper(bool debug)
{
	if(!IsHaystackUp())
	{
		if(StartHaystack(debug))
		{
			cout << "Started Haystack" << endl;
		}
		else
		{
			cout << "Failed to start Haystack" << endl;
		}
	}
	else
	{
		cout << "Haystack is already running" << endl;
	}
}

void StopNovaWrapper()
{
	Connect();

	if(StopNovad())
	{
		cout << "Novad has been stopped" << endl;
	}
	else
	{
		cout << "There was a problem stopping Novad" << endl;
	}
}

void StopHaystackWrapper()
{
	if(StopHaystack())
	{
		cout << "Haystack has been stopped" << endl;
	}
	else
	{
		cout << "There was a problem stopping the Haystack" << endl;
	}
}

void StartCaptureWrapper()
{
	Connect();
	StartPacketCapture();
}

void StopCaptureWrapper()
{
	Connect();
	StopPacketCapture();
}

bool StartQuasarWrapper(bool debug)
{
	StopQuasarWrapper();
	if (debug)
	{
		return system("quasar --debug");
	}
	else
	{
		return system("quasar");
	}
}

bool StopQuasarWrapper()
{
	return system("forever stop /usr/share/nova/sharedFiles/Quasar/main.js");
}


void PrintSuspect(in_addr_t address, string interface)
{
	Connect();

	SuspectIdentifier id(ntohl(address), interface);

	Suspect *suspect = GetSuspect(id);

	if(suspect != NULL)
	{
		cout << suspect->ToString() << endl;
	}
	else
	{
		cout << "Error: No suspect received" << endl;
	}

	delete suspect;

	CloseNovadConnection();
}

void PrintSuspectData(in_addr_t address, string interface)
{
	Connect();

	SuspectIdentifier id(ntohl(address), interface);

	Suspect *suspect = GetSuspectWithData(id);

	if (suspect != NULL)
	{
		cout << suspect->ToString() << endl;


		cout << "Details follow" << endl;
		cout << suspect->GetFeatureSet(MAIN_FEATURES).toString() << endl;
	}
	else
	{
		cout << "Error: No suspect received" << endl;
	}

	delete suspect;

	CloseNovadConnection();


}

void PrintAllSuspects(enum SuspectListType listType, bool csv)
{
	Connect();

	vector<SuspectIdentifier> suspects = GetSuspectList(listType);

	// Print the CSV header
	if (csv)
	{
		cout << "IP,";
		cout << "INTERFACE,";
		for(int i = 0; i < DIM; i++)
		{
			cout << FeatureSet::m_featureNames[i] << ",";
		}
		cout << "CLASSIFICATION" << endl;
	}

	for(uint i = 0; i < suspects.size(); i++)
	{
		Suspect *suspect = GetSuspect(suspects.at(i));

		if(suspect != NULL)
		{
			if(!csv)
			{
				cout << suspect->ToString() << endl;
			}
			else
			{
				cout << suspect->GetIpString() << ",";
				cout << suspect->GetIdentifier().m_interface << ",";
				for(int i = 0; i < DIM; i++)
				{
					cout << suspect->GetFeatureSet().m_features[i] << ",";
				}
				cout << suspect->GetClassification() << endl;
			}

			delete suspect;
		}
		else
		{
			cout << "Error: No suspect received" << endl;
		}
	}

	CloseNovadConnection();

}

void PrintSuspectList(enum SuspectListType listType)
{
	Connect();

	vector<SuspectIdentifier> suspects = GetSuspectList(listType);

	for(uint i = 0; i < suspects.size(); i++)
	{
		in_addr tmp;
		tmp.s_addr = htonl(suspects.at(i).m_ip);
		char *address = inet_ntoa((tmp));
		cout << suspects.at(i).m_interface << " " << address << endl;
	}

	CloseNovadConnection();
}

void ClearAllSuspectsWrapper()
{
	Connect();

	if(ClearAllSuspects())
	{
		cout << "Suspects have been cleared" << endl;
	}
	else
	{
		cout << "There was an error when clearing the suspects" << endl;
	}

	CloseNovadConnection();
}

void ClearSuspectWrapper(in_addr_t address, string interface)
{
	Connect();

	SuspectIdentifier id(ntohl(address), interface);

	if(ClearSuspect(id))
	{
		cout << "Suspect data has been cleared for this suspect" << endl;
	}
	else
	{
		cout << "There was an error when trying to clear the suspect data for this suspect" << endl;
	}

	CloseNovadConnection();
}

void PrintUptime()
{
	Connect();
	cout << "Uptime is: " << GetStartTime() << endl;
	CloseNovadConnection();
}

void Connect()
{
	if(!ConnectToNovad())
	{
		cout << "ERROR: Unable to connect to Nova" << endl;
		exit(EXIT_FAILURE);
	}
}

void ReclassifySuspects()
{
	Connect();
	if (ReclassifyAllSuspects())
	{
		cout << "All suspects were reclassified" << endl;
	}
	else
	{
		cout << "Unable to reclassify suspects" << endl;
	}
}

void MonitorCallback()
{
    while (true)
    {
	    if( ! Nova::ConnectToNovad() )
	    {
	        LOG(ERROR, "CLI Unable to connect to Novad right now. Trying again in 3 seconds...","");
	        sleep(3);
	        continue;
	    }


		CallbackChange cb;
		CallbackHandler callbackHandler;
	    Suspect s;

		do
		{
			cb = callbackHandler.ProcessCallbackMessage();
			switch( cb.m_type )
			{
			case CALLBACK_NEW_SUSPECT:
				cout << "Got new suspect: " << cb.m_suspect->GetIdString() << + " with classification of " << cb.m_suspect->GetClassification() << endl;

				// We get a suspect pointer and are responsible for deleting it
				delete cb.m_suspect;
				break;

			case CALLBACK_ERROR:
				cout << "WARNING: Recieved a callback error message!" << endl;
				break;

			case CALLBACK_ALL_SUSPECTS_CLEARED:
				cout << "Got notice that all suspects were cleared" << endl;
				break;

			case CALLBACK_SUSPECT_CLEARED:
				s.SetIdentifier(cb.m_suspectIP);
				cout << "Got a notice that suspect was cleared: " + s.GetIdString() << endl;
				break;
			case CALLBACK_HUNG_UP:
				cout << "Got CALLBACK_HUNG_UP" << endl;
				break;
			default:
				cout << "WARNING: Got a callback message we don't know what to do with. Type " << cb.m_type << endl;
				break;
			}
		}
		while(cb.m_type != CALLBACK_HUNG_UP);
		cout << "Novad hung up, closing callback processing and trying again in 3 seconds" << endl;
		sleep(3);
   }
}


}
