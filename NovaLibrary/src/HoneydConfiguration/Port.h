//============================================================================
// Name        : Port.h
// Copyright   : DataSoft Corporation 2012
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
// Description : Represents a single instance of a TCP or UDP port that has been
//	scanned and identified on a host by Nmap. Small enough of a class such that
//	making copies is acceptable
//============================================================================

#ifndef PORT_H_
#define PORT_H_

#include "HoneydConfigTypes.h"

namespace Nova
{

class Port
{

public:

	Port(std::string name, enum PortProtocol protocol, uint portNumber, enum PortBehavior behavior);

	struct PortStruct GetCompatStruct();

	std::string m_name;

	enum PortProtocol m_protocol;

	uint m_portNumber;

	enum PortBehavior m_behavior;

	std::string m_service;
	std::string m_scriptName;

};

}

#endif /* PORT_H_ */
