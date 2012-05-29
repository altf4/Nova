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
//============================================================================

#include "SerializationHelper.h"
#include "FeatureSet.h"
#include "Logger.h"
#include "Config.h"
#include <math.h>
#include <sys/un.h>

using namespace std;

namespace Nova{

string FeatureSet::m_featureNames[] =
{
		"IP_TRAFFIC_DISTRIBUTION",
		"PORT_TRAFFIC_DISTRIBUTION",
		"HAYSTACK_EVENT_FREQUENCY",
		"PACKET_SIZE_MEAN",
		"PACKET_SIZE_DEVIATION",
		"DISTINCT_IPS",
		"DISTINCT_PORTS",
		"PACKET_INTERVAL_MEAN",
		"PACKET_INTERVAL_DEVIATION"
};

FeatureSet::FeatureSet()
{
	m_IPTable.set_empty_key(0);
	m_portTable.set_empty_key(0);
	m_packTable.set_empty_key(0);
	m_intervalTable.set_empty_key(~0);

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


void FeatureSet::Calculate(const uint32_t& featureDimension)
{
	switch(featureDimension)
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
			if(m_portTable.size())
			{
				double portDivisor = 0;
				for(Port_Table::iterator it = m_portTable.begin() ; it != m_portTable.end(); it++)
				{
					//get the maximum port entry for normalization
					if(it->second > portDivisor)
					{
						portDivisor = it->second;
					}
				}
				//Multiply the maximum entry with the size to get the divisor
				portDivisor = portDivisor * ((double)m_portTable.size());
				long long unsigned int temp = 0;
				for(Port_Table::iterator it = m_portTable.begin() ; it != m_portTable.end(); it++)
				{
					temp += it->second;
				}
				m_features[PORT_TRAFFIC_DISTRIBUTION] = ((double)temp)/portDivisor;
			}
			break;
		}
		///Number of ScanEvents that the suspect is responsible for per second
		case HAYSTACK_EVENT_FREQUENCY:
		{
			double haystack_events = m_packetCount - m_IPTable[1];
			// if > 0, .second is a time_t(uint) sum of all intervals across all nova instances
			if(m_totalInterval)
			{
				//Packet count - local host contacts == haystack events
				m_features[HAYSTACK_EVENT_FREQUENCY] = haystack_events / ((double)m_totalInterval);
			}
			else
			{
				//If interval is 0, no time based information, use a default of 1 for the interval
				m_features[HAYSTACK_EVENT_FREQUENCY] = haystack_events;
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
				variance += (it->second *pow((it->first - mean), 2))/ count;
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
			break;
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

void FeatureSet::UpdateEvidence(Evidence *evidence)
{
	// Ensure our assumptions about valid packet fields are true
	if(evidence->m_evidencePacket.ip_dst == 0)
	{
		LOG(DEBUG, "Got packet with invalid source IP address of 0. Skipping.", "");
		return;
	}
	switch(evidence->m_evidencePacket.ip_p)
	{
		//If UDP
		case 17:
		{
			m_portTable[evidence->m_evidencePacket.dst_port]++;
			break;
		}
		//If TCP
		case 6:
		{
			m_portTable[evidence->m_evidencePacket.dst_port]++;
			break;
		}
		//If ICMP
		case 1:
		{
			break;
		}
		//If untracked IP protocol or error case ignore it
		default:
		{
			//LOG(DEBUG, "Dropping packet with unhandled IP protocol." , "");
			return;
		}
	}

	m_packetCount++;
	m_bytesTotal += evidence->m_evidencePacket.ip_len;
	m_IPTable[evidence->m_evidencePacket.ip_dst]++;
	m_packTable[evidence->m_evidencePacket.ip_len]++;

	if(m_lastTime != 0)
	{
		m_intervalTable[evidence->m_evidencePacket.ts - m_lastTime]++;
	}

	m_lastTime = evidence->m_evidencePacket.ts;

	//Accumulate to find the lowest Start time and biggest end time.
	if(evidence->m_evidencePacket.ts < m_startTime)
	{
		m_startTime = evidence->m_evidencePacket.ts;
	}
	if(evidence->m_evidencePacket.ts > m_endTime)
	{
		m_endTime =  evidence->m_evidencePacket.ts;
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

uint32_t FeatureSet::SerializeFeatureSet(u_char *buf, uint32_t bufferSize)
{
	uint32_t offset = 0;

	//Clears a chunk of the buffer for the FeatureSet
	bzero(buf, (sizeof m_features[0])*DIM);

	//Copies the value and increases the offset
	for(uint32_t i = 0; i < DIM; i++)
	{
		SerializeChunk(buf, &offset, (char*)&m_features[i], sizeof m_features[i], bufferSize);
	}

	return offset;
}


uint32_t FeatureSet::DeserializeFeatureSet(u_char *buf, uint32_t bufferSize)
{
	uint32_t offset = 0;

	//Copies the value and increases the offset
	for(uint32_t i = 0; i < DIM; i++)
	{
		DeserializeChunk(buf, &offset, (char*)&m_features[i], sizeof m_features[i], bufferSize);
	}

	return offset;
}

void FeatureSet::ClearFeatureData()
{
		m_totalInterval = 0;
		m_packetCount = 0;
		m_bytesTotal = 0;

		m_startTime = ~0;
		m_endTime = 0;
		m_lastTime = 0;
		m_intervalTable.clear();
		m_packTable.clear();
		m_IPTable.clear();
		m_portTable.clear();
}

uint32_t FeatureSet::SerializeFeatureData(u_char *buf, uint32_t bufferSize)
{
	uint32_t offset = 0;
	uint32_t count = 0;
	uint32_t table_entries = 0;

	//Required, individual variables for calculation
	CalculateTimeInterval();

	SerializeChunk(buf, &offset, (char*)&m_totalInterval, sizeof m_totalInterval, bufferSize);
	SerializeChunk(buf, &offset, (char*)&m_packetCount, sizeof m_packetCount, bufferSize);
	SerializeChunk(buf, &offset, (char*)&m_bytesTotal, sizeof m_bytesTotal, bufferSize);
	SerializeChunk(buf, &offset, (char*)&m_startTime, sizeof m_startTime, bufferSize);
	SerializeChunk(buf, &offset, (char*)&m_endTime, sizeof m_endTime, bufferSize);
	SerializeChunk(buf, &offset, (char*)&m_lastTime, sizeof m_lastTime, bufferSize);

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
	SerializeChunk(buf, &offset, (char*)&tempInt, sizeof tempInt, bufferSize);

	for(Interval_Table::iterator it = m_intervalTable.begin(); (it != m_intervalTable.end()) && (table_entries < count); it++)
	{
		if(it->second)
		{
			table_entries++;
			SerializeChunk(buf, &offset, (char*)&it->first, sizeof it->first, bufferSize);
			SerializeChunk(buf, &offset, (char*)&it->second, sizeof it->second, bufferSize);
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
	SerializeChunk(buf, &offset, (char*)&tempInt, sizeof tempInt, bufferSize);

	for(Packet_Table::iterator it = m_packTable.begin(); (it != m_packTable.end()) && (table_entries < count); it++)
	{
		if(it->second)
		{
			table_entries++;
			SerializeChunk(buf, &offset, (char*)&it->first, sizeof it->first, bufferSize);
			SerializeChunk(buf, &offset, (char*)&it->second, sizeof it->second, bufferSize);
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
	SerializeChunk(buf, &offset, (char*)&tempInt, sizeof tempInt, bufferSize);

	for(IP_Table::iterator it = m_IPTable.begin(); (it != m_IPTable.end()) && (table_entries < count); it++)
	{
		if(it->second)
		{
			table_entries++;
			SerializeChunk(buf, &offset, (char*)&it->first, sizeof it->first, bufferSize);
			SerializeChunk(buf, &offset, (char*)&it->second, sizeof it->second, bufferSize);
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
	SerializeChunk(buf, &offset, (char*)&tempInt, sizeof tempInt, bufferSize);

	for(Port_Table::iterator it = m_portTable.begin(); (it != m_portTable.end()) && (table_entries < count); it++)
	{
		if(it->second)
		{
			table_entries++;
			SerializeChunk(buf, &offset, (char*)&it->first, sizeof it->first, bufferSize);
			SerializeChunk(buf, &offset, (char*)&it->second, sizeof it->second, bufferSize);
		}
	}
	return offset;
}

uint32_t FeatureSet::GetFeatureDataLength()
{
	uint32_t out = 0, count = 0;

	//Vars we need to send useable Data
	out += sizeof(m_totalInterval) + sizeof(m_packetCount) + sizeof(m_bytesTotal)
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

	for(Packet_Table::iterator it = m_packTable.begin(); it != m_packTable.end(); it++)
	{
		if(it->second)
		{
			count++;
		}
	}
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


uint32_t FeatureSet::DeserializeFeatureData(u_char *buf, uint32_t bufferSize)
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
	uint16_t tempShort = 0;

	//Required, individual variables for calculation
	DeserializeChunk(buf, &offset, (char*)&temp, sizeof m_totalInterval, bufferSize);
	m_totalInterval += temp;

	DeserializeChunk(buf, &offset, (char*)&temp, sizeof m_packetCount, bufferSize);
	m_packetCount += temp;

	DeserializeChunk(buf, &offset, (char*)&temp, sizeof m_bytesTotal, bufferSize);
	m_bytesTotal += temp;

	DeserializeChunk(buf, &offset, (char*)&temp, sizeof m_startTime, bufferSize);
	if(m_startTime > (time_t)temp)
	{
		m_startTime = temp;
	}

	DeserializeChunk(buf, &offset, (char*)&temp, sizeof m_endTime, bufferSize);
	if(m_endTime < (time_t)temp)
	{
		m_endTime = temp;
	}

	DeserializeChunk(buf, &offset, (char*)&temp, sizeof m_lastTime, bufferSize);
	if(m_lastTime < (time_t)temp)
	{
		m_lastTime = temp;
	}

	/***************************************************************************************************
	* For all of these tables we extract, the key (bin identifier) followed by the data (packet count)
	*  i += the # of packets in the bin, if we haven't reached packet count we know there's another item
	****************************************************************************************************/
	DeserializeChunk(buf, &offset, (char*)&table_size, sizeof table_size, bufferSize);
	//Packet interval table
	for(uint32_t i = 0; i < table_size;)
	{
		DeserializeChunk(buf, &offset, (char*)&temp, sizeof timeTemp, bufferSize);
		DeserializeChunk(buf, &offset, (char*)&tempCount, sizeof tempCount, bufferSize);

		m_intervalTable[temp] += tempCount;
		i++;
	}

	DeserializeChunk(buf, &offset, (char*)&table_size, sizeof table_size, bufferSize);
	//Packet size table
	for(uint32_t i = 0; i < table_size;)
	{
		DeserializeChunk(buf, &offset, (char*)&tempShort, sizeof tempShort, bufferSize);
		DeserializeChunk(buf, &offset, (char*)&tempCount, sizeof tempCount, bufferSize);

		m_packTable[tempShort] += tempCount;
		i++;
	}

	DeserializeChunk(buf, &offset, (char*)&table_size, sizeof table_size, bufferSize);
	//IP table
	for(uint32_t i = 0; i < table_size;)
	{
		DeserializeChunk(buf, &offset, (char*)&temp, sizeof temp, bufferSize);
		DeserializeChunk(buf, &offset, (char*)&tempCount, sizeof tempCount, bufferSize);

		m_IPTable[temp] += tempCount;
		i++;
	}

	DeserializeChunk(buf, &offset, (char*)&table_size, sizeof table_size, bufferSize);
	//Port table
	for(uint32_t i = 0; i < table_size;)
	{
		DeserializeChunk(buf, &offset, (char*)&tempShort, sizeof tempShort, bufferSize);
		DeserializeChunk(buf, &offset, (char*)&tempCount, sizeof tempCount, bufferSize);

		m_portTable[tempShort] += tempCount;
		i++;
	}
	return offset;
}

bool FeatureSet::operator ==(const FeatureSet &rhs) const
{
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
