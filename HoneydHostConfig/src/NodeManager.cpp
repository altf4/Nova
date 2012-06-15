//============================================================================
// Name        :
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
// Description :
//============================================================================

#include "NodeManager.h"

using namespace std;

namespace Nova
{

NodeManager::NodeManager(HoneydConfiguration *hdconfig)
{
	if(hdconfig == NULL)
	{
		m_hdconfig = new HoneydConfiguration();
	}
	else
	{
		m_hdconfig = hdconfig;
	}
}

vector<Node> NodeManager::NodeManager(profile *prof, int numNodes)
{
	return vector<Node> {};
}

void NodeManager::GenerateProfileCounters()
{
	GenerateMacCounters();
	GeneratePortCounters();
}

void NodeManager::GenerateMacCounters()
{

}

void NodeManager::GeneratePortCounters()
{

}

}
