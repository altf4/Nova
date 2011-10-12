//============================================================================
// Name        : FeatureSet.h
// Author      : DataSoft Corporation
// Copyright   :
// Description : A set of features for use in the NOVA utility
//============================================================================/*

#ifndef FEATURESET_H_
#define FEATURESET_H_

#include "TrafficEvent.h"
#include <vector>
#include <set>

///The number of distinct IP addresses that the Suspect has contacted
#define DISTINCT_IP_COUNT 0

///The number of distinct ports that the Suspect has contacted
#define DISTINCT_PORT_COUNT 1

///Number of ScanEvents that the suspect is responsible for per second
#define HAYSTACK_EVENT_FREQUENCY 2

///Ratio of received ScanEvents to TrafficEvents
#define HAYSTACK_EVENT_TO_HOST_EVENT_RATIO 3

///Measures the distribution of packet sizes
#define PACKET_SIZE_MEAN 4
#define PACKET_SIZE_VARIANCE 5

///A measure of how uniform the Suspect's port distributions are
/// 0 - 10. Low value means low uniformity, high means high uniformity.
#define PORT_UNIFORMITY 6

///Measures the distribution of packet counts over IPs
#define PACKET_COUNT_ACROSS_IPS_VARIANCE 7

///Measures the distribution of packet counts over ports
#define PACKET_COUNT_ACROSS_PORTS_VARIANCE 8

///Measures the distribution of intervals between events
#define EVENT_INTERVAL_VARIANCE 9

//TODO: This is a duplicate from the "dim" in ClassificationEngine.cpp. Maybe move to a global?
///	This is the number of features in a feature set.
#define DIMENSION 5

namespace Nova{
namespace ClassificationEngine{

///A Feature Set represents a point in N dimensional space, which the Classification Engine uses to
///	determine a classification. Each member of the FeatureSet class represents one of these dimensions.
///	Each member must therefore be either a double or int type.
class FeatureSet
{

public:
	/// The actual feature values
	double features[DIMENSION];

	FeatureSet();
	///Clears out the current values, and also any temp variables used to calculate them
	void ClearFeatureSet();
	///Calculates a feature, given a new piece of evidence
	void CalculateDistinctIPCount( TrafficEvent *event );
	///Calculates a feature, given a new piece of evidence
	void CalculateDistinctPortCount( TrafficEvent *event );
	///Calculates a feature, given a new piece of evidence
	void CalculateHaystackToHostEventRatio(TrafficEvent *event);
	///Calculates a feature, given a new piece of evidence
	void CalculateHaystackEventFrequency(TrafficEvent *event);
	///Calculates a feature, given a new piece of evidence
	void CalculatePacketSizeMean(TrafficEvent *event);
	///Calculates a feature, given a new piece of evidence
	void CalculatePacketSizeVariance(TrafficEvent *event);
	///Updates all the member variables, given a new piece of evidence
	void UpdateMemberVariables(TrafficEvent *event);


private:
	//Temporary variables used to calculate Features
	set<int> IPTable;
	set<int> portTable;
	uint haystackEvents;
	uint hostEvents;
	time_t startTime ;
	time_t endTime;
	//Number of packets total
	uint packetCount;
	//Total number of bytes in all packets
	uint bytesTotal;
	//A vector of each packet's size
	vector <int> packetSizes;

    friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		for(uint i=0; i < DIMENSION; i++)
		{
			ar & features[i];
		}

	}
};
}
}
#endif /* FEATURESET_H_ */
