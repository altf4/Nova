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
#include <sstream>

using namespace std;

namespace Nova{


Suspect::Suspect()
{
	pthread_rwlock_init(&m_lock, NULL);
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
	m_features = FeatureSet();
	m_features.m_unsentData = new FeatureSet();
	m_annPoint = NULL;
	m_evidence.clear();

	for(int i = 0; i < DIM; i++)
		m_featureAccuracy[i] = 0;
}


Suspect::~Suspect()
{
	//Lock the suspect before deletion to prevent further access
	WrlockSuspect();
	if(m_annPoint != NULL)
	{
		annDeallocPt(m_annPoint);
	}
}


Suspect::Suspect(Packet packet)
{
	pthread_rwlock_init(&m_lock, NULL);
	m_IpAddress = packet.ip_hdr.ip_src;
	m_hostileNeighbors = 0;
	m_classification = -1;
	m_isHostile = false;
	m_needsClassificationUpdate = true;
	m_needsFeatureUpdate = true;
	m_isLive = true;
	m_features = FeatureSet();
	m_features.m_unsentData = new FeatureSet();
	m_annPoint = NULL;
	m_flaggedByAlarm = false;
	AddEvidence(packet);

	for(int i = 0; i < DIM; i++)
		m_featureAccuracy[i] = 0;
}


string Suspect::ToString(bool featureEnabled[])
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


	if (featureEnabled[DISTINCT_IPS])
	{
		ss << " Distinct IPs Contacted: " << m_features.m_features[DISTINCT_IPS] << "\n";
	}

	if (featureEnabled[IP_TRAFFIC_DISTRIBUTION])
	{
		ss << " Haystack Traffic Distribution: " << m_features.m_features[IP_TRAFFIC_DISTRIBUTION] << "\n";
	}

	if (featureEnabled[DISTINCT_PORTS])
	{
		ss << " Distinct Ports Contacted: " << m_features.m_features[DISTINCT_PORTS] << "\n";
	}

	if (featureEnabled[PORT_TRAFFIC_DISTRIBUTION])
	{
		ss << " Port Traffic Distribution: "  <<  m_features.m_features[PORT_TRAFFIC_DISTRIBUTION]  <<  "\n";
	}

	if (featureEnabled[HAYSTACK_EVENT_FREQUENCY])
	{
		ss << " Haystack Events: " << m_features.m_features[HAYSTACK_EVENT_FREQUENCY] <<  " per second\n";
	}

	if (featureEnabled[PACKET_SIZE_MEAN])
	{
		ss << " Mean Packet Size: " << m_features.m_features[PACKET_SIZE_MEAN] << "\n";
	}

	if (featureEnabled[PACKET_SIZE_DEVIATION])
	{
		ss << " Packet Size Variance: " << m_features.m_features[PACKET_SIZE_DEVIATION] << "\n";
	}

	if (featureEnabled[PACKET_INTERVAL_MEAN])
	{
		ss << " Mean Packet Interval: " << m_features.m_features[PACKET_INTERVAL_MEAN] << "\n";
	}

	if (featureEnabled[PACKET_INTERVAL_DEVIATION])
	{
		ss << " Packet Interval Variance: " << m_features.m_features[PACKET_INTERVAL_DEVIATION] << "\n";
	}

	UnlockSuspect();
	return ss.str();
}


void Suspect::AddEvidence(Packet packet)
{
	WrlockSuspect();
	m_evidence.push_back(packet);
	m_needsFeatureUpdate = true;
	UnlockSuspect();
}

//Returns a copy of the evidence vector so that it can be read.
vector <Packet> Suspect::GetEvidence() //TODO
{
	RdlockSuspect();
	vector<Packet>ret = m_evidence;
	UnlockSuspect();
	return ret;
}

//Clears the evidence vector, returns 0 on success
void Suspect::ClearEvidence() //TODO
{
		WrlockSuspect();
		m_evidence.clear();
		UnlockSuspect();
}

void Suspect::CalculateFeatures(uint32_t featuresEnabled)
{
	WrlockSuspect();
	m_features.CalculateAll(featuresEnabled);
	UnlockSuspect();
}


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
		offset += m_features.m_unsentData->DeserializeFeatureData(buf+offset);
	else
		offset += m_features.DeserializeFeatureData(buf+offset);

	m_needsFeatureUpdate = true;
	m_needsClassificationUpdate = true;
	UnlockSuspect();


	return offset;
}

//Returns a copy of the suspects in_addr.s_addr, must not be locked or is locked by the owner
//Returns: Suspect's in_addr.s_addr or 0 on failure
in_addr_t Suspect::GetIpAddress() //TODO
{
	RdlockSuspect();
	in_addr_t ret = m_IpAddress.s_addr;
	UnlockSuspect();
	return ret;

}
//Sets the suspects in_addr, must have the lock to perform this operation
//Returns: 0 on success
void Suspect::SetIpAddress(in_addr_t ip) //TODO
{
	WrlockSuspect();
	m_IpAddress.s_addr = ip;
	UnlockSuspect();

}


//Returns a copy of the Suspects classification double, must not be locked or is locked by the owner
// Returns -1 on failure
double Suspect::GetClassification() //TODO
{
	RdlockSuspect();
	double ret = m_classification;
	UnlockSuspect();
	return ret;

}

//Sets the suspect's classification, must have the lock to perform this operation
//Returns 0 on success
void Suspect::SetClassification(double n) //TODO
{
	WrlockSuspect();
	m_classification = n;
	UnlockSuspect();

}


//Returns the number of hostile neighbors, must not be locked or is locked by the owner
int Suspect::GetHostileNeighbors() //TODO
{
	RdlockSuspect();
	int ret = m_hostileNeighbors;
	UnlockSuspect();
	return ret;

}
//Sets the number of hostile neighbors, must have the lock to perform this operation
void Suspect::SetHostileNeighbors(int i) //TODO
{
	WrlockSuspect();
	m_hostileNeighbors = i;
	UnlockSuspect();
}


//Returns the hostility bool of the suspect, must not be locked or is locked by the owner
bool Suspect::GetIsHostile() //TODO
{
	RdlockSuspect();
	return m_isHostile;
	UnlockSuspect();
}
//Sets the hostility bool of the suspect, must have the lock to perform this operation
void Suspect::SetIsHostile(bool b) //TODO
{
	WrlockSuspect();
	m_isHostile = b;
	UnlockSuspect();
}


//Returns the needs classification bool, must not be locked or is locked by the owner
bool Suspect::GetNeedsClassificationUpdate() //TODO
{
	RdlockSuspect();
	return m_needsClassificationUpdate;
	UnlockSuspect();
}
//Sets the needs classification bool, must have the lock to perform this operation
void Suspect::SetNeedsClassificationUpdate(bool b) //TODO
{
	WrlockSuspect();
	m_needsClassificationUpdate = b;
	UnlockSuspect();
}


//Returns the needs feature update bool, must not be locked or is locked by the owner
bool Suspect::GetNeedsFeatureUpdate() //TODO
{
	RdlockSuspect();
	return m_needsFeatureUpdate;
	UnlockSuspect();
}
//Sets the neeeds feature update bool, must have the lock to perform this operation
void Suspect::SetNeedsFeatureUpdate(bool b) //TODO
{
	WrlockSuspect();
	m_needsFeatureUpdate = b;
	UnlockSuspect();
}


//Returns the flagged by silent alarm bool, must not be locked or is locked by the owner
bool Suspect::GetFlaggedByAlarm() //TODO
{
	RdlockSuspect();
	return m_flaggedByAlarm;
	UnlockSuspect();
}
//Sets the flagged by silent alarm bool, must have the lock to perform this operation
void Suspect::SetFlaggedByAlarm(bool b) //TODO
{
	WrlockSuspect();
	m_flaggedByAlarm = b;
	UnlockSuspect();
}


//Returns the 'from live capture' bool, must not be locked or is locked by the owner
bool Suspect::GetIsLive() //TODO
{
	RdlockSuspect();
	return m_isLive;
	UnlockSuspect();
}
//Sets the 'from live capture' bool, must have the lock to perform this operation
void Suspect::SetIsLive(bool b) //TODO
{
	WrlockSuspect();
	m_isLive = b;
	UnlockSuspect();
}


//Returns a copy of the suspects FeatureSet, must not be locked or is locked by the owner
FeatureSet Suspect::GetFeatures() //TODO
{
	RdlockSuspect();
	FeatureSet ret = m_features;
	UnlockSuspect();
	return ret;
}

//Sets or overwrites the suspects FeatureSet, must have the lock to perform this operation
void Suspect::SetFeatures(FeatureSet fs) //TODO
{
	WrlockSuspect();
	m_features = fs;
	UnlockSuspect();
}

//Adds the feature set 'fs' to the suspect's feature set
void Suspect::AddFeatureSet(FeatureSet fs) //TODO
{
	WrlockSuspect();
	m_features += fs;
	UnlockSuspect();
}
//Subtracts the feature set 'fs' from the suspect's feature set
void Suspect::SubtractFeatureSet(FeatureSet fs) //TODO
{
	WrlockSuspect();
	m_features -= fs;
	UnlockSuspect();

}

//Clears the feature set of the suspect
void Suspect::ClearFeatures() //TODO
{
	WrlockSuspect();
	m_features.ClearFeatureSet();
	UnlockSuspect();

}


//Returns the accuracy double of the feature using featureIndex fi, must not be locked or is locked by the owner
double Suspect::GetFeatureAccuracy(featureIndex fi) //TODO
{
	RdlockSuspect();
	double ret =  m_featureAccuracy[fi];
	UnlockSuspect();
	return ret;
}
//Sets the accuracy double of the feature using featureIndex fi, must have the lock to perform this operation
void Suspect::SetFeatureAccuracy(featureIndex fi, double d) //TODO
{
	WrlockSuspect();
	m_featureAccuracy[fi] = d;
	UnlockSuspect();
}

//Returns a copy of the suspect's ANNpoint, must not be locked or is locked by the owner
ANNpoint Suspect::GetAnnPoint() //TODO
{
	RdlockSuspect();
	ANNpoint ret =  m_annPoint;
	UnlockSuspect();
	return ret;
}

//Write locks the suspect
void Suspect::WrlockSuspect() //TODO
{
	pthread_rwlock_wrlock(&m_lock);
}

//Read locks the suspect
void Suspect::RdlockSuspect() //TODO
{
	pthread_rwlock_rdlock(&m_lock);
}

//Unlocks the suspect
void Suspect::UnlockSuspect() //TODO
{
	pthread_rwlock_unlock(&m_lock);
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
void Suspect::SetOwner(pthread_t tid)
{
	WrlockSuspect();
	m_hasOwner = true;
	m_owner = tid;
	UnlockSuspect();
	RdlockSuspect();
}

//Flags the suspect as no longer 'checked out'
// Returns (-1) if the caller is not the owner, (1) if the Suspect has no owner or (0) on success
int Suspect::UnsetOwner()
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
