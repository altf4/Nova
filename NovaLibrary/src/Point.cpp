//============================================================================
// Name        : Point.cpp
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
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

Point::~Point()
{
	annDeallocPt(annPoint);
	classification = 0;
}
}
}
