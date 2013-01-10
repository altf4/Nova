//============================================================================
// Name        : Suspect.h
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
// Description : A Suspect object with identifying information and traffic
//					features so that each entity can be monitored and classified.
//============================================================================

#ifndef SUSPECT_H_
#define SUSPECT_H_

#include "SerializationHelper.h"
#include "SuspectIdentifer.h"
#include "Point.h"
#include "Evidence.h"
#include "FeatureSet.h"

enum SerializeFeatureMode: uint8_t
{
	NO_FEATURE_DATA = 0,
	UNSENT_FEATURE_DATA = 1,
	MAIN_FEATURE_DATA = 2,
	ALL_FEATURE_DATA = 3
};

enum FeatureMode: bool
{
	UNSENT_FEATURES = true,
	MAIN_FEATURES = false,
};

namespace Nova
{

enum FeatureIndex: uint8_t
{
	IP_TRAFFIC_DISTRIBUTION = 0,
	PORT_TRAFFIC_DISTRIBUTION,
	PACKET_SIZE_MEAN,
	PACKET_SIZE_DEVIATION,
	DISTINCT_IPS,
	DISTINCT_TCP_PORTS,
	DISTINCT_UDP_PORTS,
	AVG_TCP_PORTS_PER_HOST,
	AVG_UDP_PORTS_PER_HOST,
	TCP_PERCENT_SYN,
	TCP_PERCENT_FIN,
	TCP_PERCENT_RST,
	TCP_PERCENT_SYNACK,
	HAYSTACK_PERCENT_CONTACTED
};

// A Suspect represents a single actor on the network, whether good or bad.
// Suspects are the target of classification and a major part of Nova.
class Suspect
{

public:

	// Default Constructor
	Suspect();

	// Destructor. Has to delete the FeatureSet object within.
	~Suspect();

	// This gets a copy of the suspect without the featureset hash tables being copied for performance reasons
	Suspect GetShallowCopy();

	SuspectIdentifier GetIdentifier();
	void SetIdentifier(SuspectIdentifier id);

	// Converts suspect into a human readable std::string
	//		featureEnabled: Array of size DIM that specifies which features to return in the std::string
	// Returns: Human readable std::string of the given feature
	std::string ToString();
	std::string GetIdString();
	std::string GetIpString();
	std::string GetInterface();

	// Proccesses a packet in m_evidence and puts them into the suspects unsent FeatureSet data
	void ReadEvidence(Evidence *evidence, bool deleteEvidence);

	// Calculates the feature set for this suspect
	void CalculateFeatures();

	// Stores the Suspect information into the buffer, retrieved using deserializeSuspect
	//		buf - Pointer to buffer where serialized data will be stored
	// Returns: number of bytes set in the buffer
	uint32_t Serialize(u_char *buf, uint32_t bufferSize, SerializeFeatureMode whichFeatures);

	// Returns an unsigned, 32 bit integer that represents the length of the
	// Suspect to be serialized (in bytes).
	//      GetData - If true, include the FeatureSetData length in this calculation;
	//                if false, don't.
	// Returns: number of bytes to allocate to serialization buffer
	uint32_t GetSerializeLength(SerializeFeatureMode whichFeatures);

	// Reads Suspect information from a buffer originally populated by serializeSuspect
	//		buf - Pointer to buffer where the serialized suspect is
	// Returns: number of bytes read from the buffer
	uint32_t Deserialize(u_char *buf, uint32_t bufferSize, SerializeFeatureMode whichFeatures);


	//Returns a copy of the suspects in_addr
	//Returns: Suspect's in_addr_t or NULL on failure
	in_addr_t GetIpAddress();
	//Sets the suspects in_addr_t
	void SetIpAddress(in_addr_t ip);

	//Returns a copy of the Suspects classification double
	// Returns -1 on failure
	double GetClassification();
	//Sets the suspect's classification
	void SetClassification(double n);

	//Returns the number of hostile neighbors
	int GetHostileNeighbors();
	//Sets the number of hostile neighbors
	void SetHostileNeighbors(int i);

	//Returns the hostility bool of the suspect
	bool GetIsHostile();
	//Sets the hostility bool of the suspect
	void SetIsHostile(bool b);

	//Returns a copy of the suspects FeatureSet
	FeatureSet GetFeatureSet(FeatureMode whichFeatures = MAIN_FEATURES);
	//Sets or overwrites the suspects FeatureSet
	void SetFeatureSet(FeatureSet *fs, FeatureMode whichFeatures = MAIN_FEATURES);

	//Adds the feature set 'fs' to the suspect's feature set
	void AddFeatureSet(FeatureSet *fs, FeatureMode whichFeatures = MAIN_FEATURES);


	//Returns the accuracy double of the feature using featureIndex 'fi'
	// 	fi: featureIndex enum of the feature you wish to set, (see FeatureSet.h for values)
	// Returns the value of the feature accuracy for the feature specified
	double GetFeatureAccuracy(FeatureIndex fi);

	//Sets the accuracy double of the feature using featureIndex 'fi'
	// 	fi: featureIndex enum of the feature you wish to set, (see FeatureSet.h for values)
	// 	 d: the value you wish to set the feature accuracy to
	void SetFeatureAccuracy(FeatureIndex fi, double d);

	// Get the last time we saw this suspect
	long int GetLastPacketTime();

	Suspect& operator=(const Suspect &rhs);
	Suspect(const Suspect &rhs);

	// Equality operator, mainly used for test cases
	bool operator==(const Suspect &rhs) const;
	bool operator!=(const Suspect &rhs) const;

	// Just used for the web UI
	uint64_t GetRstCount() {return m_features.m_rstCount;}
	uint64_t GetAckCount() {return m_features.m_ackCount;}
	uint64_t GetSynCount() {return m_features.m_synCount;}
	uint64_t GetFinCount() {return m_features.m_finCount;}
	uint64_t GetSynAckCount() {return m_features.m_synAckCount;}

	uint64_t GetTcpPacketCount() {return m_features.m_tcpPacketCount;}
	uint64_t GetUdpPacketCount() {return m_features.m_udpPacketCount;}
	uint64_t GetIcmpPacketCount() {return m_features.m_icmpPacketCount;}
	uint64_t GetOtherPacketCount() {return m_features.m_otherPacketCount;}

	bool m_needsClassificationUpdate;

	// The main FeatureSet for this Suspect
	FeatureSet m_features;
	// FeatureSet containing data not yet sent through a SA
	FeatureSet m_unsentFeatures;

	std::string m_classificationNotes;

private:
	SuspectIdentifier m_id;

	// Array of values that represent the quality of suspect classification on each feature
	double m_featureAccuracy[DIM];

	// The current classification assigned to this suspect.
	//	0-1, where 0 is almost surely benign, and 1 is almost surely hostile.
	//	-1 indicates no classification or error.
	double m_classification;
	//The number of datapoints flagged as hostile that were matched to the suspect (max val == k in the config)
	int32_t m_hostileNeighbors;
	// Is the classification above the current threshold? IE: What conclusion has the CE come to?
	bool m_isHostile;

	long int m_lastPacketTime;
};

}

#endif /* SUSPECT_H_ */
