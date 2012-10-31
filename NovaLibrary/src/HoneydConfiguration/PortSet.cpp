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

string PortSet::ToString(const string &profileName)
{
	stringstream out;

	//Defaults
	out << "set " << profileName << " default tcp action " << PortBehaviorToString(m_defaultTCPBehavior) << '\n';
	out << "set " << profileName << " default udp action " << PortBehaviorToString(m_defaultUDPBehavior) << '\n';
	out << "set " << profileName << " default icmp action " << PortBehaviorToString(m_defaultICMPBehavior) << '\n';

	//TCP Exceptions
	for(uint i = 0; i < m_TCPexceptions.size(); i++)
	{
		//If it's a script then we need to print the script path, not "script"
		if((m_TCPexceptions[i].m_behavior == PORT_SCRIPT) || (m_TCPexceptions[i].m_behavior == PORT_TARPIT_SCRIPT))
		{
			out << "add " << profileName << " tcp port " << m_TCPexceptions[i].m_portNumber << "\"" << m_TCPexceptions[i].m_scriptName << "\"";
		}
		else
		{
			out << "add " << profileName << " tcp port " << m_TCPexceptions[i].m_portNumber << PortBehaviorToString(m_TCPexceptions[i].m_behavior);
		}
	}

	//UDP Exceptions
	for(uint i = 0; i < m_UDPexceptions.size(); i++)
	{
		//If it's a script then we need to print the script path, not "script"
		if((m_UDPexceptions[i].m_behavior == PORT_SCRIPT) || (m_UDPexceptions[i].m_behavior == PORT_TARPIT_SCRIPT))
		{
			out << "add " << profileName << " udp port " << m_UDPexceptions[i].m_portNumber << "\"" << m_UDPexceptions[i].m_scriptName << "\"";
		}
		else
		{
			out << "add " << profileName << " udp port " << m_UDPexceptions[i].m_portNumber << PortBehaviorToString(m_UDPexceptions[i].m_behavior);
		}
	}

	return out.str();
}

}

