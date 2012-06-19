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
// Description : Source file for the ScriptsTable class. This class will
//               maintain a hashmap of services to <osclass, path-to-script>
//               pairs that will be used to add scripts to ports if a script
//               exists for that service and osclass.
//============================================================================

#include "ScriptsTable.h"

using namespace std;
namespace Nova
{

ScriptsTable::ScriptsTable()
{
	m_scripts.set_deleted_key("DELETED");
	m_scripts.set_empty_key("EMPTY");
}

ScriptsTable::ScriptsTable(ScriptTable pull)
{
	m_scripts.set_deleted_key("DELETED");
	m_scripts.set_empty_key("EMPTY");

	//There's no notion of a count for the values in the ScriptsTable
	// HashMap, so regardless of whether it's there or not, push_back the
	// pair into the value vector. If it's not there, it'll make a new one and
	// push the pair; if it is, it'll just append it to the existing vector.
	for(ScriptTable::iterator it = pull.begin(); it != pull.end(); it++)
	{
			pair<string, string> add;
			add.first = it->second.m_osclass;
			add.second = it->second.m_name;
			m_scripts[it->second.m_service].push_back(add);
	}
}

ScriptsTable::~ScriptsTable()
{

}

void ScriptsTable::PopulateScriptsTable(ScriptTable pull)
{
	for(ScriptTable::iterator it = pull.begin(); it != pull.end(); it++)
	{
			pair<string, string> add;
			add.first = it->second.m_osclass;
			add.second = it->second.m_name;
			m_scripts[it->second.m_service].push_back(add);
	}
}

void ScriptsTable::PrintScriptsTable()
{
	for(Scripts_Table::iterator it = m_scripts.begin(); it != m_scripts.end(); it++)
	{
		cout << endl;
		cout << "Service " << it->first << " has the following constituents:" << endl;

		for(uint i = 0; i < it->second.size(); i++)
		{
			cout << "\tOS Class: " << it->second[i].first << endl;
			cout << "\tName: " << it->second[i].second << endl;
		}

		cout << endl;
	}
}

Scripts_Table ScriptsTable::GetScriptsTable()
{
	return m_scripts;
}
}



