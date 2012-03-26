//============================================================================
// Name        : Doppelganger.cpp
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
// Description : Set of functions used by Novad for masking local host information
//============================================================================

#include "Config.h"
#include "Logger.h"
#include "NovaUtil.h"
#include "Doppelganger.h"

#include <string>
#include <sstream>
#include <sys/types.h>
#include <arpa/inet.h>

std::string hostIP;

namespace Nova
{

Doppelganger::Doppelganger(SuspectTable& suspects)
: m_suspectTable(suspects)
{
	hostIP = GetLocalIP(Config::Inst()->GetInterface().c_str());
	InitDoppelganger();
}

Doppelganger::~Doppelganger()
{

}

void Doppelganger::UpdateDoppelganger()
{
	//Get latest list of hostile suspects
	std::vector<uint64_t> keys = m_suspectTable.GetHostileSuspectKeys();
	std::vector<uint64_t> keysCopy = keys;

	//A few variable declarations
	uint64_t temp = 0, i = 0;
	bool found = false;

	std::string prefix = "sudo iptables -t nat -I DOPP -s ";
	std::string suffix = " -j DNAT --to-destination "+Config::Inst()->GetDoppelIp();

	std::stringstream ss;
	in_addr inAddr;

	//Until we've finished checking each hostile suspect
	while(!keys.empty())
	{
		//Get a suspect
		temp = keys.back();
		keys.pop_back();
		found = false;

		//Look for the suspect
		for(i = 0; i < m_suspectKeys.size(); i++)
		{
			//If we find the suspect
			if(m_suspectKeys[i] == temp)
			{
				found = true;
				break;
			}
		}

		//If the suspect is new
		if(!found)
		{
			ss.str("");
			inAddr.s_addr = htonl((in_addr_t)temp);
			ss << prefix << inet_ntoa(inAddr) << suffix;
			if(system(ss.str().c_str()) != 0)
			{
				LOG(ERROR, "Error routing suspect to Doppelganger", "Command '"+ss.str()+"' was unsuccessful.");
			}
		}
		//Erase matched suspect from previous suspect list, this tells us if any suspects were removed
		m_suspectKeys.erase(m_suspectKeys.begin() + i);
	}

	prefix = "sudo iptables -t nat -D DOPP -s ";

	//If any suspects remain in m_suspectKeys then they need to be removed from the rule chain
	while(m_suspectKeys.size())
	{
		temp = m_suspectKeys.back();
		m_suspectKeys.pop_back();

		ss.str("");
		inAddr.s_addr = htonl((in_addr_t)temp);
		ss << prefix << inet_ntoa(inAddr) << suffix;
		if(system(ss.str().c_str()) != 0)
		{
			LOG(ERROR, "Error removing suspect from Doppelganger list.", "Command '"+ss.str()+"' was unsuccessful.");
		}
	}
	m_suspectKeys = keysCopy;
}

void Doppelganger::ClearDoppelganger()
{
	std::string commandLine, prefix = "sudo iptables -F";

	commandLine = prefix + "-D FORWARD -i "+  Config::Inst()->GetDoppelInterface() + " -j DROP";
	if(system(commandLine.c_str()) != 0)
	{
		LOG(DEBUG, "Unable to remove Doppelganger rule, does it exist?", "Command '"+commandLine+"' was unsuccessful.");
	}

	prefix = "sudo iptables -t nat ";
	commandLine = prefix + "-F DOPP";
	if(system(commandLine.c_str()) != 0)
	{
		LOG(DEBUG, "Unable to remove Doppelganger rule, does it exist?", "Command '"+commandLine+"' was unsuccessful.");
	}

	commandLine = prefix + "-D PREROUTING -d "+ hostIP + " -j DOPP";
	if(system(commandLine.c_str()) != 0)
	{
		LOG(DEBUG, "Unable to remove Doppelganger rule, does it exist?", "Command '"+commandLine+"' was unsuccessful.");
	}

	commandLine = prefix + "-X DOPP";
	if(system(commandLine.c_str()) != 0)
	{
		LOG(DEBUG, "Unable to remove Doppelganger rule, does it exist?", "Command '"+commandLine+"' was unsuccessful.");
	}

	commandLine = "sudo route del "+Config::Inst()->GetDoppelIp();
	if(system(commandLine.c_str()) != 0)
	{
		LOG(DEBUG, "Unable to remove Doppelganger host, does it exist?", "Command '"+commandLine+"' was unsuccessful.");
	}
}

void Doppelganger::InitDoppelganger()
{
	std::string commandLine = "sudo iptables -A FORWARD -i "+Config::Inst()->GetDoppelInterface()+" -j DROP";

	if(system(commandLine.c_str()) != 0)
	{
		LOG(ERROR, "Error setting up system for Doppelganger", "Command '"+commandLine+"' was unsuccessful.");
	}

	commandLine = "sudo route add -host "+Config::Inst()->GetDoppelIp()+" dev "+Config::Inst()->GetDoppelInterface();

	if(system(commandLine.c_str()) != 0)
	{
		LOG(ERROR, "Error setting up system for Doppelganger", "Command '"+commandLine+"' was unsuccessful.");
	}

	commandLine = "sudo iptables -t nat -N DOPP";
	if(system(commandLine.c_str()) != 0)
	{
		LOG(WARNING, "Error setting up system for Doppelganger", "Command '"+commandLine+"' was unsuccessful."
			" Attempting to flush 'DOPP' rule chain if it already exists.");
		commandLine = "sudo iptables -t nat -F DOPP";
		if(system(commandLine.c_str()) != 0)
		{
			LOG(ERROR, "Error setting up system for Doppelganger", "Command '"+commandLine+"' was unsuccessful."
				" Unable to flush or create 'DOPP' rule-chain");
		}
	}

	commandLine = "sudo iptables -t nat -I PREROUTING -d "+hostIP+" -j DOPP";
	if(system(commandLine.c_str()) != 0)
	{
		LOG(ERROR, "Error setting up system for Doppelganger", "Command '"+commandLine+"' was unsuccessful.");
	}
}

void Doppelganger::ResetDoppelganger()
{
	ClearDoppelganger();
	InitDoppelganger();

	hostIP = GetLocalIP(Config::Inst()->GetInterface().c_str());
	std::string buf, commandLine, prefix = "sudo ipables -t nat ";

	commandLine = prefix + "-F DOPP";
	if(system(commandLine.c_str()) != 0)
	{
		LOG(DEBUG, "Unable to flush Doppelganger rules.", "Command '"+commandLine+"' was unsuccessful.");
	}
	m_suspectKeys.clear();
	m_suspectKeys = m_suspectTable.GetHostileSuspectKeys();

	prefix = "sudo iptables -t nat -I DOPP -s ";
	std::string suffix = " -j DNAT --to-destination "+Config::Inst()->GetDoppelIp();

	std::stringstream ss;
	in_addr inAddr;

	for(uint i = 0; i < m_suspectKeys.size(); i++)
	{
		ss.str("");
		inAddr.s_addr = htonl((in_addr_t)m_suspectKeys[i]);
		ss << prefix << inet_ntoa(inAddr) << suffix;
		if(system(ss.str().c_str()) != 0)
		{
			LOG(ERROR, "Error routing suspect to Doppelganger", "Command '"+commandLine+"' was unsuccessful.");
		}
	}
}

}
