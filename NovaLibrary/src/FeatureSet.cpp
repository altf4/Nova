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
	IPTable.set_empty_key(0);
	portTable.set_empty_key(0);
	packTable.set_empty_key(0);
	SATable.set_empty_key(0);
	IPTable.clear();
	portTable.clear();
	packTable.clear();
	SATable.clear();
	IPTable.resize(INITIAL_IP_SIZE);
	portTable.resize(INITIAL_PORT_SIZE);
	packTable.resize(INITIAL_PACKET_SIZE);
	haystackEvents = 0;
	packetCount = 0;
	bytesTotal = 0;
	portMax = 0;
	IPMax = 0;
	//Features
	for(int i = 0; i < DIMENSION; i++)
	{
		features[i] = 0;
	}
}

///Clears out the current values, and also any temp variables used to calculate them
void FeatureSet::ClearFeatureSet()
{
	//Temp variables
	startTime = 2147483647; //2^31 - 1 (IE: Y2.038K bug) (IE: The largest standard Unix time value)
	endTime = 0;
	IPTable.clear();
	portTable.clear();
	packTable.clear();
	SATable.clear();
	haystackEvents = 0;
	packetCount = 0;
	bytesTotal = 0;
	portMax = 0;
	IPMax = 0;
	//Features
	for(int i = 0; i < DIMENSION; i++)
	{
		features[i] = 0;
	}
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
		features[HAYSTACK_EVENT_FREQUENCY] = ((double)(haystackEvents)) / (endTime - startTime);
	}
}


void FeatureSet::CalculatePacketIntervalMean()
{
	features[PACKET_INTERVAL_MEAN] = ((double)(endTime - startTime) / (double)(packetCount));
}

void FeatureSet::CalculatePacketIntervalDeviation()
{
	double count = packet_intervals.size();
	double mean = 0;
	double variance = 0;
	CalculatePacketIntervalMean();
	mean = features[PACKET_INTERVAL_MEAN];

	for(uint i = 0; i < count; i++)
	{
		variance += (pow((packet_intervals[i] - mean), 2))/count;
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
	//for(uint i = 0; i < count; i++)
	for(Packet_Table::iterator it = packTable.begin() ; it != packTable.end(); it++)
	{
		// (x - mean)^2
		// number of packets multiplied by (packet_size - mean)^2 divided by count
		variance += (it->second*pow(it->first - mean,2))/count;
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
		if(IPTable.count(event->dst_IP.s_addr) == 0)
			IPTable[event->dst_IP.s_addr] = event->packet_count;
		else
		//Put the packet count into a bin associated with the haystack
		IPTable[event->dst_IP.s_addr] += event->packet_count;
	}
	//Else from a host
	else
	{
		//Put the packet count into a bin associated with source so that
		// all host events for a suspect go into the same bin
		if(IPTable.count(event->src_IP.s_addr) == 0)
			IPTable[event->src_IP.s_addr] = event->packet_count;
		else
		IPTable[event->src_IP.s_addr] +=  event->packet_count;
	}

	if(portTable.count(event->dst_port) == 0)
		portTable[event->dst_port] =  event->packet_count;
	else
		portTable[event->dst_port] +=  event->packet_count;
	//Checks for the max to avoid iterating through the entire table every update
	//Since number of ports can grow very large during a scan this will distribute the computation more evenly
	//Since the IP will tend to be relatively small compared to number of events, it's max is found during the call.
	if(portTable[event->dst_port] > portMax)
	{
		portMax = portTable[event->dst_port];
	}
	if(packet_times.size() > 1)
	{
		for(uint i = 0; i < event->IP_packet_sizes.size(); i++)
		{
			packTable[event->IP_packet_sizes[i]]++;
			packet_intervals.push_back(event->packet_intervals[i] - packet_times[packet_times.size()-1]);
			packet_times.push_back(event->packet_intervals[i]);
		}
	}
	else
	{
		for(uint i = 0; i < event->IP_packet_sizes.size(); i++)
		{
			packTable[event->IP_packet_sizes[i]]++;
			packet_times.push_back(event->packet_intervals[i]);
		}
		packet_intervals.clear();
		for(uint i = 1; i < packet_times.size(); i++)
		{
			packet_intervals.push_back(packet_times[i] - packet_times[i-1]);
		}
	}

	//Accumulate to find the lowest Start time and biggest end time.
	if( event->from_haystack)
	{
		haystackEvents++;
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

uint FeatureSet::serializeFeatureData(u_char *buf)
{
	uint offset = 0;
	//Bytes in a word, used for everything but port #'s
	uint size = 4;
	//Port # is 2 bytes
	uint psize = 2;

	//Total interval time for this feature set
	time_t totalInterval = endTime - startTime;
	memcpy(buf+offset, &totalInterval, size);
	offset += size;

	memcpy(buf+offset, &haystackEvents, size);
	offset += size;
	memcpy(buf+offset, &packetCount, size);
	offset += size;
	memcpy(buf+offset, &bytesTotal, size);
	offset += size;

	//Vector of packet intervals
	for(uint i = 0; i < packet_intervals.size(); i++)
	{
		memcpy(buf+offset, &packet_intervals[i], size);
		offset += size;
	}

	for(Packet_Table::iterator it = packTable.begin(); it != packTable.end(); it++)
	{

		memcpy(buf+offset, &it->first, size);
		offset += size;
		memcpy(buf+offset, &it->second, size);
		offset += size;
	}

	for(IP_Table::iterator it = IPTable.begin(); it != IPTable.end(); it++)
	{
		memcpy(buf+offset, &it->first, size);
		offset += size;
		memcpy(buf+offset, &it->second, size);
		offset += size;
	}

	for(Port_Table::iterator it = portTable.begin(); it != portTable.end(); it++)
	{
		memcpy(buf+offset, &it->first, psize);
		offset += psize;
		memcpy(buf+offset, &it->second, size);
		offset += size;
	}

	cout << "Interval: " << totalInterval << " HSEvents: " << haystackEvents << endl;
	cout << " Packet Count: " << packetCount << "Bytes Total: " << bytesTotal << endl;

	return offset;
}

uint FeatureSet::deserializeFeatureData(u_char *buf, in_addr_t hostAddr)
{
	uint offset = 0;
	//Bytes in a word, used for everything but port #'s
	uint size = 4;
	//Bytes in a port #
	uint psize = 2;
	//Temporary struct to store SA sender's information
	struct silentAlarmFeatureData SAData;
	for(uint i = 0; i < DIMENSION; i++)
	{
		SAData.features[i] = SATable[hostAddr].features[i];
	}

	cout << "Features copied" << endl;

	//Temporary variables to store and track data during deserialization
	uint temp;
	uint tempCount;
	uint16_t ptemp;
	cout << "Total Interval" << endl;

	memcpy(&SAData.totalInterval, buf+offset, size);
	offset += size;

	memcpy(&SAData.haystackEvents, buf+offset, size);
	offset += size;
	memcpy(&SAData.packetCount, buf+offset, size);
	offset += size;
	memcpy(&SAData.bytesTotal, buf+offset, size);
	offset += size;

	cout << "Packet Intervals" << endl;

	//Packet intervals
	SAData.packet_intervals.clear();
	cout << "Packet Count: " << SAData.packetCount << endl;
	for(uint i = 1; i < (SAData.packetCount); i++)
	{
		//cout << "Interval #: " << i << endl;
		memcpy(&temp, buf+offset, size);
		offset += size;
		SAData.packet_intervals.push_back(temp);
	}

	cout << "Packet Sizes" << endl;

	//Packet size table
	tempCount = 0;
	SAData.packTable.set_empty_key(0);
	SAData.packTable.clear();

	for(uint i = 0; i < SAData.packetCount;)
	{
		memcpy(&temp, buf+offset, size);
		offset += size;
		memcpy(&tempCount, buf+offset, size);
		offset += size;
		cout <<"Size: " << temp << " Count: " << tempCount << endl;
		SAData.packTable[(int)temp] = tempCount;
		i += tempCount;
	}

	cout << "IP Table" << endl;

	//IP table
	tempCount = 0;
	SAData.IPTable.set_empty_key(0);
	SAData.IPTable.clear();

	for(uint i = 0; i < SAData.packetCount;)
	{
		memcpy(&temp, buf+offset, size);
		offset += size;
		memcpy(&tempCount, buf+offset, size);
		offset += size;
		SAData.IPTable[(in_addr_t)temp] = tempCount;
		i += tempCount;
	}

	cout << "Port Table" << endl;

	//Port table
	tempCount = 0;
	SAData.portTable.set_empty_key(0);
	SAData.portTable.clear();

	for(uint i = 0; i < SAData.packetCount;)
	{
		memcpy(&ptemp, buf+offset, psize);
		offset += psize;
		memcpy(&tempCount, buf+offset, size);
		offset += size;
		SAData.portTable[ptemp] = tempCount;
		i += tempCount;
	}
	cout << "Interval: " << SAData.totalInterval << " HSEvents: " << SAData.haystackEvents << endl;
	cout << " Packet Count: " << SAData.packetCount << "Bytes Total: " << SAData.bytesTotal << endl;

	SATable[hostAddr] = SAData;
	return offset;

}

}
}
