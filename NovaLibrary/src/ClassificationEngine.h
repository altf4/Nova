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
	// Factory method for classification engine creation. Creates instance of correct subclass based on config file settings.
	// Returns NULL if the configuration file entry is invalid, otherwise a pointer to a new ClassificationEngine.
	static ClassificationEngine* MakeEngine();
	static std::vector<std::string> GetSupportedEngines();

	virtual ~ClassificationEngine();

	// Classify a suspect, returns the classification and also sets the suspect's classification
	virtual double Classify(Suspect *suspect) = 0;

	// (Re)loads any configuration settings needed. Must be called before classification.
	virtual void LoadConfiguration();

protected:
	ClassificationEngine();

};

} /* namespace Nova */
#endif /* CLASSIFICATIONENGINE_H_ */
