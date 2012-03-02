//============================================================================
// Name        : ClassificationEngine.h
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
// Description : Suspect classification engine
//============================================================================

#ifndef CLASSIFICATIONENGINE_H_
#define CLASSIFICATIONENGINE_H_

#include <string>
#include <fstream>

#include "ANN/ANN.h"

#include "Suspect.h"
#include "Logger.h"
#include "Config.h"
#include "SuspectTable.h"


enum normalizationType {
	NONORM, 		// Does no data normalization. Feature must already be in a range from 0 to 1
	LINEAR,			// Simple linear normalization by dividing data by the max
	LINEAR_SHIFT, 	// Shifts min value to 0 before doing linear normalization
	LOGARITHMIC		// Logarithmic normalization, larger outlier value will have less of an effect
};

class ClassificationEngine
{
public:
	ClassificationEngine(Logger *logger, SuspectHashTable *table);

	~ClassificationEngine();

	// Performs classification on given suspect
	//		suspect - suspect to classify based on current evidence
	// Note: this updates the classification of the suspect in dataPtsWithClass as well as it's isHostile variable
	void Classify(Suspect *suspect);

	// Calculates normalized data points and stores into 'normalizedDataPts'
	void NormalizeDataPoints();

	// Forms the normalized kd tree, called once on start up
	// Will be called again if the a suspect's max value for a feature exceeds the current maximum for normalization
	void FormKdTree();

	// Reads into the list of suspects from a file specified by inFilePath
	//		inFilePath - path to input file, should contain Feature dimensions
	//					 followed by hostile classification (0 or 1), all space separated
	void LoadDataPointsFromFile(string inFilePath);

	// Writes the list of suspects out to a file specified by outFilePath
	//		outFilePath - path to output file
	void WriteDataPointsToFile(string outFilePath, ANNkd_tree* tree);

	// Normalized a single value
	static double Normalize(normalizationType type, double value, double min, double max);

	// Prints a single ANN point, p, to stream, out
	//		out - steam to print to
	//		p 	- ANN point to print
	void PrintPt(ostream &out, ANNpoint p);

	void SetEnabledFeatures(string enabledFeatureMask);

	// Used for disabling features
	uint32_t featureMask;
	bool featureEnabled[DIM];
	uint32_t enabledFeatures;

private:
	// Disable the empty constructor, we need the logger/config/suspect table to do anything
	ClassificationEngine();

	// Types of normalization to apply to our features
	static normalizationType normalization[];

	//Used in classification algorithm. Store it here so we only need to calculate it once
	double sqrtDIM;

	vector <Point*> dataPtsWithClass;

	// kdtree stuff
	int nPts;						//actual number of data points
	ANNpointArray dataPts;				//data points
	ANNpointArray normalizedDataPts;	//normalized data points
	ANNkd_tree*	kdTree;					// search structure

	// Used for data normalization
	double maxFeatureValues[DIM];
	double minFeatureValues[DIM];
	double meanFeatureValues[DIM];

	Logger *logger;
	SuspectHashTable *suspects;
};


#endif /* CLASSIFICATIONENGINE_H_ */
