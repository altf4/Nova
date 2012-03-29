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

	// Add an additional piece of evidence to this suspect
	// Does not take actions like reclassifying or calculating features.
	//		packet - Packet headers to extract evidence from
	// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
	int AddEvidence(Packet packet);

	// Proccesses all packets in m_evidence and puts them into the suspects unsent FeatureSet data
	// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
	int UpdateEvidence();

	// Returns a copy of the evidence std::vector so that it can be read.
	std::vector <Packet> GetEvidence();

	//Clears the evidence std::vector
	// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
	int ClearEvidence();

	// Calculates the feature set for this suspect
	int CalculateFeatures();

	// Stores the Suspect information into the buffer, retrieved using deserializeSuspect
	//		buf - Pointer to buffer where serialized data will be stored
	// Returns: number of bytes set in the buffer
	uint32_t SerializeSuspect(u_char * buf);

	// Stores the Suspect and FeatureSet information into the buffer, retrieved using deserializeSuspectWithData
	//		buf - Pointer to buffer where serialized data will be stored
	// Returns: number of bytes set in the buffer
	uint32_t SerializeSuspectWithData(u_char * buf);

	// Reads Suspect information from a buffer originally populated by serializeSuspect
	//		buf - Pointer to buffer where the serialized suspect is
	// Returns: number of bytes read from the buffer
	uint32_t DeserializeSuspect(u_char * buf);

	// Reads Suspect information from a buffer originally populated by serializeSuspect
	// expects featureSet data appended by serializeFeatureData after serializeSuspect
	//		buf - Pointer to buffer where serialized data resides
	//		isLocal - Specifies whether this data is from a Silent Alarm (false) or local packets (true)
	// Returns: number of bytes read from the buffer
	uint32_t DeserializeSuspectWithData(u_char * buf, bool isLocal);


	//Returns a copy of the suspects in_addr, must not be locked or is locked by the owner
	//Returns: Suspect's in_addr_t or NULL on failure
	in_addr_t GetIpAddress();
	//Sets the suspects in_addr_t, must have the lock to perform this operation
	//Returns: 0 on success
	// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
	int SetIpAddress(in_addr_t ip);

	//Returns a copy of the suspects in_addr, must not be locked or is locked by the owner
	//Returns: Suspect's in_addr or NULL on failure
	in_addr GetInAddr();
	//Sets the suspects in_addr, must have the lock to perform this operation
	//Returns: 0 on success
	// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
	int SetInAddr(in_addr in);


	//Returns a copy of the Suspects classification double, must not be locked or is locked by the owner
	// Returns -1 on failure
	double GetClassification();
	//Sets the suspect's classification, must have the lock to perform this operation
	//Returns 0 on success
	// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
	int SetClassification(double n);


	//Returns the number of hostile neighbors, must not be locked or is locked by the owner
	int GetHostileNeighbors();
	//Sets the number of hostile neighbors, must have the lock to perform this operation
	// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
	int SetHostileNeighbors(int i);


	//Returns the hostility bool of the suspect, must not be locked or is locked by the owner
	bool GetIsHostile();
	//Sets the hostility bool of the suspect, must have the lock to perform this operation
	// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
	int SetIsHostile(bool b);


	//Returns the needs classification bool, must not be locked or is locked by the owner
	bool GetNeedsClassificationUpdate();
	//Sets the needs classification bool, must have the lock to perform this operation
	// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
	int SetNeedsClassificationUpdate(bool b);


	//Returns the flagged by silent alarm bool, must not be locked or is locked by the owner
	bool GetFlaggedByAlarm();
	//Sets the flagged by silent alarm bool, must have the lock to perform this operation
	// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
	int SetFlaggedByAlarm(bool b);


	//Returns the 'from live capture' bool, must not be locked or is locked by the owner
	bool GetIsLive();
	//Sets the 'from live capture' bool, must have the lock to perform this operation
	// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
	int SetIsLive(bool b);


	//Returns a copy of the suspects FeatureSet, must not be locked or is locked by the owner
	FeatureSet GetFeatureSet();
	FeatureSet GetUnsentFeatureSet();
	void UpdateFeatureData(bool include);
	//Sets or overwrites the suspects FeatureSet, must have the lock to perform this operation
	// Returns (0) on Success and (-1) if the suspect is currently owned by another thread
	// Note: If you wish to block until the Suspect can be set, release any other locked resources and
	// using the blocking SetOwner function to wait on the suspects lock.
	// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
	int SetFeatureSet(FeatureSet *fs);
	int SetUnsentFeatureSet(FeatureSet *fs);


	//Adds the feature set 'fs' to the suspect's feature set
	// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
	int AddFeatureSet(FeatureSet *fs);
	//Subtracts the feature set 'fs' from the suspect's feature set
	// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
	int SubtractFeatureSet(FeatureSet *fs);

	//Clears the feature set of the suspect
	// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
	int ClearFeatureSet();

	//Clears the unsent feature set of the suspect
	// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
	int ClearUnsentData();

	//Returns the accuracy double of the feature using featureIndex fi, must not be locked or is locked by the owner
	double GetFeatureAccuracy(featureIndex fi);
	//Sets the accuracy double of the feature using featureIndex fi, must have the lock to perform this operation
	// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
	int SetFeatureAccuracy(featureIndex fi, double d);

	// must not be locked or is locked by the owner
	ANNpoint GetAnnPoint();
	//Sets the suspect's ANNpoint, must have the lock to perform this operation
	// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
	int SetAnnPoint(ANNpoint a);

	//Deallocates the suspect's ANNpoint and sets it to NULL, must have the lock to perform this operation
	// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
	int ClearAnnPoint();

	//Returns the pthread_t owner, returns NULL if suspect is not checked out
	pthread_t GetOwner();

	//Returns true if the suspect is checked out by a thread
	bool HasOwner();

	//Sets the pthread_t 'owner' to the calling thread
	void SetOwner();

	//Flags the suspect as no longer 'checked out'
	int ResetOwner();

	//Unlocks the suspect if the thread is the owner but preserves ownership flags and values.
	void UnlockAsOwner();

	Suspect& operator=(const Suspect &rhs);
	Suspect(const Suspect &rhs);
	Suspect& operator*(Suspect* rhs);

	// Equality operator, mainly used for test cases
	bool operator==(const Suspect &rhs) const;
	bool operator!=(const Suspect &rhs) const;

private:

	// The Feature Set for this Suspect
	FeatureSet m_features;
	FeatureSet m_unsentFeatures;

	// The feature set in the format that ANN requires.
	ANNpoint m_annPoint;

	// A listing of all the events (evidence) that originated from this suspect
	std::vector <Packet> m_evidence;

	double m_featureAccuracy[DIM];

	// The IP address of the suspect. This field serves as a unique identifier for the Suspect
	struct in_addr m_IpAddress;

	// The current classification assigned to this suspect.
	//		0-1, where 0 is almost surely benign, and 1 is almost surely hostile.
	//		-1 indicates no classification or error.
	double m_classification;
	int32_t m_hostileNeighbors;

	// Is the classification above the current threshold? IE: What conclusion has the CE come to?
	bool m_isHostile;

	// Does the classification need updating?
	//		IE: Has the evidence changed since last it was calculated?
	bool m_needsClassificationUpdate;

	// Has this suspect been the subject of an alarm from another Nova instance?
	bool m_flaggedByAlarm;

	// Is this a live capture or is NOVA reading from a pcap file?
	bool m_isLive;

	//Lock used to maintain concurrency between threads
	pthread_rwlock_t m_lock;

	pthread_t m_owner;
	bool m_hasOwner;

// Make these public when running unit tests
#ifdef GTEST_INCLUDE_GTEST_GTEST_H_
public:
#endif

	//Write locks the suspect
	void WrlockSuspect();

	//Read Locks the suspect
	void RdlockSuspect();

	//Unlocks the suspect
	void UnlockSuspect();

};

}

#endif /* SUSPECT_H_ */
