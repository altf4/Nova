/*
 * NOVAConfiguration.h
 *
 *  Created on: Dec 22, 2011
 *      Author: root
 */

#ifndef NOVACONFIGURATION_H_
#define NOVACONFIGURATION_H_


#include <errno.h>
#include <fstream>
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>

namespace Nova {

using namespace std;

struct NovaOption {
	bool isValid;
	string data;
};


class NOVAConfiguration {
public:
	NOVAConfiguration();
	void LoadConfig(char* input, string homePath);
	virtual ~NOVAConfiguration();


public:
	map<string, NovaOption> options;
};

}

#endif /* NOVACONFIGURATION_H_ */
