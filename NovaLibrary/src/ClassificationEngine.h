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
// Description : 
//============================================================================/*

#ifndef CLASSIFICATIONENGINE_H_
#define CLASSIFICATIONENGINE_H_

#include "Suspect.h"

namespace Nova
{

class ClassificationEngine
{
public:
	// Create the correct subclass from the current configuration
	static ClassificationEngine* MakeEngine();

	virtual ~ClassificationEngine() = 0;

	// Classify a suspect, returns the classification and also sets the suspect's classification
	virtual double Classify(Suspect *suspect) = 0;

	// (Re)loads any configuration settings needed. Must be called before classification.
	virtual void LoadConfiguration() = 0;

protected:
	ClassificationEngine();

};

} /* namespace Nova */
#endif /* CLASSIFICATIONENGINE_H_ */
