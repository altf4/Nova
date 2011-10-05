//============================================================================
// Name        : Haystack.h
// Author      : DataSoft Corporation
// Copyright   :
// Description : 	Nova utility for transforming Honeyd log files into
//						ScanEvents usable by Nova's Classification Engine.
//============================================================================

#ifndef HAYSTACK_H_
#define HAYSTACK_H_

///	Filename of the file to be used as an IPC key
#define KEY_FILENAME "/etc/NovaIPCKey"

#include <TrafficEvent.h>

namespace Nova{
namespace Haystack{

//Loads configuration variables from NOVAConfig_HS.txt or specified config file
void LoadConfig(char* input);

using namespace std;

/// Callback function that is passed to pcap_loop(..) and called each time
/// a packet is recieved
void Packet_Handler(u_char *useless,const struct pcap_pkthdr* pkthdr,const u_char* packet);

/// Thread for periodically checking for TCP timeout.
///	IE: Not all TCP sessions get torn down properly. Sometimes they just end midstram
///	This thread looks for old tcp sessions and declares them terminated
void *TCPTimeout( void *ptr );

///Sends the given TrafficEvent to the Classification Engine
///	Returns success or failure
bool SendToCE( TrafficEvent *event );

///Parse through the honeyd config file and get the list of IP addresses used
vector <string> GetHaystackAddresses(string honeydConfigPath);

///Usage tips
string Usage();

///Hash table for current TCP Sessions
///Table key is the source network socket, comprised of IP and Port in string format
///	IE: "192.168.1.1-8080"
///The Value is a vector of IP headers
typedef std::tr1::unordered_map<string, vector<struct Packet> > TCPSessionHashTable;
}
}

#endif /* HAYSTACK_H_ */
