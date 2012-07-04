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

#include "Logger.h"
#include "Suspect.h"
#include "Doppelganger.h"

namespace Nova
{

enum normalizationType {
	NONORM, 		// Does no data normalization. Feature must already be in a range from 0 to 1
	LINEAR,			// Simple linear normalization by dividing data by the max
	LINEAR_SHIFT, 	// Shifts min value to 0 before doing linear normalization
	LOGARITHMIC		// Logarithmic normalization, larger outlier value will have less of an effect
};


class ClassificationEngine
{
public:
	ClassificationEngine(SuspectTable &table);

	~ClassificationEngine();

	// Performs classification on given suspect
	//		suspect - suspect to classify based on current evidence
	// 		returns: suspect classification
	// Note: this updates the classification of the suspect in dataPtsWithClass as well as it's isHostile variable
	double Classify(Suspect *suspect);

	// Forms the normalized kd tree, called once on start up
	// Will be called again if the a suspect's max value for a feature exceeds the current maximum for normalization
	void FormKdTree();

	// Reads into the list of suspects from a file specified by inFilePath
	//		inFilePath - path to input file, should contain Feature dimensions
	//					 followed by hostile classification (0 or 1), all space separated
	void LoadDataPointsFromFile(std::string inFilePath);
	void LoadDataPointsFromVector(std::vector<double*> points);

	// Writes the list of suspects out to a file specified by outFilePath
	//		outFilePath - path to output file
	void WriteDataPointsToFile(std::string outFilePath, ANNkd_tree *tree);

	// Normalized a single value
	static double Normalize(normalizationType type, double value, double min, double max);

	// Prints a single ANN point, p, to stream, out
	//		out - steam to print to
	//		p 	- ANN point to print
	void PrintPt(std::ostream &out, ANNpoint p);

	Doppelganger *m_dopp;

private:
	// Disable the empty constructor, we need the logger/config/suspect table to do anything
	ClassificationEngine();

	// Types of normalization to apply to our features
	static normalizationType m_normalization[];

	std::vector <Point*> m_dataPtsWithClass;

	// kdtree stuff
	int m_nPts;						//actual number of data points
	ANNpointArray m_dataPts;				//data points
	ANNpointArray m_normalizedDataPts;	//normalized data points
	ANNkd_tree*	m_kdTree;					// search structure

	// Used for data normalization
	double m_maxFeatureValues[DIM];
	double m_minFeatureValues[DIM];
	double m_meanFeatureValues[DIM];

};

} // End namespace

#endif /* CLASSIFICATIONENGINE_H_ */
