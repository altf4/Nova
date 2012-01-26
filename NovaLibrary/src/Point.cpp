//============================================================================
// Name        : Point.cpp
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
// Description : Points paired with a classification for use in the Approximate
//					Nearest Neighbor algorithm.
//============================================================================/*

#include "Point.h"

using namespace std;

namespace Nova{
namespace ClassificationEngine{

Point::Point()
{
	annPoint = annAllocPt(DIM);
	classification = 0;
}

Point::Point(uint enabledFeatures)
{
	annPoint = annAllocPt(enabledFeatures);
	classification = 0;
}


Point::~Point()
{
	annDeallocPt(annPoint);
	classification = 0;
}
}
}
