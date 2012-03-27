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

#include <string>

#include "sys/types.h"

namespace Nova{

// Encrpyts/decrypts a char buffer of size 'size' depending on mode
// TODO: Comment more on this once it's written
void CryptBuffer(u_char * buf, uint size, bool mode);

// Gets local IP address for interface
//		dev - Device name, e.g. "eth0"
// Returns: IP addresses
std::string GetLocalIP(const char *dev);

}

#endif /* NOVAUTIL_H_ */
