//============================================================================
// Name        : HashMapStructs.h
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
// Description : Common hash map structure definitions for use with hashmap
//============================================================================/*

#ifndef HASHMAPSTRUCTS_H_
#define HASHMAPSTRUCTS_H_

#include "HashMap.h"
#include <arpa/inet.h>
#include <vector>
#include <string>

/************************************************************************/
/********** Equality operators used by hash maps *********/
/************************************************************************/

struct eqstr
{
  bool operator()(std::string s1, std::string s2) const
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

struct eqkey
{
	bool operator()(uint64_t k1, uint64_t k2) const
	{
		return (k1 == k2);
	}
};

///The Value is a vector of IP headers
typedef Nova::HashMap<std::string, struct Session, std::tr1::hash<std::string>, eqstr > TCPSessionHashTable;

#endif /* HASHMAPSTRUCTS_H_ */
