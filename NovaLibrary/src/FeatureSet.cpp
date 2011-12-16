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
	totalInterval.first = 0;
	totalInterval.second = 0;

	IPTable.set_empty_key(0);
	portTable.set_empty_key(0);
	packTable.set_empty_key(0);
	SATable.set_empty_key(0);

	packet_intervals.clear();
	IPTable.clear();
	portTable.clear();
	packTable.clear();
	SATable.clear();

	IPTable.resize(INITIAL_IP_SIZE);
	portTable.resize(INITIAL_PORT_SIZE);
	packTable.resize(INITIAL_PACKET_SIZE);

	haystackEvents.first = 0;
	packetCount.first = 0;
	bytesTotal.first = 0;
	haystackEvents.second = 0;
	packetCount.second = 0;
	bytesTotal.second = 0;

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
	SATable.clear();
	packet_intervals.clear();

	haystackEvents.first = 0;
	packetCount.first = 0;
	bytesTotal.first = 0;
	haystackEvents.second = 0;
	packetCount.second = 0;
	bytesTotal.second = 0;

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
	//Each set of feature data (local and SATable items) have packetCount-1 intervals
	//Subtract (# of SA Data entries + 1) from total packet count to get total # of intervals
	features[PACKET_INTERVAL_MEAN] = (((double) totalInterval.second)
							/ ((double) (packetCount.second - (SATable.size() + 1) )));
}

void FeatureSet::CalculatePacketIntervalDeviation()
{
	double count = packet_intervals.size();
	//Each set of feature data (local and SATable items) have packetCount-1 intervals
	//Subtract (# of SA Data entries + 1) from total packet count to get total # of intervals
	double totalCount = (packetCount.second - (SATable.size()+1));
	double mean = 0;
	double variance = 0;

	CalculatePacketIntervalMean();
	mean = features[PACKET_INTERVAL_MEAN];

	for(uint i = 0; i < count; i++)
	{
		variance += (pow(((double)packet_intervals[i] - mean), 2)) / totalCount;
	}

	for(Silent_Alarm_Table::iterator it = SATable.begin(); it != SATable.end(); it++)
	{
		count = it->second.packet_intervals.size();
		for(uint i = 0; i < count; i++)
		{
			variance += (pow(((double)it->second.packet_intervals[i] - mean), 2)) / totalCount;
		}
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
	if( event->from_haystack)
	{
		IPTable[event->dst_IP.s_addr].first += event->packet_count;
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
	if(portTable[event->dst_port].second > portMax)
	{
		portMax = portTable[event->dst_port].second;
	}
	if(packet_times.size() > 1)
	{
		for(uint i = 0; i < event->IP_packet_sizes.size(); i++)
		{
			packTable[event->IP_packet_sizes[i]].first++;

			packet_intervals.push_back(event->packet_intervals[i] - packet_times[packet_times.size()-1]);
			packet_times.push_back(event->packet_intervals[i]);

		}
	}
	else
	{
		for(uint i = 0; i < event->IP_packet_sizes.size(); i++)
		{
			packTable[event->IP_packet_sizes[i]].first++;

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
		haystackEvents.first++;
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

void FeatureSet::UpdateFeatureData(bool include)
{
	//Updates timeInterval.first with the latest end and start times
	CalculateTimeInterval();

	if(include)
	{
		totalInterval.second += totalInterval.first;
		packetCount.second += packetCount.first;
		bytesTotal.second += bytesTotal.second;
		haystackEvents.second += haystackEvents.first;

		for(IP_Table::iterator it = IPTable.begin(); it != IPTable.end(); it++)
		{
			it->second.second += it->second.first;
		}

		for(Port_Table::iterator it = portTable.begin(); it != portTable.end(); it++)
		{
			it->second.second += it->second.first;
		}

		for(Packet_Table::iterator it = packTable.begin(); it != packTable.end(); it++)
		{
			it->second.second += it->second.first;
		}
	}
	else
	{
		totalInterval.second -= totalInterval.first;
		packetCount.second -= packetCount.first;
		bytesTotal.second -= bytesTotal.second;
		haystackEvents.second -= haystackEvents.first;

		for(IP_Table::iterator it = IPTable.begin(); it != IPTable.end(); it++)
		{
			it->second.second -= it->second.first;
		}

		for(Port_Table::iterator it = portTable.begin(); it != portTable.end(); it++)
		{
			it->second.second -= it->second.first;
		}

		for(Packet_Table::iterator it = packTable.begin(); it != packTable.end(); it++)
		{
			it->second.second -= it->second.first;
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
	//Bytes in a word, used for everything but port #'s
	const uint size = 4;

	//Required, individual variables for calculation
	memcpy(buf+offset, &totalInterval.first, size);
	offset += size;
	memcpy(buf+offset, &haystackEvents.first, size);
	offset += size;
	memcpy(buf+offset, &packetCount.first, size);
	offset += size;
	memcpy(buf+offset, &bytesTotal.first, size);
	offset += size;
	memcpy(buf+offset, &portMax, size);
	offset += size;

	//Vector of packet intervals
	for(uint i = 0; i < packet_intervals.size(); i++)
	{
		memcpy(buf+offset, &packet_intervals[i], size);
		offset += size;
	}

	//These tables all just place their key followed by the data
	for(Packet_Table::iterator it = packTable.begin(); it != packTable.end(); it++)
	{

		memcpy(buf+offset, &it->first, size);
		offset += size;
		memcpy(buf+offset, &it->second.first, size);
		offset += size;
	}
	for(IP_Table::iterator it = IPTable.begin(); it != IPTable.end(); it++)
	{
		memcpy(buf+offset, &it->first, size);
		offset += size;
		memcpy(buf+offset, &it->second.first, size);
		offset += size;
	}

	for(Port_Table::iterator it = portTable.begin(); it != portTable.end(); it++)
	{
		//uint temp = it->first; Might need to use 4 bytes here?
		memcpy(buf+offset, &it->first, size);
		offset += size;
		memcpy(buf+offset, &it->second.first, size);
		offset += size;
	}

	return offset;
}

uint FeatureSet::deserializeFeatureData(u_char *buf, in_addr_t hostAddr)
{
	uint offset = 0;

	//Bytes in a word, used for everything but port #'s
	const uint size = 4;

	//Temporary struct to store SA sender's information
	struct silentAlarmFeatureData SAData;
	for(uint i = 0; i < DIMENSION; i++)
	{
		SAData.features[i] = SATable[hostAddr].features[i];
	}

	//Temporary variables to store and track data during deserialization
	uint temp;
	uint tempCount;

	//Required, individual variables for calculation
	memcpy(&SAData.totalInterval, buf+offset, size);
	offset += size;
	memcpy(&SAData.haystackEvents, buf+offset, size);
	offset += size;
	memcpy(&SAData.packetCount, buf+offset, size);
	offset += size;
	memcpy(&SAData.bytesTotal, buf+offset, size);
	offset += size;
	memcpy(&temp, buf+offset, size);
	offset += size;

	if(temp > portMax)
		portMax = temp;

	//Packet intervals
	SAData.packet_intervals.clear();
	for(uint i = 1; i < (SAData.packetCount); i++)
	{
		memcpy(&temp, buf+offset, size);
		offset += size;
		SAData.packet_intervals.push_back(temp);
	}

	/***************************************************************************************************
	* For all of these tables we extract, the key (bin identifier) followed by the data (packet count)
	*  i += the # of packets in the bin, if we haven't reached packet count we know there's another item
	****************************************************************************************************/

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

		SAData.packTable[(int)temp].first = tempCount;
		i += tempCount;
	}

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
		SAData.IPTable[(in_addr_t)temp].first = tempCount;
		i += tempCount;
	}


	//Port table
	tempCount = 0;
	SAData.portTable.set_empty_key(0);
	SAData.portTable.clear();

	for(uint i = 0; i < SAData.packetCount;)
	{
		memcpy(&temp, buf+offset, size);
		offset += size;
		memcpy(&tempCount, buf+offset, size);
		offset += size;
		SAData.portTable[temp].first = tempCount;
		i += tempCount;
	}

	//If this host has previous data from this sender, remove the old information
	if(SATable[hostAddr].packTable.size())
	{
		silentAlarmFeatureData * host = &SATable[hostAddr];

		packetCount.second -= host->packetCount;
		bytesTotal.second -= host->bytesTotal;
		haystackEvents.second -= host->haystackEvents;
		totalInterval.second -= host->totalInterval;

		for(Packet_Table::iterator it = host->packTable.begin(); it != host->packTable.end(); it++)
		{
			packTable[it->first].second -= it->second.first;
		}

		for(IP_Table::iterator it = host->IPTable.begin(); it != host->IPTable.end(); it++)
		{
			IPTable[it->first].second -= it->second.first;
		}

		for(Port_Table::iterator it = host->portTable.begin(); it != host->portTable.end(); it++)
		{
			portTable[it->first].second -= it->second.first;
		}
	}

	//Include the new information

	packetCount.second += SAData.packetCount;
	bytesTotal.second += SAData.bytesTotal;
	haystackEvents.second += SAData.haystackEvents;
	totalInterval.second += SAData.totalInterval;

	for(Packet_Table::iterator it = SAData.packTable.begin(); it != SAData.packTable.end(); it++)
	{
			packTable[it->first].second += it->second.first;
	}
	for(IP_Table::iterator it = SAData.IPTable.begin(); it != SAData.IPTable.end(); it++)
	{
			IPTable[it->first].second += it->second.first;
	}
	for(Port_Table::iterator it = SAData.portTable.begin(); it != SAData.portTable.end(); it++)
	{
			portTable[it->first].second += it->second.first;
	}

	//Copy the Data over
	SATable[hostAddr] = SAData;

	return offset;
}

}
}
