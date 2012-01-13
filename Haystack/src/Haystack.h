//============================================================================
// Name        : Haystack.h
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Nova utility for transforming Honeyd log files into
//					TrafficEvents usable by Nova's Classification Engine.
//============================================================================

#ifndef HAYSTACK_H_
#define HAYSTACK_H_

#include <NovaUtil.h>
#include <Suspect.h>

using namespace std;

namespace Nova{
namespace Haystack{

// Hash table for current list of suspects
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

// Startup rotuine for thread periodically checking for TCP timeout.
// IE: Not all TCP sessions get torn down properly. Sometimes they just end midstram
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

// Startup routine for thread updated evidence on suspsects
//		ptr - Required for pthread start routines
void *SuspectLoop(void *ptr);

// Parse through the honeyd config file and get the list of IP addresses used
//		honeyDConfigPath - path to honeyd configuration file
// Returns: vector containing IP addresses of all honeypots
vector <string> GetHaystackAddresses(string honeyDConfigPath);

// Loads configuration variables
//		configFilePath - Location of configuration file
void LoadConfig(char* configFilePath);

}
}

#endif /* HAYSTACK_H_ */
