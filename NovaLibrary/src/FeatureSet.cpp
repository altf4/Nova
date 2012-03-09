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
#include "Config.h"
#include <math.h>
#include <sys/un.h>

using namespace std;
namespace Nova{


FeatureSet::FeatureSet()
{
	m_IPTable.set_empty_key(0);
	m_portTable.set_empty_key(0);
	m_packTable.set_empty_key(0);
	m_intervalTable.set_empty_key(2147483647);

	ClearFeatureSet();

	m_intervalTable.resize(INIT_SIZE_SMALL);
	m_IPTable.resize(INIT_SIZE_SMALL);
	m_portTable.resize(INIT_SIZE_MEDIUM);
	m_packTable.resize(INIT_SIZE_LARGE);
}

FeatureSet::~FeatureSet()
{

}


void FeatureSet::ClearFeatureSet()
{
	//Temp variables
	m_startTime = 2147483647; //2^31 - 1 (IE: Y2.038K bug) (IE: The largest standard Unix time value)
	m_endTime = 0;
	m_totalInterval = 0;

	m_IPTable.clear();
	m_portTable.clear();
	m_packTable.clear();
	m_intervalTable.clear();

	m_haystackEvents = 0;
	m_packetCount = 0;
	m_bytesTotal = 0;
	m_last_time = 0;

	m_portMax = 0;
	//Features
	for(int i = 0; i < DIM; i++)
		m_features[i] = 0;
}


void FeatureSet::CalculateAll()
{
	CalculateTimeInterval();

	if (Config::Inst()->isFeatureEnabled(IP_TRAFFIC_DISTRIBUTION))
	{
			Calculate(IP_TRAFFIC_DISTRIBUTION);
	}
	if (Config::Inst()->isFeatureEnabled(PORT_TRAFFIC_DISTRIBUTION))
	{
			Calculate(PORT_TRAFFIC_DISTRIBUTION);
	}
	if (Config::Inst()->isFeatureEnabled(HAYSTACK_EVENT_FREQUENCY))
	{
			Calculate(HAYSTACK_EVENT_FREQUENCY);
	}
	if (Config::Inst()->isFeatureEnabled(PACKET_SIZE_MEAN))
	{
			Calculate(PACKET_SIZE_MEAN);
	}
	if (Config::Inst()->isFeatureEnabled(PACKET_SIZE_DEVIATION))
	{
		if (!Config::Inst()->isFeatureEnabled(PACKET_SIZE_MEAN))
			Calculate(PACKET_SIZE_MEAN);
		Calculate(PACKET_SIZE_DEVIATION);
	}
	if (Config::Inst()->isFeatureEnabled(DISTINCT_IPS))
	{
			Calculate(DISTINCT_IPS);
	}
	if (Config::Inst()->isFeatureEnabled(DISTINCT_PORTS))
	{
			Calculate(DISTINCT_PORTS);
	}
	if (Config::Inst()->isFeatureEnabled(PACKET_INTERVAL_MEAN))
	{
			Calculate(PACKET_INTERVAL_MEAN);
	}
	if (Config::Inst()->isFeatureEnabled(PACKET_INTERVAL_DEVIATION))
	{
		if (!Config::Inst()->isFeatureEnabled(PACKET_INTERVAL_MEAN))
				Calculate(PACKET_INTERVAL_MEAN);
			Calculate(PACKET_INTERVAL_DEVIATION);
	}
}


void FeatureSet::Calculate(uint32_t featureDimension)
{
	switch (featureDimension)
	{

	///The traffic distribution across the haystacks relative to host traffic
	case IP_TRAFFIC_DISTRIBUTION:
	{
		m_features[IP_TRAFFIC_DISTRIBUTION] = 0;

		//Max packet count to an IP, used for normalizing
		uint32_t IPMax = 0;
		for (IP_Table::iterator it = m_IPTable.begin() ; it != m_IPTable.end(); it++)
		{
			if(it->second > IPMax)
				IPMax = it->second;
		}
		for (IP_Table::iterator it = m_IPTable.begin() ; it != m_IPTable.end(); it++)
		{
			m_features[IP_TRAFFIC_DISTRIBUTION] += ((double)it->second / (double)IPMax);
		}

		m_features[IP_TRAFFIC_DISTRIBUTION] = m_features[IP_TRAFFIC_DISTRIBUTION] / (double)m_IPTable.size();
		break;
	}


	///The traffic distribution across ports contacted
	case PORT_TRAFFIC_DISTRIBUTION:
	{
		m_features[PORT_TRAFFIC_DISTRIBUTION] = 0;
		for (Port_Table::iterator it = m_portTable.begin() ; it != m_portTable.end(); it++)
		{
			m_features[PORT_TRAFFIC_DISTRIBUTION] += ((double)it->second / (double)m_portMax);
		}

		m_features[PORT_TRAFFIC_DISTRIBUTION] = m_features[PORT_TRAFFIC_DISTRIBUTION] / (double)m_portTable.size();
		break;
	}

	///Number of ScanEvents that the suspect is responsible for per second
	case HAYSTACK_EVENT_FREQUENCY:
	{
		// if > 0, .second is a time_t(uint) sum of all intervals across all nova instances
		if(m_totalInterval)
		{
			m_features[HAYSTACK_EVENT_FREQUENCY] = ((double)(m_haystackEvents)) / (double)(m_totalInterval);
		}
		else
		{
			//If interval is 0, no time based information, use a default of 1 for the interval
			m_features[HAYSTACK_EVENT_FREQUENCY] = (double)(m_haystackEvents);
		}
		break;
	}


	///Measures the distribution of packet sizes
	case PACKET_SIZE_MEAN:
	{
		m_features[PACKET_SIZE_MEAN] = (double)m_bytesTotal / (double)m_packetCount;
		break;
	}


	///Measures the distribution of packet sizes
	case PACKET_SIZE_DEVIATION:
	{
		double count = m_packetCount;
		double mean = 0;
		double variance = 0;
		//Calculate mean
		mean = m_features[PACKET_SIZE_MEAN];

		//Calculate variance
		for(Packet_Table::iterator it = m_packTable.begin() ; it != m_packTable.end(); it++)
		{
			// number of packets multiplied by (packet_size - mean)^2 divided by count
			variance += (it->second * pow((it->first - mean), 2))/ count;
		}

		m_features[PACKET_SIZE_DEVIATION] = sqrt(variance);
		break;
	}


	/// Number of distinct IP addresses contacted
	case DISTINCT_IPS:
	{
		m_features[DISTINCT_IPS] = m_IPTable.size();
		break;
	}


	/// Number of distinct ports contacted
	case DISTINCT_PORTS:
	{
		m_features[DISTINCT_PORTS] =  m_portTable.size();
		break;
	}

	///Measures the distribution of intervals between packets
	case PACKET_INTERVAL_MEAN:
	{
		m_features[PACKET_INTERVAL_MEAN] = (((double) m_totalInterval)
								/ ((double) (m_intervalTable.size())));
		break;
	}


	///Measures the distribution of intervals between packets
	case PACKET_INTERVAL_DEVIATION:
	{
		double totalCount = m_intervalTable.size();
		double mean = 0;
		double variance = 0;

		mean = m_features[PACKET_INTERVAL_MEAN];

		for (Interval_Table::iterator it = m_intervalTable.begin() ; it != m_intervalTable.end(); it++)
		{
			variance += it->second*(pow((it->first - mean), 2)/totalCount);
		}

		m_features[PACKET_INTERVAL_DEVIATION] = sqrt(variance);
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
	if(m_endTime > m_startTime)
	{
		m_totalInterval = m_endTime - m_startTime;
	}
	else
	{
		m_totalInterval = 0;
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

	m_packetCount += packet_count;
	m_bytesTotal += ntohs(packet.ip_hdr.ip_len);;

	//If from haystack
	if(packet.fromHaystack)
	{
		m_IPTable[packet.ip_hdr.ip_dst.s_addr] += packet_count;
		m_haystackEvents++;
	}
	//Else from a host
	else
	{
		//Put the packet count into a bin that is never used so that
		// all host events for a suspect go into the same bin
		m_IPTable[1] +=  packet_count;
	}

	m_portTable[dst_port] += packet_count;

	//Checks for the max to avoid iterating through the entire table every update
	//Since number of ports can grow very large during a scan this will distribute the computation more evenly
	//Since the IP will tend to be relatively small compared to number of events, it's max is found during the call.
	if(m_portTable[dst_port] > m_portMax)
	{
		m_portMax = m_portTable[dst_port];
	}

	for(uint32_t i = 0; i < IP_packet_sizes.size(); i++)
	{
		m_packTable[IP_packet_sizes[i]]++;
	}

	if(m_last_time != 0)
		m_intervalTable[packet_intervals[0] - m_last_time]++;

	for(uint32_t i = 1; i < packet_intervals.size(); i++)
	{
		m_intervalTable[packet_intervals[i] - packet_intervals[i-1]]++;
	}
	m_last_time = packet_intervals.back();

	//Accumulate to find the lowest Start time and biggest end time.
	if(packet.pcap_header.ts.tv_sec < m_startTime)
	{
		m_startTime = packet.pcap_header.ts.tv_sec;
	}
	if( packet.pcap_header.ts.tv_sec > m_endTime)
	{
		m_endTime =  packet.pcap_header.ts.tv_sec;
		CalculateTimeInterval();
	}
}

FeatureSet& FeatureSet::operator+=(FeatureSet &rhs)
{
	if(m_startTime > rhs.m_startTime)
	{
		m_startTime = rhs.m_startTime;
	}
	if(m_endTime < rhs.m_endTime)
	{
		m_endTime = rhs.m_endTime;
	}
	if(m_last_time < rhs.m_last_time)
	{
		m_last_time = rhs.m_last_time;
	}
	m_totalInterval += rhs.m_totalInterval;
	m_packetCount += rhs.m_packetCount;
	m_bytesTotal += rhs.m_bytesTotal;
	m_haystackEvents += rhs.m_haystackEvents;

	for(IP_Table::iterator it = rhs.m_IPTable.begin(); it != rhs.m_IPTable.end(); it++)
		m_IPTable[it->first] += rhs.m_IPTable[it->first];

	for(Port_Table::iterator it = rhs.m_portTable.begin(); it != rhs.m_portTable.end(); it++)
	{
		m_portTable[it->first] += rhs.m_portTable[it->first];
		if (m_portTable[it->first] > m_portMax)
			m_portMax = m_portTable[it->first];
	}

	for(Packet_Table::iterator it = rhs.m_packTable.begin(); it != rhs.m_packTable.end(); it++)
		m_packTable[it->first] += rhs.m_packTable[it->first];

	for(Interval_Table::iterator it = rhs.m_intervalTable.begin(); it != rhs.m_intervalTable.end(); it++)
		m_intervalTable[it->first] += rhs.m_intervalTable[it->first];

	return *this;
}

FeatureSet& FeatureSet::operator-=(FeatureSet &rhs)
{
	if(m_startTime > rhs.m_startTime)
	{
		m_startTime = rhs.m_startTime;
	}
	if(m_endTime < rhs.m_endTime)
	{
		m_endTime = rhs.m_endTime;
	}
	if(m_last_time < rhs.m_last_time)
	{
		m_last_time = rhs.m_last_time;
	}
	m_totalInterval -= rhs.m_totalInterval;
	m_packetCount -= rhs.m_packetCount;
	m_bytesTotal -= rhs.m_bytesTotal;
	m_haystackEvents -= rhs.m_haystackEvents;

	for(IP_Table::iterator it = rhs.m_IPTable.begin(); it != rhs.m_IPTable.end(); it++)
		m_IPTable[it->first] -= rhs.m_IPTable[it->first];

	for(Port_Table::iterator it = rhs.m_portTable.begin(); it != rhs.m_portTable.end(); it++)
		m_portTable[it->first] -= rhs.m_portTable[it->first];

	for(Packet_Table::iterator it = rhs.m_packTable.begin(); it != rhs.m_packTable.end(); it++)
		m_packTable[it->first] -= rhs.m_packTable[it->first];

	for(Interval_Table::iterator it = rhs.m_intervalTable.begin(); it != rhs.m_intervalTable.end(); it++)
		m_intervalTable[it->first] -= rhs.m_intervalTable[it->first];

	return *this;
}

uint32_t FeatureSet::SerializeFeatureSet(u_char * buf)
{
	uint32_t offset = 0;

	//Clears a chunk of the buffer for the FeatureSet
	bzero(buf, (sizeof m_features[0])*DIM);

	//Copies the value and increases the offset
	for(uint32_t i = 0; i < DIM; i++)
	{
		memcpy(buf+offset, &m_features[i], sizeof m_features[i]);
		offset+= sizeof m_features[i];
	}

	return offset;
}


uint32_t FeatureSet::DeserializeFeatureSet(u_char * buf)
{
	uint32_t offset = 0;

	//Copies the value and increases the offset
	for(uint32_t i = 0; i < DIM; i++)
	{
		memcpy(&m_features[i], buf+offset, sizeof m_features[i]);
		offset+= sizeof m_features[i];
	}

	return offset;
}

void FeatureSet::clearFeatureData()
{
		m_totalInterval = 0;
		m_haystackEvents = 0;
		m_packetCount = 0;
		m_bytesTotal = 0;

		m_startTime = m_endTime;

		for(Interval_Table::iterator it = m_intervalTable.begin(); it != m_intervalTable.end(); it++)
			m_intervalTable[it->first] = 0;

		for(Packet_Table::iterator it = m_packTable.begin(); it != m_packTable.end(); it++)
			m_packTable[it->first] = 0;

		for(IP_Table::iterator it = m_IPTable.begin(); it != m_IPTable.end(); it++)
			m_IPTable[it->first] = 0;

		for(Port_Table::iterator it = m_portTable.begin(); it != m_portTable.end(); it++)
			m_portTable[it->first] = 0;
}

uint32_t FeatureSet::SerializeFeatureData(u_char *buf)
{
	uint32_t offset = 0;
	uint32_t count = 0;
	uint32_t table_entries = 0;

	//Required, individual variables for calculation
	CalculateTimeInterval();

	memcpy(buf+offset, &m_totalInterval, sizeof m_totalInterval);
	offset += sizeof m_totalInterval;

	memcpy(buf+offset, &m_haystackEvents, sizeof m_haystackEvents);
	offset += sizeof m_haystackEvents;

	memcpy(buf+offset, &m_packetCount, sizeof m_packetCount);
	offset += sizeof m_packetCount;

	memcpy(buf+offset, &m_bytesTotal, sizeof m_bytesTotal);
	offset += sizeof m_bytesTotal;

	memcpy(buf+offset, &m_portMax, sizeof m_portMax);
	offset += sizeof m_portMax;

	//These tables all just place their key followed by the data
	uint32_t tempInt;

	for(Interval_Table::iterator it = m_intervalTable.begin(); (it != m_intervalTable.end()) && (count < MAX_TABLE_ENTRIES); it++)
		if(it->second)
			count++;

	//The size of the Table
	tempInt = count - table_entries;
	memcpy(buf+offset, &tempInt, sizeof tempInt);
	offset += sizeof tempInt;

	for(Interval_Table::iterator it = m_intervalTable.begin(); (it != m_intervalTable.end()) && (table_entries < count); it++)
	{
		if(it->second)
		{
			table_entries++;
			memcpy(buf+offset, &it->first, sizeof it->first);
			offset += sizeof it->first;
			memcpy(buf+offset, &it->second, sizeof it->second);
			offset += sizeof it->second;
		}
	}

	for(Packet_Table::iterator it = m_packTable.begin(); (it != m_packTable.end()) && (count < MAX_TABLE_ENTRIES); it++)
		if(it->second)
			count++;

	//The size of the Table
	tempInt = count - table_entries;
	memcpy(buf+offset, &tempInt, sizeof tempInt);
	offset += sizeof tempInt;

	for(Packet_Table::iterator it = m_packTable.begin(); (it != m_packTable.end()) && (table_entries < count); it++)
	{
		if(it->second)
		{
			table_entries++;
			memcpy(buf+offset, &it->first, sizeof it->first);
			offset += sizeof it->first;
			memcpy(buf+offset, &it->second, sizeof it->second);
			offset += sizeof it->second;
		}
	}

	for(IP_Table::iterator it = m_IPTable.begin(); (it != m_IPTable.end()) && (count < MAX_TABLE_ENTRIES); it++)
		if(it->second)
			count++;

	//The size of the Table
	tempInt = count - table_entries;
	memcpy(buf+offset, &tempInt, sizeof tempInt);
	offset += sizeof tempInt;

	for(IP_Table::iterator it = m_IPTable.begin(); (it != m_IPTable.end()) && (table_entries < count); it++)
	{
		if(it->second)
		{
			table_entries++;
			memcpy(buf+offset, &it->first, sizeof it->first);
			offset += sizeof it->first;
			memcpy(buf+offset, &it->second, sizeof it->second);
			offset += sizeof it->second;
		}
	}

	for(Port_Table::iterator it = m_portTable.begin(); (it != m_portTable.end()) && (count < MAX_TABLE_ENTRIES); it++)
		if(it->second)
			count++;

	//The size of the Table
	tempInt = count - table_entries;
	memcpy(buf+offset, &tempInt, sizeof tempInt);
	offset += sizeof tempInt;

	for(Port_Table::iterator it = m_portTable.begin(); (it != m_portTable.end()) && (table_entries < count); it++)
	{
		if(it->second)
		{
			table_entries++;
			memcpy(buf+offset, &it->first, sizeof it->first);
			offset += sizeof it->first;
			memcpy(buf+offset, &it->second, sizeof it->second);
			offset += sizeof it->second;
		}
	}
	return offset;
}


uint32_t FeatureSet::DeserializeFeatureData(u_char *buf)
{
	uint32_t offset = 0;

	//Bytes in a word, used for everything but port #'s
	uint32_t table_size = 0;

	//Temporary variables to store and track data during deserialization
	uint32_t temp = 0;
	uint32_t tempCount = 0;
	in_port_t port = 0;

	//Required, individual variables for calculation
	memcpy(&temp, buf+offset, sizeof m_totalInterval);
	m_totalInterval += temp;
	offset += sizeof m_totalInterval;

	memcpy(&temp, buf+offset, sizeof m_haystackEvents);
	m_haystackEvents += temp;
	offset += sizeof m_haystackEvents;

	memcpy(&temp, buf+offset, sizeof m_packetCount);
	m_packetCount += temp;
	offset += sizeof m_packetCount;

	memcpy(&temp, buf+offset, sizeof m_bytesTotal);
	m_bytesTotal += temp;
	offset += sizeof m_bytesTotal;

	memcpy(&temp, buf+offset, sizeof m_portMax);
	offset += sizeof m_portMax;

	if(temp > m_portMax)
		m_portMax = temp;

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
		m_intervalTable[temp] += tempCount;
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
		m_packTable[temp] += tempCount;
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
		m_IPTable[temp] += tempCount;
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
		m_portTable[port] += tempCount;
		i++;
	}

	return offset;
}

bool FeatureSet::operator ==(const FeatureSet &rhs) const
{
	if (m_packTable != rhs.m_packTable)
		return false;

	if (m_portTable != rhs.m_portTable)
		return false;

	if (m_portMax != rhs.m_portMax)
		return false;

	if (m_haystackEvents != rhs.m_haystackEvents)
		return false;


	/* These don't get serialized/deserialized.. should they?
	 if (m_startTime != rhs.m_startTime)
		return false;

	if (m_endTime != rhs.m_endTime)
		return false;

	if (m_last_time != rhs.m_last_time)
		return false;
	 */

	if (m_totalInterval != rhs.m_totalInterval)
		return false;

	if (m_bytesTotal != rhs.m_bytesTotal)
		return false;

	if (m_intervalTable != rhs.m_intervalTable)
		return false;

	if (m_IPTable != rhs.m_IPTable)
		return false;

	if (m_packetCount != rhs.m_packetCount)
		return false;

	for (int i = 0; i < DIM; i++)
		if (m_features[i] != rhs.m_features[i])
			return false;

	return true;
}

bool FeatureSet::operator !=(const FeatureSet &rhs) const
{
	return !(*this == rhs);
}

/*
FeatureSet& FeatureSet::operator=(FeatureSet &rhs)
{
	this->m_IPTable = rhs.m_IPTable;
	this->m_bytesTotal = rhs.m_bytesTotal;
	this->m_endTime = rhs.m_endTime;
	*this->m_features = *rhs.m_features;
	this->m_haystackEvents = rhs.m_haystackEvents;
	this->m_intervalTable = rhs.m_intervalTable;
	this->m_last_time = rhs.m_last_time;
	this->m_packTable = rhs.m_packTable;
	this->m_packetCount = rhs.m_packetCount;
	this->m_portMax = rhs.m_portMax;
	this->m_portTable = rhs.m_portTable;
	this->m_startTime = rhs.m_startTime;
	this->m_totalInterval = rhs.m_totalInterval;

	return *this;
}

FeatureSet& FeatureSet::operator=(FeatureSet rhs)
{
	delete m_unsentData;
	this->m_IPTable = rhs.m_IPTable;
	this->m_bytesTotal = rhs.m_bytesTotal;
	this->m_endTime = rhs.m_endTime;
	*this->m_features = *rhs.m_features;
	this->m_haystackEvents = rhs.m_haystackEvents;
	this->m_intervalTable = rhs.m_intervalTable;
	this->m_last_time = rhs.m_last_time;
	this->m_packTable = rhs.m_packTable;
	this->m_packetCount = rhs.m_packetCount;
	this->m_portMax = rhs.m_portMax;
	this->m_portTable = rhs.m_portTable;
	this->m_startTime = rhs.m_startTime;
	this->m_totalInterval = rhs.m_totalInterval;

	return *this;
}*/

}
