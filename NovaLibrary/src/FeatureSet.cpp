//============================================================================
// Name        : FeatureSet.cpp
// Copyright   : DataSoft Corporation 2011-2012
//	Nova is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//   
//   Nova is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//   
//   You should have received a copy of the GNU General Public License
//   along with Nova.  If not, see <http://www.gnu.org/licenses/>.
// Description : Maintains and calculates distinct features for individual Suspects
//					for use in classification of the Suspect.
//============================================================================/*

#include "FeatureSet.h"
#include "NovaUtil.h"

using namespace std;
namespace Nova{


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


void FeatureSet::CalculateAll(uint32_t featuresEnabled)
{
	CalculateTimeInterval();

	UpdateFeatureData(INCLUDE);

	if(featuresEnabled & IP_TRAFFIC_DISTRIBUTION_MASK)
	{
			Calculate(IP_TRAFFIC_DISTRIBUTION);
	}
	if(featuresEnabled & PORT_TRAFFIC_DISTRIBUTION_MASK)
	{
			Calculate(PORT_TRAFFIC_DISTRIBUTION);
	}
	if(featuresEnabled & HAYSTACK_EVENT_FREQUENCY_MASK)
	{
			Calculate(HAYSTACK_EVENT_FREQUENCY);
	}
	if(featuresEnabled & PACKET_SIZE_MEAN_MASK)
	{
			Calculate(PACKET_SIZE_MEAN);
	}
	if(featuresEnabled & PACKET_SIZE_DEVIATION_MASK)
	{
		if(!(featuresEnabled & PACKET_SIZE_MEAN_MASK))
			Calculate(PACKET_SIZE_MEAN);
		Calculate(PACKET_SIZE_DEVIATION);
	}
	if(featuresEnabled & DISTINCT_IPS_MASK)
	{
			Calculate(DISTINCT_IPS);
	}
	if(featuresEnabled & DISTINCT_PORTS_MASK)
	{
			Calculate(DISTINCT_PORTS);
	}
	if(featuresEnabled & PACKET_INTERVAL_MEAN_MASK)
	{
			Calculate(PACKET_INTERVAL_MEAN);
	}
	if(featuresEnabled & PACKET_INTERVAL_DEVIATION_MASK)
	{
			if(!(featuresEnabled & PACKET_INTERVAL_MEAN_MASK))
				Calculate(PACKET_INTERVAL_MEAN);
			Calculate(PACKET_INTERVAL_DEVIATION);
	}


	UpdateFeatureData(REMOVE);
}


void FeatureSet::Calculate(uint32_t featureDimension)
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
	uint32_t packet_count;
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

	for(uint32_t i = 0; i < IP_packet_sizes.size(); i++)
	{
		packTable[IP_packet_sizes[i]].first++;
	}

	if(last_time != 0)
		intervalTable[packet_intervals[0] - last_time].first++;

	for(uint32_t i = 1; i < packet_intervals.size(); i++)
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


uint32_t FeatureSet::SerializeFeatureSet(u_char * buf)
{
	uint32_t offset = 0;

	//Clears a chunk of the buffer for the FeatureSet
	bzero(buf, (sizeof features[0])*DIM);

	//Copies the value and increases the offset
	for(uint32_t i = 0; i < DIM; i++)
	{
		memcpy(buf+offset, &features[i], sizeof features[i]);
		offset+= sizeof features[i];
	}

	return offset;
}


uint32_t FeatureSet::DeserializeFeatureSet(u_char * buf)
{
	uint32_t offset = 0;

	//Copies the value and increases the offset
	for(uint32_t i = 0; i < DIM; i++)
	{
		memcpy(&features[i], buf+offset, sizeof features[i]);
		offset+= sizeof features[i];
	}

	return offset;
}


uint32_t FeatureSet::SerializeFeatureDataBroadcast(u_char *buf)
{
	uint32_t offset = 0;
	uint32_t count = 0;
	uint32_t table_entries = 0;

	//Required, individual variables for calculation
	CalculateTimeInterval();
	startTime = endTime;
	memcpy(buf+offset, &totalInterval.first, sizeof totalInterval.first);
	totalInterval.second += totalInterval.first;
	totalInterval.first = 0;
	offset += sizeof totalInterval.first;

	memcpy(buf+offset, &haystackEvents.first, sizeof haystackEvents.first);
	haystackEvents.second += haystackEvents.first;
	haystackEvents.first = 0;
	offset += sizeof haystackEvents.first;

	memcpy(buf+offset, &packetCount.first, sizeof packetCount.first);
	packetCount.second += packetCount.first;
	packetCount.first = 0;
	offset += sizeof packetCount.first;

	memcpy(buf+offset, &bytesTotal.first, sizeof bytesTotal.first);
	bytesTotal.second += bytesTotal.first;
	bytesTotal.first = 0;
	offset += sizeof bytesTotal.first;

	memcpy(buf+offset, &portMax, sizeof portMax);
	offset += sizeof portMax;

	//These tables all just place their key followed by the data
	uint32_t tempInt;

	for(Interval_Table::iterator it = intervalTable.begin(); (it != intervalTable.end()) && (count < MAX_TABLE_ENTRIES); it++)
		if(it->second.first)
			count++;

	//The size of the Table
	tempInt = count - table_entries;
	memcpy(buf+offset, &tempInt, sizeof tempInt);
	offset += sizeof tempInt;

	for(Interval_Table::iterator it = intervalTable.begin(); (it != intervalTable.end()) && (table_entries < count); it++)
	{
		if(it->second.first)
		{
			table_entries++;
			memcpy(buf+offset, &it->first, sizeof it->first);
			offset += sizeof it->first;
			memcpy(buf+offset, &it->second.first, sizeof it->second.first);
			offset += sizeof it->second.first;
			intervalTable[it->first].second += it->second.first;
			intervalTable[it->first].first = 0;
		}
	}

	for(Packet_Table::iterator it = packTable.begin(); (it != packTable.end()) && (count < MAX_TABLE_ENTRIES); it++)
		if(it->second.first)
			count++;

	//The size of the Table
	tempInt = count - table_entries;
	memcpy(buf+offset, &tempInt, sizeof tempInt);
	offset += sizeof tempInt;

	for(Packet_Table::iterator it = packTable.begin(); (it != packTable.end()) && (table_entries < count); it++)
	{
		if(it->second.first)
		{
			table_entries++;
			memcpy(buf+offset, &it->first, sizeof it->first);
			offset += sizeof it->first;
			memcpy(buf+offset, &it->second.first, sizeof it->second.first);
			offset += sizeof it->second.first;
			packTable[it->first].second += it->second.first;
			packTable[it->first].first = 0;
		}
	}

	for(IP_Table::iterator it = IPTable.begin(); (it != IPTable.end()) && (count < MAX_TABLE_ENTRIES); it++)
		if(it->second.first)
			count++;

	//The size of the Table
	tempInt = count - table_entries;
	memcpy(buf+offset, &tempInt, sizeof tempInt);
	offset += sizeof tempInt;

	for(IP_Table::iterator it = IPTable.begin(); (it != IPTable.end()) && (table_entries < count); it++)
	{
		if(it->second.first)
		{
			table_entries++;
			memcpy(buf+offset, &it->first, sizeof it->first);
			offset += sizeof it->first;
			memcpy(buf+offset, &it->second.first, sizeof it->second.first);
			offset += sizeof it->second.first;
			IPTable[it->first].second += it->second.first;
			IPTable[it->first].first = 0;
		}
	}

	for(Port_Table::iterator it = portTable.begin(); (it != portTable.end()) && (count < MAX_TABLE_ENTRIES); it++)
		if(it->second.first)
			count++;

	//The size of the Table
	tempInt = count - table_entries;
	memcpy(buf+offset, &tempInt, sizeof tempInt);
	offset += sizeof tempInt;

	for(Port_Table::iterator it = portTable.begin(); (it != portTable.end()) && (table_entries < count); it++)
	{
		if(it->second.first)
		{
			table_entries++;
			memcpy(buf+offset, &it->first, sizeof it->first);
			offset += sizeof it->first;
			memcpy(buf+offset, &it->second.first, sizeof it->second.first);
			offset += sizeof it->second.first;
			portTable[it->first].second += it->second.first;
			portTable[it->first].first = 0;
		}
	}
	return offset;
}


uint32_t FeatureSet::DeserializeFeatureDataBroadcast(u_char *buf)
{
	uint32_t offset = 0;

	//Bytes in a word, used for everything but port #'s
	uint32_t table_size = 0;

	//Temporary variables to store and track data during deserialization
	uint32_t temp = 0;
	uint32_t tempCount = 0;
	in_port_t port = 0;

	//Required, individual variables for calculation
	memcpy(&temp, buf+offset, sizeof bytesTotal.first);
	totalInterval.second += temp;
	offset += sizeof bytesTotal.first;

	memcpy(&temp, buf+offset, sizeof bytesTotal.first);
	haystackEvents.second += temp;
	offset += sizeof bytesTotal.first;

	memcpy(&temp, buf+offset, sizeof bytesTotal.first);
	packetCount.second += temp;
	offset += sizeof bytesTotal.first;

	memcpy(&temp, buf+offset, sizeof bytesTotal.first);
	bytesTotal.second += temp;
	offset += sizeof bytesTotal.first;

	memcpy(&temp, buf+offset, sizeof bytesTotal.first);
	offset += sizeof bytesTotal.first;

	if(temp > portMax)
		portMax = temp;

	/***************************************************************************************************
	* For all of these tables we extract, the key (bin identifier) followed by the data (packet count)
	*  i += the # of packets in the bin, if we haven't reached packet count we know there's another item
	****************************************************************************************************/

	memcpy(&table_size, buf+offset, sizeof table_size);
	offset += sizeof table_size;

	//Packet interval table
	for(uint32_t i = 0; i < table_size;)
	{
		memcpy(&temp, buf+offset, sizeof temp);
		offset += sizeof temp;
		memcpy(&tempCount, buf+offset, sizeof tempCount);
		offset += sizeof tempCount;
		intervalTable[temp].second += tempCount;
		i++;
	}

	memcpy(&table_size, buf+offset, sizeof table_size);
	offset += sizeof table_size;

	//Packet size table
	for(uint32_t i = 0; i < table_size;)
	{
		memcpy(&temp, buf+offset, sizeof temp);
		offset += sizeof temp;
		memcpy(&tempCount, buf+offset, sizeof tempCount);
		offset += sizeof tempCount;
		packTable[temp].second += tempCount;
		i++;
	}

	memcpy(&table_size, buf+offset, sizeof table_size);
	offset += sizeof table_size;

	//IP table
	for(uint32_t i = 0; i < table_size;)
	{
		memcpy(&temp, buf+offset, sizeof temp);
		offset += sizeof temp;
		memcpy(&tempCount, buf+offset, sizeof tempCount);
		offset += sizeof tempCount;
		IPTable[temp].second += tempCount;
		i++;
	}

	memcpy(&table_size, buf+offset, sizeof table_size);
	offset += sizeof table_size;

	//Port table
	for(uint32_t i = 0; i < table_size;)
	{
		memcpy(&port, buf+offset, sizeof port);
		offset += sizeof port;
		memcpy(&tempCount, buf+offset, sizeof tempCount);
		offset += sizeof tempCount;
		portTable[port].second += tempCount;
		i++;
	}

	return offset;
}


uint32_t FeatureSet::SerializeFeatureDataLocal(u_char *buf)
{
	uint32_t offset = 0;
	uint32_t count = 0;
	uint32_t table_entries = 0;

	//Required, individual variables for calculation
	CalculateTimeInterval();
	startTime = endTime;
	memcpy(buf+offset, &totalInterval.first, sizeof totalInterval.first);
	totalInterval.first = 0;
	offset += sizeof totalInterval.first;

	memcpy(buf+offset, &haystackEvents.first, sizeof haystackEvents.first);
	haystackEvents.first = 0;
	offset += sizeof haystackEvents.first;

	memcpy(buf+offset, &packetCount.first, sizeof packetCount.first);
	packetCount.first = 0;
	offset += sizeof packetCount.first;

	memcpy(buf+offset, &bytesTotal.first, sizeof bytesTotal.first);
	bytesTotal.first = 0;
	offset += sizeof bytesTotal.first;

	memcpy(buf+offset, &portMax, sizeof portMax);
	offset += sizeof portMax;

	//These tables all just place their key followed by the data
	uint32_t tempInt;

	for(Interval_Table::iterator it = intervalTable.begin(); (it != intervalTable.end()) && (count < MAX_TABLE_ENTRIES); it++)
		if(it->second.first)
			count++;

	//The size of the Table
	tempInt = count - table_entries;
	memcpy(buf+offset, &tempInt, sizeof tempInt);
	offset += sizeof tempInt;

	for(Interval_Table::iterator it = intervalTable.begin(); (it != intervalTable.end()) && (table_entries < count); it++)
	{
		if(it->second.first)
		{
			table_entries++;
			memcpy(buf+offset, &it->first, sizeof it->first);
			offset += sizeof it->first;
			memcpy(buf+offset, &it->second.first, sizeof it->second.first);
			offset += sizeof it->second.first;
			intervalTable[it->first].first = 0;
		}
	}

	for(Packet_Table::iterator it = packTable.begin(); (it != packTable.end()) && (count < MAX_TABLE_ENTRIES); it++)
		if(it->second.first)
			count++;

	//The size of the Table
	tempInt = count - table_entries;
	memcpy(buf+offset, &tempInt, sizeof tempInt);
	offset += sizeof tempInt;

	for(Packet_Table::iterator it = packTable.begin(); (it != packTable.end()) && (table_entries < count); it++)
	{
		if(it->second.first)
		{
			table_entries++;
			memcpy(buf+offset, &it->first, sizeof it->first);
			offset += sizeof it->first;
			memcpy(buf+offset, &it->second.first, sizeof it->second.first);
			offset += sizeof it->second.first;
			packTable[it->first].first = 0;
		}
	}

	for(IP_Table::iterator it = IPTable.begin(); (it != IPTable.end()) && (count < MAX_TABLE_ENTRIES); it++)
		if(it->second.first)
			count++;

	//The size of the Table
	tempInt = count - table_entries;
	memcpy(buf+offset, &tempInt, sizeof tempInt);
	offset += sizeof tempInt;

	for(IP_Table::iterator it = IPTable.begin(); (it != IPTable.end()) && (table_entries < count); it++)
	{
		if(it->second.first)
		{
			table_entries++;
			memcpy(buf+offset, &it->first, sizeof it->first);
			offset += sizeof it->first;
			memcpy(buf+offset, &it->second.first, sizeof it->second.first);
			offset += sizeof it->second.first;
			IPTable[it->first].first = 0;
		}
	}

	for(Port_Table::iterator it = portTable.begin(); (it != portTable.end()) && (count < MAX_TABLE_ENTRIES); it++)
		if(it->second.first)
			count++;

	//The size of the Table
	tempInt = count - table_entries;
	memcpy(buf+offset, &tempInt, sizeof tempInt);
	offset += sizeof tempInt;

	for(Port_Table::iterator it = portTable.begin(); (it != portTable.end()) && (table_entries < count); it++)
	{
		if(it->second.first)
		{
			table_entries++;
			memcpy(buf+offset, &it->first, sizeof it->first);
			offset += sizeof it->first;
			memcpy(buf+offset, &it->second.first, sizeof it->second.first);
			offset += sizeof it->second.first;
			portTable[it->first].first = 0;
		}
	}
	return offset;
}


uint32_t FeatureSet::DeserializeFeatureDataLocal(u_char *buf)
{
	uint32_t offset = 0;

	//Bytes in a word, used for everything but port #'s
	uint32_t table_size = 0;

	//Temporary variables to store and track data during deserialization
	uint32_t temp = 0;
	uint32_t tempCount = 0;
	in_port_t port = 0;

	//Required, individual variables for calculation
	memcpy(&temp, buf+offset, sizeof totalInterval.first);
	totalInterval.first += temp;
	offset += sizeof totalInterval.first;

	memcpy(&temp, buf+offset, sizeof haystackEvents.first);
	haystackEvents.first += temp;
	offset += sizeof haystackEvents.first;

	memcpy(&temp, buf+offset, sizeof packetCount.first);
	packetCount.first += temp;
	offset += sizeof packetCount.first;

	memcpy(&temp, buf+offset, sizeof bytesTotal.first);
	bytesTotal.first += temp;
	offset += sizeof bytesTotal.first;

	memcpy(&temp, buf+offset, sizeof portMax);
	offset += sizeof portMax;

	if(temp > portMax)
		portMax = temp;

	/***************************************************************************************************
	* For all of these tables we extract, the key (bin identifier) followed by the data (packet count)
	*  i += the # of packets in the bin, if we haven't reached packet count we know there's another item
	****************************************************************************************************/

	memcpy(&table_size, buf+offset, sizeof table_size);
	offset += sizeof table_size;

	//Packet interval table
	for(uint32_t i = 0; i < table_size;)
	{
		memcpy(&temp, buf+offset, sizeof temp);
		offset += sizeof temp;
		memcpy(&tempCount, buf+offset, sizeof tempCount);
		offset += sizeof tempCount;
		intervalTable[temp].first += tempCount;
		i++;
	}

	memcpy(&table_size, buf+offset, sizeof table_size);
	offset += sizeof table_size;

	//Packet size table
	for(uint32_t i = 0; i < table_size;)
	{
		memcpy(&temp, buf+offset, sizeof temp);
		offset += sizeof temp;
		memcpy(&tempCount, buf+offset, sizeof tempCount);
		offset += sizeof tempCount;
		packTable[temp].first += tempCount;
		i++;
	}

	memcpy(&table_size, buf+offset, sizeof table_size);
	offset += sizeof table_size;

	//IP table
	for(uint32_t i = 0; i < table_size;)
	{
		memcpy(&temp, buf+offset, sizeof temp);
		offset += sizeof temp;
		memcpy(&tempCount, buf+offset, sizeof tempCount);
		offset += sizeof tempCount;
		IPTable[temp].first += tempCount;
		i++;
	}

	memcpy(&table_size, buf+offset, sizeof table_size);
	offset += sizeof table_size;

	//Port table
	for(uint32_t i = 0; i < table_size;)
	{
		memcpy(&port, buf+offset, sizeof port);
		offset += sizeof port;
		memcpy(&tempCount, buf+offset, sizeof tempCount);
		offset += sizeof tempCount;
		portTable[port].first += tempCount;
		i++;
	}

	return offset;
}

}
