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

using namespace std;

namespace Nova{


Suspect::Suspect()
{
	IP_address.s_addr = 0;
	classification = -1;
	needs_classification_update = true;
	needs_feature_update = true;
	flaggedByAlarm = false;
	isHostile = false;
	isLive = false;
	features = FeatureSet();
	annPoint = NULL;
	evidence.clear();

	for(int i = 0; i < DIM; i++)
		featureAccuracy[i] = 0;
}


Suspect::~Suspect()
{
	if(annPoint != NULL)
	{
		annDeallocPt(annPoint);
	}
}


Suspect::Suspect(Packet packet)
{
	IP_address = packet.ip_hdr.ip_src;
	classification = -1;
	isHostile = false;
	needs_classification_update = true;
	needs_feature_update = true;
	isLive = true;
	features = FeatureSet();
	annPoint = NULL;
	flaggedByAlarm = false;
	AddEvidence(packet);

	for(int i = 0; i < DIM; i++)
		featureAccuracy[i] = 0;
}


string Suspect::ToString(bool featureEnabled[])
{
	stringstream ss;
	ss << "whyyy?" << endl;
	if(&IP_address != NULL)
	{
		ss << "Suspect: "<< inet_ntoa(IP_address) << "\n\n";
	}
	else
	{
		ss << "Suspect: Null IP\n\n";
	}


	if (featureEnabled[DISTINCT_IPS])
	{
		ss << " Distinct IPs Contacted: " << features.features[DISTINCT_IPS] << "\n";
		ss << "  Average Distance to Neighbors: " << featureAccuracy[DISTINCT_IPS] << "\n\n";
	}

	if (featureEnabled[IP_TRAFFIC_DISTRIBUTION])
	{
		ss << " Haystack Traffic Distribution: " << features.features[IP_TRAFFIC_DISTRIBUTION] << "\n";
		ss << "  Average Distance to Neighbors: " << featureAccuracy[IP_TRAFFIC_DISTRIBUTION] << "\n\n";
	}

	if (featureEnabled[DISTINCT_PORTS])
	{
		ss << " Distinct Ports Contacted: " << features.features[DISTINCT_PORTS] << "\n";
		ss << "  Average Distance to Neighbors: " << featureAccuracy[DISTINCT_PORTS] << "\n\n";
	}

	if (featureEnabled[PORT_TRAFFIC_DISTRIBUTION])
	{
		ss << " Port Traffic Distribution: "  <<  features.features[PORT_TRAFFIC_DISTRIBUTION]  <<  "\n";
		ss << "  Average Distance to Neighbors: " << featureAccuracy[PORT_TRAFFIC_DISTRIBUTION] << "\n\n";
	}

	if (featureEnabled[HAYSTACK_EVENT_FREQUENCY])
	{
		ss << " Haystack Events: " << features.features[HAYSTACK_EVENT_FREQUENCY] <<  " per second\n";
		ss << "  Average Distance to Neighbors: " << featureAccuracy[HAYSTACK_EVENT_FREQUENCY] << "\n\n";
	}

	if (featureEnabled[PACKET_SIZE_MEAN])
	{
		ss << " Mean Packet Size: " << features.features[PACKET_SIZE_MEAN] << "\n";
		ss << "  Average Distance to Neighbors: " << featureAccuracy[PACKET_SIZE_MEAN] << "\n\n";
	}

	if (featureEnabled[PACKET_SIZE_DEVIATION])
	{
		ss << " Packet Size Variance: " << features.features[PACKET_SIZE_DEVIATION] << "\n";
		ss << "  Average Distance to Neighbors: " << featureAccuracy[PACKET_SIZE_DEVIATION] << "\n\n";
	}

	if (featureEnabled[PACKET_INTERVAL_MEAN])
	{
		ss << " Mean Packet Interval: " << features.features[PACKET_INTERVAL_MEAN] << "\n";
		ss << "  Average Distance to Neighbors: " << featureAccuracy[PACKET_INTERVAL_MEAN] << "\n\n";
	}

	if (featureEnabled[PACKET_INTERVAL_DEVIATION])
	{
		ss << " Packet Interval Variance: " << features.features[PACKET_INTERVAL_DEVIATION] << "\n";
		ss << "  Average Distance to Neighbors: " << featureAccuracy[PACKET_INTERVAL_DEVIATION] << "\n\n";
	}

	ss << " Suspect is ";
	if(!isHostile)
		ss << "not ";
	ss << "hostile\n";
	ss <<  " Classification: " <<  classification <<  "\n";

	return ss.str();
}


void Suspect::AddEvidence(Packet packet)
{
	evidence.push_back(packet);
	needs_feature_update = true;
}


void Suspect::CalculateFeatures(uint32_t featuresEnabled)
{
	features.CalculateAll(featuresEnabled);
}


uint32_t Suspect::SerializeSuspect(u_char * buf)
{
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

	//Copies the value and increases the offset
	for(uint32_t i = 0; i < DIM; i++)
	{
		memcpy(buf+offset, &featureAccuracy[i], sizeof featureAccuracy[i]);
		offset+= sizeof featureAccuracy[i];
	}

	//Stores the FeatureSet information into the buffer, retrieved using deserializeFeatureSet
	//	returns the number of bytes set in the buffer
	offset += features.SerializeFeatureSet(buf+offset);

	return offset;
}


uint32_t Suspect::DeserializeSuspect(u_char * buf)
{
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

	//Copies the value and increases the offset
	for(uint32_t i = 0; i < DIM; i++)
	{
		memcpy(&featureAccuracy[i], buf+offset, sizeof featureAccuracy[i]);
		offset+= sizeof featureAccuracy[i];
	}

	//Reads FeatureSet information from a buffer originally populated by serializeFeatureSet
	//	returns the number of bytes read from the buffer
	offset += features.DeserializeFeatureSet(buf+offset);

	return offset;
}


uint32_t Suspect::DeserializeSuspectWithData(u_char * buf, bool isLocal)
{
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
		offset += features.DeserializeFeatureDataLocal(buf+offset);
	else
		offset += features.DeserializeFeatureDataBroadcast(buf+offset);

	needs_feature_update = true;
	needs_classification_update = true;

	return offset;
}

}
