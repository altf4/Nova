//============================================================================
// Name        : LocalTrafficMonitor.h
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Monitors local network traffic and sends detailed TrafficEvents
//					to the Classification Engine.
//============================================================================/*

#ifndef LOCALTRAFFICMONITOR_H_
#define LOCALTRAFFICMONITOR_H_


#endif /* LOCALTRAFFICMONITOR_H_ */

namespace Nova{
namespace LocalTrafficMonitor{

///	Filename of the file to be used as an IPC key
#define KEY_FILENAME "/.nova/keys/NovaIPCKey"
//Number of values read from the NOVAConfig file
#define CONFIG_FILE_LINE_COUNT 6

///Hash table for current TCP Sessions
///Table key is the source network socket, comprised of IP and Port in string format
///	IE: "192.168.1.1-8080"

struct Session
{
	bool fin;
	vector<struct Packet> session;
};

///The Value is a vector of IP headers
typedef std::tr1::unordered_map<string, struct Session> TCPSessionHashTable;

//Loads configuration variables from NOVAConfig_LTM.txt or specified config file
void LoadConfig(char* input);

/// Callback function that is passed to pcap_loop(..) and called each time
/// a packet is recieved
void Packet_Handler(u_char *useless,const struct pcap_pkthdr* pkthdr,const u_char* packet);

///Returns a string representation of the specified device's IP address
string getLocalIP(const char *dev);

/// Thread for periodically checking for TCP timeout.
///	IE: Not all TCP sessions get torn down properly. Sometimes they just end midstram
///	This thread looks for old tcp sessions and declares them terminated
void *TCPTimeout( void *ptr );

///Sends the given TrafficEvent to the Classification Engine
///	Returns success or failure
bool SendToCE( TrafficEvent *event );

///Returns usage tips
string Usage();

}
}
