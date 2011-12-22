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

//Empty constructor
FeatureSet::FeatureSet()
{
	//Temp variables
	startTime = 2147483647; //2^31 - 1 (IE: Y2.038K bug) (IE: The largest standard Unix time value)
	endTime = 0;
	totalInterval.first = 0;
	totalInterval.second = 0;

	IPTable.set_empty_key(0);
	portTable.set_empty_key(0);
	packTable.set_empty_key(0);
	intervalTable.set_empty_key(2147483647);

	IPTable.clear();
	portTable.clear();
	packTable.clear();
	intervalTable.clear();

	intervalTable.resize(INITIAL_IP_SIZE);
	IPTable.resize(INITIAL_IP_SIZE);
	portTable.resize(INITIAL_PORT_SIZE);
	packTable.resize(INITIAL_PACKET_SIZE);

	haystackEvents.first = 0;
	packetCount.first = 0;
	bytesTotal.first = 0;
	haystackEvents.second = 0;
	packetCount.second = 0;
	bytesTotal.second = 0;
	last_time = 0;

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
	totalInterval.first = 0;
	totalInterval.second = 0;

	IPTable.clear();
	portTable.clear();
	packTable.clear();
	intervalTable.clear();

	haystackEvents.first = 0;
	packetCount.first = 0;
	bytesTotal.first = 0;
	haystackEvents.second = 0;
	packetCount.second = 0;
	bytesTotal.second = 0;
	last_time = 0;

	portMax = 0;
	IPMax = 0;
	//Features
	for(int i = 0; i < DIMENSION; i++)
	{
		features[i] = 0;
	}
}

//Calculates all features in the feature set
void FeatureSet::CalculateAll()
{
	CalculateTimeInterval();

	UpdateFeatureData(INCLUDE);

	CalculateDistinctIPs();
	CalculateDistinctPorts();

	CalculateIPTrafficDistribution();
	CalculatePortTrafficDistribution();

	CalculateHaystackEventFrequency();

	CalculatePacketSizeDeviation();
	CalculatePacketIntervalDeviation();

	UpdateFeatureData(REMOVE);
}

///Calculates the total interval for time based features using latest timestamps
void FeatureSet::CalculateTimeInterval()
{
	if(endTime > startTime)
	{
		totalInterval.first = endTime - startTime;
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
	features[DISTINCT_PORTS] =  portTable.size();
}

///Calculates the ip traffic distribution for the suspect
void FeatureSet::CalculateIPTrafficDistribution()
{
	features[IP_TRAFFIC_DISTRIBUTION] = 0;
	IPMax = 0;
	for (IP_Table::iterator it = IPTable.begin() ; it != IPTable.end(); it++)
	{
		if(it->second.second > IPMax)
			IPMax = it->second.second;
	}
	for (IP_Table::iterator it = IPTable.begin() ; it != IPTable.end(); it++)
	{
		features[IP_TRAFFIC_DISTRIBUTION] += ((double)it->second.second / (double)IPMax);
	}

	features[IP_TRAFFIC_DISTRIBUTION] = features[IP_TRAFFIC_DISTRIBUTION] / (double)IPTable.size();
}

///Calculates the port traffic distribution for the suspect
void FeatureSet::CalculatePortTrafficDistribution()
{
	features[PORT_TRAFFIC_DISTRIBUTION] = 0;
	for (Port_Table::iterator it = portTable.begin() ; it != portTable.end(); it++)
	{
		features[PORT_TRAFFIC_DISTRIBUTION] += ((double)it->second.second / (double)portMax);
	}

	features[PORT_TRAFFIC_DISTRIBUTION] = features[PORT_TRAFFIC_DISTRIBUTION] / (double)portTable.size();
}

void FeatureSet::CalculateHaystackEventFrequency()
{
	// if > 0, .second is a time_t(uint) sum of all intervals across all nova instances
	if(totalInterval.second)
	{
		features[HAYSTACK_EVENT_FREQUENCY] = ((double)(haystackEvents.second)) / (double)(totalInterval.second);
	}
	else
	{
		//If interval is 0, no time based information, use a default of 1 for the interval
		features[HAYSTACK_EVENT_FREQUENCY] = (double)(haystackEvents.second);
	}
}


void FeatureSet::CalculatePacketIntervalMean()
{

	features[PACKET_INTERVAL_MEAN] = (((double) totalInterval.second)
							/ ((double) (packetCount.second)));
}

void FeatureSet::CalculatePacketIntervalDeviation()
{
	double totalCount = packetCount.second;
	double mean = 0;
	double variance = 0;

	CalculatePacketIntervalMean();
	mean = features[PACKET_INTERVAL_MEAN];

	for (Interval_Table::iterator it = intervalTable.begin() ; it != intervalTable.end(); it++)
	{
		variance += it->second.second*(pow((it->first - mean), 2)/totalCount);
	}

	features[PACKET_INTERVAL_DEVIATION] = sqrt(variance);
}

///Calculates Packet Size Mean for a suspect
void FeatureSet::CalculatePacketSizeMean()
{
	features[PACKET_SIZE_MEAN] = (double)bytesTotal.second / (double)packetCount.second;
}

///Calculates Packet Size Variance for a suspect
void FeatureSet::CalculatePacketSizeDeviation()
{
	double count = packetCount.second;
	double mean = 0;
	double variance = 0;
	//Calculate mean
	CalculatePacketSizeMean();
	mean = features[PACKET_SIZE_MEAN];

	//Calculate variance
	for(Packet_Table::iterator it = packTable.begin() ; it != packTable.end(); it++)
	{
		// number of packets multiplied by (packet_size - mean)^2 divided by count
		variance += (it->second.second * pow((it->first - mean), 2))/ count;
	}

	features[PACKET_SIZE_DEVIATION] = sqrt(variance);
}

void FeatureSet::UpdateEvidence(TrafficEvent *event)
{
	packetCount.first += event->packet_count;
	bytesTotal.first += event->IP_total_data_bytes;

	//If from haystack
	if(event->from_haystack)
	{
		IPTable[event->dst_IP.s_addr].first += event->packet_count;
		haystackEvents.first++;
	}
	//Else from a host
	else
	{
		//Put the packet count into a bin associated with source so that
		// all host events for a suspect go into the same bin
		IPTable[event->src_IP.s_addr].first +=  event->packet_count;
	}

	portTable[event->dst_port].first +=  event->packet_count;

	//Checks for the max to avoid iterating through the entire table every update
	//Since number of ports can grow very large during a scan this will distribute the computation more evenly
	//Since the IP will tend to be relatively small compared to number of events, it's max is found during the call.
	if(portTable[event->dst_port].first > portMax)
	{
		portMax = portTable[event->dst_port].first;
	}

	for(uint i = 0; i < event->IP_packet_sizes.size(); i++)
	{
		packTable[event->IP_packet_sizes[i]].first++;
	}

	if(last_time != 0)
		intervalTable[event->packet_intervals[0] - last_time].first++;

	for(uint i = 1; i < event->packet_intervals.size(); i++)
	{
		intervalTable[event->packet_intervals[i] - event->packet_intervals[i-1]].first++;
	}
	last_time = event->packet_intervals.back();

	//Accumulate to find the lowest Start time and biggest end time.
	if( event->start_timestamp < startTime)
	{
		startTime = event->start_timestamp;
	}
	if(  event->end_timestamp > endTime)
	{
		endTime = event->end_timestamp;
	}
}

void FeatureSet::UpdateFeatureData(bool include)
{
	if(include)
	{
		totalInterval.second += totalInterval.first;
		packetCount.second += packetCount.first;
		bytesTotal.second += bytesTotal.first;
		haystackEvents.second += haystackEvents.first;

		for(IP_Table::iterator it = IPTable.begin(); it != IPTable.end(); it++)
		{
			IPTable[it->first].second += it->second.first;
		}

		for(Port_Table::iterator it = portTable.begin(); it != portTable.end(); it++)
		{
			portTable[it->first].second += it->second.first;
		}

		for(Packet_Table::iterator it = packTable.begin(); it != packTable.end(); it++)
		{
			packTable[it->first].second += it->second.first;
		}
		for(Interval_Table::iterator it = intervalTable.begin(); it != intervalTable.end(); it++)
		{
			intervalTable[it->first].second += it->second.first;
		}
	}
	else
	{
		totalInterval.second -= totalInterval.first;
		packetCount.second -= packetCount.first;
		bytesTotal.second -= bytesTotal.first;
		haystackEvents.second -= haystackEvents.first;

		for(IP_Table::iterator it = IPTable.begin(); it != IPTable.end(); it++)
		{
			IPTable[it->first].second -= it->second.first;
		}

		for(Port_Table::iterator it = portTable.begin(); it != portTable.end(); it++)
		{
			portTable[it->first].second -= it->second.first;
		}

		for(Packet_Table::iterator it = packTable.begin(); it != packTable.end(); it++)
		{
			packTable[it->first].second -= it->second.first;
		}

		for(Interval_Table::iterator it = intervalTable.begin(); it != intervalTable.end(); it++)
		{
			intervalTable[it->first].second -= it->second.first;
		}
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
	uint count = 0;
	uint table_entries = 0;

	//Bytes in a word, used for everything but port #'s
	const uint size = 4;

	//Required, individual variables for calculation
	CalculateTimeInterval();
	memcpy(buf+offset, &totalInterval.first, size);
	offset += size;

	memcpy(buf+offset, &haystackEvents.first, size);
	haystackEvents.second += haystackEvents.first;
	haystackEvents.first = 0;
	offset += size;

	memcpy(buf+offset, &packetCount.first, size);
	packetCount.second += packetCount.first;
	packetCount.first = 0;
	offset += size;

	memcpy(buf+offset, &bytesTotal.first, size);
	bytesTotal.second += bytesTotal.first;
	bytesTotal.first = 0;
	offset += size;

	memcpy(buf+offset, &portMax, size);
	offset += size;

	//These tables all just place their key followed by the data
	uint tempInt;

	for(Interval_Table::iterator it = intervalTable.begin(); (it != intervalTable.end()) && (count < MAX_TABLE_ENTRIES); it++)
		if(it->second.first)
			count++;

	//The size of the Table
	tempInt = count - table_entries;
	memcpy(buf+offset, &tempInt, size);
	offset += size;

	for(Interval_Table::iterator it = intervalTable.begin(); (it != intervalTable.end()) && (table_entries < count); it++)
	{
		if(it->second.first)
		{
			table_entries++;
			memcpy(buf+offset, &it->first, size);
			offset += size;
			memcpy(buf+offset, &it->second.first, size);
			offset += size;
			intervalTable[it->first].second += it->second.first;
			intervalTable[it->first].first = 0;
		}
	}

	for(Packet_Table::iterator it = packTable.begin(); (it != packTable.end()) && (count < MAX_TABLE_ENTRIES); it++)
		if(it->second.first)
			count++;

	//The size of the Table
	tempInt = count - table_entries;
	memcpy(buf+offset, &tempInt, size);
	offset += size;

	for(Packet_Table::iterator it = packTable.begin(); (it != packTable.end()) && (table_entries < count); it++)
	{
		if(it->second.first)
		{
			table_entries++;
			memcpy(buf+offset, &it->first, size);
			offset += size;
			memcpy(buf+offset, &it->second.first, size);
			offset += size;
			packTable[it->first].second += it->second.first;
			packTable[it->first].first = 0;
		}
	}

	for(IP_Table::iterator it = IPTable.begin(); (it != IPTable.end()) && (count < MAX_TABLE_ENTRIES); it++)
		if(it->second.first)
			count++;

	//The size of the Table
	tempInt = count - table_entries;
	memcpy(buf+offset, &tempInt, size);
	offset += size;

	for(IP_Table::iterator it = IPTable.begin(); (it != IPTable.end()) && (table_entries < count); it++)
	{
		if(it->second.first)
		{
			table_entries++;
			memcpy(buf+offset, &it->first, size);
			offset += size;
			memcpy(buf+offset, &it->second.first, size);
			offset += size;
			IPTable[it->first].second += it->second.first;
			IPTable[it->first].first = 0;
		}
	}

	for(Port_Table::iterator it = portTable.begin(); (it != portTable.end()) && (count < MAX_TABLE_ENTRIES); it++)
		if(it->second.first)
			count++;

	//The size of the Table
	tempInt = count - table_entries;
	memcpy(buf+offset, &tempInt, size);
	offset += size;

	for(Port_Table::iterator it = portTable.begin(); (it != portTable.end()) && (table_entries < count); it++)
	{
		if(it->second.first)
		{
			table_entries++;
			memcpy(buf+offset, &it->first, size);
			offset += size;
			memcpy(buf+offset, &it->second.first, size);
			offset += size;
			portTable[it->first].second += it->second.first;
			portTable[it->first].first = 0;
		}
	}
	return offset;
}

uint FeatureSet::deserializeFeatureDataLocal(u_char *buf)
{
	uint offset = 0;

	//Bytes in a word, used for everything but port #'s
	const uint size = 4;
	uint table_size = 0;

	//Temporary variables to store and track data during deserialization
	uint temp = 0;
	uint tempCount = 0;

	//Required, individual variables for calculation
	memcpy(&temp, buf+offset, size);
	totalInterval.first += temp;
	offset += size;

	memcpy(&temp, buf+offset, size);
	haystackEvents.first += temp;
	offset += size;

	memcpy(&temp, buf+offset, size);
	packetCount.first += temp;
	offset += size;

	memcpy(&temp, buf+offset, size);
	bytesTotal.first += temp;
	offset += size;

	memcpy(&temp, buf+offset, size);
	offset += size;

	if(temp > portMax)
		portMax = temp;

	/***************************************************************************************************
	* For all of these tables we extract, the key (bin identifier) followed by the data (packet count)
	*  i += the # of packets in the bin, if we haven't reached packet count we know there's another item
	****************************************************************************************************/

	memcpy(&table_size, buf+offset, size);
	offset += size;

	//Packet interval table
	for(uint i = 0; i < table_size;)
	{
		memcpy(&temp, buf+offset, size);
		offset += size;
		memcpy(&tempCount, buf+offset, size);
		offset += size;
		intervalTable[temp].first += tempCount;
		i++;
	}

	memcpy(&table_size, buf+offset, size);
	offset += size;

	//Packet size table
	for(uint i = 0; i < table_size;)
	{
		memcpy(&temp, buf+offset, size);
		offset += size;
		memcpy(&tempCount, buf+offset, size);
		offset += size;
		packTable[temp].first += tempCount;
		i++;
	}

	memcpy(&table_size, buf+offset, size);
	offset += size;

	//IP table
	for(uint i = 0; i < table_size;)
	{
		memcpy(&temp, buf+offset, size);
		offset += size;
		memcpy(&tempCount, buf+offset, size);
		offset += size;
		IPTable[temp].first += tempCount;
		i++;
	}

	memcpy(&table_size, buf+offset, size);
	offset += size;

	//Port table
	for(uint i = 0; i < table_size;)
	{
		memcpy(&temp, buf+offset, size);
		offset += size;
		memcpy(&tempCount, buf+offset, size);
		offset += size;
		portTable[temp].first += tempCount;
		i++;
	}

	return offset;
}

uint FeatureSet::deserializeFeatureDataBroadcast(u_char *buf)
{
	uint offset = 0;

	//Bytes in a word, used for everything but port #'s
	const uint size = 4;
	uint table_size = 0;

	//Temporary variables to store and track data during deserialization
	uint temp = 0;
	uint tempCount = 0;

	//Required, individual variables for calculation
	memcpy(&temp, buf+offset, size);
	totalInterval.second += temp;
	offset += size;

	memcpy(&temp, buf+offset, size);
	haystackEvents.second += temp;
	offset += size;

	memcpy(&temp, buf+offset, size);
	packetCount.second += temp;
	offset += size;

	memcpy(&temp, buf+offset, size);
	bytesTotal.second += temp;
	offset += size;

	memcpy(&temp, buf+offset, size);
	offset += size;

	if(temp > portMax)
		portMax = temp;

	/***************************************************************************************************
	* For all of these tables we extract, the key (bin identifier) followed by the data (packet count)
	*  i += the # of packets in the bin, if we haven't reached packet count we know there's another item
	****************************************************************************************************/

	memcpy(&table_size, buf+offset, size);
	offset += size;

	//Packet interval table
	for(uint i = 0; i < table_size;)
	{
		memcpy(&temp, buf+offset, size);
		offset += size;
		memcpy(&tempCount, buf+offset, size);
		offset += size;
		intervalTable[temp].second += tempCount;
		i++;
	}

	memcpy(&table_size, buf+offset, size);
	offset += size;

	//Packet size table
	for(uint i = 0; i < table_size;)
	{
		memcpy(&temp, buf+offset, size);
		offset += size;
		memcpy(&tempCount, buf+offset, size);
		offset += size;
		packTable[temp].second += tempCount;
		i++;
	}

	memcpy(&table_size, buf+offset, size);
	offset += size;

	//IP table
	for(uint i = 0; i < table_size;)
	{
		memcpy(&temp, buf+offset, size);
		offset += size;
		memcpy(&tempCount, buf+offset, size);
		offset += size;
		IPTable[temp].second += tempCount;
		i++;
	}

	memcpy(&table_size, buf+offset, size);
	offset += size;

	//Port table
	for(uint i = 0; i < table_size;)
	{
		memcpy(&temp, buf+offset, size);
		offset += size;
		memcpy(&tempCount, buf+offset, size);
		offset += size;
		portTable[temp].second += tempCount;
		i++;
	}

	return offset;
}

}
