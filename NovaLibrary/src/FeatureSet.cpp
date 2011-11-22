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
	packTable.clear();
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

///Clears out the current values, and also any temp variables used to calculate them
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
///Calculates distinct IPs contacted
void FeatureSet::CalculateDistinctIPs()
{
	features[DISTINCT_IPS] = IPTable.size();
}
///Calculates distinct ports contacted
void FeatureSet::CalculateDistinctPorts()
{
	features[DISTINCT_PORTS] = portTable.size();
}

///Calculates the ip traffic distribution for the suspect
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
}

///Calculates the port traffic distribution for the suspect
void FeatureSet::CalculatePortTrafficDistribution()
{
	features[PORT_TRAFFIC_DISTRIBUTION] = 0;
	for (Port_Table::iterator it = portTable.begin() ; it != portTable.end(); it++)
	{
		features[PORT_TRAFFIC_DISTRIBUTION] += ((double)it->second / (double)portMax);
	}
	features[PORT_TRAFFIC_DISTRIBUTION] = features[PORT_TRAFFIC_DISTRIBUTION] / (double)portTable.size();
}

void FeatureSet::CalculateHaystackEventFrequency()
{
	if(endTime - startTime > 0)
	{
		features[HAYSTACK_EVENT_FREQUENCY] = ((double)haystackEvents) / (endTime - startTime);
	}
}


void FeatureSet::CalculatePacketIntervalMean()
{
	features[PACKET_INTERVAL_MEAN] = ((double)(endTime - startTime) / (double)(packetCount));
}

void FeatureSet::CalculatePacketIntervalDeviation()
{
	//since we have n-1 intervals between n packets
	double count = packetCount-1;
	double mean = 0;
	double variance = 0;
	double interval = 0;
	CalculatePacketIntervalMean();
	mean = features[PACKET_INTERVAL_MEAN];

	for(uint i = 0; i < count-1; i++)
	{
		interval = this->packet_intervals[i+1]-this->packet_intervals[i];
		// (x - mean)^2
		variance += (pow((interval - mean), 2))/count;
	}
	features[PACKET_INTERVAL_DEVIATION] = sqrt(variance);
}

///Calculates Packet Size Mean for a suspect
void FeatureSet::CalculatePacketSizeMean()
{
	features[PACKET_SIZE_MEAN] = (double)bytesTotal / (double)packetCount;
}

///Calculates Packet Size Variance for a suspect
void FeatureSet::CalculatePacketSizeDeviation()
{
	double count = packetCount;
	double mean = 0;
	double variance = 0;
	//Calculate mean
	CalculatePacketSizeMean();
	mean = features[PACKET_SIZE_MEAN];
	//Calculate variance
	for(uint i = 0; i < count; i++)
	{
		// (x - mean)^2
		variance += (packTable[this->packetSizes[i]]*pow(((double)this->packetSizes[i] - mean), 2))/count;
	}
	features[PACKET_SIZE_DEVIATION] = sqrt(variance);
}

void FeatureSet::UpdateEvidence(TrafficEvent *event)
{
	packetCount += event->packet_count;
	bytesTotal += event->IP_total_data_bytes;
	//If from haystack
	if( event->from_haystack)
	{
		//Put the packet count into a bin associated with the haystack
		IPTable[ event->dst_IP.s_addr] += event->packet_count;
	}
	//Else from a host
	else
	{
		//Put the packet count into a bin associated with source so that
		// all host events for a suspect go into the same bin
		IPTable[ event->src_IP.s_addr] +=  event->packet_count;
	}

	portTable[ event->dst_port] +=  event->packet_count;
	//Checks for the max to avoid iterating through the entire table every update
	//Since number of ports can grow very large during a scan this will distribute the computation more evenly
	//Since the IP will tend to be relatively small compared to number of events, it's max is found during the call.
	if(portTable[ event->dst_port] > portMax)
	{
		portMax = portTable[ event->dst_port];
	}

	for(uint i = 0; i < event->IP_packet_sizes.size(); i++)
	{
		packTable[event->IP_packet_sizes[i]]++;
		packetSizes.push_back(event->IP_packet_sizes[i]);
		packet_intervals.push_back(event->packet_intervals[i]);
	}
	//Accumulate to find the lowest Start time and biggest end time.
	if( event->from_haystack)
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
	if(  event->end_timestamp > endTime)
	{
		endTime = event->end_timestamp;
	}
}

//Stores the FeatureSet information into the buffer, retrieved using deserializeFeatureSet
//	returns the number of bytes set in the buffer
uint FeatureSet::serializeFeatureSet(u_char * buf)
{
	uint offset = 0;
	uint size = 8; //All features are doubles.

	//Clears a chunk of the buffer for the FeatureSet
	bzero(buf, size*DIMENSION);

	//Copies the value and increases the offset
	for(uint i = 0; i < DIMENSION; i++)
	{
		memcpy(buf+offset, &features[i], size);
		offset+= size;
	}

	return offset;
}

//Reads FeatureSet information from a buffer originally populated by serializeFeatureSet
//	returns the number of bytes read from the buffer
uint FeatureSet::deserializeFeatureSet(u_char * buf)
{
	uint offset = 0;
	uint size = 8;

	//Copies the value and increases the offset
	for(uint i = 0; i < DIMENSION; i++)
	{
		memcpy(&features[i], buf+offset, size);
		offset+= size;
	}

	return offset;
}

}
}
