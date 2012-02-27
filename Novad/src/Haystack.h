//============================================================================
// Name        : Haystack.h
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
// Description : Nova utility for transforming Honeyd log files into
//					TrafficEvents usable by Nova's Classification Engine.
//============================================================================

#ifndef HAYSTACK_H_
#define HAYSTACK_H_

#include "Suspect.h"

using namespace std;

namespace Nova{


//The main thread for the Haystack
// ptr - Unused pointer required by pthread
// returns - Does not return (main loop)
void *HaystackMain(void *ptr);

// Callback function that is passed to pcap_loop(..) and called each time a packet is received
//		useless - Unused
//		pkthdr - pcap packet header
//		packet - packet data
void HSPacket_Handler(u_char *useless,const struct pcap_pkthdr* pkthdr,const u_char* packet);

// Start routine for the GUI command listening thread
//		ptr - Required for pthread start routines
void *HS_GUILoop(void *ptr);

// Receives input commands from the GUI
// This is a blocking function. If nothing is received, then wait on this thread for an answer
void HSReceiveGUICommand(int socket);

// Startup rotuine for thread periodically checking for TCP timeout.
// IE: Not all TCP sessions get torn down properly. Sometimes they just end midstram
// This thread looks for old tcp sessions and declares them terminated
//		ptr - Required for pthread start routines
void *HSTCPTimeout( void *ptr );

// Sends the given Suspect to the Classification Engine
//		suspect - Suspect to send
// Returns: true for success, false for failure
bool HSSendToCE(Suspect *suspect);

// Updates a suspect with evidence to be processed later
//		packet : Packet headers to used for the evidence
void HSUpdateSuspect(Packet packet);

// Startup routine for thread updated evidence on suspsects
//		ptr - Required for pthread start routines
void *HSSuspectLoop(void *ptr);

void *UpdateIPFilter(void *ptr);
string ConstructFilterString();

// Loads configuration variables
//		configFilePath - Location of configuration file
void HSLoadConfig(char* configFilePath);

}

#endif /* HAYSTACK_H_ */
