//============================================================================
// Name        : Suspect.h
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
// Description : A Suspect object with identifying information and traffic
//					features so that each entity can be monitored and classified.
//============================================================================/*

#ifndef SUSPECT_H_
#define SUSPECT_H_

#include "FeatureSet.h"
#include "Point.h"

namespace Nova{

// A Suspect represents a single actor on the network, whether good or bad.
// Suspects are the target of classification and a major part of Nova.
class Suspect
{

public:

	// Default Constructor
	Suspect();

	// Destructor. Has to delete the FeatureSet object within.
	~Suspect();

	// Constructor from a Packet
	//		packet - Used to set the IP address and initial evidence of the suspect
	Suspect(Packet packet);

	// Converts suspect into a human readable std::string
	//		featureEnabled: Array of size DIM that specifies which features to return in the std::string
	// Returns: Human readable std::string of the given feature
	std::string ToString();

	// Calculates the feature set for this suspect
	void CalculateFeatures();

	// Stores the Suspect information into the buffer, retrieved using deserializeSuspect
	//		buf - Pointer to buffer where serialized data will be stored
	// Returns: number of bytes set in the buffer
	uint32_t SerializeSuspect(u_char * buf);
	// Reads Suspect information from a buffer originally populated by serializeSuspect
	//		buf - Pointer to buffer where the serialized suspect is
	// Returns: number of bytes read from the buffer
	uint32_t DeserializeSuspect(u_char * buf);

	// Stores the Suspect and FeatureSet information into the buffer, retrieved using deserializeSuspectWithData
	//		buf - Pointer to buffer where serialized data will be stored
	// Returns: number of bytes set in the buffer
	uint32_t SerializeSuspectWithData(u_char * buf);
	// Reads Suspect information from a buffer originally populated by serializeSuspect
	// expects featureSet data appended by serializeFeatureData after serializeSuspect
	//		buf - Pointer to buffer where serialized data resides
	//		isLocal - Specifies whether this data is from a Silent Alarm (false) or local packets (true)
	// Returns: number of bytes read from the buffer
	uint32_t DeserializeSuspectWithData(u_char * buf, bool isLocal);

	// Includes and Excludes separately tracked data for feature set calculation
	// include - a value of true adds unsent data into the main table
	// 			 a value of false subtracts unsent data from the main table
	void UpdateFeatureData(bool include);

	//Returns a copy of the suspects FeatureSet
	FeatureSet GetFeatureSet();
	//Sets or overwrites the suspects FeatureSet
	void SetFeatureSet(FeatureSet *fs);
	//Adds the feature set 'fs' to the suspect's feature set
	void AddFeatureSet(FeatureSet *fs);
	//Subtracts the feature set 'fs' from the suspect's feature set
	void SubtractFeatureSet(FeatureSet *fs);
	//Clears the feature set of the suspect
	void ClearFeatureSet();

	//Returns a copy of the suspects unsent FeatureSet
	FeatureSet GetUnsentFeatureSet();
	//Sets or overwrites the suspects unsent FeatureSet
	void SetUnsentFeatureSet(FeatureSet *fs);
	//Clears the unsent feature set of the suspect
	void ClearUnsentData();

	//Returns the accuracy double of the feature using featureIndex fi
	double GetFeatureAccuracy(featureIndex fi);
	//Sets the accuracy double of the feature using featureIndex fi
	void SetFeatureAccuracy(featureIndex fi, double d);

	// Returns a copy of the evidence std::vector so that it can be read.
	std::vector <Packet> GetEvidence();
	// Sets the evidence vector to the one passed
	void SetEvidence(std::vector<Packet>);

	// Add an additional piece of evidence to this suspect
	// Does not take actions like reclassifying or calculating features.
	//		packet - Packet headers to extract evidence from
	void AddEvidence(Packet packet);
	// Proccesses all packets in m_evidence and puts them into the suspects unsent FeatureSet data
	void UpdateEvidence();
	//Clears the evidence std::vector
	void ClearEvidence();

	//Returns a copy of the suspects in_addr
	//Returns: Suspect's in_addr or NULL on failure
	in_addr GetInAddr();
	//Sets the suspects in_addr
	void SetInAddr(in_addr in);

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

	//Returns the needs classification bool
	bool GetNeedsClassificationUpdate();
	//Sets the needs classification bool
	void SetNeedsClassificationUpdate(bool b);

	//Returns the flagged by silent alarm bool
	bool GetFlaggedByAlarm();
	//Sets the flagged by silent alarm bool
	void SetFlaggedByAlarm(bool b);

	//Returns the 'from live capture' bool
	bool GetIsLive();
	//Sets the 'from live capture' bool
	void SetIsLive(bool b);

	Suspect& operator=(const Suspect &rhs);
	Suspect(const Suspect &rhs);
	Suspect& operator*(Suspect* rhs);

	// Equality operator, mainly used for test cases
	bool operator==(const Suspect &rhs) const;
	bool operator!=(const Suspect &rhs) const;

private:

	// The main FeatureSet for this Suspect
	FeatureSet m_features;
	// FeatureSet containing data not yet sent through a SA
	FeatureSet m_unsentFeatures;
	// Array of values that represent the quality of suspect classification on each feature
	double m_featureAccuracy[DIM];
	// A vector of packets that have yet to be processed
	std::vector <Packet> m_evidence;
	// The IP address of the suspect. Serves as a unique identifier for the Suspect
	struct in_addr m_IpAddress;
	// The current classification assigned to this suspect.
	//	0-1, where 0 is almost surely benign, and 1 is almost surely hostile.
	//	-1 indicates no classification or error.
	double m_classification;
	//The number of datapoints flagged as hostile that were matched to the suspect (max val == k in the config)
	int32_t m_hostileNeighbors;
	// Is the classification above the current threshold? IE: What conclusion has the CE come to?
	bool m_isHostile;
	// Does the classification need updating?
	//	IE: Has the evidence changed since last it was calculated?
	bool m_needsClassificationUpdate;
	// Has this suspect been the subject of an alarm from another Nova instance?
	bool m_flaggedByAlarm;
	// Is this a live capture or is NOVA reading from a pcap file?
	bool m_isLive;
};

}

#endif /* SUSPECT_H_ */
