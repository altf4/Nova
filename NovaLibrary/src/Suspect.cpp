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
	pthread_rwlock_init(&lock, NULL);
	this->IP_address.s_addr = 0;
	this->hostileNeighbors = 0;
	this->classification = -1;
	this->needs_classification_update = true;
	this->needs_feature_update = true;
	this->flaggedByAlarm = false;
	this->isHostile = false;
	this->isLive = false;
	this->features = FeatureSet();
	this->features.unsentData = new FeatureSet();
	this->annPoint = NULL;
	this->evidence.clear();

	for(int i = 0; i < DIM; i++)
		this->featureAccuracy[i] = 0;
}


Suspect::~Suspect()
{
	//Lock the suspect before deletion to prevent further access
	pthread_rwlock_wrlock(&lock);
	if(this->annPoint != NULL)
	{
		annDeallocPt(this->annPoint);
	}
}


Suspect::Suspect(Packet packet)
{
	pthread_rwlock_init(&lock, NULL);
	IP_address = packet.ip_hdr.ip_src;
	hostileNeighbors = 0;
	classification = -1;
	isHostile = false;
	needs_classification_update = true;
	needs_feature_update = true;
	isLive = true;
	features = FeatureSet();
	features.unsentData = new FeatureSet();
	annPoint = NULL;
	flaggedByAlarm = false;
	AddEvidence(packet);

	for(int i = 0; i < DIM; i++)
		featureAccuracy[i] = 0;
}


string Suspect::ToString(bool featureEnabled[])
{
	pthread_rwlock_rdlock(&lock);
	stringstream ss;
	if(&IP_address != NULL)
	{
		ss << "Suspect: "<< inet_ntoa(IP_address) << "\n";
	}
	else
	{
		ss << "Suspect: Null IP\n\n";
	}

	ss << " Suspect is ";
	if(!isHostile)
		ss << "not ";
	ss << "hostile\n";
	ss <<  " Classification: " <<  classification <<  "\n";
	ss <<  " Hostile neighbors: " << hostileNeighbors << "\n";


	if (featureEnabled[DISTINCT_IPS])
	{
		ss << " Distinct IPs Contacted: " << features.features[DISTINCT_IPS] << "\n";
	}

	if (featureEnabled[IP_TRAFFIC_DISTRIBUTION])
	{
		ss << " Haystack Traffic Distribution: " << features.features[IP_TRAFFIC_DISTRIBUTION] << "\n";
	}

	if (featureEnabled[DISTINCT_PORTS])
	{
		ss << " Distinct Ports Contacted: " << features.features[DISTINCT_PORTS] << "\n";
	}

	if (featureEnabled[PORT_TRAFFIC_DISTRIBUTION])
	{
		ss << " Port Traffic Distribution: "  <<  features.features[PORT_TRAFFIC_DISTRIBUTION]  <<  "\n";
	}

	if (featureEnabled[HAYSTACK_EVENT_FREQUENCY])
	{
		ss << " Haystack Events: " << features.features[HAYSTACK_EVENT_FREQUENCY] <<  " per second\n";
	}

	if (featureEnabled[PACKET_SIZE_MEAN])
	{
		ss << " Mean Packet Size: " << features.features[PACKET_SIZE_MEAN] << "\n";
	}

	if (featureEnabled[PACKET_SIZE_DEVIATION])
	{
		ss << " Packet Size Variance: " << features.features[PACKET_SIZE_DEVIATION] << "\n";
	}

	if (featureEnabled[PACKET_INTERVAL_MEAN])
	{
		ss << " Mean Packet Interval: " << features.features[PACKET_INTERVAL_MEAN] << "\n";
	}

	if (featureEnabled[PACKET_INTERVAL_DEVIATION])
	{
		ss << " Packet Interval Variance: " << features.features[PACKET_INTERVAL_DEVIATION] << "\n";
	}

	pthread_rwlock_unlock(&lock);
	return ss.str();
}


void Suspect::AddEvidence(Packet packet)
{
	pthread_rwlock_wrlock(&lock);
	evidence.push_back(packet);
	needs_feature_update = true;
	pthread_rwlock_unlock(&lock);
}

//Returns a copy of the evidence vector so that it can be read.
vector <Packet> Suspect::get_evidence() //TODO
{
	pthread_rwlock_rdlock(&lock);
	vector<Packet>ret = evidence;
	pthread_rwlock_unlock(&lock);
	return ret;
}

//Clears the evidence vector, returns 0 on success
void Suspect::clear_evidence() //TODO
{
		pthread_rwlock_wrlock(&lock);
		evidence.clear();
		pthread_rwlock_unlock(&lock);
}

void Suspect::CalculateFeatures(uint32_t featuresEnabled)
{
	pthread_rwlock_wrlock(&lock);
	features.CalculateAll(featuresEnabled);
	pthread_rwlock_unlock(&lock);
}


uint32_t Suspect::SerializeSuspect(u_char * buf)
{
	pthread_rwlock_rdlock(&lock);
	uint32_t offset = 0;

	//Copies the value and increases the offset
	memcpy(buf, &IP_address.s_addr, sizeof IP_address.s_addr);
	offset+= sizeof IP_address.s_addr;
	memcpy(buf+offset, &classification, sizeof classification);
	offset+= sizeof classification;
	memcpy(buf+offset, &isHostile, sizeof isHostile);
	offset+= sizeof isHostile;
	memcpy(buf+offset, &needs_classification_update, sizeof needs_classification_update);
	offset+= sizeof needs_classification_update;
	memcpy(buf+offset, &needs_feature_update, sizeof needs_feature_update);
	offset+= sizeof needs_feature_update;
	memcpy(buf+offset, &flaggedByAlarm, sizeof flaggedByAlarm);
	offset+= sizeof flaggedByAlarm;
	memcpy(buf+offset, &isLive, sizeof isLive);
	offset+= sizeof isLive;
	memcpy(buf+offset, &hostileNeighbors, sizeof hostileNeighbors);
	offset+= sizeof hostileNeighbors;

	//Copies the value and increases the offset
	for(uint32_t i = 0; i < DIM; i++)
	{
		memcpy(buf+offset, &featureAccuracy[i], sizeof featureAccuracy[i]);
		offset+= sizeof featureAccuracy[i];
	}

	//Stores the FeatureSet information into the buffer, retrieved using deserializeFeatureSet
	//	returns the number of bytes set in the buffer
	offset += features.SerializeFeatureSet(buf+offset);
	pthread_rwlock_unlock(&lock);

	return offset;
}


uint32_t Suspect::DeserializeSuspect(u_char * buf)
{
	pthread_rwlock_wrlock(&lock);
	uint32_t offset = 0;

	//Copies the value and increases the offset
	memcpy(&IP_address.s_addr, buf, sizeof IP_address.s_addr);
	offset+= sizeof IP_address.s_addr;
	memcpy(&classification, buf+offset, sizeof classification);
	offset+= sizeof classification;
	memcpy(&isHostile, buf+offset, sizeof isHostile);
	offset+= sizeof isHostile;
	memcpy(&needs_classification_update, buf+offset, sizeof needs_classification_update);
	offset+= sizeof needs_classification_update;
	memcpy(&needs_feature_update, buf+offset, sizeof needs_feature_update);
	offset+= sizeof needs_feature_update;
	memcpy(&flaggedByAlarm, buf+offset, sizeof flaggedByAlarm);
	offset+= sizeof flaggedByAlarm;
	memcpy(&isLive, buf+offset, sizeof isLive);
	offset+= sizeof isLive;
	memcpy(&hostileNeighbors, buf+offset, sizeof hostileNeighbors);
	offset+= sizeof hostileNeighbors;

	//Copies the value and increases the offset
	for(uint32_t i = 0; i < DIM; i++)
	{
		memcpy(&featureAccuracy[i], buf+offset, sizeof featureAccuracy[i]);
		offset+= sizeof featureAccuracy[i];
	}

	//Reads FeatureSet information from a buffer originally populated by serializeFeatureSet
	//	returns the number of bytes read from the buffer
	offset += features.DeserializeFeatureSet(buf+offset);
	pthread_rwlock_unlock(&lock);


	return offset;
}


uint32_t Suspect::DeserializeSuspectWithData(u_char * buf, bool isLocal)
{
	pthread_rwlock_wrlock(&lock);
	uint32_t offset = 0;

	//Copies the value and increases the offset
	memcpy(&IP_address.s_addr, buf, sizeof IP_address.s_addr);
	offset+= sizeof IP_address.s_addr;
	memcpy(&classification, buf+offset, sizeof classification);
	offset+= sizeof classification;
	memcpy(&isHostile, buf+offset, sizeof isHostile);
	offset+= sizeof isHostile;
	memcpy(&needs_classification_update, buf+offset, sizeof needs_classification_update);
	offset+= sizeof needs_classification_update;
	memcpy(&needs_feature_update, buf+offset, sizeof needs_feature_update);
	offset+= sizeof needs_feature_update;
	memcpy(&flaggedByAlarm, buf+offset, sizeof flaggedByAlarm);
	offset+= sizeof flaggedByAlarm;
	memcpy(&isLive, buf+offset, sizeof isLive);
	offset+= sizeof isLive;
	memcpy(&hostileNeighbors, buf+offset, sizeof hostileNeighbors);
	offset+= sizeof hostileNeighbors;

	//Copies the value and increases the offset
	for(uint32_t i = 0; i < DIM; i++)
	{
		memcpy(&featureAccuracy[i], buf+offset, sizeof featureAccuracy[i]);
		offset+= sizeof featureAccuracy[i];
	}

	//Reads FeatureSet information from a buffer originally populated by serializeFeatureSet
	//	returns the number of bytes read from the buffer
	offset += features.DeserializeFeatureSet(buf+offset);

	if(isLocal)
		offset += features.unsentData->DeserializeFeatureData(buf+offset);
	else
		offset += features.DeserializeFeatureData(buf+offset);

	needs_feature_update = true;
	needs_classification_update = true;
	pthread_rwlock_unlock(&lock);


	return offset;
}

//Returns a copy of the suspects in_addr.s_addr, must not be locked or is locked by the owner
//Returns: Suspect's in_addr.s_addr or 0 on failure
in_addr_t Suspect::get_IP_address() //TODO
{
	pthread_rwlock_rdlock(&lock);
	in_addr_t ret = IP_address.s_addr;
	pthread_rwlock_unlock(&lock);
	return ret;

}
//Sets the suspects in_addr, must have the lock to perform this operation
//Returns: 0 on success
void Suspect::set_IP_Address(in_addr_t ip) //TODO
{
	pthread_rwlock_wrlock(&lock);
	IP_address.s_addr = ip;
	pthread_rwlock_unlock(&lock);

}


//Returns a copy of the Suspects classification double, must not be locked or is locked by the owner
// Returns -1 on failure
double Suspect::get_classification() //TODO
{
	pthread_rwlock_rdlock(&lock);
	double ret = classification;
	pthread_rwlock_unlock(&lock);
	return ret;

}

//Sets the suspect's classification, must have the lock to perform this operation
//Returns 0 on success
void Suspect::set_classification(double n) //TODO
{
	pthread_rwlock_wrlock(&lock);
	classification = n;
	pthread_rwlock_unlock(&lock);

}


//Returns the number of hostile neighbors, must not be locked or is locked by the owner
int Suspect::get_hostileNeighbors() //TODO
{
	pthread_rwlock_rdlock(&lock);
	int ret = hostileNeighbors;
	pthread_rwlock_unlock(&lock);
	return ret;

}
//Sets the number of hostile neighbors, must have the lock to perform this operation
void Suspect::set_hostileNeighbors(int i) //TODO
{
	pthread_rwlock_wrlock(&lock);
	hostileNeighbors = i;
	pthread_rwlock_unlock(&lock);
}


//Returns the hostility bool of the suspect, must not be locked or is locked by the owner
bool Suspect::get_isHostile() //TODO
{
	pthread_rwlock_rdlock(&lock);
	return isHostile;
	pthread_rwlock_unlock(&lock);
}
//Sets the hostility bool of the suspect, must have the lock to perform this operation
void Suspect::set_isHostile(bool b) //TODO
{
	pthread_rwlock_wrlock(&lock);
	isHostile = b;
	pthread_rwlock_unlock(&lock);
}


//Returns the needs classification bool, must not be locked or is locked by the owner
bool Suspect::get_needs_classification_update() //TODO
{
	pthread_rwlock_rdlock(&lock);
	return needs_classification_update;
	pthread_rwlock_unlock(&lock);
}
//Sets the needs classification bool, must have the lock to perform this operation
void Suspect::set_needs_classification_update(bool b) //TODO
{
	pthread_rwlock_wrlock(&lock);
	needs_classification_update = b;
	pthread_rwlock_unlock(&lock);
}


//Returns the needs feature update bool, must not be locked or is locked by the owner
bool Suspect::get_needs_feature_update() //TODO
{
	pthread_rwlock_rdlock(&lock);
	return needs_feature_update;
	pthread_rwlock_unlock(&lock);
}
//Sets the neeeds feature update bool, must have the lock to perform this operation
void Suspect::set_needs_feature_update(bool b) //TODO
{
	pthread_rwlock_wrlock(&lock);
	needs_feature_update = b;
	pthread_rwlock_unlock(&lock);
}


//Returns the flagged by silent alarm bool, must not be locked or is locked by the owner
bool Suspect::get_flaggedByAlarm() //TODO
{
	pthread_rwlock_rdlock(&lock);
	return flaggedByAlarm;
	pthread_rwlock_unlock(&lock);
}
//Sets the flagged by silent alarm bool, must have the lock to perform this operation
void Suspect::set_flaggedByAlarm(bool b) //TODO
{
	pthread_rwlock_wrlock(&lock);
	flaggedByAlarm = b;
	pthread_rwlock_unlock(&lock);
}


//Returns the 'from live capture' bool, must not be locked or is locked by the owner
bool Suspect::get_isLive() //TODO
{
	pthread_rwlock_rdlock(&lock);
	return isLive;
	pthread_rwlock_unlock(&lock);
}
//Sets the 'from live capture' bool, must have the lock to perform this operation
void Suspect::set_isLive(bool b) //TODO
{
	pthread_rwlock_wrlock(&lock);
	isLive = b;
	pthread_rwlock_unlock(&lock);
}


//Returns a copy of the suspects FeatureSet, must not be locked or is locked by the owner
FeatureSet Suspect::get_features() //TODO
{
	pthread_rwlock_rdlock(&lock);
	FeatureSet ret = features;
	pthread_rwlock_unlock(&lock);
	return ret;
}

//Sets or overwrites the suspects FeatureSet, must have the lock to perform this operation
void Suspect::set_features(FeatureSet fs) //TODO
{
	pthread_rwlock_wrlock(&lock);
	features = fs;
	pthread_rwlock_unlock(&lock);
}

//Adds the feature set 'fs' to the suspect's feature set
void Suspect::add_featureSet(FeatureSet fs) //TODO
{
	pthread_rwlock_wrlock(&lock);
	features += fs;
	pthread_rwlock_unlock(&lock);
}
//Subtracts the feature set 'fs' from the suspect's feature set
void Suspect::subtract_featureSet(FeatureSet fs) //TODO
{
	pthread_rwlock_wrlock(&lock);
	features -= fs;
	pthread_rwlock_unlock(&lock);

}

//Clears the feature set of the suspect
void Suspect::clear_features() //TODO
{
	pthread_rwlock_wrlock(&lock);
	features.ClearFeatureSet();
	pthread_rwlock_unlock(&lock);

}


//Returns the accuracy double of the feature using featureIndex fi, must not be locked or is locked by the owner
double Suspect::get_featureAccuracy(featureIndex fi) //TODO
{
	pthread_rwlock_rdlock(&lock);
	double ret =  featureAccuracy[fi];
	pthread_rwlock_unlock(&lock);
	return ret;
}
//Sets the accuracy double of the feature using featureIndex fi, must have the lock to perform this operation
void Suspect::setFeatureAccuracy(featureIndex fi, double d) //TODO
{
	pthread_rwlock_wrlock(&lock);
	featureAccuracy[fi] = d;
	pthread_rwlock_unlock(&lock);
}

//Returns a copy of the suspect's ANNpoint, must not be locked or is locked by the owner
ANNpoint Suspect::get_annPoint() //TODO
{
	pthread_rwlock_rdlock(&lock);
	ANNpoint ret =  annPoint;
	pthread_rwlock_unlock(&lock);
	return ret;
}

//Write locks the suspect
void Suspect::wrlockSuspect() //TODO
{
	pthread_rwlock_wrlock(&lock);
}

//Read locks the suspect
void Suspect::rdlockSuspect() //TODO
{
	pthread_rwlock_rdlock(&lock);
}

//Unlocks the suspect
void Suspect::unlockSuspect() //TODO
{
	pthread_rwlock_unlock(&lock);
}

}
