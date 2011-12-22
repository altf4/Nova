//============================================================================
// Name        : LocalTrafficMonitor.h
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Monitors local network traffic and sends detailed TrafficEvents
//					to the Classification Engine.
//============================================================================/*

#ifndef LOCALTRAFFICMONITOR_H_
#define LOCALTRAFFICMONITOR_H_

///	Filename of the file to be used as an IPC key
#define KEY_FILENAME "/keys/NovaIPCKey"
/// File name of the file to be used as GUI Input IPC key.
#define GUI_FILENAME "/keys/GUI_LTMKey"
//Number of values read from the NOVAConfig file
#define CONFIG_FILE_LINE_COUNT 8
///The maximum message, as defined in /proc/sys/kernel/msgmax
#define MAX_MSG_SIZE 65535
//Number of messages to queue in a listening socket before ignoring requests until the queue is open
#define SOCKET_QUEUE_SIZE 50
//Sets the Initial Table size for faster operations
#define INITIAL_SESSION_TABLESIZE 65535
//Sets the Initial Table size for faster operations
#define INITIAL_SUSPECT_TABLESIZE 256

#include <TrafficEvent.h>
#include <google/dense_hash_map>
#include <Suspect.h>
#include <ANN/ANN.h>

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
void updateSuspect(TrafficEvent *event);

/// Thread for listening for GUI commands
void *SuspectLoop(void *ptr);

///Returns a string representation of the specified device's IP address
string getLocalIP(const char *dev);

//Loads configuration variables from NOVAConfig_LTM.txt or specified config file
void LoadConfig(char* input);


//Equality operator used by google's dense hash map
struct eq
{
  bool operator()(string s1, string s2) const
  {
    return !(s1.compare(s2));
  }
};

///Hash table for current TCP Sessions
///Table key is the source network socket, comprised of IP and Port in string format
///	IE: "192.168.1.1-8080"
struct Session
{
	bool fin;
	vector<struct Packet> session;
};

///The Value is a vector of IP headers
typedef google::dense_hash_map<string, struct Session, tr1::hash<string>, eq > TCPSessionHashTable;

//Equality operator used by google's dense hash map
struct eqaddrt
{
  bool operator()(in_addr_t s1, in_addr_t s2) const
  {
    return (s1 == s2);
  }
};

//Hash table for current list of suspects
typedef google::dense_hash_map<in_addr_t, Suspect*, tr1::hash<in_addr_t>, eqaddrt > SuspectHashTable;

}
}
#endif /* LOCALTRAFFICMONITOR_H_ */
