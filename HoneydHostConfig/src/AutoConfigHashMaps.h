//============================================================================
// Name        : AutoConfigHashMaps.h
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
// Description : Header file to contain definitions of the different Hash maps used
//               throughout the HoneydHostConfig project
//============================================================================

#ifndef AUTOCONFIGHASHMAPS_H_
#define AUTOCONFIGHASHMAPS_H_

#include "HashMapStructs.h"
#include "HashMap.h"

//HashMap of MACs; Key is Vendor, Value is number of times the MAC vendor is seen for hosts of this personality type
typedef Nova::HashMap<std::string, uint16_t, std::tr1::hash<std::string>, eqstr > MAC_Table;

//HashMap of ports; Key is port (format: <NUM>_<PROTOCOL>), Value is a uint16_t count
typedef Nova::HashMap<std::string, uint16_t, std::tr1::hash<std::string>, eqstr > Port_Table;

#endif /* AUTOCONFIGHASHMAPS_H_ */
