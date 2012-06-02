/*
 * PersonalityTable.cpp
 *
 *  Created on: Jun 1, 2012
 *      Author: victim
 */


//total num_hosts is the total num of unique hosts counted.
//total avail_addrs is the total num of ip addresses avail on the subnet

//Mapping of Personality objects using the OS name as the key
// contains:
/* map of occuring ports (Number_Protocol for key Ex: TCP_22)
 *  - all behaviors assumed to be open if we have an entry.
 *  - determine default behaviors for TCP, UDP & ICMP somehow.
 * m_count of number of hosts w/ this OS.
 * m_port_count - number of open ports counted for hosts w/ this OS
 * map of occuring MAC addr vendors, so we know what types of NIC's are used for machines of a similar type on a network.
 */
