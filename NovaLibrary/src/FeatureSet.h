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

#include "HashMap.h"
#include "HashMapStructs.h"
#include "Defines.h"
#include <pcap.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>

///	A struct version of a packet, as received from libpcap
struct _packet
{
	///	Meta information about packet
	struct pcap_pkthdr pcap_header;
	///	Pointer to an IP header
	struct ip ip_hdr;
	/// Pointer to a TCP Header
	struct tcphdr tcp_hdr;
	/// Pointer to a UDP Header
	struct udphdr udp_hdr;
	/// Pointer to an ICMP Header
	struct icmphdr icmp_hdr;

	bool fromHaystack;
};

typedef struct _packet Packet;

///Hash table for current TCP Sessions
///Table key is the source network socket, comprised of IP and Port in string format
///	IE: "192.168.1.1-8080"
struct Session
{
	bool fin;
	std::vector<Packet> session;
};


//boolean values for updateFeatureData()
#define INCLUDE true
#define REMOVE false

//Table of IP destinations and a count;
typedef Nova::HashMap<uint32_t, uint32_t, std::tr1::hash<time_t>, eqtime > IP_Table;
//Table of destination ports and a count;
typedef Nova::HashMap<in_port_t, uint32_t, std::tr1::hash<in_port_t>, eqport > Port_Table;
//Table of packet sizes and a count
typedef Nova::HashMap<uint32_t, uint32_t, std::tr1::hash<int>, eqint > Packet_Table;
//Table of packet intervals and a count
typedef Nova::HashMap<time_t, uint32_t, std::tr1::hash<time_t>, eqtime > Interval_Table;

enum featureIndex: uint8_t
{
	IP_TRAFFIC_DISTRIBUTION = 0,
	PORT_TRAFFIC_DISTRIBUTION = 1,
	HAYSTACK_EVENT_FREQUENCY = 2,
	PACKET_SIZE_MEAN = 3,
	PACKET_SIZE_DEVIATION = 4,
	DISTINCT_IPS = 5,
	DISTINCT_PORTS = 6,
	PACKET_INTERVAL_MEAN = 7,
	PACKET_INTERVAL_DEVIATION = 8
};

namespace Nova{

///A Feature Set represents a point in N dimensional space, which the Classification Engine uses to
///	determine a classification. Each member of the FeatureSet class represents one of these dimensions.
///	Each member must therefore be either a double or int type.
class FeatureSet
{

public:
	/// The actual feature values
	double m_features[DIM];

	//Number of packets total
	uint32_t m_packetCount;

	FeatureSet();
	~FeatureSet();

	// Clears out the current values, and also any temp variables used to calculate them
	void ClearFeatureSet();

	void ClearFeatureData();


	FeatureSet& operator-=(FeatureSet &rhs);
	FeatureSet& operator+=(FeatureSet &rhs);
	bool operator ==(const FeatureSet &rhs) const;
	bool operator !=(const FeatureSet &rhs) const;


	// Calculates all features in the feature set
	//		featuresEnabled - Bitmask of which features are enabled, e.g. 0b111 would enable the first 3
	void CalculateAll();

	// Calculates the local time interval for time-dependent features using the latest time stamps
	void CalculateTimeInterval();

	// Calculates a feature's value
	//		featureDimension: feature to calculate, e.g. PACKET_INTERVAL_DEVIATION
	// Note: this will update the global 'features' array for the given featureDimension index
	void Calculate(uint featureDimension);

	// Processes incoming evidence before calculating the features
	//		packet - packet headers of new packet
	void UpdateEvidence(Packet packet);

	// Serializes the contents of the global 'features' array
	//		buf - Pointer to buffer where serialized feature set is to be stored
	// Returns: number of bytes set in the buffer
	uint32_t SerializeFeatureSet(u_char *buf);

	// Deserializes the buffer into the contents of the global 'features' array
	//		buf - Pointer to buffer where serialized feature set resides
	// Returns: number of bytes read from the buffer
	uint32_t DeserializeFeatureSet(u_char *buf);

	// Reads the feature set data from a buffer originally populated by serializeFeatureData
	// and stores it in broadcast data (the second member of uint pairs)
	//		buf - Pointer to buffer where the serialized Feature data broadcast resides
	// Returns: number of bytes read from the buffer
	uint32_t DeserializeFeatureData(u_char *buf);

	// Stores the feature set data into the buffer, retrieved using deserializeFeatureData
	// This function doesn't keep data once serialized. Used by the LocalTrafficMonitor and Haystack for sending suspect information
	//		buf - Pointer to buffer to store serialized data in
	// Returns: number of bytes set in the buffer
	uint32_t SerializeFeatureData(u_char *buf);

	// Method that will return the sizeof of all values in the given feature set;
	// for use in SerializeSuspect
	// Returns: sum of the sizeof of all elements in the feature data
	// 			that would be serialized
	uint32_t GetFeatureDataLength();

	//FeatureSet(const FeatureSet &rhs);
	//FeatureSet& operator=(FeatureSet &rhs);
private:
	//Temporary variables used to calculate Features

	//Table of Packet sizes and counts for variance calc
	Packet_Table m_packTable;

	//Table of Ports and associated packet counts
	Port_Table m_portTable;

	//Tracks the number of HS events
	uint32_t m_haystackEvents;

	time_t m_startTime;
	time_t m_endTime;
	time_t m_lastTime;

	time_t m_totalInterval;

	//Total number of bytes in all packets
	uint32_t m_bytesTotal;

	///A table of the intervals between packet arrival times for tracking traffic over time.
	Interval_Table m_intervalTable;

	//Table of IP addresses and associated packet counts
	IP_Table m_IPTable;

	//XXX Temporarily using SANITY_CHECK/2, rather than that, we should serialize a total byte size before the
	// feature data then proceed like before, this will allow Deserialized to perform a real sanity checking and
	// this max table entries var can be replaced with a total bytesize check
	static const uint32_t m_maxTableEntries = (((SANITY_CHECK) -
		//(Message Handling m_messageType + (m_callbackType || m_requestType) + m_suspectLength
		((3*sizeof(uint32_t)
		//(Suspect Members) IP + Classificiation + 5x flags + hostile neighbors + featureAccuracy[DIM] + features[DIM]
		+ sizeof(in_addr_t)+ sizeof(double) + 5*(sizeof(bool)) +  sizeof(int32_t) + 2*DIM*(sizeof(double)))
		// + 2*(FeatureSetData constant var byte size) (could serialized two feature sets)
		+ (2*(7*(sizeof(uint32_t)) + 4*(sizeof(time_t))))))
		// All of the Above divided by (8 bytes per table entry) * (up to two feature sets per serialization)
		/(8*2));

};
}

#endif /* FEATURESET_H_ */
