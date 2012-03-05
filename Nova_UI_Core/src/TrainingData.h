//============================================================================
// Name        : TrainingData.h
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

#ifndef TRAININGDATA_H_
#define TRAININGDATA_H_

#include <string>
#include <vector>

#include "HashMapStructs.h"

// Header for training data
struct _trainingSuspect
{
	bool isHostile;
	bool isIncluded;
	string uid;
	string description;
	vector<string>* points;
};

typedef struct _trainingSuspect trainingSuspect;

typedef google::dense_hash_map<string, trainingSuspect*, tr1::hash<string>, eqstr > trainingSuspectMap;
typedef google::dense_hash_map<string, vector<string>*, tr1::hash<string>, eqstr > trainingDumpMap;

class TrainingData
{
public:
	// Convert CE dump to Training DB format and append it
	static bool CaptureToTrainingDb(string dbFile, trainingSuspectMap* selectionOptions);

	// Parse a CE dump file
	static trainingDumpMap* ParseEngineCaptureFile(string captureFile);

	// Parse a Training DB file
	static trainingSuspectMap* ParseTrainingDb(string dbPath);

	// Create a CE data file from a subset of the Training DB file
	static string MakaDataFile(trainingSuspectMap& db);

	// Removes consecutive points who's squared distance is less than a specified distance
	static void ThinTrainingPoints(trainingDumpMap* suspects, double distanceThreshhold);

};

#endif /* TRAININGDATA_H_ */
