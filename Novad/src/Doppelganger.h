//============================================================================
// Name        : Doppelganger.h
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

#ifndef DOPPELGANGER_H_
#define DOPPELGANGER_H_

#include "SuspectTable.h"

namespace Nova
{

class Doppelganger
{

public:

	Doppelganger(SuspectTable& suspects);

	~Doppelganger();

	void UpdateDoppelganger();

	void ClearDoppelganger();

	void InitDoppelganger();

	void ResetDoppelganger();

private:

	SuspectTable& m_suspectTable;
	std::vector<uint64_t> m_suspectKeys;

};
}

#endif /* DOPPELGANGER_H_ */
