#ifndef CLASSIFICATIONAGGREGATOR_H_
#define CLASSIFICATIONAGGREGATOR_H_

#include "ClassificationEngine.h"
#include "Config.h"

namespace Nova
{

class ClassificationAggregator : public ClassificationEngine
{
public:
	ClassificationAggregator();

	// TODO: Make this a single struct/class
	std::vector<ClassificationEngine*> m_engines;
	std::vector<CLASSIFIER_MODES> m_modes;
	std::vector<double> m_engineWeights;

	double Classify(Suspect *s);

private:
	void LoadConfiguration(std::string filePath);

};

} /* namespace Nova */
#endif /* CLASSIFICATIONAGGREGATOR_H_ */
