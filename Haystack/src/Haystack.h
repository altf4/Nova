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

//Number of values read from the NOVAConfig file
#define CONFIG_FILE_LINE_COUNT 9

using namespace std;

namespace Nova{
namespace Haystack{

/// Callback function that is passed to pcap_loop(..) and called each time
/// a packet is recieved
void Packet_Handler(u_char *useless,const struct pcap_pkthdr* pkthdr,const u_char* packet);

/// Thread for listening for GUI commands
void *GUILoop(void *ptr);

/// Receives input commands from the GUI
void ReceiveGUICommand(int socket);

/// Thread for periodically checking for TCP timeout.
///	IE: Not all TCP sessions get torn down properly. Sometimes they just end midstram
///	This thread looks for old tcp sessions and declares them terminated
void *TCPTimeout( void *ptr );

///Sends the given Suspect to the Classification Engine
///	Returns success or failure
bool SendToCE(Suspect *suspect);

//Updates a suspect with evidence to be processed later
void updateSuspect(TrafficEvent *event);

/// Thread for listening for GUI commands
void *SuspectLoop(void *ptr);

///Parse through the honeyd config file and get the list of IP addresses used
vector <string> GetHaystackAddresses(string honeyDConfigPath);

//Loads configuration variables from NOVAConfig_HS.txt or specified config file
void LoadConfig(char* input);

//Hash table for current list of suspects
typedef google::dense_hash_map<in_addr_t, Suspect*, tr1::hash<in_addr_t>, eqaddr > SuspectHashTable;

}
}

#endif /* HAYSTACK_H_ */
