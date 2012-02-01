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
}


string Suspect::ToString(bool featureEnabled[])
{
	stringstream ss;
	if(&IP_address != NULL)
	{
		ss << "Suspect: "<< inet_ntoa(IP_address) << "\n";
	}
	else
	{
		ss << "Suspect: Null IP\n";
	}

	//if (isLive)
	//	ss << " Suspect Status: Live Capture" << "\n";
	//else
	//	ss << " Suspect Status: Loaded from PCAP" << "\n";

	if (featureEnabled[DISTINCT_IPS])
		ss << " Distinct IPs Contacted: " << features.features[DISTINCT_IPS] << "\n";

	if (featureEnabled[IP_TRAFFIC_DISTRIBUTION])
		ss << " Haystack Traffic Distribution: " << features.features[IP_TRAFFIC_DISTRIBUTION] << "\n";

	if (featureEnabled[DISTINCT_PORTS])
		ss << " Distinct Ports Contacted: " << features.features[DISTINCT_PORTS] << "\n";

	if (featureEnabled[PORT_TRAFFIC_DISTRIBUTION])
		ss << " Port Traffic Distribution: "  <<  features.features[PORT_TRAFFIC_DISTRIBUTION]  <<  "\n";

	if (featureEnabled[HAYSTACK_EVENT_FREQUENCY])
		ss << " Haystack Events: " << features.features[HAYSTACK_EVENT_FREQUENCY] <<  " per second\n";

	if (featureEnabled[PACKET_SIZE_MEAN])
		ss << " Mean Packet Size: " << features.features[PACKET_SIZE_MEAN] << "\n";

	if (featureEnabled[PACKET_SIZE_DEVIATION])
		ss << " Packet Size Variance: " << features.features[PACKET_SIZE_DEVIATION] << "\n";

	if (featureEnabled[PACKET_INTERVAL_MEAN])
		ss << " Mean Packet Interval: " << features.features[PACKET_INTERVAL_MEAN] << "\n";

	if (featureEnabled[PACKET_INTERVAL_DEVIATION])
		ss << " Packet Interval Variance: " << features.features[PACKET_INTERVAL_DEVIATION] << "\n";

	ss << " Suspect is ";
	if(!isHostile)
	{
		ss << "not ";
	}
	ss << "hostile\n";
	ss <<  " Classification: " <<  classification <<  "\n";
//	ss << "\tHaystack hits: " << features.haystackEvents << "\n";
//	ss << "\tHost hits: " << features.hostEvents << "\n";
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


uint Suspect::SerializeSuspect(u_char * buf)
{
	uint offset = 0;
	uint bsize = 1; //bools
	uint isize = 4; //s_addr, int etc
	uint dsize = 8; //doubles

	//Copies the value and increases the offset
	memcpy(buf, &IP_address.s_addr, isize);
	offset+= isize;
	memcpy(buf+offset, &classification, dsize);
	offset+= dsize;
	memcpy(buf+offset, &isHostile, bsize);
	offset+= bsize;
	memcpy(buf+offset, &needs_classification_update, bsize);
	offset+= bsize;
	memcpy(buf+offset, &needs_feature_update, bsize);
	offset+= bsize;
	memcpy(buf+offset, &flaggedByAlarm, bsize);
	offset+= bsize;
	memcpy(buf+offset, &isLive, bsize);
	offset+= bsize;

	//Stores the FeatureSet information into the buffer, retrieved using deserializeFeatureSet
	//	returns the number of bytes set in the buffer
	offset += features.SerializeFeatureSet(buf+offset);

	return offset;
}


uint Suspect::DeserializeSuspect(u_char * buf)
{
	uint offset = 0;
	uint bsize = 1; //bools
	uint isize = 4; //s_addr, int etc
	uint dsize = 8; //doubles

	//Copies the value and increases the offset
	memcpy(&IP_address.s_addr, buf, isize);
	offset+= isize;
	memcpy(&classification, buf+offset, dsize);
	offset+= dsize;
	memcpy(&isHostile, buf+offset, bsize);
	offset+= bsize;
	memcpy(&needs_classification_update, buf+offset, bsize);
	offset+= bsize;
	memcpy(&needs_feature_update, buf+offset, bsize);
	offset+= bsize;
	memcpy(&flaggedByAlarm, buf+offset, bsize);
	offset+= bsize;
	memcpy(&isLive, buf+offset, bsize);
	offset+= bsize;

	//Reads FeatureSet information from a buffer originally populated by serializeFeatureSet
	//	returns the number of bytes read from the buffer
	offset += features.DeserializeFeatureSet(buf+offset);

	return offset;
}


uint Suspect::DeserializeSuspectWithData(u_char * buf, bool isLocal)
{
	uint offset = 0;
	uint bsize = 1; //bools
	uint isize = 4; //s_addr, int etc
	uint dsize = 8; //doubles, annPoints

	//Copies the value and increases the offset
	memcpy(&IP_address.s_addr, buf, isize);
	offset+= isize;
	memcpy(&classification, buf+offset, dsize);
	offset+= dsize;
	memcpy(&isHostile, buf+offset, bsize);
	offset+= bsize;
	memcpy(&needs_classification_update, buf+offset, bsize);
	offset+= bsize;
	memcpy(&needs_feature_update, buf+offset, bsize);
	offset+= bsize;
	memcpy(&flaggedByAlarm, buf+offset, bsize);
	offset+= bsize;
	memcpy(&isLive, buf+offset, bsize);
	offset+= bsize;

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
