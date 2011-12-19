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
	packet_intervals.push_back(0);
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
	packet_intervals.push_back(0);

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
	CalculateTimeInterval();

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
	double count = packet_intervals.size();
	double totalCount = packetCount.second;
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
		for(uint i = 1; i < packet_times.size(); i++)
		{
			packet_intervals.push_back(packet_times[i] - packet_times[i-1]);
		}
	}

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
	//Updates timeInterval.first with the latest end and start times
	CalculateTimeInterval();

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

uint FeatureSet::serializeFeatureData(u_char *buf, in_addr_t hostAddr)
{
	uint offset = 0;
	uint count = 0;

	//Bytes in a word, used for everything but port #'s
	const uint size = 4;

	struct silentAlarmFeatureData temp;

	//If we have a silentAlarmFeatureData struct for the local host
	if(SATable.find(hostAddr) != SATable.end())
		temp = SATable[hostAddr];

	//Else create one
	else
	{
		temp.totalInterval = 0;
		temp.packetCount = 0;
		temp.haystackEvents = 0;
		temp.bytesTotal = 0;
		temp.packet_intervals.clear();
	}

	//Required, individual variables for calculation
	memcpy(buf+offset, &totalInterval.first, size);
	temp.totalInterval = totalInterval.first;
	offset += size;

	//If we will have plenty of room, send everything
	if(packetCount.first < MAX_PACKETS_PER_MSG)
	{
		memcpy(buf+offset, &haystackEvents.first, size);
		haystackEvents.second += haystackEvents.first;
		temp.haystackEvents += haystackEvents.first;
		haystackEvents.first = 0;
		offset += size;

		memcpy(buf+offset, &packetCount.first, size);
		packetCount.second += packetCount.first;
		temp.packetCount += packetCount.first;
		count = packetCount.first;
		packetCount.first = 0;
		offset += size;

		memcpy(buf+offset, &bytesTotal.first, size);
		bytesTotal.second += bytesTotal.first;
		temp.bytesTotal += bytesTotal.first;
		bytesTotal.first = 0;
		offset += size;
	}
	//If we might run out of room, we need to limit the size of what we send
	else
	{
		//how much of the information we will be sending
		double ratio = (double)MAX_PACKETS_PER_MSG / (double)packetCount.first;

		uint tempInt = (uint)(haystackEvents.first*ratio);
		memcpy(buf+offset, &tempInt, size);
		haystackEvents.second += tempInt;
		temp.haystackEvents += tempInt;
		haystackEvents.first -= tempInt;
		offset += size;

		tempInt = MAX_PACKETS_PER_MSG;
		memcpy(buf+offset, &tempInt, size);
		packetCount.second += tempInt;
		temp.packetCount += tempInt;
		count = tempInt;
		packetCount.first -= tempInt;
		offset += size;

		tempInt = (uint)(bytesTotal.first*ratio);
		memcpy(buf+offset, &tempInt, size);
		bytesTotal.second += tempInt;
		temp.bytesTotal += tempInt;
		bytesTotal.first -= tempInt;
		offset += size;
	}

	memcpy(buf+offset, &portMax, size);
	offset += size;

	//Vector of packet intervals
	for(uint i = 0; i < count; i++)
	{
		memcpy(buf+offset, &packet_intervals[packet_intervals.size()-1], size);
		temp.packet_intervals.push_back(packet_intervals[packet_intervals.size()-1]);
		packet_intervals.pop_back();
		offset += size;
	}

	//These tables all just place their key followed by the data
	uint tempInt, i = 0;
	for(Packet_Table::iterator it = packTable.begin(); (it != packTable.end()) && (i < count); it++)
	{
		if(it->second.first)
		{
			tempInt = it->second.first;
			//We only send data on the number of packets serialized earlier in the message
			// if we send more it might be expecting information that isn't there later
			if((i+tempInt) > count)
			{
				tempInt = count - i;
			}
			i+= tempInt;
			memcpy(buf+offset, &it->first, size);
			offset += size;
			memcpy(buf+offset, &tempInt, size);
			packTable[it->first].second += tempInt;
			packTable[it->first].first -= tempInt;
			offset += size;
		}
	}
	i = 0;
	for(IP_Table::iterator it = IPTable.begin(); (it != IPTable.end()) && (i < count); it++)
	{
		if(it->second.first)
		{
			tempInt = it->second.first;
			//We only send data on the number of packets serialized earlier in the message
			// if we send more it might be expecting information that isn't there later
			if((i+tempInt) > count)
			{
				tempInt = count - i;
			}
			i+= tempInt;
			memcpy(buf+offset, &it->first, size);
			offset += size;
			memcpy(buf+offset, &tempInt, size);
			IPTable[it->first].second += tempInt;
			IPTable[it->first].first -= tempInt;
			offset += size;
		}
	}
	i = 0;
	for(Port_Table::iterator it = portTable.begin(); (it != portTable.end()) && (i < count); it++)
	{
		if(it->second.first)
		{
			tempInt = it->second.first;
			//We only send data on the number of packets serialized earlier in the message
			// if we send more it might be expecting information that isn't there later
			if((i+tempInt) > count)
			{
				tempInt = count - i;
			}
			i+= tempInt;
			memcpy(buf+offset, &it->first, size);
			offset += size;
			memcpy(buf+offset, &tempInt, size);
			portTable[it->first].second += tempInt;
			portTable[it->first].first -= tempInt;
			offset += size;
		}
	}
	SATable[hostAddr] = temp;
	return offset;
}

uint FeatureSet::deserializeFeatureData(u_char *buf, in_addr_t hostAddr, struct silentAlarmFeatureData * sender)
{
	uint offset = 0;

	//Bytes in a word, used for everything but port #'s
	const uint size = 4;
	uint pkt_count = 0;

	//Temporary variables to store and track data during deserialization
	uint temp = 0;
	uint tempCount = 0;


	//Required, individual variables for calculation
	totalInterval.second -= sender->totalInterval;
	memcpy(&sender->totalInterval, buf+offset, size);
	totalInterval.second += sender->totalInterval;
	offset += size;

	memcpy(&temp, buf+offset, size);
	sender->haystackEvents += temp;
	haystackEvents.second += temp;
	offset += size;

	memcpy(&pkt_count, buf+offset, size);
	sender->packetCount += pkt_count;
	packetCount.second += pkt_count;
	offset += size;

	memcpy(&temp, buf+offset, size);
	sender->bytesTotal += temp;
	bytesTotal.second += temp;
	offset += size;

	memcpy(&temp, buf+offset, size);
	offset += size;

	if(temp > portMax)
		portMax = temp;

	//Packet intervals
	for(uint i = 0; i < pkt_count; i++)
	{
		memcpy(&temp, buf+offset, size);
		offset += size;
		sender->packet_intervals.push_back(temp);
	}

	/***************************************************************************************************
	* For all of these tables we extract, the key (bin identifier) followed by the data (packet count)
	*  i += the # of packets in the bin, if we haven't reached packet count we know there's another item
	****************************************************************************************************/

	//Packet size table
	for(uint i = 0; i < pkt_count;)
	{
		memcpy(&temp, buf+offset, size);
		offset += size;
		memcpy(&tempCount, buf+offset, size);
		offset += size;
		if(temp)
			packTable[temp].second += tempCount;
		i += tempCount;
	}

	//IP table
	for(uint i = 0; i < pkt_count;)
	{
		memcpy(&temp, buf+offset, size);
		offset += size;
		memcpy(&tempCount, buf+offset, size);
		offset += size;
		if(temp)
			IPTable[temp].second += tempCount;
		i += tempCount;
	}


	//Port table
	for(uint i = 0; i < pkt_count;)
	{
		memcpy(&temp, buf+offset, size);
		offset += size;
		memcpy(&tempCount, buf+offset, size);
		offset += size;
		if(temp)
			portTable[temp].second += tempCount;
		i += tempCount;
	}

	return offset;
}

}
}
