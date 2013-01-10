//============================================================================
// Name        : SuspectIdentifer.h
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

#ifndef SUSPECTIDENTIFER_H_
#define SUSPECTIDENTIFER_H_

#include <string>
#include <inttypes.h>

namespace Nova
{

class SuspectIdentifier
{
public:
	// Used internally for empty/deleted keys
	unsigned char m_internal;

	// IP address of the suspect
	uint32_t m_ip;

	// Ethernet interface the suspect was seen on
	std::string m_interface;

	SuspectIdentifier();
	SuspectIdentifier(uint32_t ip, std::string interface);
	bool operator != (const SuspectIdentifier &rhs) const;
	bool operator ==(const SuspectIdentifier &rhs) const;
	std::string GetInterface();
	uint32_t GetSerializationLength();

	uint32_t Serialize(u_char *buf, uint32_t bufferSize);
	uint32_t Deserialize(u_char *buf, uint32_t bufferSize);
};

struct equalityChecker
{
	bool operator()(Nova::SuspectIdentifier k1, Nova::SuspectIdentifier k2) const
	{
		return (k1 == k2);
	}
};

} /* namespace Nova */

#endif /* SUSPECTIDENTIFER_H_ */
