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
	IPTable.set_empty_key(0);
	portTable.set_empty_key(0);
	packTable.set_empty_key(0);
	intervalTable.set_empty_key(2147483647);

	ClearFeatureSet();

	intervalTable.resize(INIT_SIZE_SMALL);
	IPTable.resize(INIT_SIZE_SMALL);
	portTable.resize(INIT_SIZE_MEDIUM);
	packTable.resize(INIT_SIZE_LARGE);
}


void FeatureSet::ClearFeatureSet()
{
	//Temp variables
	startTime = 2147483647; //2^31 - 1 (IE: Y2.038K bug) (IE: The largest standard Unix time value)
	endTime = 0;
	totalInterval = 0;

	IPTable.clear();
	portTable.clear();
	packTable.clear();
	intervalTable.clear();

	haystackEvents = 0;
	packetCount = 0;
	bytesTotal = 0;
	last_time = 0;

	portMax = 0;
	//Features
	for(int i = 0; i < DIM; i++)
		features[i] = 0;
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

		//Max packet count to an IP, used for normalizing
		uint32_t IPMax = 0;
		for (IP_Table::iterator it = IPTable.begin() ; it != IPTable.end(); it++)
		{
			if(it->second > IPMax)
				IPMax = it->second;
		}
		for (IP_Table::iterator it = IPTable.begin() ; it != IPTable.end(); it++)
		{
			features[IP_TRAFFIC_DISTRIBUTION] += ((double)it->second / (double)IPMax);
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
			features[PORT_TRAFFIC_DISTRIBUTION] += ((double)it->second / (double)portMax);
		}

		features[PORT_TRAFFIC_DISTRIBUTION] = features[PORT_TRAFFIC_DISTRIBUTION] / (double)portTable.size();
		break;
	}

	///Number of ScanEvents that the suspect is responsible for per second
	case HAYSTACK_EVENT_FREQUENCY:
	{
		// if > 0, .second is a time_t(uint) sum of all intervals across all nova instances
		if(totalInterval)
		{
			features[HAYSTACK_EVENT_FREQUENCY] = ((double)(haystackEvents)) / (double)(totalInterval);
		}
		else
		{
			//If interval is 0, no time based information, use a default of 1 for the interval
			features[HAYSTACK_EVENT_FREQUENCY] = (double)(haystackEvents);
		}
		break;
	}


	///Measures the distribution of packet sizes
	case PACKET_SIZE_MEAN:
	{
		features[PACKET_SIZE_MEAN] = (double)bytesTotal / (double)packetCount;
		break;
	}


	///Measures the distribution of packet sizes
	case PACKET_SIZE_DEVIATION:
	{
		double count = packetCount;
		double mean = 0;
		double variance = 0;
		//Calculate mean
		mean = features[PACKET_SIZE_MEAN];

		//Calculate variance
		for(Packet_Table::iterator it = packTable.begin() ; it != packTable.end(); it++)
		{
			// number of packets multiplied by (packet_size - mean)^2 divided by count
			variance += (it->second * pow((it->first - mean), 2))/ count;
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
		features[PACKET_INTERVAL_MEAN] = (((double) totalInterval)
								/ ((double) (packetCount)));
		break;
	}


	///Measures the distribution of intervals between packets
	case PACKET_INTERVAL_DEVIATION:
	{
		double totalCount = packetCount;
		double mean = 0;
		double variance = 0;

		mean = features[PACKET_INTERVAL_MEAN];

		for (Interval_Table::iterator it = intervalTable.begin() ; it != intervalTable.end(); it++)
		{
			variance += it->second*(pow((it->first - mean), 2)/totalCount);
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
		unsentData->totalInterval = endTime - startTime;
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

	unsentData->packetCount += packet_count;
	unsentData->bytesTotal += ntohs(packet.ip_hdr.ip_len);;

	//If from haystack
	if(packet.fromHaystack)
	{
		unsentData->IPTable[packet.ip_hdr.ip_dst.s_addr] += packet_count;
		unsentData->haystackEvents++;
	}
	//Else from a host
	else
	{
		//Put the packet count into a bin that is never used so that
		// all host events for a suspect go into the same bin
		unsentData->IPTable[1] +=  packet_count;
	}

	unsentData->portTable[dst_port] += packet_count;

	//Checks for the max to avoid iterating through the entire table every update
	//Since number of ports can grow very large during a scan this will distribute the computation more evenly
	//Since the IP will tend to be relatively small compared to number of events, it's max is found during the call.
	if(unsentData->portTable[dst_port] > portMax)
	{
		portMax = unsentData->portTable[dst_port];
	}

	for(uint32_t i = 0; i < IP_packet_sizes.size(); i++)
	{
		unsentData->packTable[IP_packet_sizes[i]]++;
	}

	if(last_time != 0)
		unsentData->intervalTable[packet_intervals[0] - last_time]++;

	for(uint32_t i = 1; i < packet_intervals.size(); i++)
	{
		unsentData->intervalTable[packet_intervals[i] - packet_intervals[i-1]]++;
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

FeatureSet& FeatureSet::operator+=(FeatureSet &rhs) {
	totalInterval += rhs.totalInterval;
	packetCount += rhs.packetCount;
	bytesTotal += rhs.bytesTotal;
	haystackEvents += rhs.haystackEvents;

	for(IP_Table::iterator it = rhs.IPTable.begin(); it != rhs.IPTable.end(); it++)
		IPTable[it->first] += rhs.IPTable[it->first];

	for(Port_Table::iterator it = rhs.portTable.begin(); it != rhs.portTable.end(); it++)
		portTable[it->first] += rhs.portTable[it->first];

	for(Packet_Table::iterator it = rhs.packTable.begin(); it != rhs.packTable.end(); it++)
		packTable[it->first] += rhs.packTable[it->first];

	for(Interval_Table::iterator it = rhs.intervalTable.begin(); it != rhs.intervalTable.end(); it++)
		intervalTable[it->first] += rhs.intervalTable[it->first];

	return *this;
}

FeatureSet& FeatureSet::operator-=(FeatureSet &rhs) {
	totalInterval -= rhs.totalInterval;
	packetCount -= rhs.packetCount;
	bytesTotal -= rhs.bytesTotal;
	haystackEvents -= rhs.haystackEvents;

	for(IP_Table::iterator it = rhs.IPTable.begin(); it != rhs.IPTable.end(); it++)
		IPTable[it->first] -= rhs.IPTable[it->first];

	for(Port_Table::iterator it = rhs.portTable.begin(); it != rhs.portTable.end(); it++)
		portTable[it->first] -= rhs.portTable[it->first];

	for(Packet_Table::iterator it = rhs.packTable.begin(); it != rhs.packTable.end(); it++)
		packTable[it->first] -= rhs.packTable[it->first];

	for(Interval_Table::iterator it = rhs.intervalTable.begin(); it != rhs.intervalTable.end(); it++)
		intervalTable[it->first] -= rhs.intervalTable[it->first];

	return *this;
}

void FeatureSet::UpdateFeatureData(bool include)
{
	if(include)
		*this += *unsentData;
	else
		*this -= *unsentData;
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


uint32_t FeatureSet::SerializeFeatureData(u_char *buf, bool isBroadcast)
{
	uint32_t offset = 0;
	uint32_t count = 0;
	uint32_t table_entries = 0;

	//Required, individual variables for calculation
	CalculateTimeInterval();
	startTime = endTime;
	memcpy(buf+offset, &unsentData->totalInterval, sizeof unsentData->totalInterval);
	if (isBroadcast)
		totalInterval += unsentData->totalInterval;
	unsentData->totalInterval = 0;
	offset += sizeof unsentData->totalInterval;

	memcpy(buf+offset, &unsentData->haystackEvents, sizeof unsentData->haystackEvents);
	if (isBroadcast)
		haystackEvents += unsentData->haystackEvents;
	unsentData->haystackEvents = 0;
	offset += sizeof unsentData->haystackEvents;

	memcpy(buf+offset, &unsentData->packetCount, sizeof unsentData->packetCount);
	if (isBroadcast)
		packetCount += unsentData->packetCount;
	unsentData->packetCount = 0;
	offset += sizeof unsentData->packetCount;

	memcpy(buf+offset, &unsentData->bytesTotal, sizeof unsentData->bytesTotal);
	if (isBroadcast)
		bytesTotal += unsentData->bytesTotal;
	unsentData->bytesTotal = 0;
	offset += sizeof unsentData->bytesTotal;

	memcpy(buf+offset, &portMax, sizeof portMax);
	offset += sizeof portMax;

	//These tables all just place their key followed by the data
	uint32_t tempInt;

	for(Interval_Table::iterator it = unsentData->intervalTable.begin(); (it != unsentData->intervalTable.end()) && (count < MAX_TABLE_ENTRIES); it++)
		if(it->second)
			count++;

	//The size of the Table
	tempInt = count - table_entries;
	memcpy(buf+offset, &tempInt, sizeof tempInt);
	offset += sizeof tempInt;

	for(Interval_Table::iterator it = unsentData->intervalTable.begin(); (it != unsentData->intervalTable.end()) && (table_entries < count); it++)
	{
		if(it->second)
		{
			table_entries++;
			memcpy(buf+offset, &it->first, sizeof it->first);
			offset += sizeof it->first;
			memcpy(buf+offset, &it->second, sizeof it->second);
			offset += sizeof it->second;
			if (isBroadcast)
				intervalTable[it->first] += it->second;
			unsentData->intervalTable[it->first] = 0;
		}
	}

	for(Packet_Table::iterator it = unsentData->packTable.begin(); (it != unsentData->packTable.end()) && (count < MAX_TABLE_ENTRIES); it++)
		if(it->second)
			count++;

	//The size of the Table
	tempInt = count - table_entries;
	memcpy(buf+offset, &tempInt, sizeof tempInt);
	offset += sizeof tempInt;

	for(Packet_Table::iterator it = unsentData->packTable.begin(); (it != unsentData->packTable.end()) && (table_entries < count); it++)
	{
		if(it->second)
		{
			table_entries++;
			memcpy(buf+offset, &it->first, sizeof it->first);
			offset += sizeof it->first;
			memcpy(buf+offset, &it->second, sizeof it->second);
			offset += sizeof it->second;
			if (isBroadcast)
				packTable[it->first] += it->second;
			unsentData->packTable[it->first] = 0;
		}
	}

	for(IP_Table::iterator it = unsentData->IPTable.begin(); (it != unsentData->IPTable.end()) && (count < MAX_TABLE_ENTRIES); it++)
		if(it->second)
			count++;

	//The size of the Table
	tempInt = count - table_entries;
	memcpy(buf+offset, &tempInt, sizeof tempInt);
	offset += sizeof tempInt;

	for(IP_Table::iterator it = unsentData->IPTable.begin(); (it != unsentData->IPTable.end()) && (table_entries < count); it++)
	{
		if(it->second)
		{
			table_entries++;
			memcpy(buf+offset, &it->first, sizeof it->first);
			offset += sizeof it->first;
			memcpy(buf+offset, &it->second, sizeof it->second);
			offset += sizeof it->second;
			if (isBroadcast)
				IPTable[it->first] += it->second;
			unsentData->IPTable[it->first] = 0;
		}
	}

	for(Port_Table::iterator it = unsentData->portTable.begin(); (it != unsentData->portTable.end()) && (count < MAX_TABLE_ENTRIES); it++)
		if(it->second)
			count++;

	//The size of the Table
	tempInt = count - table_entries;
	memcpy(buf+offset, &tempInt, sizeof tempInt);
	offset += sizeof tempInt;

	for(Port_Table::iterator it = unsentData->portTable.begin(); (it != unsentData->portTable.end()) && (table_entries < count); it++)
	{
		if(it->second)
		{
			table_entries++;
			memcpy(buf+offset, &it->first, sizeof it->first);
			offset += sizeof it->first;
			memcpy(buf+offset, &it->second, sizeof it->second);
			offset += sizeof it->second;
			if (isBroadcast)
				portTable[it->first] += it->second;
			unsentData->portTable[it->first] = 0;
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
	memcpy(&temp, buf+offset, sizeof unsentData->totalInterval);
	totalInterval += temp;
	offset += sizeof unsentData->totalInterval;

	memcpy(&temp, buf+offset, sizeof unsentData->haystackEvents);
	haystackEvents += temp;
	offset += sizeof unsentData->haystackEvents;

	memcpy(&temp, buf+offset, sizeof unsentData->packetCount);
	packetCount += temp;
	offset += sizeof unsentData->packetCount;

	memcpy(&temp, buf+offset, sizeof unsentData->bytesTotal);
	bytesTotal += temp;
	offset += sizeof unsentData->bytesTotal;

	memcpy(&temp, buf+offset, sizeof unsentData->portMax);
	offset += sizeof unsentData->portMax;

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
		intervalTable[temp] += tempCount;
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
		packTable[temp] += tempCount;
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
		IPTable[temp] += tempCount;
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
		portTable[port] += tempCount;
		i++;
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
	memcpy(&temp, buf+offset, sizeof unsentData->totalInterval);
	unsentData->totalInterval += temp;
	offset += sizeof unsentData->totalInterval;

	memcpy(&temp, buf+offset, sizeof unsentData->haystackEvents);
	unsentData->haystackEvents += temp;
	offset += sizeof unsentData->haystackEvents;

	memcpy(&temp, buf+offset, sizeof unsentData->packetCount);
	unsentData->packetCount += temp;
	offset += sizeof unsentData->packetCount;

	memcpy(&temp, buf+offset, sizeof unsentData->bytesTotal);
	unsentData->bytesTotal += temp;
	offset += sizeof unsentData->bytesTotal;

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
		unsentData->intervalTable[temp] += tempCount;
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
		unsentData->packTable[temp] += tempCount;
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
		unsentData->IPTable[temp] += tempCount;
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
		unsentData->portTable[port] += tempCount;
		i++;
	}

	return offset;
}

}
