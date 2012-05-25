//============================================================================
// Name        : EvidenceQueue.h
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
// Description : The EvidenceQueue object is a specialized linked-list
//			 of Evidence objects for high performance IP packet processing
//============================================================================/*

#ifndef EVIDENCEQUEUE_H_
#define EVIDENCEQUEUE_H_

#include "Evidence.h"

namespace Nova
{

class EvidenceQueue
{

public:

	EvidenceQueue();

	Evidence * Pop();

	//Returns true if this is the first piece of Evidence
	bool Push(Evidence * evidence);

	Evidence * PopAll();

private:

	Evidence * m_first;
	Evidence * m_last;
};

}

#endif /* EVIDENCEQUEUE_H_ */
