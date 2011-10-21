//============================================================================
// Name        : FeatureSet.cpp
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Maintains and calculates distinct features for individual Suspects
//					for use in classification of the Suspect.
//============================================================================/*

#include "FeatureSet.h"
#include <math.h>

using namespace std;
namespace Nova{
namespace ClassificationEngine{

//Empty constructor
FeatureSet::FeatureSet()
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
	portMax = 0;
	IPMax = 0;
	//Features
	for(int i = 0; i < DIMENSION; i++)
	{
		features[i] = 0;
	}
	packetSizes.clear();
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
	portMax = 0;
	IPMax = 0;
	//Features
	for(int i = 0; i < DIMENSION; i++)
	{
		features[i] = 0;
	}
	packetSizes.clear();
}

//Calculates the distinct_ip_count for a new piece of evidence
void FeatureSet::CalculateIPTrafficDistribution()
{
	features[IP_TRAFFIC_DISTRIBUTION] = 0;
	IPMax = 0;
	for (IP_Table::iterator it = IPTable.begin() ; it != IPTable.end(); it++)
	{
		if(it->second > IPMax) IPMax = it->second;
	}
	for (IP_Table::iterator it = IPTable.begin() ; it != IPTable.end(); it++)
	{
		features[IP_TRAFFIC_DISTRIBUTION] += ((double)it->second / (double)IPMax);
	}
	features[IP_TRAFFIC_DISTRIBUTION] = features[IP_TRAFFIC_DISTRIBUTION] / (double)IPTable.size();
	if(IPTable.size() < 10)
	{
		features[IP_TRAFFIC_DISTRIBUTION] = features[IP_TRAFFIC_DISTRIBUTION]*IPTable.size();
	}
	else
	{
		features[IP_TRAFFIC_DISTRIBUTION] = features[IP_TRAFFIC_DISTRIBUTION]*10;
	}
}

//Calculates the distinct_port_count for a new piece of evidence
void FeatureSet::CalculatePortTrafficDistribution()
{
	features[PORT_TRAFFIC_DISTRIBUTION] = 0;
	for (Port_Table::iterator it = portTable.begin() ; it != portTable.end(); it++)
	{
		features[PORT_TRAFFIC_DISTRIBUTION] += ((double)it->second / (double)portMax);
	}
	features[PORT_TRAFFIC_DISTRIBUTION] = features[PORT_TRAFFIC_DISTRIBUTION] / (double)portTable.size();
	if(portTable.size() < 10)
	{
		features[PORT_TRAFFIC_DISTRIBUTION] = features[PORT_TRAFFIC_DISTRIBUTION]*portTable.size();
	}
	else
	{
		features[PORT_TRAFFIC_DISTRIBUTION] = features[PORT_TRAFFIC_DISTRIBUTION]*10;
	}
}

//Side effect warning!! Run this function before CalculateHaystackEventFrequency.
/*void FeatureSet::CalculateHaystackToHostEventRatio()
{
	features[HAYSTACK_EVENT_TO_HOST_EVENT_RATIO] = (double)haystackEvents / (double)(hostEvents+1);
	//HostEvents +1 to handle infinity case, be aware data will be skewed accordingly
}*/

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
	//double count = this->packetSizes.size();
	//double mean = 0;
	//double variance = 0;
	//Calculate mean
	CalculatePacketSizeMean();
	//mean = features[PACKET_SIZE_MEAN];
	//Calculate variance
	/*for(uint i = 0; i < count; i++)
	{
		// (x - mean)^2
		variance += pow(((double)this->packetSizes[i] - mean), 2);
	}
	variance = variance / count;
	features[PACKET_SIZE_VARIANCE] = variance;*/
	features[PACKET_SIZE_VARIANCE] = 0;
}

void FeatureSet::UpdateEvidence(TrafficEvent *event)
{
	packetCount += event->packet_count;
	bytesTotal += event->IP_total_data_bytes;
	//If from haystack
	if(event->from_haystack)
	{
		//Put the packet count into a bin associated with the haystack
		IPTable[event->dst_IP.s_addr] += event->packet_count;
	}
	//Else from a host
	else
	{
		//Put the packet count into a bin associated with source so that
		// all host events for a suspect go into the same bin
		IPTable[event->src_IP.s_addr] += event->packet_count;
	}

	portTable[event->dst_port] += event->packet_count;
	//Checks for the max to avoid iterating through the entire table every update
	//Since number of ports can grow very large during a scan this will distribute the computation more evenly
	//Since the IP will tend to be relatively small compared to number of events, it's max is found during the call.
	if(portTable[event->dst_port] > portMax)
	{
		portMax = portTable[event->dst_port];
	}

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
