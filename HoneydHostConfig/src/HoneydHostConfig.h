#ifndef HONEYDHOSTCONFIG_
#define HONEYDHOSTCONFIG_

#include "HashMapStructs.h"
#include "HashMap.h"
#include "PersonalityTable.h"
#include <boost/property_tree/ptree.hpp>

typedef Nova::HashMap<std::string, std::pair<uint16_t,std::string>, std::tr1::hash<std::string>, eqstr > Pers_Table;
typedef Nova::HashMap<std::string, uint16_t, std::tr1::hash<std::string>, eqstr > Ports_Table;
// make a vendor MAC db to query for MAC

namespace Nova
{
enum ErrCode : int
{
	OKAY = 0,
	AUTODETECTFAIL,
	GETNAMEINFOFAIL,
	GETBITMASKFAIL,
	DONTADDSELF
};

void load_nmap(const std::string &filename);
void parseHost(boost::property_tree::ptree pt2);
std::vector<std::string> getSubnetsToScan(ErrCode * errVar);
void calculateDistributionMetrics();
void printRecv(std::vector<std::string> recv);
ErrCode LoadPersonalityTable(std::vector<std::string> recv);
}
#endif
