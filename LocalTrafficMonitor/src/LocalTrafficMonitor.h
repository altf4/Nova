//============================================================================
// Name        : LocalTrafficMonitor.h
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
// Description : Monitors local network traffic and sends detailed TrafficEvents
//					to the Classification Engine.
//============================================================================/*

#ifndef LOCALTRAFFICMONITOR_H_
#define LOCALTRAFFICMONITOR_H_

#include "Suspect.h"

using namespace std;

namespace Nova{
namespace LocalTrafficMonitor{

//Hash table for current list of suspects
typedef google::dense_hash_map<in_addr_t, Suspect*, tr1::hash<in_addr_t>, eqaddr > SuspectHashTable;

// Callback function that is passed to pcap_loop(..) and called each time a packet is received
//		useless - Unused
//		pkthdr - pcap packet header
//		packet - packet data
void Packet_Handler(u_char *useless,const struct pcap_pkthdr* pkthdr,const u_char* packet);

// Start routine for the GUI command listening thread
//		ptr - Required for pthread start routines
void *GUILoop(void *ptr);

// Receives input commands from the GUI
// This is a blocking function. If nothing is received, then wait on this thread for an answer
void ReceiveGUICommand(int socket);

// Startup routine for thread periodically checking for TCP timeout.
// IE: Not all TCP sessions get torn down properly. Sometimes they just end midstream
// This thread looks for old tcp sessions and declares them terminated
//		ptr - Required for pthread start routines
void *TCPTimeout( void *ptr );

// Sends the given Suspect to the Classification Engine
//		suspect - Suspect to send
// Returns: true for success, false for failure
bool SendToCE(Suspect *suspect);

// Updates a suspect with evidence to be processed later
//		packet : Packet headers to used for the evidence
void UpdateSuspect(Packet packet);

// Startup routine for thread updated evidence on suspects
//		ptr - Required for pthread start routines
void *SuspectLoop(void *ptr);

// Loads configuration variables
//		configFilePath - Location of configuration file
void LoadConfig(char* configFilePath);

// Checks the udp packet payload associated with event for a port knocking request,
// opens/closes the port using iptables for the sender depending on the payload
//		packet - header information
//		payload - actual packet data
void KnockRequest(Packet packet, u_char * payload);

}
}
#endif /* LOCALTRAFFICMONITOR_H_ */
