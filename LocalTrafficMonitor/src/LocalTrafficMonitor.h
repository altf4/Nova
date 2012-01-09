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
void updateSuspect(Packet packet);

/// Thread for listening for GUI commands
void *SuspectLoop(void *ptr);

///Returns a string representation of the specified device's IP address
string getLocalIP(const char *dev);

//Loads configuration variables from NOVAConfig_LTM.txt or specified config file
void LoadConfig(char* input);

//Checks the udp packet payload associated with event for a port knocking request,
// opens/closes the port for the sender depending on the payload
void knockRequest(Packet packet, u_char * payload);

//Hash table for current list of suspects
typedef google::dense_hash_map<in_addr_t, Suspect*, tr1::hash<in_addr_t>, eqaddr > SuspectHashTable;

}
}
#endif /* LOCALTRAFFICMONITOR_H_ */
