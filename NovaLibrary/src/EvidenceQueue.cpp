//============================================================================
// Name        : EvidenceQueue.cpp
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

#include "EvidenceQueue.h"

using namespace std;

namespace Nova
{

EvidenceQueue::EvidenceQueue()
{
	m_first = NULL;
	m_last = NULL;
}

Evidence * EvidenceQueue::Pop()
{
	Evidence * ret = m_first;
	if(ret != NULL)
	{
		m_first = ret->m_next;
		//If this was the last item, set m_last to NULL, designates empty
		if(m_first == NULL)
		{
			m_last = NULL;
		}
		ret->m_next = NULL;
	}
	return ret;
}

Evidence * EvidenceQueue::PopAll()
{
	Evidence *ret = m_first;
	m_first = NULL;
	m_last = NULL;
	return ret;
}

//Returns true if this is the first piece of Evidence
bool EvidenceQueue::Push(Evidence * evidence)
{
	//If m_last != NULL (There is evidence in the queue)
	if(m_last != NULL)
	{
		//Link the next node against the previous
		m_last->m_next = evidence;
		//m_last == last pushed evidence
		m_last = evidence;
		return false;
	}
	//If this is the first piece of evidence, set the front
	m_last = evidence;
	m_first = evidence;
	return true;
}

}
