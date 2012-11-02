//============================================================================
// Name        : HoneydConfigTypes.cpp
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
// Description : Common structs and typedefs for NovaGUI
//============================================================================

#include "HoneydConfigTypes.h"

namespace Nova
{

std::string PortBehaviorToString(enum PortBehavior behavior)
{
	switch(behavior)
	{
		case PORT_OPEN:
		{
			return "open";
		}
		case PORT_CLOSED:
		{
			return "closed";
		}
		case PORT_FILTERED:
		{
			return "filtered";
		}
		case PORT_SCRIPT:
		{
			return "script";
		}
		case PORT_TARPIT_OPEN:
		{
			return "tarpit open";
		}
		case PORT_TARPIT_SCRIPT:
		{
			return "tarpit script";
		}
		default:
		{
			return "";
		}
	}
}

enum PortBehavior StringToPortBehavior(std::string behavior)
{
	if(!behavior.compare("open"))
	{
		return PORT_OPEN;
	}
	else if(!behavior.compare("closed"))
	{
		return PORT_CLOSED;
	}
	else if(!behavior.compare("filtered"))
	{
		return PORT_FILTERED;
	}
	else if(!behavior.compare("script"))
	{
		return PORT_SCRIPT;
	}
	else if(!behavior.compare("tarpit open"))
	{
		return PORT_TARPIT_OPEN;
	}
	else if(!behavior.compare("tarpit script"))
	{
		return PORT_TARPIT_SCRIPT;
	}
	else
	{
		return PORT_ERROR;
	}
}

std::string PortProtocolToString(enum PortProtocol protocol)
{
	switch(protocol)
	{
		case PROTOCOL_UDP:
		{
			return "udp";
		}
		case PROTOCOL_TCP:
		{
			return "tcp";
		}
		case PROTOCOL_ICMP:
		{
			return "icmp";
		}
		default:
		{
			return "";
		}
	}
}

enum PortProtocol StringToPortProtocol(std::string protocol)
{
	if(!protocol.compare("tcp"))
	{
		return PROTOCOL_TCP;
	}
	else if(!protocol.compare("udp"))
	{
		return PROTOCOL_UDP;
	}
	else if(!protocol.compare("icmp"))
	{
		return PROTOCOL_ICMP;
	}
	else
	{
		return PROTOCOL_ERROR;
	}
}

}
