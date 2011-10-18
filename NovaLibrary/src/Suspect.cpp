//============================================================================
// Name        : Suspect.cpp
// Author      : DataSoft Corporation
// Copyright   :
// Description : Suspect object for use in the NOVA utility
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
	features = NULL;
	annPoint = annAllocPt(DIMENSION);
	evidence.clear();
}

//Destructor. Has to delete the FeatureSet object within.
Suspect::~Suspect()
{
	if(features != NULL)
	{
		delete features;
		features = NULL;
	}
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
	this->IP_address = event->src_IP;
	this->classification = -1;
	this->features = new FeatureSet();
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
	if((features != NULL) && (features->features != NULL))
	{
		ss << " Distinct IP's contacted: " << features->features[DISTINCT_IP_COUNT] << "\n";
		ss << " Distinct Ports's contacted: "  <<  features->features[DISTINCT_PORT_COUNT]  <<  "\n";
		ss <<  " Ratio of Haystack to Host Events: " << features->features[HAYSTACK_EVENT_TO_HOST_EVENT_RATIO]  <<  "\n";
		ss <<  " Haystack Events: " << features->features[HAYSTACK_EVENT_FREQUENCY] <<  " per second\n";
		ss << " Mean Packet Size: " << features->features[PACKET_SIZE_MEAN] << "\n";
	}
	else
	{
		ss << " Null Feature Set\n";
	}
	ss <<  " Classification: " <<  classification <<  "\n";
//	ss << "\tHaystack hits: " << features->haystackEvents << "\n";
//	ss << "\tHost hits: " << features->hostEvents << "\n";
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
	TrafficEvent *event;
	//Clear any existing feature data
	for(uint i = 0; i < this->evidence.size(); i++)
	{
		this->features->UpdateEvidence(this->evidence[i]);
	}
	this->evidence.clear();
	//For-each piece of evidence
	this->features->CalculateDistinctIPCount();
	this->features->CalculateDistinctPortCount();
	this->features->CalculateHaystackToHostEventRatio();
	this->features->CalculateHaystackEventFrequency();
	this->features->CalculatePacketSizeVariance();
	this->needs_feature_update = false;
	if(isTraining)
	{
		if(this->features->features[3] > 2)
		{
			classification = 1;
		}
		else
		{
			classification = 0;
		}
		/*//Calculate classification on the basis of how many Evil Events it has
		uint sum = 0;
		for(uint j = 0; j < evidence.size(); j++)
		{
			if( evidence[j]->isHostile )
			{
				sum++;
			}
		}
		if(sum > ( evidence.size() / 2 ) )
		{
			classification = 1;
		}
		else
		{
			classification = 0;
		}*/

	}
}
}
}
