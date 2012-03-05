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

#include "GUIMsg.h"
#include "HashMapStructs.h"
#include <string>
#include <vector>
#include <arpa/inet.h>

using namespace std;
using namespace Nova;

/************************************************/
/********* Common Typedefs and Structs **********/
/************************************************/


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

}

#endif /* NOVAUTIL_H_ */
