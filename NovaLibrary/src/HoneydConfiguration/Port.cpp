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
//	scanned and identified on a host by Nmap.
//============================================================================

#include "Port.h"

using namespace std;

namespace Nova
{

Port::Port(string name, enum PortProtocol protocol, uint portNumber, enum PortBehavior behavior)
{

	m_name = name;
	m_protocol = protocol;
	m_portNumber = portNumber;
	m_behavior = behavior;
}

struct PortStruct GetCompatStruct()
{
	//TODO: Fill this in
	struct PortStruct retPort;
	return retPort;
}

}
