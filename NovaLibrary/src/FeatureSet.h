//============================================================================
// Name        : FeatureSet.h
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
// Description : Maintains and calculates distinct features for individual Suspects
//					for use in classification of the Suspect.
//============================================================================/*

#ifndef FEATURESET_H_
#define FEATURESET_H_

#include "NovaUtil.h"

using namespace std;

///The traffic distribution across the haystacks relative to host traffic
#define IP_TRAFFIC_DISTRIBUTION 0
#define IP_TRAFFIC_DISTRIBUTION_MASK 1

///The traffic distribution across ports contacted
#define PORT_TRAFFIC_DISTRIBUTION 1
#define PORT_TRAFFIC_DISTRIBUTION_MASK 2

///Number of ScanEvents that the suspect is responsible for per second
#define HAYSTACK_EVENT_FREQUENCY 2
#define HAYSTACK_EVENT_FREQUENCY_MASK 4

///Measures the distribution of packet sizes
#define PACKET_SIZE_MEAN 3
#define PACKET_SIZE_MEAN_MASK 8
#define PACKET_SIZE_DEVIATION 4
#define PACKET_SIZE_DEVIATION_MASK 16

/// Number of distinct IP addresses contacted
#define DISTINCT_IPS 5
#define DISTINCT_IPS_MASK 32

/// Number of distinct ports contacted
#define DISTINCT_PORTS 6
#define DISTINCT_PORTS_MASK 64

///Measures the distribution of intervals between packets
#define PACKET_INTERVAL_MEAN 7
#define PACKET_INTERVAL_MEAN_MASK 128
#define PACKET_INTERVAL_DEVIATION 8
#define PACKET_INTERVAL_DEVIATION_MASK 256

//UDP has max payload of 65535 bytes
//serializeSuspect requires 89 bytes, serializeFeatureData requires 36 bytes, bytes left = 65410
// each entry in a table takes 8 bytes 65410/8 = 8176.25
#define MAX_TABLE_ENTRIES 8176

//boolean values for updateFeatureData()
#define INCLUDE true
#define REMOVE false

//Table of IP destinations and a count;
typedef google::dense_hash_map<in_addr_t, pair<uint32_t, uint32_t>, tr1::hash<in_addr_t>, eqaddr > IP_Table;
//Table of destination ports and a count;
typedef google::dense_hash_map<in_port_t, pair<uint32_t, uint32_t>, tr1::hash<in_port_t>, eqport > Port_Table;
//Table of packet sizes and a count
typedef google::dense_hash_map<int, pair<uint32_t, uint32_t>, tr1::hash<int>, eqint > Packet_Table;
//Table of packet intervals and a count
typedef google::dense_hash_map<time_t, pair<uint32_t, uint32_t>, tr1::hash<time_t>, eqtime > Interval_Table;

namespace Nova{

///A Feature Set represents a point in N dimensional space, which the Classification Engine uses to
///	determine a classification. Each member of the FeatureSet class represents one of these dimensions.
///	Each member must therefore be either a double or int type.
class FeatureSet
{

public:
	/// The actual feature values
	double features[DIM];

	//Number of packets total
	pair<uint, uint> packetCount;

	//Table of IP addresses and associated packet counts
	IP_Table IPTable;

	FeatureSet();
	// Clears out the current values, and also any temp variables used to calculate them
	void ClearFeatureSet();

	// Calculates all features in the feature set
	//		featuresEnabled - Bitmask of which features are enabled, e.g. 0b111 would enable the first 3
	void CalculateAll(uint32_t featuresEnabled);

	// Calculates the local time interval for time-dependent features using the latest time stamps
	void CalculateTimeInterval();

	// Calculates a feature's value
	//		featureDimension: feature to calculate, e.g. PACKET_INTERVAL_DEVIATION
	// Note: this will update the global 'features' array for the given featureDimension index
	void Calculate(uint featureDimension);

	// Processes incoming evidence before calculating the features
	//		packet - packet headers of new packet
	void UpdateEvidence(Packet packet);

	// Adds/removes the local data to the silent alarm data before calculation
	// this is done to improve the performance of UpdateEvidence
	//		include - true: adds data
	//				- false: removes data
	void UpdateFeatureData(bool include);

	// Serializes the contents of the global 'features' array
	//		buf - Pointer to buffer where serialized feature set is to be stored
	// Returns: number of bytes set in the buffer
	uint32_t SerializeFeatureSet(u_char * buf);

	// Deserializes the buffer into the contents of the global 'features' array
	//		buf - Pointer to buffer where serialized feature set resides
	// Returns: number of bytes read from the buffer
	uint32_t DeserializeFeatureSet(u_char * buf);

	// TODO: Might be a good idea to combine SerializeFeatureDataBroadcast/SerializeFeatureDataLocal and
	// the deserializing compliments. Very small changes in the code, we could pass another input arg as a switch.

	// Stores the feature set data into the buffer, retrieved using deserializeFeatureData
	// This function saves serialized data used by the ClassificationEngine for sending silentAlarms, needs data to classify
	//		buf - Pointer to buffer to store serialized data
	// Returns: number of bytes set in the buffer
	uint32_t SerializeFeatureDataBroadcast(u_char * buf);

	// Reads the feature set data from a buffer originally populated by serializeFeatureData
	// and stores it in broadcast data (the second member of uint pairs)
	//		buf - Pointer to buffer where the serialized Feature data broadcast resides
	// Returns: number of bytes read from the buffer
	uint32_t DeserializeFeatureDataBroadcast(u_char * buf);

	// Stores the feature set data into the buffer, retrieved using deserializeFeatureData
	// This function doesn't keep data once serialized. Used by the LocalTrafficMonitor and Haystack for sending suspect information
	//		buf - Pointer to buffer to store serialized data in
	// Returns: number of bytes set in the buffer
	uint32_t SerializeFeatureDataLocal(u_char * buf);

	//Reads the feature set data from a buffer originally populated by serializeFeatureData
	// and stores it in local data (the first member of uint pairs)
	//		buf - Pointer to buffer where the serialized data resides
	// Returns: number of bytes read from the buffer
	uint32_t DeserializeFeatureDataLocal(u_char * buf);

private:
	//Temporary variables used to calculate Features

	//Table of Packet sizes and counts for variance calc
	Packet_Table packTable;

	//Max packet count to an IP, used for normalizing
	uint32_t IPMax;

	//Table of Ports and associated packet counts
	Port_Table portTable;
	//Max packet count to a port, used for normalizing
	uint32_t portMax;

	//Tracks the number of HS events
	pair<uint32_t, uint32_t> haystackEvents;

	time_t startTime;
	time_t endTime;
	pair<time_t, time_t> totalInterval;

	//Total number of bytes in all packets
	pair<uint32_t, uint32_t> bytesTotal;

	///A vector of packet arrival times for tracking traffic over time.
	vector <time_t> packet_times;
	///A vector of the intervals between packet arrival times for tracking traffic over time.
	vector <time_t> packet_intervals;

	///A table of the intervals between packet arrival times for tracking traffic over time.
	Interval_Table intervalTable;

	time_t last_time;
};
}

#endif /* FEATURESET_H_ */
