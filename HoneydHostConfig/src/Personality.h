/*
 * Personality.h
 *
 *  Created on: Jun 1, 2012
 *      Author: victim
 */

using namespace std;

class Personality
{

public:

	Personality();
	~Personality();

	unsigned int m_count;
	unsigned long long int m_port_count;

	//MAP[std::string], val == num of times port seen for hosts w/ this personality, string == <port #>_<port type> ex: 10_TCP or 643_UDP
	//MAP[std::string], val == num of times mac vendor is seen for hosts
	// w/ this personality, string == vendor name we can look up in the MAC vendor tables

};


