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
	classification = 0;
	needs_classification_update = true;
	needs_feature_update = true;
	flaggedByAlarm = false;
	isHostile = false;
	features = FeatureSet();
	annPoint = annAllocPt(DIMENSION);
	evidence.clear();
}

//Destructor. Has to delete the FeatureSet object within.
Suspect::~Suspect()
{
	for ( uint i = 0; i < evidence.size(); i++ )
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
	this->IP_address = event->info.src_IP;
	this->classification = -1;
	this->isHostile = false;
	this->features = FeatureSet();
	this->annPoint = NULL;
	this->flaggedByAlarm = false;
	this->AddEvidence(event);
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
	this->evidence.push_back(event);
	this->needs_classification_update = true;
	this->needs_feature_update = true;
}

//Calculates the feature set for this suspect
void Suspect::CalculateFeatures(bool isTraining)
{
	//Clear any existing feature data
	for(uint i = 0; i < this->evidence.size(); i++)
	{
		this->features.UpdateEvidence(this->evidence[i]);
	}
	this->evidence.clear();
	//For-each piece of evidence
	this->features.CalculateDistinctIPs();
	this->features.CalculateDistinctPorts();
	this->features.CalculateIPTrafficDistribution();
	this->features.CalculatePortTrafficDistribution();
	this->features.CalculateHaystackEventFrequency();
	this->features.CalculatePacketSizeDeviation();
	this->features.CalculatePacketIntervalDeviation();
	if(isTraining)
	{
		if(this->features.features[DISTINCT_IPS] > 2)
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
}
}
