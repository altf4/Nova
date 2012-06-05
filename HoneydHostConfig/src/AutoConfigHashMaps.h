/*
 * AutoConfigHashMaps.h
 *
 *  Created on: Jun 4, 2012
 *      Author: victim
 */

#ifndef AUTOCONFIGHASHMAPS_H_
#define AUTOCONFIGHASHMAPS_H_

#include "HashMapStructs.h"
#include "HashMap.h"

//HashMap of MACs; Key is Vendor, Value is number of times the MAC vendor is seen for hosts of this personality type
typedef Nova::HashMap<std::string, uint16_t, std::tr1::hash<std::string>, eqstr > MAC_Table;

//HashMap of ports; Key is port (format: <NUM>_<PROTOCOL>), Value is a uint16_t count
typedef Nova::HashMap<std::string, uint16_t, std::tr1::hash<std::string>, eqstr > Port_Table;


#endif /* AUTOCONFIGHASHMAPS_H_ */
