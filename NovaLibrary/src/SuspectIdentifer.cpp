//============================================================================
// Name        : SuspectIdentifer.cpp
// Copyright   : DataSoft Corporation 2011-2013
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
// Description : TODO: Description here
//============================================================================

#include "SuspectIdentifer.h"
#include "SerializationHelper.h"

namespace Nova
{

uint32_t SuspectIdentifier::Serialize(u_char *buf, uint32_t bufferSize)
{
	uint32_t offset = 0;

	SerializeChunk(buf, &offset, (char*)&m_internal, sizeof m_internal, bufferSize);
	SerializeChunk(buf, &offset, (char*)&m_ip, sizeof m_ip, bufferSize);
	SerializeString(buf, &offset, m_interface, bufferSize);

	return offset;
}

uint32_t SuspectIdentifier::Deserialize(u_char *buf, uint32_t bufferSize)
{
	uint32_t offset = 0;

	DeserializeChunk(buf, &offset, (char*)&m_internal, sizeof m_internal, bufferSize);
	DeserializeChunk(buf, &offset, (char*)&m_ip, sizeof m_ip, bufferSize);
	m_interface = DeserializeString(buf, &offset, bufferSize);

	return offset;
}

uint32_t SuspectIdentifier::GetSerializationLength()
{
	return sizeof(m_internal) + sizeof(m_ip) + (sizeof(uint32_t) + m_interface.size());
}

std::string SuspectIdentifier::GetInterface()
{
	return m_interface;
}

bool SuspectIdentifier::operator ==(const SuspectIdentifier &rhs) const
{
	// This is for checking equality of empty/deleted keys
	if (m_internal != 0 || rhs.m_internal != 0)
	{
		return m_internal == rhs.m_internal;
	}
	else
	{
		return (m_ip == rhs.m_ip && m_interface == rhs.m_interface);
	}
}

bool SuspectIdentifier::operator != (const SuspectIdentifier &rhs) const
{
	return !(this->operator ==(rhs));
}

SuspectIdentifier::SuspectIdentifier()
	:m_internal(0), m_ip(0)
{}

SuspectIdentifier::SuspectIdentifier(uint32_t ip, std::string interface)
	:m_internal(0), m_ip(ip), m_interface(interface)
{}

} /* namespace Nova */
