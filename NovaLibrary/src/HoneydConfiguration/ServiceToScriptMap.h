//============================================================================
// Name        : ScriptTable.h
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
// Description : Header file for the ServiceToScriptMap class. Provides
//               declarations of functions and member variables for
//               the class.
//============================================================================

//Essentially a listing of scripts we currently offer and the types of operating systems they make sense for.
// ****** More of an advanced feature to be implemented when we have more than 5 scripts, for now just set the ports to 'open'
/* We can then iterate over this table (populating only for the OS's we care about from persistent storage or something)
 * and randomly fuzz outside the bounds of what is seen on the network and also more accurately mimic what we do see
 * with appropriate scripts versus just an open port
*/

#ifndef ServiceToScriptMap_H_
#define ServiceToScriptMap_H_

#include "AutoConfigHashMaps.h"

namespace Nova
{
class ServiceToScriptMap
{
public:

	//Default constructor. Just sets the empty and deleted
	// key for the m_scripts member HashMap.
	ServiceToScriptMap();

	//This constructor is meant to take a HoneydConfiguration object's
	// ScriptTable (after it's read from the XML) and using select values
	// in the script structs within the table, populate the m_scripts
	// member variable.
	ServiceToScriptMap(ScriptTable *targetScriptTable);

	//Deconstructor. Does nothing special right now.
	~ServiceToScriptMap();

	//This method will essentially do what the second constructor
	// will do, but any time you want. Mainly used for cases
	// where you've instantiated the ServiceToScriptMap object but
	// used the default constructor instead of the second one.
	bool PopulateServiceToScriptMap(ScriptTable *targetScriptTable);

	//Prints out the contents of the scripts table in a nice,
	// debug format.
	std::string ToString();

	std::vector<Nova::Script> GetScripts(std::string);

private:
	ServiceTable m_serviceMap;

};
}

#endif
