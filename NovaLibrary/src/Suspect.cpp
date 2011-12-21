//============================================================================
// Name        : Suspect.cpp
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : A Suspect object with identifying information and traffic
//					features so that each entity can be monitored and classified.
//============================================================================/*

#include "Suspect.h"
#include <arpa/inet.h>
#include <sstream>

using namespace std;
namespace Nova{
namespace ClassificationEngine{

//Blank Constructor
Suspect::Suspect()
{
	IP_address.s_addr = 0;
	classification = -1;
	needs_classification_update = false;
	needs_feature_update = false;
	flaggedByAlarm = false;
	isHostile = false;
	features = FeatureSet();
	annPoint = annAllocPt(DIMENSION);
	evidence.clear();
}

//Destructor. Has to delete the FeatureSet object within.
Suspect::~Suspect()
{
	for (uint i = 0; i < evidence.size(); i++)
	{
		if(evidence[i] != NULL)
		{
			delete evidence[i];
		}
	}
	if(annPoint != NULL)
	{
		annDeallocPt(annPoint);
	}
}

//Constructor from a TrafficEvent
Suspect::Suspect(TrafficEvent *event)
{
	IP_address = event->src_IP;
	classification = -1;
	isHostile = false;
	features = FeatureSet();
	annPoint = NULL;
	flaggedByAlarm = false;
	AddEvidence(event);
}

//Converts suspect into a human readable string and returns it
string Suspect::ToString()
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
	ss << " Distinct IPs Contacted: " << features.features[DISTINCT_IPS] << "\n";
	ss << " Haystack Traffic Distribution: " << features.features[IP_TRAFFIC_DISTRIBUTION] << "\n";
	ss << " Distinct Ports Contacted: " << features.features[DISTINCT_PORTS] << "\n";
	ss << " Port Traffic Distribution: "  <<  features.features[PORT_TRAFFIC_DISTRIBUTION]  <<  "\n";
	ss << " Haystack Events: " << features.features[HAYSTACK_EVENT_FREQUENCY] <<  " per second\n";
	ss << " Mean Packet Size: " << features.features[PACKET_SIZE_MEAN] << "\n";
	ss << " Packet Size Variance: " << features.features[PACKET_SIZE_DEVIATION] << "\n";
	ss << " Mean Packet Interval: " << features.features[PACKET_INTERVAL_MEAN] << "\n";
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

//Add an additional piece of evidence to this suspect
//	Does not take actions like reclassifying or calculating features.
void Suspect::AddEvidence(TrafficEvent *event)
{
	evidence.push_back(event);
	needs_classification_update = true;
	needs_feature_update = true;
}

//Calculates the feature set for this suspect
void Suspect::CalculateFeatures(bool isTraining)
{
	//Clear any existing feature data
	for(uint i = 0; i < evidence.size(); i++)
	{
		features.UpdateEvidence(evidence[i]);
	}
	evidence.clear();
	//For-each piece of evidence
	features.CalculateAll();

	if(isTraining)
	{
		if(features.features[DISTINCT_IPS] > 2)
		{
			classification = 1;
			isHostile = true;
		}
		else
		{
			classification = 0;
			isHostile = false;
		}
	}
}

//Stores the Suspect information into the buffer, retrieved using deserializeSuspect
//	returns the number of bytes set in the buffer
uint Suspect::serializeSuspect(u_char * buf)
{
	uint offset = 0;
	uint bsize = 1; //bools
	uint isize = 4; //s_addr, int etc
	uint dsize = 8; //doubles, annPoints

	//Clears a chunk of the buffer for everything but FeatureSet
	bzero(buf, (isize + dsize*(DIMENSION+1) + 4*bsize));

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

	for(uint i = 0; i < DIMENSION; i++)
	{
		memcpy(buf+offset, &annPoint[i], dsize);
		offset+= dsize;
	}

	//Stores the FeatureSet information into the buffer, retrieved using deserializeFeatureSet
	//	returns the number of bytes set in the buffer
	offset += features.serializeFeatureSet(buf+offset);

	return offset;
}

//Reads Suspect information from a buffer originally populated by serializeSuspect
//	returns the number of bytes read from the buffer
uint Suspect::deserializeSuspect(u_char * buf)
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

	for(uint i = 0; i < DIMENSION; i++)
	{
		memcpy(&annPoint[i], buf+offset, dsize);
		offset+= dsize;
	}

	//Reads FeatureSet information from a buffer originally populated by serializeFeatureSet
	//	returns the number of bytes read from the buffer
	offset += features.deserializeFeatureSet(buf+offset);

	return offset;
}

//Reads Suspect information from a buffer originally populated by serializeSuspect
// returns the number of bytes read from the buffer
uint Suspect::deserializeSuspectWithData(u_char * buf, in_addr_t hostAddr)
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

	for(uint i = 0; i < DIMENSION; i++)
	{
		memcpy(&annPoint[i], buf+offset, dsize);
		offset+= dsize;
	}

	//Reads FeatureSet information from a buffer originally populated by serializeFeatureSet
	//	returns the number of bytes read from the buffer
	offset += features.deserializeFeatureSet(buf+offset);

	offset += features.deserializeFeatureData(buf+offset, hostAddr);


	return offset;
}

//Extracts and returns the IP Address from a serialized suspect located at buf
uint getSerializedAddr(u_char * buf)
{
	uint addr = 0;
	memcpy(&addr, buf, 4);
	return addr;
}

}
}
