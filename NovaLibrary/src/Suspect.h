//============================================================================
// Name        : Suspect.h
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : A Suspect object with identifying information and traffic
//					features so that each entity can be monitored and classified.
//============================================================================/*

#ifndef SUSPECT_H_
#define SUSPECT_H_
#define DIMENSION 5

#include "FeatureSet.h"
#include <ANN/ANN.h>

namespace Nova{
namespace ClassificationEngine{

///	A Suspect represents a single actor on the network, whether good or bad.
///Suspects are the target of classification and a major part of Nova.
class Suspect
{

public:
	///The IP address of the suspect. This field serves as a unique identifier for the Suspect
	struct in_addr IP_address;

	///	The current classification assigned to this suspect.
	///		0-1, where 0 is almost surely benign, and 1 is almost surely hostile.
	///		-1 indicates no classification or error.
	double classification;

	/// Does the classification need updating?
	///		IE: Has the evidence changed since last it was calculated?
	bool needs_classification_update;

	///	Does the FeatureSet need updating?
	///		IE: Has the evidence changed since last it was calculated?
	bool needs_feature_update;

	///	Has this suspect been the subject of an alarm from another Nova instance?
	bool flaggedByAlarm;

	///	The Feature Set for this Suspect
	FeatureSet *features;

	///	The feature set in the format that ANN requires.
	ANNpoint annPoint;

	///	A listing of all the events (evidence) that originated from this suspect
	vector <TrafficEvent*> evidence;

	///	Blank Constructor
	Suspect();

	///	Destructor. Has to delete the FeatureSet object within.
	~Suspect();

	///	Constructor from a TrafficEvent
	Suspect(TrafficEvent *event);

	///	Converts suspect into a human readable string and returns it
	string ToString();

	///	Add an additional piece of evidence to this suspect
	///		Does not take actions like reclassifying or calculating features.
	void AddEvidence(TrafficEvent *event);

	///	Calculates the feature set for this suspect
	void CalculateFeatures(bool isTraining);

private:
    friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & IP_address.s_addr;
		ar & classification;
		ar & needs_classification_update;
		ar & needs_feature_update;
		ar & features;
		for(uint i = 0; i < DIMENSION; i++)
		{
			ar & annPoint[i];
		}
		ar & flaggedByAlarm;
	}
};
}
}
#endif /* SUSPECT_H_ */
