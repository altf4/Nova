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
#define PACKET_SIZE_DEVIATION 4

/// Number of distinct IP addresses contacted
#define DISTINCT_IPS 5
/// Number of distinct ports contacted
#define DISTINCT_PORTS 6

///Measures the distribution of intervals between packets
#define PACKET_INTERVAL_MEAN 7
///Measures the distribution of intervals between packets
#define PACKET_INTERVAL_DEVIATION 8



//TODO: This is a duplicate from the "dim" in ClassificationEngine.cpp. Maybe move to a global?
///	This is the number of features in a feature set.
#define DIMENSION 9

namespace Nova{
namespace ClassificationEngine{

typedef std::tr1::unordered_map<in_addr_t, uint > IP_Table;
typedef std::tr1::unordered_map<in_port_t, uint > Port_Table;
typedef std::tr1::unordered_map<int, uint > Packet_Table;

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
	///Calculates distinct IPs contacted
	void CalculateDistinctIPs();
	///Calculates distinct ports contacted
	void CalculateDistinctPorts();
	///Calculates a feature
	void CalculateHaystackEventFrequency();
	///Calculates a feature
	void CalculatePacketSizeMean();
	///Calculates a feature
	void CalculatePacketSizeDeviation();
	///Calculates a feature
	void CalculatePacketIntervalMean();
	///Calculates a feature
	void CalculatePacketIntervalDeviation();
	/// Processes incoming evidence before calculating the features
	void UpdateEvidence(TrafficEvent *event);

private:
	//Temporary variables used to calculate Features

	//Table of Packet sizes and counts for variance calc
	Packet_Table packTable;
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

	///A vector of packet sizes for the event
	vector <int> packetSizes;
	///A vector of packet arrival times for tracking traffic over time.
	vector <time_t> packet_intervals;

    friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		uint i = version;
		for(i=0; i < DIMENSION; i++)
		{
			ar & features[i];
		}
	}
};
}
}
BOOST_IS_BITWISE_SERIALIZABLE(Nova::ClassificationEngine::FeatureSet);

#endif /* FEATURESET_H_ */
