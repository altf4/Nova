//============================================================================
// Name        : LocalTrafficMonitor.h
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Monitors local network traffic and sends detailed TrafficEvents
//					to the Classification Engine.
//============================================================================/*

#ifndef LOCALTRAFFICMONITOR_H_
#define LOCALTRAFFICMONITOR_H_

#include <Suspect.h>
#include <NovaUtil.h>

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
//		ptr - Required for pthread start routiness
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
