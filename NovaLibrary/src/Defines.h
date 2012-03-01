//============================================================================
// Name        : Defines.h
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
// Description : Temporary Defines header during project reorganization
//============================================================================/*


/*********************************/
/******** Common Defines *********/
/*********************************/

//The maximum message size, packets can only fragment up to this size
#define MAX_MSG_SIZE 65535
//dimension
#define DIM 9

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

//If the feature data is local
#define LOCAL_DATA true
//If the feature data is broadcast from another nova instance
#define BROADCAST_DATA false

//From the Haystack or Doppelganger Module
#define FROM_HAYSTACK_DP true
//From the Local Traffic Monitor
#define FROM_LTM false

///	Filename of the file to be used as an Classification Engine IPC key
#define NOVAD_IN_FILENAME "/keys/CEKey"
/// File name of the file to be used as CE_GUI Input IPC key.
#define NOVAD_OUT_FILENAME "/keys/NovaIPCKey"

#define PATHS_FILE "/etc/nova/paths"

/// Simple define for the ORing of these two integer values. Used for syslog.
#define SYSL_ERR (LOG_ERR | LOG_AUTHPRIV)
#define SYSL_INFO (LOG_INFO | LOG_AUTHPRIV)
/// Configs for openlog (first is with terminals, second is without
#define OPEN_SYSL (LOG_CONS | LOG_PID | LOG_NDELAY | LOG_PERROR)
#define NO_TERM_SYSL (LOG_CONS | LOG_PID | LOG_NDELAY)
