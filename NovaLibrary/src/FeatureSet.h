//============================================================================
// Name        : FeatureSet.h
// Copyright   : DataSoft Corporation 2011-2013
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
//============================================================================

#ifndef FEATURESET_H_
#define FEATURESET_H_

#include "Evidence.h"
#include "HashMapStructs.h"

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

//dimension
#define DIM 14

#define SANITY_CHECK 268435456 // 2 ^ 28

//Table of IP destinations and a count;
typedef Nova::HashMap<uint32_t, uint64_t, std::hash<time_t>, eqtime > IP_Table;
//Table of destination ports and a count;
typedef Nova::HashMap<in_port_t, uint64_t, std::hash<in_port_t>, eqport > Port_Table;
//Table of packet sizes and a count
typedef Nova::HashMap<uint16_t, uint64_t, std::hash<uint16_t>, eq_uint16_t > Packet_Table;

struct IpPortCombination
{
	uint32_t m_ip;
	uint16_t m_port;
	uint16_t m_internal;

	IpPortCombination()
	{
		m_ip = 0;
		m_port = 0;
		m_internal = 0;
	}

	static IpPortCombination GetEmptyKey()
	{
		IpPortCombination empty;
		empty.m_internal = 1;
		return empty;
	}

	bool operator ==(const IpPortCombination &rhs) const
	{
		// This is for checking equality of empty/deleted keys
		if (m_internal != 0 || rhs.m_internal != 0)
		{
			return m_internal == rhs.m_internal;
		}
		else
		{
			return (m_ip == rhs.m_ip && m_port == rhs.m_port);
		}
	}

	bool operator != (const IpPortCombination &rhs) const
	{
		return !(this->operator ==(rhs));
	}
};


// Make a IpPortCombination hash and equals function for the Google hash maps
namespace std
{
	template<>
	struct hash< IpPortCombination > {
		std::size_t operator()( const IpPortCombination &c ) const
		{
			uint32_t a = c.m_ip;

			// Thomas Wang's integer hash function
			// http://www.cris.com/~Ttwang/tech/inthash.htm
			a = (a ^ 61) ^ (a >> 16);
			a = a + (a << 3);
			a = a ^ (a >> 4);
			a = a * 0x27d4eb2d;
			a = a ^ (a >> 15);

			const int SECRET_CONSTANT = 104729; // 1,000th prime number

			// Map 16-bit port 1:1 to a random-looking number
			a += ((uint32_t)c.m_port * (SECRET_CONSTANT*4 + 1)) & 0xffff;

			return a;
		}
	};
}

struct IpPortCombinationEquals
{
	bool operator()(IpPortCombination k1, IpPortCombination k2) const
	{
		return k1 == k2;
	}
};


typedef Nova::HashMap<IpPortCombination, uint8_t, std::hash<IpPortCombination>, IpPortCombinationEquals> IpPortTable;

namespace Nova
{

///A Feature Set represents a point in N dimensional space, which the Classification Engine uses to
///	determine a classification. Each member of the FeatureSet class represents one of these dimensions.
///	Each member must therefore be either a double or int type.
class FeatureSet
{

public:
	/// The actual feature values
	double m_features[DIM];

	static std::string m_featureNames[];

	//Number of packets total
	uint64_t m_packetCount;

	uint64_t m_tcpPacketCount;
	uint64_t m_udpPacketCount;
	uint64_t m_icmpPacketCount;
	uint64_t m_otherPacketCount;

	FeatureSet();
	~FeatureSet();

	std::string toString();

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
	void Calculate(const uint& featureDimension);

	// Processes incoming evidence before calculating the features
	//		packet - packet headers of new packet
	void UpdateEvidence(const Evidence &evidence);

	// Serializes the contents of the global 'features' array
	//		buf - Pointer to buffer where serialized feature set is to be stored
	// Returns: number of bytes set in the buffer
	uint32_t SerializeFeatureSet(u_char *buf, uint32_t bufferSize);

	// Deserializes the buffer into the contents of the global 'features' array
	//		buf - Pointer to buffer where serialized feature set resides
	// Returns: number of bytes read from the buffer
	uint32_t DeserializeFeatureSet(u_char *buf, uint32_t bufferSize);

	// Reads the feature set data from a buffer originally populated by serializeFeatureData
	// and stores it in broadcast data (the second member of uint pairs)
	//		buf - Pointer to buffer where the serialized Feature data broadcast resides
	// Returns: number of bytes read from the buffer
	uint32_t DeserializeFeatureData(u_char *buf, uint32_t bufferSize);

	// Stores the feature set data into the buffer, retrieved using deserializeFeatureData
	// This function doesn't keep data once serialized. Used by the LocalTrafficMonitor and Haystack for sending suspect information
	//		buf - Pointer to buffer to store serialized data in
	// Returns: number of bytes set in the buffer
	uint32_t SerializeFeatureData(u_char *buf, uint32_t bufferSize);

	// Method that will return the sizeof of all values in the given feature set;
	// for use in SerializeSuspect
	// Returns: sum of the sizeof of all elements in the feature data
	// 			that would be serialized
	uint32_t GetFeatureDataLength();

	void SetHaystackNodes(std::vector<uint32_t> nodes);

	// For some TCP flag ratios and statistics
	uint64_t m_rstCount;
	uint64_t m_ackCount;
	uint64_t m_synCount;
	uint64_t m_finCount;
	uint64_t m_synAckCount;

private:
	//Temporary variables used to calculate Features

	//Table of Packet sizes and counts for variance calc
	Packet_Table m_packTable;

	//Table of Ports and associated packet counts
	Port_Table m_PortTCPTable;
	Port_Table m_PortUDPTable;

	time_t m_startTime;
	time_t m_endTime;
	time_t m_lastTime;
	time_t m_totalInterval;

	//Total number of bytes in all packets
	uint64_t m_bytesTotal;

	//Table of IP addresses and associated packet counts
	IP_Table m_IPTable;
	IP_Table m_HaystackIPTable;

	// Maps IP to number of ports contacted on that IP
	IP_Table m_tcpPortsContactedForIP;
	IP_Table m_udpPortsContactedForIP;

	// Maps IP/port to a bool, used for checking if m_portContactedPerIP needs incrementing for this IP
	IpPortTable m_hasTcpPortIpBeenContacted;
	IpPortTable m_hasUdpPortIpBeenContacted;

	uint32_t m_numberOfHaystackNodesContacted;
};
}

#endif /* FEATURESET_H_ */
