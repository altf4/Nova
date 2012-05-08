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
	m_lastTime = 0;

	//Features
	for(int i = 0; i < DIM; i++)
	{
		m_features[i] = 0;
	}
}


void FeatureSet::CalculateAll()
{
	CalculateTimeInterval();
	if(Config::Inst()->IsFeatureEnabled(IP_TRAFFIC_DISTRIBUTION))
	{
			Calculate(IP_TRAFFIC_DISTRIBUTION);
	}
	if(Config::Inst()->IsFeatureEnabled(PORT_TRAFFIC_DISTRIBUTION))
	{
			Calculate(PORT_TRAFFIC_DISTRIBUTION);
	}
	if(Config::Inst()->IsFeatureEnabled(HAYSTACK_EVENT_FREQUENCY))
	{
			Calculate(HAYSTACK_EVENT_FREQUENCY);
	}
	if(Config::Inst()->IsFeatureEnabled(PACKET_SIZE_MEAN))
	{
			Calculate(PACKET_SIZE_MEAN);
	}
	if(Config::Inst()->IsFeatureEnabled(PACKET_SIZE_DEVIATION))
	{
		if(!Config::Inst()->IsFeatureEnabled(PACKET_SIZE_MEAN))
		{
			Calculate(PACKET_SIZE_MEAN);
		}
		Calculate(PACKET_SIZE_DEVIATION);
	}
	if(Config::Inst()->IsFeatureEnabled(DISTINCT_IPS))
	{
			Calculate(DISTINCT_IPS);
	}
	if(Config::Inst()->IsFeatureEnabled(DISTINCT_PORTS))
	{
			Calculate(DISTINCT_PORTS);
	}
	if(Config::Inst()->IsFeatureEnabled(PACKET_INTERVAL_MEAN))
	{
			Calculate(PACKET_INTERVAL_MEAN);
	}
	if(Config::Inst()->IsFeatureEnabled(PACKET_INTERVAL_DEVIATION))
	{
		if(!Config::Inst()->IsFeatureEnabled(PACKET_INTERVAL_MEAN))
		{
			Calculate(PACKET_INTERVAL_MEAN);
		}
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
			//Max packet count to an IP, used for normalizing
			uint32_t IPMax = 0;
			m_features[IP_TRAFFIC_DISTRIBUTION] = 0;
			for(IP_Table::iterator it = m_IPTable.begin() ; it != m_IPTable.end(); it++)
			{
				if(it->second > IPMax)
				{
					IPMax = it->second;
				}
			}
			for(IP_Table::iterator it = m_IPTable.begin() ; it != m_IPTable.end(); it++)
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
			double portMax = 0;
			for(Port_Table::iterator it = m_portTable.begin() ; it != m_portTable.end(); it++)
			{
				if(it->second > portMax)
				{
					portMax = it->second;
				}
			}
			for(Port_Table::iterator it = m_portTable.begin() ; it != m_portTable.end(); it++)
			{
				m_features[PORT_TRAFFIC_DISTRIBUTION] += ((double)it->second / portMax);
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
			//Calculate Mean
			double count = m_packetCount, mean = m_features[PACKET_SIZE_MEAN], variance = 0;
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
			if(m_intervalTable.size() == 0)
			{
				m_features[PACKET_INTERVAL_MEAN] = 0;
				break;
			}
			m_features[PACKET_INTERVAL_MEAN] = (((double)m_totalInterval)/((double)(m_intervalTable.size())));
		}
		///Measures the distribution of intervals between packets
		case PACKET_INTERVAL_DEVIATION:
		{
			double mean = m_features[PACKET_INTERVAL_MEAN], variance = 0, totalCount = m_intervalTable.size();
			for(Interval_Table::iterator it = m_intervalTable.begin() ; it != m_intervalTable.end(); it++)
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
	in_port_t dst_port = 0;
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
	else if(packet.ip_hdr.ip_p == 6)
	{
		dst_port =  ntohs(packet.tcp_hdr.dest);
	}

	IP_packet_sizes.push_back(ntohs(packet.ip_hdr.ip_len));
	packet_intervals.push_back(packet.pcap_header.ts.tv_sec);

	m_packetCount += packet_count;
	m_bytesTotal += ntohs(packet.ip_hdr.ip_len);

	//If from haystack
	if(packet.fromHaystack)
	{
		m_IPTable[packet.ip_hdr.ip_dst.s_addr] += packet_count;
		m_haystackEvents++;
	}
	//If from local host, put into designated bin
	else
	{
		m_IPTable[1] +=  packet_count;
	}

	m_portTable[dst_port] += packet_count;

	for(uint32_t i = 0; i < IP_packet_sizes.size(); i++)
	{
		m_packTable[IP_packet_sizes[i]]++;
	}

	if(m_lastTime != 0)
	{
		m_intervalTable[packet_intervals[0] - m_lastTime]++;
	}

	for(uint32_t i = 1; i < packet_intervals.size(); i++)
	{
		m_intervalTable[packet_intervals[i] - packet_intervals[i-1]]++;
	}
	m_lastTime = packet_intervals.back();

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
	if(m_lastTime < rhs.m_lastTime)
	{
		m_lastTime = rhs.m_lastTime;
	}

	m_totalInterval += rhs.m_totalInterval;
	m_packetCount += rhs.m_packetCount;
	m_bytesTotal += rhs.m_bytesTotal;
	m_haystackEvents += rhs.m_haystackEvents;

	for(IP_Table::iterator it = rhs.m_IPTable.begin(); it != rhs.m_IPTable.end(); it++)
	{
		m_IPTable[it->first] += rhs.m_IPTable[it->first];
	}

	for(Port_Table::iterator it = rhs.m_portTable.begin(); it != rhs.m_portTable.end(); it++)
	{
		m_portTable[it->first] += rhs.m_portTable[it->first];
	}

	for(Packet_Table::iterator it = rhs.m_packTable.begin(); it != rhs.m_packTable.end(); it++)
	{
		m_packTable[it->first] += rhs.m_packTable[it->first];
	}

	for(Interval_Table::iterator it = rhs.m_intervalTable.begin(); it != rhs.m_intervalTable.end(); it++)
	{
		m_intervalTable[it->first] += rhs.m_intervalTable[it->first];
	}

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
	if(m_lastTime < rhs.m_lastTime)
	{
		m_lastTime = rhs.m_lastTime;
	}
	m_totalInterval -= rhs.m_totalInterval;
	m_packetCount -= rhs.m_packetCount;
	m_bytesTotal -= rhs.m_bytesTotal;
	m_haystackEvents -= rhs.m_haystackEvents;

	for(IP_Table::iterator it = rhs.m_IPTable.begin(); it != rhs.m_IPTable.end(); it++)
	{
		m_IPTable[it->first] -= rhs.m_IPTable[it->first];
	}
	for(Port_Table::iterator it = rhs.m_portTable.begin(); it != rhs.m_portTable.end(); it++)
	{
		m_portTable[it->first] -= rhs.m_portTable[it->first];
	}

	for(Packet_Table::iterator it = rhs.m_packTable.begin(); it != rhs.m_packTable.end(); it++)
	{
		m_packTable[it->first] -= rhs.m_packTable[it->first];
	}
	for(Interval_Table::iterator it = rhs.m_intervalTable.begin(); it != rhs.m_intervalTable.end(); it++)
	{
		m_intervalTable[it->first] -= rhs.m_intervalTable[it->first];
	}

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

void FeatureSet::ClearFeatureData()
{
		m_totalInterval = 0;
		m_haystackEvents = 0;
		m_packetCount = 0;
		m_bytesTotal = 0;

		m_startTime = m_endTime;
		m_intervalTable.clear_no_resize();
		m_packTable.clear_no_resize();
		m_IPTable.clear_no_resize();
		m_portTable.clear_no_resize();
}

uint32_t FeatureSet::SerializeFeatureData(u_char *buf)
{
	// Uncomment this if you want to print the line, index, and size of each item deserialized
	// This can be diffed with the DESERIALIZATION_DEBUGGING output to find offset mismatches
    //#define SERIALIZATION_DEBUGGING true
	uint32_t offset = 0;
	uint32_t count = 0;
	uint32_t table_entries = 0;

	//Required, individual variables for calculation
	CalculateTimeInterval();

	memcpy(buf+offset, &m_totalInterval, sizeof m_totalInterval);
	offset += sizeof m_totalInterval;
#ifdef SERIALIZATION_DEBUGGING
	int item = 0;
	cout << __LINE__ << " " <<++item << " Size: " << sizeof m_totalInterval << endl;
#endif

	memcpy(buf+offset, &m_haystackEvents, sizeof m_haystackEvents);
	offset += sizeof m_haystackEvents;
#ifdef SERIALIZATION_DEBUGGING
	cout << __LINE__ << " " <<++item << " Size: " << sizeof m_haystackEvents << endl;
#endif

	memcpy(buf+offset, &m_packetCount, sizeof m_packetCount);
	offset += sizeof m_packetCount;
#ifdef SERIALIZATION_DEBUGGING
	cout << __LINE__ << " " <<++item << " Size: " << sizeof m_packetCount << endl;
#endif

	memcpy(buf+offset, &m_bytesTotal, sizeof m_bytesTotal);
	offset += sizeof m_bytesTotal;
#ifdef SERIALIZATION_DEBUGGING
	cout << __LINE__ << " " <<++item << " Size: " << sizeof m_bytesTotal << endl;
#endif

	memcpy(buf+offset, &m_startTime, sizeof m_startTime);
	offset += sizeof m_startTime;
#ifdef SERIALIZATION_DEBUGGING
	cout << __LINE__ << " " <<++item << " Size: " << sizeof m_startTime << endl;
#endif

	memcpy(buf+offset, &m_endTime, sizeof m_endTime);
	offset += sizeof m_endTime;
#ifdef SERIALIZATION_DEBUGGING
	cout << __LINE__ << " " <<++item << " Size: " << sizeof m_endTime << endl;
#endif

	memcpy(buf+offset, &m_lastTime, sizeof m_lastTime);
	offset += sizeof m_lastTime;
#ifdef SERIALIZATION_DEBUGGING
	cout << __LINE__ << " " <<++item << " Size: " << sizeof m_lastTime << endl;
#endif

	//These tables all just place their key followed by the data
	uint32_t tempInt;

	for(Interval_Table::iterator it = m_intervalTable.begin(); (it != m_intervalTable.end()) && (count < m_maxTableEntries); it++)
	{
		if(it->second)
		{
			count++;
		}
	}

	//The size of the Table
	tempInt = count - table_entries;
	memcpy(buf+offset, &tempInt, sizeof tempInt);
	offset += sizeof tempInt;
#ifdef SERIALIZATION_DEBUGGING
	cout << __LINE__ << " " <<++item << " Size: " << sizeof tempInt << endl;
#endif

	for(Interval_Table::iterator it = m_intervalTable.begin(); (it != m_intervalTable.end()) && (table_entries < count); it++)
	{
		if(it->second)
		{
			table_entries++;
			memcpy(buf+offset, &it->first, sizeof it->first);
			offset += sizeof it->first;
#ifdef SERIALIZATION_DEBUGGING
			cout << __LINE__ << " " <<++item << " Size: " << sizeof it->first << endl;
#endif

			memcpy(buf+offset, &it->second, sizeof it->second);
			offset += sizeof it->second;
#ifdef SERIALIZATION_DEBUGGING
			cout << __LINE__ << " " <<++item << " Size: " << sizeof it->second << endl;
#endif
		}
	}

	for(Packet_Table::iterator it = m_packTable.begin(); (it != m_packTable.end()) && (count < m_maxTableEntries); it++)
	{
		if(it->second)
		{
			count++;
		}
	}

	//The size of the Table
	tempInt = count - table_entries;
	memcpy(buf+offset, &tempInt, sizeof tempInt);
	offset += sizeof tempInt;
#ifdef SERIALIZATION_DEBUGGING
	cout << __LINE__ << " " <<++item << " Size: " << sizeof tempInt << endl;
#endif

	for(Packet_Table::iterator it = m_packTable.begin(); (it != m_packTable.end()) && (table_entries < count); it++)
	{
		if(it->second)
		{
			table_entries++;
			memcpy(buf+offset, &it->first, sizeof it->first);
			offset += sizeof it->first;
#ifdef SERIALIZATION_DEBUGGING
			cout << __LINE__ << " " <<++item << " Size: " << sizeof it->first << endl;
#endif

			memcpy(buf+offset, &it->second, sizeof it->second);
			offset += sizeof it->second;
#ifdef SERIALIZATION_DEBUGGING
	cout << __LINE__ << " " <<++item << " Size: " << sizeof it->second << endl;
#endif
		}
	}

	for(IP_Table::iterator it = m_IPTable.begin(); (it != m_IPTable.end()) && (count < m_maxTableEntries); it++)
	{
		if(it->second)
		{
			count++;
		}
	}

	//The size of the Table
	tempInt = count - table_entries;
	memcpy(buf+offset, &tempInt, sizeof tempInt);
	offset += sizeof tempInt;
#ifdef SERIALIZATION_DEBUGGING
	cout << __LINE__ << " " <<++item << " Size: " << sizeof tempInt << endl;
#endif

	for(IP_Table::iterator it = m_IPTable.begin(); (it != m_IPTable.end()) && (table_entries < count); it++)
	{
		if(it->second)
		{
			table_entries++;
			memcpy(buf+offset, &it->first, sizeof it->first);
			offset += sizeof it->first;
#ifdef SERIALIZATION_DEBUGGING
			cout << __LINE__ << " " <<++item << " Size: " << sizeof it->first << endl;
#endif

			memcpy(buf+offset, &it->second, sizeof it->second);
			offset += sizeof it->second;
#ifdef SERIALIZATION_DEBUGGING
			cout << __LINE__ << " " <<++item << " Size: " << sizeof it->second << endl;
#endif
		}
	}

	for(Port_Table::iterator it = m_portTable.begin(); (it != m_portTable.end()) && (count < m_maxTableEntries); it++)
	{
		if(it->second)
		{
			count++;
		}
	}

	//The size of the Table
	tempInt = count - table_entries;
	memcpy(buf+offset, &tempInt, sizeof tempInt);
	offset += sizeof tempInt;
#ifdef SERIALIZATION_DEBUGGING
	cout << __LINE__ << " " <<++item << " Size: " << sizeof tempInt << endl;
#endif

	for(Port_Table::iterator it = m_portTable.begin(); (it != m_portTable.end()) && (table_entries < count); it++)
	{
		if(it->second)
		{
			table_entries++;
			memcpy(buf+offset, &it->first, sizeof it->first);
			offset += sizeof it->first;
#ifdef SERIALIZATION_DEBUGGING
			cout << __LINE__ << " " <<++item << " Size: " << sizeof it->first << endl;
#endif

			memcpy(buf+offset, &it->second, sizeof it->second);
			offset += sizeof it->second;
#ifdef SERIALIZATION_DEBUGGING
			cout << __LINE__ << " " <<++item << " Size: " << sizeof it->second << endl;
#endif
		}
	}
	return offset;
}

uint32_t FeatureSet::GetFeatureDataLength()
{
	uint32_t out = 0, count = 0;

	//Vars we need to send useable Data
	out += sizeof(m_totalInterval) + sizeof(m_haystackEvents) + sizeof(m_packetCount) + sizeof(m_bytesTotal)
		+ sizeof(m_startTime) + sizeof(m_endTime) + sizeof(m_lastTime)
		//Each table has a total num entries val before it
		+ 4*sizeof(out);

	for(Interval_Table::iterator it = m_intervalTable.begin();	it != m_intervalTable.end(); it++)
	{
		if(it->second)
		{
			count++;
		}
	}
	for(Packet_Table::iterator it = m_packTable.begin(); it != m_packTable.end(); it++)
	{
		if(it->second)
		{
			count++;
		}
	}
	for(IP_Table::iterator it = m_IPTable.begin(); it != m_IPTable.end(); it++)
	{
		if(it->second)
		{
			count++;
		}
	}
	//pair of uint32_t vars per entry, with 'count' number of entries
	out += 2*sizeof(uint32_t)*count;
	count =  0;
	for(Port_Table::iterator it = m_portTable.begin(); it != m_portTable.end(); it++)
	{
		if(it->second)
		{
			count++;
		}
	}
	out += (sizeof(uint32_t) + sizeof(in_port_t))*count;
	return out;
}


uint32_t FeatureSet::DeserializeFeatureData(u_char *buf)
{
	// Uncomment this if you want to print the line, index, and size of each item deserialized
	// This can be diffed with the SERIALIZATION_DEBUGGING output to find offset mismatches
	//#define DESERIALIZATION_DEBUGGING true

	uint32_t offset = 0;

	//Bytes in a word, used for everything but port #'s
	uint32_t table_size = 0;

	//Temporary variables to store and track data during deserialization
	uint32_t temp = 0;
	time_t timeTemp;
	uint32_t tempCount = 0;
	in_port_t port = 0;

	//Required, individual variables for calculation
	memcpy(&temp, buf+offset, sizeof m_totalInterval);
	m_totalInterval += temp;
	offset += sizeof m_totalInterval;
#ifdef DESERIALIZATION_DEBUGGING
	int item = 0;
	cout << __LINE__ << " " <<++item << " Size: " << sizeof m_totalInterval << endl;
#endif
	memcpy(&temp, buf+offset, sizeof m_haystackEvents);
	m_haystackEvents += temp;
	offset += sizeof m_haystackEvents;
#ifdef DESERIALIZATION_DEBUGGING
			cout << __LINE__ << " " <<++item << " Size: " << sizeof m_haystackEvents << endl;
#endif
	memcpy(&temp, buf+offset, sizeof m_packetCount);
	m_packetCount += temp;
	offset += sizeof m_packetCount;
#ifdef DESERIALIZATION_DEBUGGING
			cout << __LINE__ << " " <<++item << " Size: " << sizeof m_packetCount << endl;
#endif
	memcpy(&temp, buf+offset, sizeof m_bytesTotal);
	m_bytesTotal += temp;
	offset += sizeof m_bytesTotal;
#ifdef DESERIALIZATION_DEBUGGING
			cout << __LINE__ << " " <<++item << " Size: " << sizeof m_bytesTotal << endl;
#endif
	memcpy(&temp, buf+offset, sizeof m_startTime);
	offset += sizeof m_startTime;
#ifdef DESERIALIZATION_DEBUGGING
			cout << __LINE__ << " " <<++item << " Size: " << sizeof m_startTime << endl;
#endif
	if(m_startTime > (time_t)temp)
	{
		m_startTime = temp;
	}
	memcpy(&temp, buf+offset, sizeof m_endTime);
	offset += sizeof m_endTime;
#ifdef DESERIALIZATION_DEBUGGING
			cout << __LINE__ << " " <<++item << " Size: " << sizeof m_endTime << endl;
#endif
	if(m_endTime < (time_t)temp)
	{
		m_endTime = temp;
	}
	memcpy(&temp, buf+offset, sizeof m_lastTime);
	offset += sizeof m_lastTime;
#ifdef DESERIALIZATION_DEBUGGING
			cout << __LINE__ << " " <<++item << " Size: " << sizeof m_lastTime << endl;
#endif
	if(m_lastTime < (time_t)temp)
	{
		m_lastTime = temp;
	}
	/***************************************************************************************************
	* For all of these tables we extract, the key (bin identifier) followed by the data (packet count)
	*  i += the # of packets in the bin, if we haven't reached packet count we know there's another item
	****************************************************************************************************/
	memcpy(&table_size, buf+offset, sizeof table_size);
	offset += sizeof table_size;
#ifdef DESERIALIZATION_DEBUGGING
			cout << __LINE__ << " " <<++item << " Size: " << sizeof table_size << endl;
#endif
	//Packet interval table
	for(uint32_t i = 0; i < table_size;)
	{
		memcpy(&temp, buf+offset, sizeof timeTemp);
		offset += sizeof timeTemp;
#ifdef DESERIALIZATION_DEBUGGING
			cout << __LINE__ << " " <<++item << " Size: " << sizeof temp << endl;
#endif
		memcpy(&tempCount, buf+offset, sizeof tempCount);
		offset += sizeof tempCount;
#ifdef DESERIALIZATION_DEBUGGING
			cout << __LINE__ << " " <<++item << " Size: " << sizeof tempCount << endl;
#endif
		m_intervalTable[temp] += tempCount;
		i++;
	}
	memcpy(&table_size, buf+offset, sizeof table_size);
	offset += sizeof table_size;
#ifdef DESERIALIZATION_DEBUGGING
			cout << __LINE__ << " " <<++item << " Size: " << sizeof table_size << endl;
#endif
	//Packet size table
	for(uint32_t i = 0; i < table_size;)
	{
		memcpy(&temp, buf+offset, sizeof temp);
		offset += sizeof temp;
#ifdef DESERIALIZATION_DEBUGGING
			cout << __LINE__ << " " <<++item << " Size: " << sizeof temp << endl;
#endif
		memcpy(&tempCount, buf+offset, sizeof tempCount);
		offset += sizeof tempCount;
#ifdef DESERIALIZATION_DEBUGGING
			cout << __LINE__ << " " <<++item << " Size: " << sizeof tempCount << endl;
#endif
		m_packTable[temp] += tempCount;
		i++;
	}
	memcpy(&table_size, buf+offset, sizeof table_size);
	offset += sizeof table_size;
#ifdef DESERIALIZATION_DEBUGGING
			cout << __LINE__ << " " <<++item << " Size: " << sizeof table_size << endl;
#endif
	//IP table
	for(uint32_t i = 0; i < table_size;)
	{
		memcpy(&temp, buf+offset, sizeof temp);
		offset += sizeof temp;
#ifdef DESERIALIZATION_DEBUGGING
			cout << __LINE__ << " " <<++item << " Size: " << sizeof temp << endl;
#endif
		memcpy(&tempCount, buf+offset, sizeof tempCount);
		offset += sizeof tempCount;
#ifdef DESERIALIZATION_DEBUGGING
			cout << __LINE__ << " " <<++item << " Size: " << sizeof tempCount << endl;
#endif
		m_IPTable[temp] += tempCount;
		i++;
	}
	memcpy(&table_size, buf+offset, sizeof table_size);
	offset += sizeof table_size;
#ifdef DESERIALIZATION_DEBUGGING
			cout << __LINE__ << " " <<++item << " Size: " << sizeof table_size << endl;
#endif

	//Port table
	for(uint32_t i = 0; i < table_size;)
	{
		memcpy(&port, buf+offset, sizeof port);
		offset += sizeof port;
#ifdef DESERIALIZATION_DEBUGGING
			cout << __LINE__ << " " <<++item << " Size: " << sizeof port << endl;
#endif
		memcpy(&tempCount, buf+offset, sizeof tempCount);
		offset += sizeof tempCount;
#ifdef DESERIALIZATION_DEBUGGING
			cout << __LINE__ << " " <<++item << " Size: " << sizeof tempCount << endl;
#endif
		m_portTable[port] += tempCount;
		i++;
	}
	return offset;
}

bool FeatureSet::operator ==(const FeatureSet &rhs) const
{

	if(m_haystackEvents != rhs.m_haystackEvents)
	{
		return false;
	}
	if(m_startTime != rhs.m_startTime)
	{
		return false;
	}
	if(m_endTime != rhs.m_endTime)
	{
		return false;
	}
	if(m_lastTime != rhs.m_lastTime)
	{
		return false;
	}
	if(m_totalInterval != rhs.m_totalInterval)
	{
		return false;
	}
	if(m_bytesTotal != rhs.m_bytesTotal)
	{
		return false;
	}
	if(m_packetCount != rhs.m_packetCount)
	{
		return false;
	}
	for(int i = 0; i < DIM; i++)
	{
		if(m_features[i] != rhs.m_features[i])
		{
			return false;
		}
	}
	return true;
}

bool FeatureSet::operator !=(const FeatureSet &rhs) const
{
	return !(*this == rhs);
}

}
