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
// Description : Common hash map structure definitions for use with google dense hashmap
//============================================================================/*

#ifndef HASHMAPSTRUCTS_H_
#define HASHMAPSTRUCTS_H_

#include <google/dense_hash_map>
#include <arpa/inet.h>
#include <vector>
#include <string>

using namespace std;

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

struct eqkey
{
	bool operator()(uint64_t k1, uint64_t k2) const
	{
		return (k1 == k2);
	}
};

///The Value is a vector of IP headers
typedef google::dense_hash_map<string, struct Session, tr1::hash<string>, eqstr > TCPSessionHashTable;

// Header for training data
struct _trainingSuspect
{
	bool isHostile;
	bool isIncluded;
	string uid;
	string description;
	vector<string>* points;
};

typedef struct _trainingSuspect trainingSuspect;

typedef google::dense_hash_map<string, trainingSuspect*, tr1::hash<string>, eqstr > trainingSuspectMap;
typedef google::dense_hash_map<string, vector<string>*, tr1::hash<string>, eqstr > trainingDumpMap;

#endif /* HASHMAPSTRUCTS_H_ */
