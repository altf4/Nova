//============================================================================
// Name        : NovaUtil.h
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Utility class for storing functions that are used by multiple
//				 processes but don't warrant their own class
//============================================================================/*

#ifndef NOVAUTIL_H_
#define NOVAUTIL_H_

#include <string>
#include <log4cxx/xml/domconfigurator.h>
#include <stdlib.h>
#include <ANN/ANN.h>
#include <google/dense_hash_map>
#include <sys/ioctl.h>

#include "NOVAConfiguration.h"
#include "GUIMsg.h"

using namespace std;
using namespace Nova;

//dimension
#define DIM 9

///The maximum message, as defined in /proc/sys/kernel/msgmax
#define MAX_MSG_SIZE 65535

//Number of messages to queue in a listening socket before ignoring requests until the queue is open
#define SOCKET_QUEUE_SIZE 50

//Mode for encryption/decryption
#define ENCRYPT true
#define DECRYPT false

///	Filename of the file to be used as an IPC key
#define KEY_FILENAME "/keys/NovaIPCKey"


/************************************************************************/


///Hash table for current TCP Sessions
///Table key is the source network socket, comprised of IP and Port in string format
///	IE: "192.168.1.1-8080"
struct Session
{
	bool fin;
	vector<struct Packet> session;
};

//Equality operator used by google's dense hash map
struct eqstr
{
  bool operator()(string s1, string s2) const
  {
    return !(s1.compare(s2));
  }
};


//Equality operator used by google's dense hash map
struct eqaddr
{
  bool operator()(in_addr_t s1, in_addr_t s2) const
  {
    return (s1 == s2);
  }
};

//Hash table for keeping track of suspects
//	the bool represents if the suspect is hostile or not
typedef google::dense_hash_map<in_addr_t, bool, tr1::hash<in_addr_t>, eqaddr > SuspectLiteHashTable;

//Equality operator used by google's dense hash map
struct eqport
{
  bool operator()(in_port_t s1, in_port_t s2) const
  {
	    return (s1 == s2);
  }
};

//Equality operator used by google's dense hash map
struct eqint
{
  bool operator()(int s1, int s2) const
  {
	    return (s1 == s2);
  }
};

//Table of packet sizes and a count
typedef google::dense_hash_map<int, pair<uint, uint>, tr1::hash<int>, eqint > Packet_Table;


//Equality operator used by google's dense hash map
struct eqtime
{
  bool operator()(time_t s1, time_t s2) const
  {
	    return (s1 == s2);
  }
};

//Table of packet intervals and a count
typedef google::dense_hash_map<time_t, pair<uint, uint>, tr1::hash<time_t>, eqtime > Interval_Table;

/************************************************************************/


namespace Nova
{
namespace NovaUtil
{


//**********************************************************************//
//	NovaUtil is ONLY for functions repeated in multiple processes that  //
//		don't fit into a category large enough to warrant a new class	//
//**********************************************************************//


// Encrpyts/decrypts a char buffer of size 'size' depending on mode
void cryptBuffer(u_char * buf, uint size, bool mode);

// Replaces any env vars in 'path' and returns the absolute path
string resolvePathVars(string path);

//Extracts and returns the IP Address from a serialized suspect located at buf
uint getSerializedAddr(u_char * buf);




}
}
#endif /* NOVAUTIL_H_ */
