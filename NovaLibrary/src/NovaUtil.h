//============================================================================
// Name        : NovaUtil.h
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
// Description : Utility class for storing functions that are used by multiple
//				 processes but don't warrant their own class
//============================================================================/*

#ifndef NOVAUTIL_H_
#define NOVAUTIL_H_

#include <pcap.h>
#include <math.h>
#include <errno.h>
#include <fstream>
#include <net/if.h>
#include <signal.h>
#include <sys/un.h>
#include <ANN/ANN.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <vector>
#include <sstream>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <netinet/if_ether.h>
#include <google/dense_hash_map>
#include <syslog.h>
#include <string>

#include "GUIMsg.h"
#include "Point.h"

using namespace std;
using namespace Nova;

/*********************************/
/******** Common Defines *********/
/*********************************/

//dimension
#define DIM 9

//The maximum message size, packets can only fragment up to this size
#define MAX_MSG_SIZE 65535

//The num of bytes returned by suspect serialization if it hit the maximum size;
// SerializeSuspect reqs 89 bytes :: SerializeFeatureData reqs 36 bytes
// The remaining 65410 bytes can be used for table entries :: 65410/8 == 8176.25
// Max table entries takes 65408 bytes
#define MORE_DATA 65533

//The maximum GUI message size, while this can be increased up to MAX_MSG_SIZE
// it should remain as small as possible, GUI messages aren't large.
#define MAX_GUIMSG_SIZE 256

//Number of messages to queue in a listening socket before ignoring requests until the queue is open
#define SOCKET_QUEUE_SIZE 50

//Initial sizes for dense hash map tables
#define INIT_SIZE_SMALL 256
#define INIT_SIZE_MEDIUM 1024
#define INIT_SIZE_LARGE 4096
#define INIT_SIZE_HUGE 65535

//Mode for encryption/decryption
#define ENCRYPT true
#define DECRYPT false

///	Filename of the file to be used as an IPC key
#define KEY_FILENAME "/keys/NovaIPCKey"

//If the feature data is local
#define LOCAL_DATA true
//If the feature data is broadcast from another nova instance
#define BROADCAST_DATA false

//From the Haystack or Doppelganger Module
#define FROM_HAYSTACK_DP	true
//From the Local Traffic Monitor
#define FROM_LTM			0

#define KEY_FILENAME "/keys/NovaIPCKey"
///	Filename of the file to be used as an Doppelganger IPC key
#define KEY_ALARM_FILENAME "/keys/NovaDoppIPCKey"
///	Filename of the file to be used as an Classification Engine IPC key
#define CE_FILENAME "/keys/CEKey"
/// File name of the file to be used as CE_GUI Input IPC key.
#define CE_GUI_FILENAME "/keys/GUI_CEKey"
/// File name of the file to be used as DM_GUI Input IPC key.
#define DM_GUI_FILENAME "/keys/GUI_DMKey"
/// File name of the file to be used as HS_GUI Input IPC key.
#define HS_GUI_FILENAME "/keys/GUI_HSKey"
/// File name of the file to be used as LTM_GUI Input IPC key.
#define LTM_GUI_FILENAME "/keys/GUI_LTMKey"
/// File name of the file that contains nova's install locations
#define PATHS_FILE "/etc/nova/paths"

/// Simple define for the ORing of these two integer values. Used for syslog.
#define SYSL_ERR (LOG_ERR | LOG_AUTHPRIV)
#define SYSL_INFO (LOG_INFO | LOG_AUTHPRIV)
/// Configs for openlog (first is with terminals, second is without
#define OPEN_SYSL (LOG_CONS | LOG_PID | LOG_NDELAY | LOG_PERROR)
#define NO_TERM_SYSL (LOG_CONS | LOG_PID | LOG_NDELAY)
/************************************************************************/
/********** Equality operators used by google's dense hash maps *********/
/************************************************************************/

struct eqstr
{
  bool operator()(string s1, string s2) const
  {
    return !(s1.compare(s2));
  }
};

struct eqaddr
{
  bool operator()(in_addr_t s1, in_addr_t s2) const
  {
    return (s1 == s2);
  }
};

struct eqport
{
  bool operator()(in_port_t s1, in_port_t s2) const
  {
	    return (s1 == s2);
  }
};

struct eqint
{
  bool operator()(int s1, int s2) const
  {
	    return (s1 == s2);
  }
};

struct eqtime
{
  bool operator()(time_t s1, time_t s2) const
  {
	    return (s1 == s2);
  }
};

/************************************************/
/********* Common Typedefs and Structs **********/
/************************************************/


///	A struct version of a packet, as received from libpcap
struct _packet
{
	///	Meta information about packet
	struct pcap_pkthdr pcap_header;
	///	Pointer to an IP header
	struct ip ip_hdr;
	/// Pointer to a TCP Header
	struct tcphdr tcp_hdr;
	/// Pointer to a UDP Header
	struct udphdr udp_hdr;
	/// Pointer to an ICMP Header
	struct icmphdr icmp_hdr;

	bool fromHaystack;
};

typedef struct _packet Packet;

///Hash table for current TCP Sessions
///Table key is the source network socket, comprised of IP and Port in string format
///	IE: "192.168.1.1-8080"
struct Session
{
	bool fin;
	vector<Packet> session;
};

///The Value is a vector of IP headers
typedef google::dense_hash_map<string, struct Session, tr1::hash<string>, eqstr > TCPSessionHashTable;
typedef google::dense_hash_map<string, vector<string>*, tr1::hash<string>, eqstr > TrainingHashTable;

/*****************************************************************************/
/** NovaUtil namespace is ONLY for functions repeated in multiple processes **/
/** and that don't fit into a category large enough to warrant a new class ***/
/*****************************************************************************/

namespace Nova{

// Encrpyts/decrypts a char buffer of size 'size' depending on mode
// TODO: Comment more on this once it's written
void CryptBuffer(u_char * buf, uint size, bool mode);

// Reads the paths file and returns the homePath of nova
// Returns: Something like "/home/user/.nova"
string GetHomePath();
// Reads the paths file and returns the readPath of nova
// Returns: Something like "/usr/share/nova"
string GetReadPath();
// Reads the paths file and returns the writePath of nova
// Returns: Something like "/etc/nova"
string GetWritePath();
// Replaces any env vars in 'path' and returns the absolute path
// 		path - String containing a path with env vars (eg $HOME)
// Returns: Path with env vars resolved and replaced with real values
string ResolvePathVars(string path);

// Gets local IP address for interface
//		dev - Device name, e.g. "eth0"
// Returns: IP addresses
string GetLocalIP(const char *dev);

// Extracts and returns the IP Address from a serialized suspect located at buf
//		buf - Contains serialized suspect data
// Returns: IP address of the serialized suspect
uint GetSerializedAddr(u_char * buf);

// Returns the number of bits used in the mask when given in in_addr_t form
int GetMaskBits(in_addr_t range);

void reorganizeTrainingFile(string inputFile, string outputFile);
TrainingHashTable* readEngineDumpFile(string inputFile);

}

//Some includes need to occur at the end of the header to fix some linking errors during compilation
#include "NOVAConfiguration.h"

#endif /* NOVAUTIL_H_ */
