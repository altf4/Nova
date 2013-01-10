//============================================================================
// Name        : ThresholdTriggerClassification.cpp
// Copyright   : DataSoft Corporation 2011-2013
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
// Description : TODO: Description here
//============================================================================

#include "ThresholdTriggerClassification.h"
#include "Config.h"

namespace Nova
{

ThresholdTriggerClassification::ThresholdTriggerClassification()
{

}

double ThresholdTriggerClassification::Classify(Suspect *suspect)
{
	double classification = 0;

	vector<HostileThreshold> thresholds = Config::Inst()->GetHostileThresholds();
	for(uint i = 0; i < DIM; i++)
	{
		if (!Config::Inst()->IsFeatureEnabled(i))
		{
			continue;
		}

		HostileThreshold threshold = thresholds.at(i);
		if (threshold.m_hasMaxValueTrigger)
		{
			if (suspect->m_features.m_features[i] >= threshold.m_maxValueTrigger)
			{
				classification = 1;
			}
		}

		if (threshold.m_hasMinValueTrigger)
		{
			if (suspect->m_features.m_features[i] <= threshold.m_minValueTrigger)
			{
				classification = 1;
			}
		}
	}

	if (classification > Config::Inst()->GetClassificationThreshold())
	{
		suspect->SetIsHostile(true);
	}
	else
	{
		suspect->SetIsHostile(false);
	}
	suspect->SetClassification(classification);

	return classification;
}

} /* namespace Nova */
