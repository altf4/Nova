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

#include <errno.h>
#include <sstream>

using namespace std;

namespace Nova{


Suspect::Suspect()
{
	m_IpAddress.s_addr = 0;
	m_hostileNeighbors = 0;
	m_classification = -1;
	m_needsClassificationUpdate = true;
	m_flaggedByAlarm = false;
	m_isHostile = false;
	m_isLive = false;
	for(int i = 0; i < DIM; i++)
	{
		m_featureAccuracy[i] = 0;
	}
}


Suspect::~Suspect()
{
}


Suspect::Suspect(Evidence *&evidence)
{
	m_IpAddress.s_addr = htonl(evidence->m_evidencePacket.ip_src);
	m_hostileNeighbors = 0;
	m_classification = -1;
	m_isHostile = false;
	ConsumeEvidence(evidence);
	m_flaggedByAlarm = false;
	for(int i = 0; i < DIM; i++)
	{
		m_featureAccuracy[i] = 0;
	}
}

string Suspect::GetIpString()
{
	return string(inet_ntoa(m_IpAddress));
}

string Suspect::ToString()
{
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


	if (Config::Inst()->IsFeatureEnabled(DISTINCT_IPS))
	{
		ss << " Distinct IPs Contacted: " << m_features.m_features[DISTINCT_IPS] << "\n";
	}

	if (Config::Inst()->IsFeatureEnabled(IP_TRAFFIC_DISTRIBUTION))
	{
		ss << " Haystack Traffic Distribution: " << m_features.m_features[IP_TRAFFIC_DISTRIBUTION] << "\n";
	}

	if (Config::Inst()->IsFeatureEnabled(DISTINCT_PORTS))
	{
		ss << " Distinct Ports Contacted: " << m_features.m_features[DISTINCT_PORTS] << "\n";
	}

	if (Config::Inst()->IsFeatureEnabled(PORT_TRAFFIC_DISTRIBUTION))
	{
		ss << " Port Traffic Distribution: "  <<  m_features.m_features[PORT_TRAFFIC_DISTRIBUTION]  <<  "\n";
	}

	if (Config::Inst()->IsFeatureEnabled(HAYSTACK_EVENT_FREQUENCY))
	{
		ss << " Haystack Events: " << m_features.m_features[HAYSTACK_EVENT_FREQUENCY] <<  " per second\n";
	}

	if (Config::Inst()->IsFeatureEnabled(PACKET_SIZE_MEAN))
	{
		ss << " Mean Packet Size: " << m_features.m_features[PACKET_SIZE_MEAN] << "\n";
	}

	if (Config::Inst()->IsFeatureEnabled(PACKET_SIZE_DEVIATION))
	{
		ss << " Packet Size Variance: " << m_features.m_features[PACKET_SIZE_DEVIATION] << "\n";
	}

	if (Config::Inst()->IsFeatureEnabled(PACKET_INTERVAL_MEAN))
	{
		ss << " Mean Packet Interval: " << m_features.m_features[PACKET_INTERVAL_MEAN] << "\n";
	}

	if (Config::Inst()->IsFeatureEnabled(PACKET_INTERVAL_DEVIATION))
	{
		ss << " Packet Interval Variance: " << m_features.m_features[PACKET_INTERVAL_DEVIATION] << "\n";
	}

	return ss.str();
}

void Suspect::ConsumeEvidence(Evidence *&evidence)
{
	Evidence *curEvidence = evidence, *tempEv = NULL;
	while(curEvidence != NULL)
	{
		m_unsentFeatures.UpdateEvidence(curEvidence);
		tempEv = curEvidence;
		curEvidence = tempEv->m_next;
		delete tempEv;
	}
	m_needsClassificationUpdate = true;
	m_isLive = (Config::Inst()->GetReadPcap());
}

//Calculates the suspect's features based on it's feature set
void Suspect::CalculateFeatures()
{
	UpdateFeatureData(INCLUDE);
	m_features.CalculateAll();
	UpdateFeatureData(REMOVE);
}

// Stores the Suspect information into the buffer, retrieved using deserializeSuspect
//		buf - Pointer to buffer where serialized data will be stored
// Returns: number of bytes set in the buffer
uint32_t Suspect::Serialize(u_char *buf, SerializeFeatureMode whichFeatures)
{
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
	switch(whichFeatures)
	{
		case UNSENT_FEATURE_DATA:
		{
			if(offset + m_unsentFeatures.GetFeatureDataLength() >= SANITY_CHECK)
			{
				return 0;
			}
			offset += m_unsentFeatures.SerializeFeatureData(buf + offset);
			break;
		}
		case MAIN_FEATURE_DATA:
		{
			if(offset + m_features.GetFeatureDataLength() >= SANITY_CHECK)
			{
				return 0;
			}
			offset += m_features.SerializeFeatureData(buf + offset);
			break;
		}
		case ALL_FEATURE_DATA:
		{
			if(offset + m_features.GetFeatureDataLength() >= SANITY_CHECK)
			{
				return 0;
			}
			offset += m_features.SerializeFeatureData(buf + offset);
			if(offset + m_unsentFeatures.GetFeatureDataLength() >= SANITY_CHECK)
			{
				return 0;
			}
			offset += m_unsentFeatures.SerializeFeatureData(buf + offset);
			break;
		}
		case NO_FEATURE_DATA:
		default:
		{
			break;
		}
	}
	return offset;
}

uint32_t Suspect::GetSerializeLength(SerializeFeatureMode whichFeatures)
{
	//Adds the sizeof results for the static required fields to messageSize
	uint32_t messageSize =
		sizeof(m_IpAddress.s_addr)
		+ sizeof(m_classification)
		+ sizeof(m_isHostile)
		+ sizeof(m_needsClassificationUpdate)
		+ sizeof(m_flaggedByAlarm)
		+ sizeof(m_isLive)
		+ sizeof(m_hostileNeighbors);
	//Adds the dynamic elements to the messageSize
	for(uint32_t i = 0; i < DIM; i++)
	{
		messageSize += sizeof(m_featureAccuracy[i]) + sizeof(m_features.m_features[i]);
	}
	switch(whichFeatures)
	{
		case UNSENT_FEATURE_DATA:
		{
			messageSize += m_unsentFeatures.GetFeatureDataLength();
			break;
		}
		case MAIN_FEATURE_DATA:
		{
			messageSize += m_features.GetFeatureDataLength();
			break;
		}
		case ALL_FEATURE_DATA:
		{
			messageSize += m_features.GetFeatureDataLength();
			messageSize += m_unsentFeatures.GetFeatureDataLength();
			break;
		}
		case NO_FEATURE_DATA:
		default:
		{
			break;
		}
	}
	return messageSize;
}

// Reads Suspect information from a buffer originally populated by serializeSuspect
//		buf - Pointer to buffer where the serialized suspect is
// Returns: number of bytes read from the buffer
uint32_t Suspect::Deserialize(u_char *buf, SerializeFeatureMode whichFeatures)
{
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
	switch(whichFeatures)
	{
		case UNSENT_FEATURE_DATA:
		{
			offset += m_unsentFeatures.DeserializeFeatureData(buf+offset);
			break;
		}
		case MAIN_FEATURE_DATA:
		{
			offset += m_features.DeserializeFeatureData(buf+offset);
			break;
		}
		case ALL_FEATURE_DATA:
		{
			offset += m_features.DeserializeFeatureData(buf+offset);
			offset += m_unsentFeatures.DeserializeFeatureData(buf+offset);
			break;
		}
		case NO_FEATURE_DATA:
		default:
		{
			break;
		}
	}
	return offset;
}

//Returns a copy of the suspects in_addr.s_addr
//Returns: Suspect's in_addr.s_addr
in_addr_t Suspect::GetIpAddress()
{
	return m_IpAddress.s_addr;
}

//Sets the suspects in_addr
void Suspect::SetIpAddress(in_addr_t ip)
{
	m_IpAddress.s_addr = ip;
}

//Returns a copy of the suspects in_addr.s_addr
//Returns: Suspect's in_addr
in_addr Suspect::GetInAddr()
{
	return m_IpAddress;
}

//Sets the suspects in_addr
void Suspect::SetInAddr(in_addr in)
{
	m_IpAddress = in;
}

//Returns a copy of the Suspects classification double
// Returns -1 on failure
double Suspect::GetClassification()
{
	return m_classification;
}

//Sets the suspect's classification
void Suspect::SetClassification(double n)
{
	m_classification = n;
}


//Returns the number of hostile neighbors
int Suspect::GetHostileNeighbors()
{
	return m_hostileNeighbors;
}
//Sets the number of hostile neighbors
void Suspect::SetHostileNeighbors(int i)
{
	m_hostileNeighbors = i;
}


//Returns the hostility bool of the suspect
bool Suspect::GetIsHostile()
{
	return m_isHostile;
}
//Sets the hostility bool of the suspect
void Suspect::SetIsHostile(bool b)
{
	m_isHostile = b;
}


//Returns the needs classification bool
bool Suspect::GetNeedsClassificationUpdate()
{
	return m_needsClassificationUpdate;
}
//Sets the needs classification bool
void Suspect::SetNeedsClassificationUpdate(bool b)
{
	m_needsClassificationUpdate = b;
}

//Returns the flagged by silent alarm bool
bool Suspect::GetFlaggedByAlarm()
{
	return m_flaggedByAlarm;
}
//Sets the flagged by silent alarm bool
void Suspect::SetFlaggedByAlarm(bool b)
{
	m_flaggedByAlarm = b;
}


//Returns the 'from live capture' bool
bool Suspect::GetIsLive()
{
	return m_isLive;
}
//Sets the 'from live capture' bool
void Suspect::SetIsLive(bool b)
{
	m_isLive = b;
}

void Suspect::UpdateFeatureData(bool include)
{
	if(include)
	{
		m_features += m_unsentFeatures;
	}
	else
	{
		m_features -= m_unsentFeatures;
	}
}

//Clears the FeatureData of a suspect
// whichFeatures: specifies which FeatureSet's Data to clear
void Suspect::ClearFeatureData(FeatureMode whichFeatures)
{
	switch(whichFeatures)
	{
		default:
		case MAIN_FEATURES:
		{
			m_features.ClearFeatureData();
			break;
		}
		case UNSENT_FEATURES:
		{
			m_unsentFeatures.ClearFeatureData();
			break;
		}
	}
}

//Returns a copy of the suspects FeatureSet
FeatureSet Suspect::GetFeatureSet(FeatureMode whichFeatures)
{
	switch(whichFeatures)
	{
		default:
		case MAIN_FEATURES:
		{
			return m_features;
			break;
		}
		case UNSENT_FEATURES:
		{
			return m_unsentFeatures;
			break;
		}
	}
}

//Sets or overwrites the suspects FeatureSet
void Suspect::SetFeatureSet(FeatureSet *fs, FeatureMode whichFeatures)
{
	switch(whichFeatures)
	{
		default:
		case MAIN_FEATURES:
		{
			m_features.operator =(*fs);
			break;
		}
		case UNSENT_FEATURES:
		{
			m_unsentFeatures.operator =(*fs);
			break;
		}
	}

}

//Adds the feature set 'fs' to the suspect's feature set
void Suspect::AddFeatureSet(FeatureSet *fs, FeatureMode whichFeatures)
{
	switch(whichFeatures)
	{
		default:
		case MAIN_FEATURES:
		{
			m_features += *fs;
			break;
		}
		case UNSENT_FEATURES:
		{
			m_unsentFeatures += *fs;
			break;
		}
	}

}
//Subtracts the feature set 'fs' from the suspect's feature set
void Suspect::SubtractFeatureSet(FeatureSet *fs, FeatureMode whichFeatures)
{
	switch(whichFeatures)
	{
		default:
		case MAIN_FEATURES:
		{
			m_features -= *fs;
			break;
		}
		case UNSENT_FEATURES:
		{
			m_unsentFeatures -= *fs;
			break;
		}
	}
}

//Clears the feature set of the suspect
// whichFeatures: specifies which FeatureSet to clear
void Suspect::ClearFeatureSet(FeatureMode whichFeatures)
{
	switch(whichFeatures)
	{
		case MAIN_FEATURES:
		{
			m_features.ClearFeatureSet();
			break;
		}
		case UNSENT_FEATURES:
		{
			m_unsentFeatures.ClearFeatureSet();
			break;
		}
		default:
		{
			break;
		}
	}
}

//Returns the accuracy double of the feature using featureIndex 'fi'
double Suspect::GetFeatureAccuracy(featureIndex fi)
{
	return m_featureAccuracy[fi];
}
//Sets the accuracy double of the feature using featureIndex 'fi'
void Suspect::SetFeatureAccuracy(featureIndex fi, double d)
{
	m_featureAccuracy[fi] = d;
}

Suspect& Suspect::operator=(const Suspect &rhs)
{
	m_features = rhs.m_features;
	m_unsentFeatures = rhs.m_unsentFeatures;
	for(uint i = 0; i < DIM; i++)
	{
		m_featureAccuracy[i] = rhs.m_featureAccuracy[i];
	}

	m_IpAddress = rhs.m_IpAddress;
	m_classification = rhs.m_classification;
	m_hostileNeighbors = rhs.m_hostileNeighbors;
	m_isHostile = rhs.m_isHostile;
	m_needsClassificationUpdate = rhs.m_needsClassificationUpdate;
	m_flaggedByAlarm = rhs.m_flaggedByAlarm;
	m_isLive = rhs.m_isLive;
	return *this;
}

bool Suspect::operator==(const Suspect &rhs) const
{
	if (m_features != rhs.m_features)
	{
		return false;
	}
	if (m_IpAddress.s_addr != rhs.m_IpAddress.s_addr)
	{
		return false;
	}

	if (m_classification != rhs.m_classification)
	{
		return false;
	}

	if (m_hostileNeighbors != rhs.m_hostileNeighbors)
	{
		return false;
	}

	for (int i = 0; i < DIM; i++)
	{
		if (m_featureAccuracy[i] != rhs.m_featureAccuracy[i])
		{
			return false;
		}
	}

	if (m_isHostile != rhs.m_isHostile)
	{
		return false;
	}

	if (m_flaggedByAlarm != rhs.m_flaggedByAlarm)
	{
		return false;
	}
	return true;
}

bool Suspect::operator !=(const Suspect &rhs) const
{
	return !(*this == rhs);
}

Suspect::Suspect(const Suspect &rhs)
{
	m_features = rhs.m_features;
	m_unsentFeatures = rhs.m_unsentFeatures;

	for(uint i = 0; i < DIM; i++)
	{
		m_featureAccuracy[i] = rhs.m_featureAccuracy[i];
	}

	m_IpAddress = rhs.m_IpAddress;
	m_classification = rhs.m_classification;
	m_hostileNeighbors = rhs.m_hostileNeighbors;
	m_isHostile = rhs.m_isHostile;
	m_needsClassificationUpdate = rhs.m_needsClassificationUpdate;
	m_flaggedByAlarm = rhs.m_flaggedByAlarm;
	m_isLive = rhs.m_isLive;
}

}
