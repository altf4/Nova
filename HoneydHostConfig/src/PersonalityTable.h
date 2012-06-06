//============================================================================
// Name        : PersonalityTable.h
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
// Description : Contains function definitions and member variable declarations
//               for the PersonalityTable class. Adds the class to the Nova
//               namespace.
//============================================================================

#include "Personality.h"

//total num_hosts is the total num of unique hosts counted.
//total avail_addrs is the total num of ip addresses avail on the subnet

//Mapping of Personality objects using the OS name as the key
// contains:
/* map of occuring ports (Number_Protocol for key Ex: 22_TCP)
 *  - all behaviors assumed to be open if we have an entry.
 *  - determine default behaviors for TCP, UDP & ICMP somehow.
 * m_count of number of hosts w/ this OS.
 * m_port_count - number of open ports counted for hosts w/ this OS
 * map of occuring MAC addr vendors, so we know what types of NIC's are used for machines of a similar type on a network.
 */

//HashMap of Personality objects; Key is personality specific name (i.e. Linux 2.6.35-2.6.38), Value is ptr to Personality object
typedef Nova::HashMap<std::string, class Nova::Personality *, std::tr1::hash<std::string>, eqstr > Personality_Table;

namespace Nova
{

class PersonalityTable
{

public:

	PersonalityTable();

	~PersonalityTable();

	// Adds a host to the Personality Table if it's not there; if it is, just aggregate the values
	//  Personality *add - pointer to a Personality object that contains new information for the table
	// No return value
	void AddHost(Personality *add);

	//Dummy function def -> implement to produce fuzzy output from populated table
	void* GenerateFuzzyOutput();

	// Dummy function def ->
	// Generate a haystack that matches only what is seen and to near exact ratios, essentially duplicating the network n times until it's full.
	void* GenerateExactOutput();

	// Void method to print out the information stored within the Personality table into a nice format
	// Takes no arguments and returns nothing
	void ListInfo();

	//Increment every time a host is added
	unsigned long int m_num_of_hosts;

	//Start with range of the subnets, decrement every time host is added
	unsigned long int m_host_addrs_avail;

	//HashMAP[std::string key]; key == Personality, val == ptr to Personality object
	Personality_Table m_personalities;
};

}
