//============================================================================
// Name        : Suspect.h
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : A Suspect object with identifying information and traffic
//					features so that each entity can be monitored and classified.
//============================================================================/*

#ifndef SUSPECT_H_
#define SUSPECT_H_

#include "NovaUtil.h"
#include "FeatureSet.h"

using namespace std;
using namespace Nova;

namespace Nova{

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

	/// Is the classification above the current threshold? IE: What conclusion has the CE come to?
	bool isHostile;

	/// Does the classification need updating?
	///		IE: Has the evidence changed since last it was calculated?
	bool needs_classification_update;

	///	Does the FeatureSet need updating?
	///		IE: Has the evidence changed since last it was calculated?
	bool needs_feature_update;

	///	Has this suspect been the subject of an alarm from another Nova instance?
	bool flaggedByAlarm;

	/// Is this a live capture or is NOVA reading from a pcap file?
	bool isLive;

	///	The Feature Set for this Suspect
	FeatureSet features;

	///	The feature set in the format that ANN requires.
	ANNpoint annPoint;

	///	A listing of all the events (evidence) that originated from this suspect
	vector <struct Packet> evidence;

	///	Blank Constructor
	Suspect();

	///	Destructor. Has to delete the FeatureSet object within.
	~Suspect();

	///	Constructor from a TrafficEvent
	Suspect(struct Packet packet);

	///	Converts suspect into a human readable string and returns it
	string ToString();

	///	Add an additional piece of evidence to this suspect
	///		Does not take actions like reclassifying or calculating features.
	void AddEvidence(struct Packet);

	///	Calculates the feature set for this suspect
	void CalculateFeatures(bool isTraining);

	//Stores the Suspect information into the buffer, retrieved using deserializeSuspect
	//	returns the number of bytes set in the buffer
	uint serializeSuspect(u_char * buf);

	//Reads Suspect information from a buffer originally populated by serializeSuspect
	//	returns the number of bytes read from the buffer
	uint deserializeSuspect(u_char * buf);

	//Reads Suspect information from a buffer originally populated by serializeSuspect
	// expects featureSet data appended by serializeFeatureData after serializeSuspect
	//	returns the number of bytes read from the buffer
	uint deserializeSuspectWithData(u_char * buf, bool isLocal);
};

}

#endif /* SUSPECT_H_ */
