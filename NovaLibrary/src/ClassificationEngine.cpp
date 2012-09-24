//============================================================================
// Name        : ClassificationEngine.cpp
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
//============================================================================/*

#include "ThresholdTriggerClassification.h"
#include "ClassificationEngine.h"
#include "KnnClassification.h"
#include "Config.h"

#include <string>

using namespace Nova;

ClassificationEngine::ClassificationEngine() {}
ClassificationEngine::~ClassificationEngine() {}
void ClassificationEngine::LoadConfiguration() {}

// Factory method for classification engine creation
ClassificationEngine * ClassificationEngine::MakeEngine()
{
	string engine = Config::Inst()->GetClassificationEngineType();
	if (engine == "KNN")
	{
		return new KnnClassification();
	}
	else if (engine == "THRESHOLD_TRIGGER")
	{
		return new ThresholdTriggerClassification();
	}

	return NULL;
}

vector<string> ClassificationEngine::GetSupportedEngines()
{
	vector<string> supportedEngines;
	supportedEngines.push_back("KNN");
	supportedEngines.push_back("THRESHOLD_TRIGGER");

	return supportedEngines;
}


