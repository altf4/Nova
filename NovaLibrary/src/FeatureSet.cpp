//============================================================================
// Name        : FeatureSet.cpp
// Author      : DataSoft Corporation
// Copyright   :
// Description : A set of features for use in the NOVA utility
//============================================================================/*

#include "FeatureSet.h"
#include <math.h>

using namespace std;
namespace Nova{
namespace ClassificationEngine{

//Empty constructor
FeatureSet::FeatureSet()
{

}

//Clears out the current values, and also any temp variables used to calculate them
void FeatureSet::ClearFeatureSet()
{
	//Temp variables
	startTime = 2147483647; //2^31 - 1 (IE: Y2.038K bug) (IE: The largest standard Unix time value)
	endTime = 0;
	IPTable.clear();
	portTable.clear();
	haystackEvents = 0;
	hostEvents = 0;
	packetCount = 0;
	bytesTotal = 0;
	//Features
	for(int i = 0; i < DIMENSION; i++)
	{
		features[i] = 0;
	}
	packetSizes.clear();
}

//Calculates the distinct_ip_count for a new piece of evidence
void FeatureSet::CalculateDistinctIPCount()
{
	features[DISTINCT_IP_COUNT] = IPTable.size();
}

//Calculates the distinct_port_count for a new piece of evidence
void FeatureSet::CalculateDistinctPortCount()
{
	features[DISTINCT_PORT_COUNT] = portTable.size();
}

//Side effect warning!! Run this function before CalculateHaystackEventFrequency.
void FeatureSet::CalculateHaystackToHostEventRatio()
{
	features[HAYSTACK_EVENT_TO_HOST_EVENT_RATIO] = (double)haystackEvents / (double)(hostEvents+1);
	//HostEvents +1 to handle infinity case, be aware data will be skewed accordingly
}

//Side effect warning!! Run this function after CalculateHaystackToHostEventRatio.
void FeatureSet::CalculateHaystackEventFrequency()
{
	if(endTime - startTime > 0)
	{
		features[HAYSTACK_EVENT_FREQUENCY] = ((double)haystackEvents) / (endTime - startTime);
	}
}

//Calculates Packet Size Mean for a new piece of evidence
void FeatureSet::CalculatePacketSizeMean()
{
	features[PACKET_SIZE_MEAN] = (double)bytesTotal / (double)packetCount;
}

//Calculates Packet Size Variance for a new piece of evidence
void FeatureSet::CalculatePacketSizeVariance()
{
	double count = this->packetSizes.size();
	double mean = 0;
	double variance = 0;
	//Calculate mean
	CalculatePacketSizeMean(event);
	mean = features[PACKET_SIZE_MEAN];
	//Calculate variance
	for(uint i = 0; i < count; i++)
	{
		// (x - mean)^2
		variance += pow(((double)this->packetSizes[i] - mean), 2);
	}
	variance = variance / count;
	features[PACKET_SIZE_VARIANCE] = variance;
}

void FeatureSet::UpdateEvidence(TrafficEvent *event)
{
	packetCount += event->packet_count;
	bytesTotal += event->IP_total_data_bytes;
	portTable.insert(event->dst_port);
	IPTable.insert(event->dst_IP.s_addr);
	packetSizes.push_back(event->IP_total_data_bytes);
	//Accumulate to find the lowest Start time and biggest end time.
	if(event->from_haystack)
	{
		haystackEvents++;
	}
	else
	{
		hostEvents++;
	}
	if( event->start_timestamp < startTime)
	{
		startTime = event->start_timestamp;
	}
	if( event->end_timestamp > endTime)
	{
		endTime = event->end_timestamp;
	}
}
}
}
