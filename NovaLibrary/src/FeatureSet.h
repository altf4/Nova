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
#include <google/dense_hash_map>
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

#define INITIAL_IP_SIZE 256
#define INITIAL_PORT_SIZE 1024
#define INITIAL_PACKET_SIZE 4096

//UDP has max payload of 65535 bytes, suspect with data has a base size of 180 bytes
// each packet can requires up to 28 bytes, meaning we can only send up information on
// a maximum of 2334 packets
#define MAX_PACKETS_PER_MSG 2334
#define MAX_TABLE_ENTRIES 8168

//boolean values for updateFeatureData()
#define INCLUDE true
#define REMOVE false



//TODO: This is a duplicate from the "dim" in ClassificationEngine.cpp. Maybe move to a global?
///	This is the number of features in a feature set.
#define DIMENSION 9

namespace Nova{
namespace ClassificationEngine{

//Equality operator used by google's dense hash map
struct eqaddr
{
  bool operator()(in_addr_t s1, in_addr_t s2) const
  {
	    return (s1 == s2);
  }
};

//Equality operator used by google's dense hash map
struct eqport
{
  bool operator()(in_port_t s1, in_port_t s2) const
  {
	    return (s1 == s2);
  }
};

//Equality operator used by google's dense hash map
struct eqint
{
  bool operator()(int s1, int s2) const
  {
	    return (s1 == s2);
  }
};

//Equality operator used by google's dense hash map
struct eqtime
{
  bool operator()(time_t s1, time_t s2) const
  {
	    return (s1 == s2);
  }
};


typedef google::dense_hash_map<in_addr_t, pair<uint, uint>, tr1::hash<in_addr_t>, eqaddr > IP_Table;
typedef google::dense_hash_map<in_port_t, pair<uint, uint>, tr1::hash<in_port_t>, eqport > Port_Table;
typedef google::dense_hash_map<int, pair<uint, uint>, tr1::hash<int>, eqint > Packet_Table;
typedef google::dense_hash_map<time_t, pair<uint, uint>, tr1::hash<time_t>, eqtime > Interval_Table;


struct silentAlarmFeatureData
{
	/// The actual feature values
	double features[DIMENSION];

	// endTime - startTime : used to incorporate time based information correctly.
	uint totalInterval;
	//Number of TrafficEvents that originate from the haystack
	uint haystackEvents;
	//Total number of packets
	uint packetCount;
	//Total number of bytes
	uint bytesTotal;

};


typedef google::dense_hash_map<in_addr_t, struct silentAlarmFeatureData, tr1::hash<in_addr_t>, eqaddr > Silent_Alarm_Table;

///A Feature Set represents a point in N dimensional space, which the Classification Engine uses to
///	determine a classification. Each member of the FeatureSet class represents one of these dimensions.
///	Each member must therefore be either a double or int type.
class FeatureSet
{

public:
	/// The actual feature values
	double features[DIMENSION];

	//Table of Nova hosts and feature set data needed to include silent alarm information in classifications
	Silent_Alarm_Table SATable;

	//Number of packets total
	pair<uint, uint> packetCount;

	FeatureSet();
	///Clears out the current values, and also any temp variables used to calculate them
	void ClearFeatureSet();
	//Calculates all features in the feature set
	void CalculateAll();
	///Calculates the local time interval for time-dependent features using the latest time stamps
	void CalculateTimeInterval();

	///Calculates the distribution of traffic across IP address
	void CalculateIPTrafficDistribution();
	///Calculates the distribution of traffic across ports
	void CalculatePortTrafficDistribution();

	///Calculates distinct IPs contacted
	void CalculateDistinctIPs();
	///Calculates distinct ports contacted
	void CalculateDistinctPorts();

	///Calculates the rate that haystack nodes are contacted
	void CalculateHaystackEventFrequency();

	///Calculates the mean packet size
	void CalculatePacketSizeMean();
	///Calculates the standard deviation of the packet sizes
	void CalculatePacketSizeDeviation();

	///Calculates the average time between packets
	void CalculatePacketIntervalMean();
	///Calculates the standard deviation of time between packets
	void CalculatePacketIntervalDeviation();

	/// Processes incoming evidence before calculating the features
	void UpdateEvidence(TrafficEvent *event);
	//Adds the local data to the silent alarm data before calculation
	// this is done to improve the performance of UpdateEvidence
	void UpdateFeatureData(bool include);

	//Stores the FeatureSet information into the buffer, retrieved using deserializeFeatureSet
	//	returns the number of bytes set in the buffer
	uint serializeFeatureSet(u_char * buf);
	//Reads FeatureSet information from a buffer originally populated by serializeFeatureSet
	//	returns the number of bytes read from the buffer
	uint deserializeFeatureSet(u_char * buf);

	//Stores the feature set data into the buffer, retrieved using deserializeFeatureData
	//	returns the number of bytes set in the buffer
	uint serializeFeatureData(u_char * buf, in_addr_t hostAddr);
	//Reads the feature set data from a buffer originally populated by serializeFeatureData
	// and stores that information into SATable[hostAddr]
	//	returns the number of bytes read from the buffer
	uint deserializeFeatureData(u_char * buf, in_addr_t hostAddr);

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

	//Tracks the number of HS events
	pair<uint, uint> haystackEvents;

	time_t startTime;
	time_t endTime;
	pair<time_t, time_t> totalInterval;

	//Total number of bytes in all packets
	pair<uint, uint> bytesTotal;

	///A vector of packet arrival times for tracking traffic over time.
	vector <time_t> packet_times;
	///A vector of the intervals between packet arrival times for tracking traffic over time.
	vector <time_t> packet_intervals;

	///A table of the intervals between packet arrival times for tracking traffic over time.
	Interval_Table intervalTable;

	time_t last_time;

};
}
}

#endif /* FEATURESET_H_ */
