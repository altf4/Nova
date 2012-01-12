//============================================================================
// Name        : FeatureSet.cpp
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Maintains and calculates distinct features for individual Suspects
//					for use in classification of the Suspect.
//============================================================================/*

#include "FeatureSet.h"
#include "NovaUtil.h"

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

	intervalTable.resize(INIT_SIZE_SMALL);
	IPTable.resize(INIT_SIZE_SMALL);
	portTable.resize(INIT_SIZE_MEDIUM);
	packTable.resize(INIT_SIZE_LARGE);

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
	for(int i = 0; i < DIM; i++)
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
	for(int i = 0; i < DIM; i++)
	{
		features[i] = 0;
	}
}

//Calculates all features in the feature set
void FeatureSet::CalculateAll(uint32_t featuresEnabled)
{
	CalculateTimeInterval();

	UpdateFeatureData(INCLUDE);

	if(featuresEnabled & IP_TRAFFIC_DISTRIBUTION_MASK)
	{
			calculate(IP_TRAFFIC_DISTRIBUTION);
	}
	if(featuresEnabled & PORT_TRAFFIC_DISTRIBUTION_MASK)
	{
			calculate(PORT_TRAFFIC_DISTRIBUTION);
	}
	if(featuresEnabled & HAYSTACK_EVENT_FREQUENCY_MASK)
	{
			calculate(HAYSTACK_EVENT_FREQUENCY);
	}
	if(featuresEnabled & PACKET_SIZE_MEAN_MASK)
	{
			calculate(PACKET_SIZE_MEAN);
	}
	if(featuresEnabled & PACKET_SIZE_DEVIATION_MASK)
	{
		if(!(featuresEnabled & PACKET_SIZE_MEAN_MASK))
			calculate(PACKET_SIZE_MEAN);
		calculate(PACKET_SIZE_DEVIATION);
	}
	if(featuresEnabled & DISTINCT_IPS_MASK)
	{
			calculate(DISTINCT_IPS);
	}
	if(featuresEnabled & DISTINCT_PORTS_MASK)
	{
			calculate(DISTINCT_PORTS);
	}
	if(featuresEnabled & PACKET_INTERVAL_MEAN_MASK)
	{
			calculate(PACKET_INTERVAL_MEAN);
	}
	if(featuresEnabled & PACKET_INTERVAL_DEVIATION_MASK)
	{
			if(!(featuresEnabled & PACKET_INTERVAL_MEAN_MASK))
				calculate(PACKET_INTERVAL_MEAN);
			calculate(PACKET_INTERVAL_DEVIATION);
	}


	UpdateFeatureData(REMOVE);
}

void FeatureSet::calculate(uint featureDimension)
{
	switch (featureDimension)
	{

	///The traffic distribution across the haystacks relative to host traffic
	case IP_TRAFFIC_DISTRIBUTION:
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
		break;
	}


	///The traffic distribution across ports contacted
	case PORT_TRAFFIC_DISTRIBUTION:
	{
		features[PORT_TRAFFIC_DISTRIBUTION] = 0;
		for (Port_Table::iterator it = portTable.begin() ; it != portTable.end(); it++)
		{
			features[PORT_TRAFFIC_DISTRIBUTION] += ((double)it->second.second / (double)portMax);
		}

		features[PORT_TRAFFIC_DISTRIBUTION] = features[PORT_TRAFFIC_DISTRIBUTION] / (double)portTable.size();
		break;
	}

	///Number of ScanEvents that the suspect is responsible for per second
	case HAYSTACK_EVENT_FREQUENCY:
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
		break;
	}


	///Measures the distribution of packet sizes
	case PACKET_SIZE_MEAN:
	{
		features[PACKET_SIZE_MEAN] = (double)bytesTotal.second / (double)packetCount.second;
		break;
	}


	///Measures the distribution of packet sizes
	case PACKET_SIZE_DEVIATION:
	{
		double count = packetCount.second;
		double mean = 0;
		double variance = 0;
		//Calculate mean
		mean = features[PACKET_SIZE_MEAN];

		//Calculate variance
		for(Packet_Table::iterator it = packTable.begin() ; it != packTable.end(); it++)
		{
			// number of packets multiplied by (packet_size - mean)^2 divided by count
			variance += (it->second.second * pow((it->first - mean), 2))/ count;
		}

		features[PACKET_SIZE_DEVIATION] = sqrt(variance);
		break;
	}


	/// Number of distinct IP addresses contacted
	case DISTINCT_IPS:
	{
		features[DISTINCT_IPS] = IPTable.size();
		break;
	}


	/// Number of distinct ports contacted
	case DISTINCT_PORTS:
	{
		features[DISTINCT_PORTS] =  portTable.size();
		break;
	}

	///Measures the distribution of intervals between packets
	case PACKET_INTERVAL_MEAN:
	{
		features[PACKET_INTERVAL_MEAN] = (((double) totalInterval.second)
								/ ((double) (packetCount.second)));
		break;
	}


	///Measures the distribution of intervals between packets
	case PACKET_INTERVAL_DEVIATION:
	{
		double totalCount = packetCount.second;
		double mean = 0;
		double variance = 0;

		mean = features[PACKET_INTERVAL_MEAN];

		for (Interval_Table::iterator it = intervalTable.begin() ; it != intervalTable.end(); it++)
		{
			variance += it->second.second*(pow((it->first - mean), 2)/totalCount);
		}

		features[PACKET_INTERVAL_DEVIATION] = sqrt(variance);
		break;
	}

	default:
	{
		break;
	}

	}
}

///Calculates the total interval for time based features using latest timestamps
void FeatureSet::CalculateTimeInterval()
{
	if(endTime > startTime)
	{
		totalInterval.first = endTime - startTime;
	}
}




void FeatureSet::UpdateEvidence(Packet packet)
{
	in_port_t dst_port;
	uint packet_count;
	vector <int> IP_packet_sizes;
	vector <time_t> packet_intervals;
	struct ip *ip_hdr = &packet.ip_hdr;
	if(ip_hdr == NULL)
	{
		return;
	}
	//Start and end times are the same since this is a one packet event
	packet_count = 1;

	//If UDP
	if(packet.ip_hdr.ip_p == 17)
	{
		dst_port = ntohs(packet.udp_hdr.dest);
	}
	//If ICMP
	else if(packet.ip_hdr.ip_p == 1)
	{
		dst_port = -1;
	}
	// If TCP
	else if (packet.ip_hdr.ip_p == 6)
	{
		dst_port =  ntohs(packet.tcp_hdr.dest);
	}

	IP_packet_sizes.push_back(ntohs(packet.ip_hdr.ip_len));
	packet_intervals.push_back(packet.pcap_header.ts.tv_sec);

	packetCount.first += packet_count;
	bytesTotal.first += ntohs(packet.ip_hdr.ip_len);;

	//If from haystack
	if(packet.fromHaystack)
	{
		IPTable[packet.ip_hdr.ip_dst.s_addr].first += packet_count;
		haystackEvents.first++;
	}
	//Else from a host
	else
	{
		//Put the packet count into a bin that is never used so that
		// all host events for a suspect go into the same bin
		IPTable[1].first +=  packet_count;
	}

	portTable[dst_port].first += packet_count;

	//Checks for the max to avoid iterating through the entire table every update
	//Since number of ports can grow very large during a scan this will distribute the computation more evenly
	//Since the IP will tend to be relatively small compared to number of events, it's max is found during the call.
	if(portTable[dst_port].first > portMax)
	{
		portMax = portTable[dst_port].first;
	}

	for(uint i = 0; i < IP_packet_sizes.size(); i++)
	{
		packTable[IP_packet_sizes[i]].first++;
	}

	if(last_time != 0)
		intervalTable[packet_intervals[0] - last_time].first++;

	for(uint i = 1; i < packet_intervals.size(); i++)
	{
		intervalTable[packet_intervals[i] - packet_intervals[i-1]].first++;
	}
	last_time = packet_intervals.back();

	//Accumulate to find the lowest Start time and biggest end time.
	if(packet.pcap_header.ts.tv_sec < startTime)
	{
		startTime = packet.pcap_header.ts.tv_sec;
	}
	if( packet.pcap_header.ts.tv_sec > endTime)
	{
		endTime =  packet.pcap_header.ts.tv_sec;
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
	bzero(buf, size*DIM);

	//Copies the value and increases the offset
	for(uint i = 0; i < DIM; i++)
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
	for(uint i = 0; i < DIM; i++)
	{
		memcpy(&features[i], buf+offset, size);
		offset+= size;
	}

	return offset;
}

uint FeatureSet::serializeFeatureDataBroadcast(u_char *buf)
{
	uint offset = 0;
	uint count = 0;
	uint table_entries = 0;

	//Bytes in a word, used for everything but port #'s
	const uint size = 4;

	//Required, individual variables for calculation
	CalculateTimeInterval();
	startTime = endTime;
	memcpy(buf+offset, &totalInterval.first, size);
	totalInterval.second += totalInterval.first;
	totalInterval.first = 0;
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

uint FeatureSet::serializeFeatureDataLocal(u_char *buf)
{
	uint offset = 0;
	uint count = 0;
	uint table_entries = 0;

	//Bytes in a word, used for everything but port #'s
	const uint size = 4;

	//Required, individual variables for calculation
	CalculateTimeInterval();
	startTime = endTime;
	memcpy(buf+offset, &totalInterval.first, size);
	totalInterval.first = 0;
	offset += size;

	memcpy(buf+offset, &haystackEvents.first, size);
	haystackEvents.first = 0;
	offset += size;

	memcpy(buf+offset, &packetCount.first, size);
	packetCount.first = 0;
	offset += size;

	memcpy(buf+offset, &bytesTotal.first, size);
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

}
