//============================================================================
// Name        : NOVAConfiguration.h
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Loads and parses the configuration file
//============================================================================/*

#ifndef NOVACONFIGURATION_H_
#define NOVACONFIGURATION_H_

<<<<<<< HEAD
<<<<<<< HEAD
=======
#include "NovaUtil.h"
=======
<<<<<<< HEAD
>>>>>>> NovaUtil

#include <google/dense_hash_map>
=======
#include "NovaUtil.h"
>>>>>>> NovaUtil
<<<<<<< HEAD
=======
>>>>>>> master
>>>>>>> NovaUtil

namespace Nova {

using namespace std;

struct NovaOption {
	bool isValid;
	string data;
};

//Equality operator used by google's dense hash map
struct eq
{
  bool operator()(string s1, string s2) const
  {
    return !(s1.compare(s2));
  }
};

typedef google::dense_hash_map<string, struct NovaOption, tr1::hash<string>, eq > optionsMap;



class NOVAConfiguration {
public:
	NOVAConfiguration();
	void LoadConfig(char* input, string homePath);
	virtual ~NOVAConfiguration();


public:
	optionsMap options;
};

}

#endif /* NOVACONFIGURATION_H_ */
