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
	this->m_IpAddress.s_addr = 0;
	this->m_hostileNeighbors = 0;
	this->m_classification = -1;
	this->m_needsClassificationUpdate = true;
	this->m_needsFeatureUpdate = true;
	this->m_flaggedByAlarm = false;
	this->m_isHostile = false;
	this->m_isLive = false;
	this->m_features = FeatureSet();
	this->m_features.m_unsentData = new FeatureSet();
	this->m_annPoint = NULL;
	this->m_evidence.clear();

	for(int i = 0; i < DIM; i++)
		this->m_featureAccuracy[i] = 0;
}


Suspect::~Suspect()
{
	//Lock the suspect before deletion to prevent further access
	pthread_rwlock_wrlock(&m_lock);
	if(this->m_annPoint != NULL)
	{
		annDeallocPt(this->m_annPoint);
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
	pthread_rwlock_rdlock(&m_lock);
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

	pthread_rwlock_unlock(&m_lock);
	return ss.str();
}


void Suspect::AddEvidence(Packet packet)
{
	pthread_rwlock_wrlock(&m_lock);
	m_evidence.push_back(packet);
	m_needsFeatureUpdate = true;
	pthread_rwlock_unlock(&m_lock);
}

//Returns a copy of the evidence vector so that it can be read.
vector <Packet> Suspect::GetEvidence() //TODO
{
	pthread_rwlock_rdlock(&m_lock);
	vector<Packet>ret = m_evidence;
	pthread_rwlock_unlock(&m_lock);
	return ret;
}

//Clears the evidence vector, returns 0 on success
void Suspect::ClearEvidence() //TODO
{
		pthread_rwlock_wrlock(&m_lock);
		m_evidence.clear();
		pthread_rwlock_unlock(&m_lock);
}

void Suspect::CalculateFeatures(uint32_t featuresEnabled)
{
	pthread_rwlock_wrlock(&m_lock);
	m_features.CalculateAll(featuresEnabled);
	pthread_rwlock_unlock(&m_lock);
}


uint32_t Suspect::SerializeSuspect(u_char * buf)
{
	pthread_rwlock_rdlock(&m_lock);
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
	pthread_rwlock_unlock(&m_lock);

	return offset;
}


uint32_t Suspect::DeserializeSuspect(u_char * buf)
{
	pthread_rwlock_wrlock(&m_lock);
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
	pthread_rwlock_unlock(&m_lock);


	return offset;
}


uint32_t Suspect::DeserializeSuspectWithData(u_char * buf, bool isLocal)
{
	pthread_rwlock_wrlock(&m_lock);
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
	pthread_rwlock_unlock(&m_lock);


	return offset;
}

//Returns a copy of the suspects in_addr.s_addr, must not be locked or is locked by the owner
//Returns: Suspect's in_addr.s_addr or 0 on failure
in_addr_t Suspect::GetIpAddress() //TODO
{
	pthread_rwlock_rdlock(&m_lock);
	in_addr_t ret = m_IpAddress.s_addr;
	pthread_rwlock_unlock(&m_lock);
	return ret;

}
//Sets the suspects in_addr, must have the lock to perform this operation
//Returns: 0 on success
void Suspect::SetIpAddress(in_addr_t ip) //TODO
{
	pthread_rwlock_wrlock(&m_lock);
	m_IpAddress.s_addr = ip;
	pthread_rwlock_unlock(&m_lock);

}


//Returns a copy of the Suspects classification double, must not be locked or is locked by the owner
// Returns -1 on failure
double Suspect::GetClassification() //TODO
{
	pthread_rwlock_rdlock(&m_lock);
	double ret = m_classification;
	pthread_rwlock_unlock(&m_lock);
	return ret;

}

//Sets the suspect's classification, must have the lock to perform this operation
//Returns 0 on success
void Suspect::SetClassification(double n) //TODO
{
	pthread_rwlock_wrlock(&m_lock);
	m_classification = n;
	pthread_rwlock_unlock(&m_lock);

}


//Returns the number of hostile neighbors, must not be locked or is locked by the owner
int Suspect::GetHostileNeighbors() //TODO
{
	pthread_rwlock_rdlock(&m_lock);
	int ret = m_hostileNeighbors;
	pthread_rwlock_unlock(&m_lock);
	return ret;

}
//Sets the number of hostile neighbors, must have the lock to perform this operation
void Suspect::SetHostileNeighbors(int i) //TODO
{
	pthread_rwlock_wrlock(&m_lock);
	m_hostileNeighbors = i;
	pthread_rwlock_unlock(&m_lock);
}


//Returns the hostility bool of the suspect, must not be locked or is locked by the owner
bool Suspect::GetIsHostile() //TODO
{
	pthread_rwlock_rdlock(&m_lock);
	return m_isHostile;
	pthread_rwlock_unlock(&m_lock);
}
//Sets the hostility bool of the suspect, must have the lock to perform this operation
void Suspect::SetIsHostile(bool b) //TODO
{
	pthread_rwlock_wrlock(&m_lock);
	m_isHostile = b;
	pthread_rwlock_unlock(&m_lock);
}


//Returns the needs classification bool, must not be locked or is locked by the owner
bool Suspect::GetNeedsClassificationUpdate() //TODO
{
	pthread_rwlock_rdlock(&m_lock);
	return m_needsClassificationUpdate;
	pthread_rwlock_unlock(&m_lock);
}
//Sets the needs classification bool, must have the lock to perform this operation
void Suspect::SetNeedsClassificationUpdate(bool b) //TODO
{
	pthread_rwlock_wrlock(&m_lock);
	m_needsClassificationUpdate = b;
	pthread_rwlock_unlock(&m_lock);
}


//Returns the needs feature update bool, must not be locked or is locked by the owner
bool Suspect::GetNeedsFeatureUpdate() //TODO
{
	pthread_rwlock_rdlock(&m_lock);
	return m_needsFeatureUpdate;
	pthread_rwlock_unlock(&m_lock);
}
//Sets the neeeds feature update bool, must have the lock to perform this operation
void Suspect::SetNeedsFeatureUpdate(bool b) //TODO
{
	pthread_rwlock_wrlock(&m_lock);
	m_needsFeatureUpdate = b;
	pthread_rwlock_unlock(&m_lock);
}


//Returns the flagged by silent alarm bool, must not be locked or is locked by the owner
bool Suspect::GetFlaggedByAlarm() //TODO
{
	pthread_rwlock_rdlock(&m_lock);
	return m_flaggedByAlarm;
	pthread_rwlock_unlock(&m_lock);
}
//Sets the flagged by silent alarm bool, must have the lock to perform this operation
void Suspect::SetFlaggedByAlarm(bool b) //TODO
{
	pthread_rwlock_wrlock(&m_lock);
	m_flaggedByAlarm = b;
	pthread_rwlock_unlock(&m_lock);
}


//Returns the 'from live capture' bool, must not be locked or is locked by the owner
bool Suspect::GetIsLive() //TODO
{
	pthread_rwlock_rdlock(&m_lock);
	return m_isLive;
	pthread_rwlock_unlock(&m_lock);
}
//Sets the 'from live capture' bool, must have the lock to perform this operation
void Suspect::SetIsLive(bool b) //TODO
{
	pthread_rwlock_wrlock(&m_lock);
	m_isLive = b;
	pthread_rwlock_unlock(&m_lock);
}


//Returns a copy of the suspects FeatureSet, must not be locked or is locked by the owner
FeatureSet Suspect::GetFeatures() //TODO
{
	pthread_rwlock_rdlock(&m_lock);
	FeatureSet ret = m_features;
	pthread_rwlock_unlock(&m_lock);
	return ret;
}

//Sets or overwrites the suspects FeatureSet, must have the lock to perform this operation
void Suspect::SetFeatures(FeatureSet fs) //TODO
{
	pthread_rwlock_wrlock(&m_lock);
	m_features = fs;
	pthread_rwlock_unlock(&m_lock);
}

//Adds the feature set 'fs' to the suspect's feature set
void Suspect::AddFeatureSet(FeatureSet fs) //TODO
{
	pthread_rwlock_wrlock(&m_lock);
	m_features += fs;
	pthread_rwlock_unlock(&m_lock);
}
//Subtracts the feature set 'fs' from the suspect's feature set
void Suspect::SubtractFeatureSet(FeatureSet fs) //TODO
{
	pthread_rwlock_wrlock(&m_lock);
	m_features -= fs;
	pthread_rwlock_unlock(&m_lock);

}

//Clears the feature set of the suspect
void Suspect::ClearFeatures() //TODO
{
	pthread_rwlock_wrlock(&m_lock);
	m_features.ClearFeatureSet();
	pthread_rwlock_unlock(&m_lock);

}


//Returns the accuracy double of the feature using featureIndex fi, must not be locked or is locked by the owner
double Suspect::GetFeatureAccuracy(featureIndex fi) //TODO
{
	pthread_rwlock_rdlock(&m_lock);
	double ret =  m_featureAccuracy[fi];
	pthread_rwlock_unlock(&m_lock);
	return ret;
}
//Sets the accuracy double of the feature using featureIndex fi, must have the lock to perform this operation
void Suspect::SetFeatureAccuracy(featureIndex fi, double d) //TODO
{
	pthread_rwlock_wrlock(&m_lock);
	m_featureAccuracy[fi] = d;
	pthread_rwlock_unlock(&m_lock);
}

//Returns a copy of the suspect's ANNpoint, must not be locked or is locked by the owner
ANNpoint Suspect::GetAnnPoint() //TODO
{
	pthread_rwlock_rdlock(&m_lock);
	ANNpoint ret =  m_annPoint;
	pthread_rwlock_unlock(&m_lock);
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

}
