//============================================================================
// Name        : Suspect.cpp
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

#include "Suspect.h"
#include "Config.h"
#include "FeatureSet.h"

#include <sstream>

using namespace std;

namespace Nova{


Suspect::Suspect()
{
	pthread_rwlock_init(&m_lock, NULL);
	pthread_rwlock_wrlock(&m_lock);
	m_owner = 0;
	m_hasOwner = false;
	m_IpAddress.s_addr = 0;
	m_hostileNeighbors = 0;
	m_classification = -1;
	m_needsClassificationUpdate = true;
	m_needsFeatureUpdate = true;
	m_flaggedByAlarm = false;
	m_isHostile = false;
	m_isLive = false;
	m_annPoint = NULL;
	m_evidence.clear();

	for(int i = 0; i < DIM; i++)
		m_featureAccuracy[i] = 0;
	pthread_rwlock_unlock(&m_lock);
}


Suspect::~Suspect()
{
	WrlockSuspect();
	if(m_annPoint != NULL)
	{
		annDeallocPt(m_annPoint);
	}
	pthread_rwlock_destroy(&m_lock);
}


Suspect::Suspect(Packet packet)
{
	pthread_rwlock_init(&m_lock, NULL);
	pthread_rwlock_wrlock(&m_lock);
	m_owner = 0;
	m_hasOwner = false;
	m_IpAddress = packet.ip_hdr.ip_src;
	m_hostileNeighbors = 0;
	m_classification = -1;
	m_isHostile = false;
	m_needsClassificationUpdate = true;
	m_needsFeatureUpdate = true;
	m_isLive = true;
	m_annPoint = NULL;
	m_flaggedByAlarm = false;

	for(int i = 0; i < DIM; i++)
		m_featureAccuracy[i] = 0;
	pthread_rwlock_unlock(&m_lock);
	AddEvidence(packet);
}


string Suspect::ToString()
{
	RdlockSuspect();
	stringstream ss;
	if(&m_IpAddress != NULL)
	{
		ss << "Suspect: "<< inet_ntoa(m_IpAddress) << "\n";
	}
	else
	{
		ss << "Suspect: Null IP\n\n";
	}

	ss << " Suspect is ";
	if(!m_isHostile)
		ss << "not ";
	ss << "hostile\n";
	ss <<  " Classification: " <<  m_classification <<  "\n";
	ss <<  " Hostile neighbors: " << m_hostileNeighbors << "\n";


	if (Config::Inst()->isFeatureEnabled(DISTINCT_IPS))
	{
		ss << " Distinct IPs Contacted: " << m_features.m_features[DISTINCT_IPS] << "\n";
	}

	if (Config::Inst()->isFeatureEnabled(IP_TRAFFIC_DISTRIBUTION))
	{
		ss << " Haystack Traffic Distribution: " << m_features.m_features[IP_TRAFFIC_DISTRIBUTION] << "\n";
	}

	if (Config::Inst()->isFeatureEnabled(DISTINCT_PORTS))
	{
		ss << " Distinct Ports Contacted: " << m_features.m_features[DISTINCT_PORTS] << "\n";
	}

	if (Config::Inst()->isFeatureEnabled(PORT_TRAFFIC_DISTRIBUTION))
	{
		ss << " Port Traffic Distribution: "  <<  m_features.m_features[PORT_TRAFFIC_DISTRIBUTION]  <<  "\n";
	}

	if (Config::Inst()->isFeatureEnabled(HAYSTACK_EVENT_FREQUENCY))
	{
		ss << " Haystack Events: " << m_features.m_features[HAYSTACK_EVENT_FREQUENCY] <<  " per second\n";
	}

	if (Config::Inst()->isFeatureEnabled(PACKET_SIZE_MEAN))
	{
		ss << " Mean Packet Size: " << m_features.m_features[PACKET_SIZE_MEAN] << "\n";
	}

	if (Config::Inst()->isFeatureEnabled(PACKET_SIZE_DEVIATION))
	{
		ss << " Packet Size Variance: " << m_features.m_features[PACKET_SIZE_DEVIATION] << "\n";
	}

	if (Config::Inst()->isFeatureEnabled(PACKET_INTERVAL_MEAN))
	{
		ss << " Mean Packet Interval: " << m_features.m_features[PACKET_INTERVAL_MEAN] << "\n";
	}

	if (Config::Inst()->isFeatureEnabled(PACKET_INTERVAL_DEVIATION))
	{
		ss << " Packet Interval Variance: " << m_features.m_features[PACKET_INTERVAL_DEVIATION] << "\n";
	}

	UnlockSuspect();
	return ss.str();
}

//	Add an additional piece of evidence to this suspect
//		packet - Packet headers to extract evidence from
// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
// Note: Does not take actions like reclassifying or calculating features.
int Suspect::AddEvidence(Packet packet)
{
	if(m_hasOwner && !pthread_equal(m_owner, pthread_self()))
		return -1;
	WrlockSuspect();
	m_evidence.push_back(packet);
	m_needsFeatureUpdate = true;
	m_needsClassificationUpdate = true;
	UnlockSuspect();
	return 0;
}

// Proccesses all packets in m_evidence and puts them into the suspects unsent FeatureSet data
// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
int Suspect::UpdateEvidence()
{
	if(m_hasOwner && !pthread_equal(m_owner, pthread_self()))
		return -1;
	WrlockSuspect();
	for(uint i = 0; i < m_evidence.size(); i++)
	{
		this->m_unsentFeatures.UpdateEvidence(m_evidence[i]);
	}
	m_evidence.clear();
	UnlockSuspect();
	return 0;
}

//Returns a copy of the evidence vector so that it can be read.
vector <Packet> Suspect::GetEvidence()
{
	RdlockSuspect();
	vector<Packet>ret = m_evidence;
	UnlockSuspect();
	return ret;
}

//Clears the evidence vector
// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
int Suspect::ClearEvidence()
{
	if(m_hasOwner && !pthread_equal(m_owner, pthread_self()))
		return -1;
	WrlockSuspect();
	m_evidence.clear();
	UnlockSuspect();
	return 0;
}

//Calculates the suspect's features based on it's feature set
// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
int Suspect::CalculateFeatures()
{
	if(m_hasOwner && !pthread_equal(m_owner, pthread_self()))
		return -1;
	WrlockSuspect();

	m_features += m_unsentFeatures;
	m_features.CalculateAll();
	m_features -= m_unsentFeatures;
	UnlockSuspect();
	return 0;
}

// Stores the Suspect information into the buffer, retrieved using deserializeSuspect
//		buf - Pointer to buffer where serialized data will be stored
// Returns: number of bytes set in the buffer
uint32_t Suspect::SerializeSuspect(u_char * buf)
{
	RdlockSuspect();
	uint32_t offset = 0;

	//Copies the value and increases the offset
	memcpy(buf, &m_IpAddress.s_addr, sizeof m_IpAddress.s_addr);
	offset+= sizeof m_IpAddress.s_addr;
	memcpy(buf+offset, &m_classification, sizeof m_classification);
	offset+= sizeof m_classification;
	memcpy(buf+offset, &m_isHostile, sizeof m_isHostile);
	offset+= sizeof m_isHostile;
	memcpy(buf+offset, &m_needsClassificationUpdate, sizeof m_needsClassificationUpdate);
	offset+= sizeof m_needsClassificationUpdate;
	memcpy(buf+offset, &m_needsFeatureUpdate, sizeof m_needsFeatureUpdate);
	offset+= sizeof m_needsFeatureUpdate;
	memcpy(buf+offset, &m_flaggedByAlarm, sizeof m_flaggedByAlarm);
	offset+= sizeof m_flaggedByAlarm;
	memcpy(buf+offset, &m_isLive, sizeof m_isLive);
	offset+= sizeof m_isLive;
	memcpy(buf+offset, &m_hostileNeighbors, sizeof m_hostileNeighbors);
	offset+= sizeof m_hostileNeighbors;

	//Copies the value and increases the offset
	for(uint32_t i = 0; i < DIM; i++)
	{
		memcpy(buf+offset, &m_featureAccuracy[i], sizeof m_featureAccuracy[i]);
		offset+= sizeof m_featureAccuracy[i];
	}

	//Stores the FeatureSet information into the buffer, retrieved using deserializeFeatureSet
	//	returns the number of bytes set in the buffer
	offset += m_features.SerializeFeatureSet(buf+offset);
	UnlockSuspect();

	return offset;
}

// Stores the Suspect and FeatureSet information into the buffer, retrieved using deserializeSuspectWithData
//		buf - Pointer to buffer where serialized data will be stored
// Returns: number of bytes set in the buffer
uint32_t Suspect::SerializeSuspectWithData(u_char * buf)
{
	RdlockSuspect();  //Double read lock should be ok
	uint32_t offset = SerializeSuspect(buf);
	offset += m_features.SerializeFeatureData(buf + offset);
	UnlockSuspect();
	return offset;
}

// Reads Suspect information from a buffer originally populated by serializeSuspect
//		buf - Pointer to buffer where the serialized suspect is
// Returns: number of bytes read from the buffer
uint32_t Suspect::DeserializeSuspect(u_char * buf)
{
	WrlockSuspect();
	uint32_t offset = 0;

	//Copies the value and increases the offset
	memcpy(&m_IpAddress.s_addr, buf, sizeof m_IpAddress.s_addr);
	offset+= sizeof m_IpAddress.s_addr;
	memcpy(&m_classification, buf+offset, sizeof m_classification);
	offset+= sizeof m_classification;
	memcpy(&m_isHostile, buf+offset, sizeof m_isHostile);
	offset+= sizeof m_isHostile;
	memcpy(&m_needsClassificationUpdate, buf+offset, sizeof m_needsClassificationUpdate);
	offset+= sizeof m_needsClassificationUpdate;
	memcpy(&m_needsFeatureUpdate, buf+offset, sizeof m_needsFeatureUpdate);
	offset+= sizeof m_needsFeatureUpdate;
	memcpy(&m_flaggedByAlarm, buf+offset, sizeof m_flaggedByAlarm);
	offset+= sizeof m_flaggedByAlarm;
	memcpy(&m_isLive, buf+offset, sizeof m_isLive);
	offset+= sizeof m_isLive;
	memcpy(&m_hostileNeighbors, buf+offset, sizeof m_hostileNeighbors);
	offset+= sizeof m_hostileNeighbors;

	//Copies the value and increases the offset
	for(uint32_t i = 0; i < DIM; i++)
	{
		memcpy(&m_featureAccuracy[i], buf+offset, sizeof m_featureAccuracy[i]);
		offset+= sizeof m_featureAccuracy[i];
	}

	//Reads FeatureSet information from a buffer originally populated by serializeFeatureSet
	//	returns the number of bytes read from the buffer
	offset += m_features.DeserializeFeatureSet(buf+offset);
	UnlockSuspect();


	return offset;
}


uint32_t Suspect::DeserializeSuspectWithData(u_char * buf, bool isLocal)
{
	WrlockSuspect();
	uint32_t offset = 0;

	//Copies the value and increases the offset
	memcpy(&m_IpAddress.s_addr, buf, sizeof m_IpAddress.s_addr);
	offset+= sizeof m_IpAddress.s_addr;
	memcpy(&m_classification, buf+offset, sizeof m_classification);
	offset+= sizeof m_classification;
	memcpy(&m_isHostile, buf+offset, sizeof m_isHostile);
	offset+= sizeof m_isHostile;
	memcpy(&m_needsClassificationUpdate, buf+offset, sizeof m_needsClassificationUpdate);
	offset+= sizeof m_needsClassificationUpdate;
	memcpy(&m_needsFeatureUpdate, buf+offset, sizeof m_needsFeatureUpdate);
	offset+= sizeof m_needsFeatureUpdate;
	memcpy(&m_flaggedByAlarm, buf+offset, sizeof m_flaggedByAlarm);
	offset+= sizeof m_flaggedByAlarm;
	memcpy(&m_isLive, buf+offset, sizeof m_isLive);
	offset+= sizeof m_isLive;
	memcpy(&m_hostileNeighbors, buf+offset, sizeof m_hostileNeighbors);
	offset+= sizeof m_hostileNeighbors;

	//Copies the value and increases the offset
	for(uint32_t i = 0; i < DIM; i++)
	{
		memcpy(&m_featureAccuracy[i], buf+offset, sizeof m_featureAccuracy[i]);
		offset+= sizeof m_featureAccuracy[i];
	}

	//Reads FeatureSet information from a buffer originally populated by serializeFeatureSet
	//	returns the number of bytes read from the buffer
	offset += m_features.DeserializeFeatureSet(buf+offset);

	if(isLocal)
		offset += m_unsentFeatures.DeserializeFeatureData(buf+offset);
	else
		offset += m_features.DeserializeFeatureData(buf+offset);

	m_needsFeatureUpdate = true;
	m_needsClassificationUpdate = true;
	UnlockSuspect();

	return offset;
}

//Returns a copy of the suspects in_addr.s_addr
//Returns: Suspect's in_addr.s_addr
in_addr_t Suspect::GetIpAddress()
{
	RdlockSuspect();
	in_addr_t ret = m_IpAddress.s_addr;
	UnlockSuspect();
	return ret;
}

//Sets the suspects in_addr
// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
int Suspect::SetIpAddress(in_addr_t ip)
{
	if(m_hasOwner && !pthread_equal(m_owner, pthread_self()))
		return -1;
	WrlockSuspect();
	m_IpAddress.s_addr = ip;
	UnlockSuspect();
	return 0;
}

//Returns a copy of the suspects in_addr.s_addr
//Returns: Suspect's in_addr
in_addr Suspect::GetInAddr()
{
	RdlockSuspect();
	in_addr ret = m_IpAddress;
	UnlockSuspect();
	return ret;
}

//Sets the suspects in_addr
// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
int Suspect::SetInAddr(in_addr in)
{
	if(m_hasOwner && !pthread_equal(m_owner, pthread_self()))
		return -1;
	WrlockSuspect();
	m_IpAddress = in;
	UnlockSuspect();
	return 0;
}

//Returns a copy of the Suspects classification double
// Returns -1 on failure
double Suspect::GetClassification()
{
	RdlockSuspect();
	double ret = m_classification;
	UnlockSuspect();
	return ret;
}

//Sets the suspect's classification
//Returns 0 on success
// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
int Suspect::SetClassification(double n)
{
	if(m_hasOwner && !pthread_equal(m_owner, pthread_self()))
		return -1;
	WrlockSuspect();
	m_classification = n;
	UnlockSuspect();
	return 0;
}


//Returns the number of hostile neighbors
int Suspect::GetHostileNeighbors()
{
	RdlockSuspect();
	int ret = m_hostileNeighbors;
	UnlockSuspect();
	return ret;
}
//Sets the number of hostile neighbors
// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
int Suspect::SetHostileNeighbors(int i)
{
	if(m_hasOwner && !pthread_equal(m_owner, pthread_self()))
		return -1;
	WrlockSuspect();
	m_hostileNeighbors = i;
	UnlockSuspect();
	return 0;
}


//Returns the hostility bool of the suspect
bool Suspect::GetIsHostile()
{
	RdlockSuspect();
	bool ret = m_isHostile;
	UnlockSuspect();
	return ret;
}
//Sets the hostility bool of the suspect
// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
int Suspect::SetIsHostile(bool b)
{
	if(m_hasOwner && !pthread_equal(m_owner, pthread_self()))
		return -1;
	WrlockSuspect();
	m_isHostile = b;
	UnlockSuspect();
	return 0;
}


//Returns the needs classification bool
bool Suspect::GetNeedsClassificationUpdate()
{
	RdlockSuspect();
	bool ret = m_needsClassificationUpdate;
	UnlockSuspect();
	return ret;
}
//Sets the needs classification bool
// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
int Suspect::SetNeedsClassificationUpdate(bool b)
{
	WrlockSuspect();
	m_needsClassificationUpdate = b;
	UnlockSuspect();
	return 0;
}


//Returns the needs feature update bool
bool Suspect::GetNeedsFeatureUpdate()
{
	RdlockSuspect();
	bool ret = m_needsFeatureUpdate;
	UnlockSuspect();
	return ret;
}
//Sets the neeeds feature update bool
// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
int Suspect::SetNeedsFeatureUpdate(bool b)
{
	if(m_hasOwner && !pthread_equal(m_owner, pthread_self()))
		return -1;
	WrlockSuspect();
	m_needsFeatureUpdate = b;
	UnlockSuspect();
	return 0;
}


//Returns the flagged by silent alarm bool
bool Suspect::GetFlaggedByAlarm()
{
	RdlockSuspect();
	bool ret = m_flaggedByAlarm;
	UnlockSuspect();
	return ret;
}
//Sets the flagged by silent alarm bool
// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
int Suspect::SetFlaggedByAlarm(bool b)
{
	if(m_hasOwner && !pthread_equal(m_owner, pthread_self()))
		return -1;
	WrlockSuspect();
	m_flaggedByAlarm = b;
	UnlockSuspect();
	return 0;
}


//Returns the 'from live capture' bool
bool Suspect::GetIsLive()
{
	RdlockSuspect();
	bool ret = m_isLive;
	UnlockSuspect();
	return ret;
}
//Sets the 'from live capture' bool
// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
int Suspect::SetIsLive(bool b)
{
	if(m_hasOwner && !pthread_equal(m_owner, pthread_self()))
		return -1;
	WrlockSuspect();
	m_isLive = b;
	UnlockSuspect();
	return 0;
}


//Returns a copy of the suspects FeatureSet
FeatureSet Suspect::GetFeatureSet()
{
	RdlockSuspect();
	FeatureSet ret = m_features;
	UnlockSuspect();
	return ret;
}

//Returns a copy of the suspects FeatureSet
FeatureSet Suspect::GetUnsentFeatureSet()
{
	RdlockSuspect();
	FeatureSet ret = m_unsentFeatures;
	UnlockSuspect();
	return ret;
}

void Suspect::UpdateFeatureData(bool include)
{
	if(include)
		m_features += m_unsentFeatures;
	else
		m_features -= m_unsentFeatures;
}


//Sets or overwrites the suspects FeatureSet
// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
int Suspect::SetFeatureSet(FeatureSet *fs)
{
	if(m_hasOwner && !pthread_equal(m_owner, pthread_self()))
		return -1;
	WrlockSuspect();
	m_features.operator =(*fs);
	UnlockSuspect();
	return 0;
}

//Sets or overwrites the suspects FeatureSet
// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
int Suspect::SetUnsentFeatureSet(FeatureSet *fs)
{
	if(m_hasOwner && !pthread_equal(m_owner, pthread_self()))
		return -1;
	WrlockSuspect();
	m_unsentFeatures.operator =(*fs);
	UnlockSuspect();
	return 0;
}

//Adds the feature set 'fs' to the suspect's feature set
// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
int Suspect::AddFeatureSet(FeatureSet *fs)
{
	if(m_hasOwner && !pthread_equal(m_owner, pthread_self()))
		return -1;
	WrlockSuspect();
	m_features += *fs;
	UnlockSuspect();
	return 0;
}
//Subtracts the feature set 'fs' from the suspect's feature set
// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
int Suspect::SubtractFeatureSet(FeatureSet *fs)
{
	if(m_hasOwner && !pthread_equal(m_owner, pthread_self()))
		return -1;
	WrlockSuspect();
	m_features -= *fs;
	UnlockSuspect();
	return 0;
}

//Clears the feature set of the suspect
// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
int Suspect::ClearFeatureSet()
{
	if(m_hasOwner && !pthread_equal(m_owner, pthread_self()))
		return -1;
	WrlockSuspect();
	m_features.ClearFeatureSet();
	UnlockSuspect();
	return 0;
}

//Clears the unsent feature set of the suspect
// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
int Suspect::ClearUnsentData()
{
	if(m_hasOwner && !pthread_equal(m_owner, pthread_self()))
		return -1;
	WrlockSuspect();
	m_unsentFeatures.ClearFeatureSet();
	UnlockSuspect();
	return 0;
}

//Returns the accuracy double of the feature using featureIndex 'fi'
double Suspect::GetFeatureAccuracy(featureIndex fi)
{
	RdlockSuspect();
	double ret =  m_featureAccuracy[fi];
	UnlockSuspect();
	return ret;
}
//Sets the accuracy double of the feature using featureIndex 'fi'
// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
int Suspect::SetFeatureAccuracy(featureIndex fi, double d)
{
	if(m_hasOwner && !pthread_equal(m_owner, pthread_self()))
		return -1;
	WrlockSuspect();
	m_featureAccuracy[fi] = d;
	UnlockSuspect();
	return 0;
}

//Returns a copy of the suspect's ANNpoint
ANNpoint Suspect::GetAnnPoint()
{
	RdlockSuspect();
	ANNpoint ret =  m_annPoint;
	UnlockSuspect();
	return ret;
}

//Sets the suspect's ANNpoint, must have the lock to perform this operation
// Returns (0) on Success, (-1) if the Suspect is checked out by someone else.
int Suspect::SetAnnPoint(ANNpoint a)
{
	if(m_hasOwner && !pthread_equal(m_owner, pthread_self()))
		return -1;
	WrlockSuspect();
	*m_annPoint = *a;
	UnlockSuspect();
	return 0;
}

//Write locks the suspect
void Suspect::WrlockSuspect()
{
	if(m_hasOwner && pthread_equal(m_owner, pthread_self()))
		pthread_rwlock_unlock(&m_lock);
	pthread_rwlock_wrlock(&m_lock);
}

//Read locks the suspect
void Suspect::RdlockSuspect()
{
	if(m_hasOwner && pthread_equal(m_owner, pthread_self()))
		pthread_rwlock_unlock(&m_lock);
	pthread_rwlock_rdlock(&m_lock);
}

//Unlocks the suspect
void Suspect::UnlockSuspect()
{
	pthread_rwlock_unlock(&m_lock);
	if(m_hasOwner && pthread_equal(m_owner, pthread_self()))
		pthread_rwlock_rdlock(&m_lock);
}

//Returns the pthread_t owner
// Note: The results of this are only applicable if HasOwner == true;
// If HasOwner == false then this value is the tid of the last thread that was the owner or the initialized value of 0
pthread_t Suspect::GetOwner()
{
	RdlockSuspect();
	pthread_t ret = m_owner;
	UnlockSuspect();
	return ret;
}

//Returns true if the suspect is checked out by a thread
bool Suspect::HasOwner()
{
	RdlockSuspect();
	bool ret = m_hasOwner;
	UnlockSuspect();
	return ret;
}

//Sets the pthread_t 'owner'
//		tid: unique thread identifier retrieved from pthread_self();
// Note: This function will block until the owner can be set, use HasOwner()
// if you want to prevent a blocking call.
void Suspect::SetOwner()
{
	WrlockSuspect();
	m_hasOwner = true;
	m_owner = pthread_self();
	UnlockSuspect();
	RdlockSuspect();
}

//Flags the suspect as no longer 'checked out'
// Returns (-1) if the caller is not the owner, (1) if the Suspect has no owner or (0) on success
int Suspect::ResetOwner()
{
	if(m_hasOwner == false)
		return 1;
	if(pthread_equal(m_owner, pthread_self()))
	{
		m_hasOwner = false;
		UnlockSuspect();
		return 0;
	}
	else
		return -1;
}
}
