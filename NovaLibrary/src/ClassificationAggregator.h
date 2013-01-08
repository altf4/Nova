#ifndef CLASSIFICATIONAGGREGATOR_H_
#define CLASSIFICATIONAGGREGATOR_H_

#include "ClassificationEngine.h"

namespace Nova
{

enum CLASSIFIER_MODES {
	CLASSIFIER_WEIGHTED,
	CLASSIFIER_HOSTILE_OVERRIDE,
	CLASSIFIER_BENIGN_OVERRIDE
};

class ClassificationAggregator
{
public:
	ClassificationAggregator();

	// TODO: Make this a single struct/class
	std::vector<ClassificationEngine*> m_engines;
	std::vector<CLASSIFIER_MODES> m_modes;
	std::vector<double> m_engineWeights;

	double Classify(Suspect *s);

private:
	void Init();

};

} /* namespace Nova */
#endif /* CLASSIFICATIONAGGREGATOR_H_ */
