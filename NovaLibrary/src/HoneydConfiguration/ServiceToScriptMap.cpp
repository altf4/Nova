//============================================================================
// Name        : ScriptTable.cpp
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
// Description : Source file for the ServiceToScriptMap class. This class will
//               maintain a hashmap of services to <osclass, path-to-script>
//               pairs that will be used to add scripts to ports if a script
//               exists for that service and osclass.
//============================================================================

#include "ServiceToScriptMap.h"
#include "../Logger.h"
#include <sstream>

using namespace std;
namespace Nova
{

ServiceToScriptMap::ServiceToScriptMap()
{
	m_serviceMap.set_deleted_key("DELETED");
	m_serviceMap.set_empty_key("EMPTY");
}

ServiceToScriptMap::ServiceToScriptMap(ScriptTable *targetScriptTable)
{
	m_serviceMap.set_deleted_key("DELETED");
	m_serviceMap.set_empty_key("EMPTY");
	PopulateServiceToScriptMap(targetScriptTable);
}

ServiceToScriptMap::~ServiceToScriptMap()
{

}

bool ServiceToScriptMap::PopulateServiceToScriptMap(ScriptTable *targetScriptTable)
{
	if(targetScriptTable == NULL)
	{
		LOG(ERROR, "NULL ScriptTable given, unable to populate the map!", "");
		return false;
	}
	//There's no notion of a count for the values in the ServiceToScriptMap
	// HashMap, so regardless of whether it's there or not, push_back the
	// pair into the value vector. If it's not there, it'll make a new one and
	// push the pair; if it is, it'll just append it to the existing vector.
	for(ScriptTable::iterator it = targetScriptTable->begin(); it != targetScriptTable->end(); it++)
	{
		m_serviceMap[it->second.m_service].push_back(it->second);
	}
	return true;
}

std::string ServiceToScriptMap::ToString()
{
	stringstream ss;
	for(ServiceMap::iterator it = m_serviceMap.begin(); it != m_serviceMap.end(); it++)
	{
		ss << '\n';
		ss << "Service " << it->first << " has the following constituents:" << '\n';

		for(uint i = 0; i < it->second.size(); i++)
		{
			ss << "\tOS Class: " << it->second[i].m_osclass << '\n';
			ss << "\tName: " << it->second[i].m_name << '\n';
		}
	}
	return ss.str();
}

std::vector<Nova::Script> ServiceToScriptMap::GetScripts(std::string targetService)
{
	std::vector<Nova::Script> ret;
	ret.clear();
	if(m_serviceMap.keyExists(targetService))
	{
		ret = m_serviceMap[targetService];
	}
	return ret;
}
}



