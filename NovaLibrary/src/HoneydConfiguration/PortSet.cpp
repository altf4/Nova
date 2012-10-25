//============================================================================
// Name        : PortSet.cpp
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
// Description : Represents a collection of ports for a ScannedHost and their
//		respective observed behaviors. ICMP is included as a "port" for the
//		purposes of this structure
//============================================================================

#include "PortSet.h"

using namespace std;

namespace Nova
{

PortSet::PortSet()
{
	m_defaultTCPBehavior = PORT_CLOSED;
	m_defaultUDPBehavior = PORT_CLOSED;
	m_defaultICMPBehavior = PORT_OPEN;
}

bool PortSet::AddPort(Port port)
{
	if(port.m_protocol == PROTOCOL_TCP)
	{
		m_TCPexceptions.push_back(port);
	}
	else if(port.m_protocol == PROTOCOL_UDP)
	{
		m_UDPexceptions.push_back(port);
	}
	else
	{
		return false;
	}
	return true;
}

bool PortSet::SetTCPBehavior(const string &behavior)
{
	if(!behavior.compare("open"))
	{
		m_defaultTCPBehavior = PORT_OPEN;
	}
	else if(!behavior.compare("closed"))
	{
		m_defaultTCPBehavior = PORT_CLOSED;
	}
	else if(!behavior.compare("filtered"))
	{
		m_defaultTCPBehavior = PORT_FILTERED;
	}
	else
	{
		return false;
	}
	return true;
}
}

