//============================================================================
// Name        : FeatureSet.h
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Maintains and calculates distinct features for individual Suspects
//					for use in classification of the Suspect.
//============================================================================/*

#ifndef FEATURESET_H_
#define FEATURESET_H_

#include "TrafficEvent.h"
#include <vector>
#include <set>

///The traffic distribution across the haystacks relative to host traffic
#define IP_TRAFFIC_DISTRIBUTION 0

///The traffic distribution across ports contacted
#define PORT_TRAFFIC_DISTRIBUTION 1

///Number of ScanEvents that the suspect is responsible for per second
#define HAYSTACK_EVENT_FREQUENCY 2

///Measures the distribution of packet sizes
#define PACKET_SIZE_MEAN 3
#define PACKET_SIZE_VARIANCE 4

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
#define DIMENSION 4

namespace Nova{
namespace ClassificationEngine{

typedef std::tr1::unordered_map<in_addr_t, uint > IP_Table;
typedef std::tr1::unordered_map<in_port_t, uint > Port_Table;

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
	///Calculates a feature
	void CalculateIPTrafficDistribution();
	///Calculates a feature
	void CalculatePortTrafficDistribution();
	///Calculates a feature
	void CalculateHaystackToHostEventRatio();
	///Calculates a feature
	void CalculateHaystackEventFrequency();
	///Calculates a feature
	void CalculatePacketSizeMean();
	///Calculates a feature
	void CalculatePacketSizeVariance();
	/// Processes incoming evidence before calculating the features
	void UpdateEvidence(TrafficEvent *event);

private:
	//Temporary variables used to calculate Features

	//Table of IP addresses and associated packet counts
	IP_Table IPTable;
	//Max packet count to an IP, used for normalizing
	uint IPMax;

	//Table of Ports and associated packet counts
	Port_Table portTable;
	//Max packet count to a port, used for normalizing
	uint portMax;

	uint haystackEvents;
	uint hostEvents;
	time_t startTime ;
	time_t endTime;
	//Number of packets total
	uint packetCount;
	//Total number of bytes in all packets
	uint bytesTotal;

	//A vector containing the sum of the packet sizes for each event
	//Not really a vector of packet sizes as tcp events can contain many packets
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
