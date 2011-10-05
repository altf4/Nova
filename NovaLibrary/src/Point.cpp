//============================================================================
// Name        : Point.cpp
// Author      : DataSoft Corporation
// Copyright   :
// Description : Point object for use in the NOVA utility
//============================================================================/*

#include "Point.h"

using namespace std;

namespace Nova{
namespace ClassificationEngine{

Point::Point()
{
	annPoint = annAllocPt(DIMENSION);
	classification = 0;
}

Point::~Point()
{
	annDeallocPt(annPoint);
	classification = 0;
}
}
}
