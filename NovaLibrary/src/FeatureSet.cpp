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
	totalInterval = 0;
	totalIntervalAll = 0;

	IPTable.set_empty_key(0);
	portTable.set_empty_key(0);
	packTable.set_empty_key(0);
	SATable.set_empty_key(0);
	IPTableAll.set_empty_key(0);
	portTableAll.set_empty_key(0);
	packTableAll.set_empty_key(0);

	packet_intervals.clear();
	IPTable.clear();
	portTable.clear();
	packTable.clear();
	SATable.clear();
	IPTableAll.clear();
	portTableAll.clear();
	packTableAll.clear();

	IPTable.resize(INITIAL_IP_SIZE);
	portTable.resize(INITIAL_PORT_SIZE);
	packTable.resize(INITIAL_PACKET_SIZE);
	IPTableAll.resize(INITIAL_IP_SIZE);
	portTableAll.resize(INITIAL_PORT_SIZE);
	packTableAll.resize(INITIAL_PACKET_SIZE);

	haystackEvents = 0;
	packetCount = 0;
	bytesTotal = 0;
	haystackEventsAll = 0;
	packetCountAll = 0;
	bytesTotalAll = 0;

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
	totalInterval = 0;
	totalIntervalAll = 0;

	IPTable.clear();
	portTable.clear();
	packTable.clear();
	SATable.clear();
	IPTableAll.clear();
	portTableAll.clear();
	packTableAll.clear();
	packet_intervals.clear();

	haystackEvents = 0;
	packetCount = 0;
	bytesTotal = 0;
	haystackEventsAll = 0;
	packetCountAll = 0;
	bytesTotalAll = 0;

	portMax = 0;
	IPMax = 0;
	//Features
	for(int i = 0; i < DIMENSION; i++)
	{
		features[i] = 0;
	}
}
///Calculates the total interval for time based features using latest timestamps

void FeatureSet::CalculateTimeInterval()
{
	if(endTime > startTime)
	{
		totalIntervalAll -= totalInterval;
		totalInterval = endTime - startTime;
		totalIntervalAll += totalInterval;
	}
}

///Calculates distinct IPs contacted
void FeatureSet::CalculateDistinctIPs()
{
	features[DISTINCT_IPS] = IPTableAll.size();
}
///Calculates distinct ports contacted
void FeatureSet::CalculateDistinctPorts()
{
	features[DISTINCT_PORTS] =  portTableAll.size();
}

///Calculates the ip traffic distribution for the suspect
void FeatureSet::CalculateIPTrafficDistribution()
{
	features[IP_TRAFFIC_DISTRIBUTION] = 0;
	IPMax = 0;
	for (IP_Table::iterator it = IPTableAll.begin() ; it != IPTableAll.end(); it++)
	{
		if(it->second > IPMax) IPMax = it->second;
	}
	for (IP_Table::iterator it = IPTableAll.begin() ; it != IPTableAll.end(); it++)
	{
		features[IP_TRAFFIC_DISTRIBUTION] += ((double)it->second / (double)IPMax);
	}

	features[IP_TRAFFIC_DISTRIBUTION] = features[IP_TRAFFIC_DISTRIBUTION] / (double)IPTableAll.size();
}

///Calculates the port traffic distribution for the suspect
void FeatureSet::CalculatePortTrafficDistribution()
{
	features[PORT_TRAFFIC_DISTRIBUTION] = 0;
	for (Port_Table::iterator it = portTableAll.begin() ; it != portTableAll.end(); it++)
	{
		features[PORT_TRAFFIC_DISTRIBUTION] += ((double)it->second / (double)portMax);
	}

	features[PORT_TRAFFIC_DISTRIBUTION] = features[PORT_TRAFFIC_DISTRIBUTION] / (double)portTableAll.size();
}

void FeatureSet::CalculateHaystackEventFrequency()
{
	if(totalIntervalAll > 0)
	{
		features[HAYSTACK_EVENT_FREQUENCY] = ((double)(haystackEventsAll)) / (double)(totalIntervalAll);
	}
}


void FeatureSet::CalculatePacketIntervalMean()
{
	//Each set of feature data (local and SATable items) have packetCount-1 intervals
	//Subtract (# of SA Data entries + 1) from total packet count to get total # of intervals
	features[PACKET_INTERVAL_MEAN] = ((double)(totalIntervalAll) / (double)(packetCountAll- (SATable.size()+1)));
}

void FeatureSet::CalculatePacketIntervalDeviation()
{
	double count = packet_intervals.size();
	//Each set of feature data (local and SATable items) have packetCount-1 intervals
	//Subtract (# of SA Data entries + 1) from total packet count to get total # of intervals
	double totalCount = (packetCountAll - (SATable.size()+1));
	double mean = 0;
	double variance = 0;

	CalculatePacketIntervalMean();
	mean = features[PACKET_INTERVAL_MEAN];

	for(uint i = 0; i < count; i++)
	{
		variance += (pow((packet_intervals[i] - mean), 2));
	}

	for(Silent_Alarm_Table::iterator it = SATable.begin(); it != SATable.end(); it++)
	{
		count = it->second.packet_intervals.size();
		for(uint i = 0; i < count; i++)
		{
			variance += (pow(( it->second.packet_intervals[i] - mean), 2));
		}
	}
	variance /= totalCount;

	features[PACKET_INTERVAL_DEVIATION] = sqrt(variance);
}

///Calculates Packet Size Mean for a suspect
void FeatureSet::CalculatePacketSizeMean()
{
	features[PACKET_SIZE_MEAN] = (double)bytesTotalAll / (double)packetCountAll;
}

///Calculates Packet Size Variance for a suspect
void FeatureSet::CalculatePacketSizeDeviation()
{
	double count = packetCountAll;
	double mean = 0;
	double variance = 0;
	//Calculate mean
	CalculatePacketSizeMean();
	mean = features[PACKET_SIZE_MEAN];

	//Calculate variance
	for(Packet_Table::iterator it = packTableAll.begin() ; it != packTableAll.end(); it++)
	{
		// number of packets multiplied by (packet_size - mean)^2 divided by count
		variance += (it->second*pow(it->first - mean,2));
	}
	variance /= count;

	features[PACKET_SIZE_DEVIATION] = sqrt(variance);
}

void FeatureSet::UpdateEvidence(TrafficEvent *event)
{
	packetCount += event->packet_count;
	packetCountAll += event->packet_count;
	bytesTotal += event->IP_total_data_bytes;
	bytesTotalAll += event->IP_total_data_bytes;


	//If from haystack
	if( event->from_haystack)
	{
		IPTable[event->dst_IP.s_addr] += event->packet_count;
		IPTableAll[event->dst_IP.s_addr] += event->packet_count;
	}
	//Else from a host
	else
	{
		//Put the packet count into a bin associated with source so that
		// all host events for a suspect go into the same bin
		IPTable[event->src_IP.s_addr] +=  event->packet_count;
		IPTableAll[event->src_IP.s_addr] +=  event->packet_count;
	}

	portTable[event->dst_port] +=  event->packet_count;
	portTableAll[event->dst_port] +=  event->packet_count;

	//Checks for the max to avoid iterating through the entire table every update
	//Since number of ports can grow very large during a scan this will distribute the computation more evenly
	//Since the IP will tend to be relatively small compared to number of events, it's max is found during the call.
	if(portTableAll[event->dst_port] > portMax)
	{
		portMax = portTableAll[event->dst_port];
	}
	if(packet_times.size() > 1)
	{
		for(uint i = 0; i < event->IP_packet_sizes.size(); i++)
		{
			packTable[event->IP_packet_sizes[i]]++;
			packTableAll[event->IP_packet_sizes[i]]++;
			packet_intervals.push_back(event->packet_intervals[i] - packet_times[packet_times.size()-1]);
			packet_times.push_back(event->packet_intervals[i]);

		}
	}
	else
	{
		for(uint i = 0; i < event->IP_packet_sizes.size(); i++)
		{
			packTable[event->IP_packet_sizes[i]]++;
			packTableAll[event->IP_packet_sizes[i]]++;
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
		haystackEventsAll++;
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

	//Total interval time for this feature set
	time_t totalInterval = endTime - startTime;

	//Required, individual variables for calculation
	memcpy(buf+offset, &totalInterval, size);
	offset += size;
	memcpy(buf+offset, &haystackEvents, size);
	offset += size;
	memcpy(buf+offset, &packetCount, size);
	offset += size;
	memcpy(buf+offset, &bytesTotal, size);
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
		uint temp = it->first;
		memcpy(buf+offset, &temp, size);
		offset += size;
		memcpy(buf+offset, &it->second, size);
		offset += size;
	}

	return offset;
}

uint FeatureSet::deserializeFeatureData(u_char *buf, in_addr_t hostAddr)
{
	uint offset = 0;
	//Bytes in a word, used for everything but port #'s
	uint size = 4;

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

		SAData.packTable[(int)temp] = tempCount;
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
		SAData.IPTable[(in_addr_t)temp] = tempCount;
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
		SAData.portTable[temp] = tempCount;
		i += tempCount;

	}

	//If this host has previous data from this sender, remove the old information
	if(SATable[hostAddr].packetCount != 0)
	{
		silentAlarmFeatureData * host = &SATable[hostAddr];

		packetCountAll -= host->packetCount;
		bytesTotalAll -= host->bytesTotal;
		haystackEventsAll -= host->haystackEvents;
		totalIntervalAll -= host->totalInterval;

		for(Packet_Table::iterator it = host->packTable.begin(); it != host->packTable.end(); it++)
		{
			packTableAll[it->first] -= it->second;
		}

		for(IP_Table::iterator it = host->IPTable.begin(); it != host->IPTable.end(); it++)
		{
			IPTableAll[it->first] -= it->second;
		}

		for(Port_Table::iterator it = host->portTable.begin(); it != host->portTable.end(); it++)
		{
			portTableAll[it->first] -= it->second;
		}
	}

	//Include the new information

	packetCountAll += SAData.packetCount;
	bytesTotalAll += SAData.bytesTotal;
	haystackEventsAll += SAData.haystackEvents;
	totalIntervalAll += SAData.totalInterval;

	for(Packet_Table::iterator it = SAData.packTable.begin(); it != SAData.packTable.end(); it++)
	{
		if(packTableAll.count(it->first) == 0)
			packTableAll[it->first] = it->second;
		else
			packTableAll[it->first] += it->second;
	}
	for(IP_Table::iterator it = SAData.IPTable.begin(); it != SAData.IPTable.end(); it++)
	{
		if(IPTableAll.count(it->first) == 0)
			IPTableAll[it->first] = it->second;
		else
			IPTableAll[it->first] += it->second;
	}
	for(Port_Table::iterator it = SAData.portTable.begin(); it != SAData.portTable.end(); it++)
	{
		if(portTableAll.count(it->first) == 0)
			portTableAll[it->first] = it->second;
		else
			portTableAll[it->first] += it->second;
	}

	//Copy the Data over
	SATable[hostAddr] = SAData;

	return offset;
}

/*void FeatureSet::combineSATables()
{
	packTableAll.clear_no_resize();
	IPTableAll.clear_no_resize();
	portTableAll.clear_no_resize();

	//Put all the SA Data in first
	for(Silent_Alarm_Table::iterator itt = SATable.begin(); itt != SATable.end(); itt++)
	{
		silentAlarmFeatureData * host = &itt->second;

		for(Packet_Table::iterator it = host->packTable.begin(); it != host->packTable.end(); it++)
		{
			if(packTableAll.count(it->first) == 0)
				packTableAll[it->first] = it->second;
			else
				packTableAll[it->first] += it->second;
		}
		for(IP_Table::iterator it = host->IPTable.begin(); it != host->IPTable.end(); it++)
		{
			if(IPTableAll.count(it->first) == 0)
				IPTableAll[it->first] = it->second;
			else
				IPTableAll[it->first] += it->second;
		}
		for(Port_Table::iterator it = host->portTable.begin(); it != host->portTable.end(); it++)
		{
			if(portTableAll.count(it->first) == 0)
				portTableAll[it->first] = it->second;
			else
				portTableAll[it->first] += it->second;
		}
	}

	//Then include the local data
	for(Packet_Table::iterator it = packTable.begin(); it != packTable.end(); it++)
	{
		if(packTableAll.count(it->first) == 0)
			packTableAll[it->first] = it->second;
		else
			packTableAll[it->first] += it->second;
	}
	for(IP_Table::iterator it = IPTable.begin(); it != IPTable.end(); it++)
	{
		if(IPTableAll.count(it->first) == 0)
			IPTableAll[it->first] = it->second;
		else
			IPTableAll[it->first] += it->second;
	}
	for(Port_Table::iterator it = portTable.begin(); it != portTable.end(); it++)
	{
		if(portTableAll.count(it->first) == 0)
			portTableAll[it->first] = it->second;
		else
			portTableAll[it->first] += it->second;
	}
	SADataCurrent = true;
}*/

}
}
