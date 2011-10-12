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
}

//Calculates the distinct_ip_count for a new piece of evidence
void FeatureSet::CalculateDistinctIPCount( TrafficEvent *event )
{
	IPTable.insert(event->dst_IP.s_addr);
	features[DISTINCT_IP_COUNT] = IPTable.size();
}

//Calculates the distinct_port_count for a new piece of evidence
void FeatureSet::CalculateDistinctPortCount( TrafficEvent *event )
{
	portTable.insert(event->dst_port);
	features[DISTINCT_PORT_COUNT] = portTable.size();
}

//Side effect warning!! Run this function before CalculateHaystackEventFrequency.
void FeatureSet::CalculateHaystackToHostEventRatio(TrafficEvent *event)
{
	features[HAYSTACK_EVENT_TO_HOST_EVENT_RATIO] = (double)haystackEvents / (double)(hostEvents+1);
	//HostEvents +1 to handle infinity case, be aware data will be skewed accordingly
}

//Side effect warning!! Run this function after CalculateHaystackToHostEventRatio.
void FeatureSet::CalculateHaystackEventFrequency(TrafficEvent *event)
{
	//Accumulate to find the lowest Start time and biggest end time.
	if( event->start_timestamp < startTime)
	{
		startTime = event->start_timestamp;
	}
	if( event->end_timestamp > endTime)
	{
		endTime = event->end_timestamp;
	}

	if(endTime - startTime > 0)
	{
		features[HAYSTACK_EVENT_FREQUENCY] = ((double)haystackEvents) / (endTime - startTime);
	}
}

//Calculates Packet Size Mean for a new piece of evidence
void FeatureSet::CalculatePacketSizeMean(TrafficEvent *event)
{
	features[PACKET_SIZE_MEAN] = (double)bytesTotal / (double)packetCount;
}

//Calculates Packet Size Variance for a new piece of evidence
void FeatureSet::CalculatePacketSizeVariance(TrafficEvent *event)
{
	double count = this->packetSizes.size();
	double mean = 0;
	double variance = 0;
	//Calculate mean
	double sum = 0;
	for(uint i = 0; i < count; i++)
	{
		sum += this->packetSizes[i];
	}
	mean = sum / count;
	//Calculate variance
	for(uint i = 0; i < count; i++)
	{
		// (x - mean)^2
		variance += pow(((double)this->packetSizes[i] - mean), 2);
	}
	variance = variance / count;
	features[PACKET_SIZE_VARIANCE] = variance;
}

///Increments all the member variables that might be used in different
///methods of the FeatureSet class, this must be called first before
///any other feature calculations are performed.
void FeatureSet::UpdateMemberVariables(TrafficEvent *event)
{
	if(event->from_haystack)
	{
		haystackEvents++;
	}
	else
	{
		hostEvents++;
	}

	packetCount += event->packet_count;
	bytesTotal += event->IP_total_data_bytes;
}
}
}
