//============================================================================
// Name        : Point.h
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Points paired with a classification for use in the Approximate
//					Nearest Neighbor Algorithm.
//============================================================================/*

#include "Suspect.h"
#include <ANN/ANN.h>

#ifndef POINT_H_
#define POINT_H_

namespace Nova{
namespace ClassificationEngine{

///Point class meant to encapsulate both an ANN Point along with a classification
class Point
{

public:

	///	The ANN Point, which represents this suspect in feature space
	ANNpoint annPoint;
	///	The classification given to the point on the basis of k-NN
	int classification;
	Point();
	~Point();

};
}
}
#endif /* POINT_H_ */
