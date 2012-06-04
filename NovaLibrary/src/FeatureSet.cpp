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
	m_PortTCPTable.set_empty_key(0);
	m_PortUDPTable.set_empty_key(0);
	m_packTable.set_empty_key(0);
	m_intervalTable.set_empty_key(~0);
	m_lastTimes.set_empty_key(0);

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
	m_PortTCPTable.clear();
	m_PortUDPTable.clear();
	m_packTable.clear();
	m_intervalTable.clear();
	m_lastTimes.clear();

	m_rstCount = 0;
	m_ackCount = 0;
	m_synCount = 0;
	m_finCount = 0;
	m_synAckCount = 0;

	m_packetCount = 0;
	m_tcpPacketCount = 0;
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

	if (Config::Inst()->IsFeatureEnabled(TCP_PERCENT_SYN))
	{
		Calculate(TCP_PERCENT_SYN);
	}

	if (Config::Inst()->IsFeatureEnabled(TCP_PERCENT_FIN))
	{
		Calculate(TCP_PERCENT_FIN);
	}

	if (Config::Inst()->IsFeatureEnabled(TCP_PERCENT_RST))
	{
		Calculate(TCP_PERCENT_RST);
	}

	if (Config::Inst()->IsFeatureEnabled(TCP_PERCENT_SYNACK))
	{
		Calculate(TCP_PERCENT_SYNACK);
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
			if(m_PortTCPTable.size() || m_PortUDPTable.size())
			{
				double portDivisor = 0;
				for(Port_Table::iterator it = m_PortTCPTable.begin() ; it != m_PortTCPTable.end(); it++)
				{
					//get the maximum port entry for normalization
					if(it->second > portDivisor)
					{
						portDivisor = it->second;
					}
				}

				for(Port_Table::iterator it = m_PortUDPTable.begin() ; it != m_PortUDPTable.end(); it++)
				{
					//get the maximum port entry for normalization
					if(it->second > portDivisor)
					{
						portDivisor = it->second;
					}
				}


				//Multiply the maximum entry with the size to get the divisor
				portDivisor = portDivisor * ((double)m_PortTCPTable.size() + (double)m_PortUDPTable.size());
				long long unsigned int temp = 0;
				for(Port_Table::iterator it = m_PortTCPTable.begin() ; it != m_PortTCPTable.end(); it++)
				{
					temp += it->second;
				}

				for(Port_Table::iterator it = m_PortUDPTable.begin() ; it != m_PortUDPTable.end(); it++)
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
			m_features[DISTINCT_PORTS] =  m_PortTCPTable.size() + m_PortUDPTable.size();
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

		case TCP_PERCENT_SYN:
		{
			m_features[TCP_PERCENT_SYN] = ((double)m_synCount)/((double)m_tcpPacketCount + 1);
			break;
		}
		case TCP_PERCENT_FIN:
		{
			m_features[TCP_PERCENT_FIN] = ((double)m_finCount)/((double)m_tcpPacketCount+ 1);
			break;
		}
		case TCP_PERCENT_RST:
		{
			m_features[TCP_PERCENT_RST] = ((double)m_rstCount)/((double)m_tcpPacketCount + 1);
			break;
		}
		case TCP_PERCENT_SYNACK:
		{
			//cout << "TCP stats: synCount: " << synCount << " synAckCount: " << synAckCount << " ackCount: " << ackCount << " finCount: " << finCount << " rstCount" << rstCount << endl;
			m_features[TCP_PERCENT_SYNACK] = ((double)m_synAckCount)/((double)m_tcpPacketCount + 1);
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
			if (evidence->m_evidencePacket.dst_port != 0)
			{
				m_PortUDPTable[evidence->m_evidencePacket.dst_port]++;
			}
			break;
		}
		//If TCP
		case 6:
		{
			m_tcpPacketCount++;
			if (evidence->m_evidencePacket.dst_port != 0)
			{
				m_PortTCPTable[evidence->m_evidencePacket.dst_port]++;
			}


			if (evidence->m_evidencePacket.tcp_hdr.syn && evidence->m_evidencePacket.tcp_hdr.ack)
			{
				m_synAckCount++;
			}
			else if (evidence->m_evidencePacket.tcp_hdr.syn)
			{
				m_synCount++;
			}
			else if (evidence->m_evidencePacket.tcp_hdr.ack)
			{
				m_ackCount++;
			}

			if (evidence->m_evidencePacket.tcp_hdr.rst)
			{
				m_rstCount++;
			}

			if (evidence->m_evidencePacket.tcp_hdr.fin)
			{
				m_finCount++;
			}

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
			break;
			//return;
		}
	}

	m_packetCount++;
	m_bytesTotal += evidence->m_evidencePacket.ip_len;
	m_IPTable[evidence->m_evidencePacket.ip_dst]++;
	m_packTable[evidence->m_evidencePacket.ip_len]++;

	//If we have already gotten a packet from the source to dest host
	if(m_lastTimes.keyExists(evidence->m_evidencePacket.ip_dst))
	{

		if (evidence->m_evidencePacket.ts - m_lastTimes[evidence->m_evidencePacket.ip_dst] < 0)
		{
			/*
			// This is the case where we have out of order packets... log a message?

			in_addr dst;
			dst.s_addr = htonl(evidence->m_evidencePacket.ip_dst);
			char *dstIp = inet_ntoa(dst);
			cout << dstIp << "<-";

			in_addr src;
			src.s_addr = htonl(evidence->m_evidencePacket.ip_src);
			char *srcIp = inet_ntoa(src);
			cout << srcIp << endl;
			*/
		}
		else
		{
			//Calculate and add the interval into the feature data
			m_intervalTable[evidence->m_evidencePacket.ts - m_lastTimes[evidence->m_evidencePacket.ip_dst]]++;
		}

	}
	//Update or Insert the timestamp value in the table
	m_lastTimes[evidence->m_evidencePacket.ip_dst] = evidence->m_evidencePacket.ts;


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
	m_tcpPacketCount += rhs.m_tcpPacketCount;
	m_bytesTotal += rhs.m_bytesTotal;

	for(IP_Table::iterator it = rhs.m_IPTable.begin(); it != rhs.m_IPTable.end(); it++)
	{
		m_IPTable[it->first] += rhs.m_IPTable[it->first];
	}

	for(Port_Table::iterator it = rhs.m_PortTCPTable.begin(); it != rhs.m_PortTCPTable.end(); it++)
	{
		m_PortTCPTable[it->first] += rhs.m_PortTCPTable[it->first];
	}

	for(Port_Table::iterator it = rhs.m_PortUDPTable.begin(); it != rhs.m_PortUDPTable.end(); it++)
	{
		m_PortUDPTable[it->first] += rhs.m_PortUDPTable[it->first];
	}

	for(Packet_Table::iterator it = rhs.m_packTable.begin(); it != rhs.m_packTable.end(); it++)
	{
		m_packTable[it->first] += rhs.m_packTable[it->first];
	}

	for(Interval_Table::iterator it = rhs.m_intervalTable.begin(); it != rhs.m_intervalTable.end(); it++)
	{
		m_intervalTable[it->first] += rhs.m_intervalTable[it->first];
	}

	m_synCount += rhs.m_synCount;
	m_ackCount += rhs.m_ackCount;
	m_finCount += rhs.m_finCount;
	m_rstCount += rhs.m_rstCount;
	m_synAckCount += rhs.m_synAckCount;

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
	m_tcpPacketCount -= rhs.m_tcpPacketCount;
	m_bytesTotal -= rhs.m_bytesTotal;

	for(IP_Table::iterator it = rhs.m_IPTable.begin(); it != rhs.m_IPTable.end(); it++)
	{
		m_IPTable[it->first] -= rhs.m_IPTable[it->first];
	}

	for(Port_Table::iterator it = rhs.m_PortTCPTable.begin(); it != rhs.m_PortTCPTable.end(); it++)
	{
		m_PortTCPTable[it->first] -= rhs.m_PortTCPTable[it->first];
	}

	for(Port_Table::iterator it = rhs.m_PortUDPTable.begin(); it != rhs.m_PortUDPTable.end(); it++)
	{
		m_PortUDPTable[it->first] -= rhs.m_PortUDPTable[it->first];
	}

	for(Packet_Table::iterator it = rhs.m_packTable.begin(); it != rhs.m_packTable.end(); it++)
	{
		m_packTable[it->first] -= rhs.m_packTable[it->first];
	}
	for(Interval_Table::iterator it = rhs.m_intervalTable.begin(); it != rhs.m_intervalTable.end(); it++)
	{
		m_intervalTable[it->first] -= rhs.m_intervalTable[it->first];
	}


	m_synCount -= rhs.m_synCount;
	m_ackCount -= rhs.m_ackCount;
	m_finCount -= rhs.m_finCount;
	m_rstCount -= rhs.m_rstCount;
	m_synAckCount -= rhs.m_synAckCount;

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
		m_tcpPacketCount = 0;
		m_bytesTotal = 0;

		m_startTime = ~0;
		m_endTime = 0;
		m_lastTime = 0;
		m_intervalTable.clear();
		m_packTable.clear();
		m_IPTable.clear();
		m_PortTCPTable.clear();
		m_PortUDPTable.clear();
		m_lastTimes.clear();
}

uint32_t FeatureSet::SerializeFeatureData(u_char *buf, uint32_t bufferSize)
{
	uint32_t offset = 0;

	//Required, individual variables for calculation
	CalculateTimeInterval();

	SerializeChunk(buf, &offset, (char*)&m_totalInterval, sizeof m_totalInterval, bufferSize);
	SerializeChunk(buf, &offset, (char*)&m_packetCount, sizeof m_packetCount, bufferSize);
	SerializeChunk(buf, &offset, (char*)&m_bytesTotal, sizeof m_bytesTotal, bufferSize);
	SerializeChunk(buf, &offset, (char*)&m_startTime, sizeof m_startTime, bufferSize);
	SerializeChunk(buf, &offset, (char*)&m_endTime, sizeof m_endTime, bufferSize);
	SerializeChunk(buf, &offset, (char*)&m_lastTime, sizeof m_lastTime, bufferSize);

	SerializeChunk(buf, &offset, (char*)&m_rstCount, sizeof m_rstCount, bufferSize);
	SerializeChunk(buf, &offset, (char*)&m_ackCount, sizeof m_ackCount, bufferSize);
	SerializeChunk(buf, &offset, (char*)&m_synCount, sizeof m_synCount, bufferSize);
	SerializeChunk(buf, &offset, (char*)&m_finCount, sizeof m_finCount, bufferSize);
	SerializeChunk(buf, &offset, (char*)&m_synAckCount, sizeof m_synAckCount, bufferSize);
	SerializeChunk(buf, &offset, (char*)&m_tcpPacketCount, sizeof m_tcpPacketCount, bufferSize);

	SerializeHashTable<Interval_Table, time_t, uint32_t> (buf, &offset, m_intervalTable, ~0, bufferSize);
	SerializeHashTable<Packet_Table, uint16_t, uint32_t> (buf, &offset, m_packTable, 0, bufferSize);
	SerializeHashTable<IP_Table, uint32_t, uint32_t>     (buf, &offset, m_IPTable, 0, bufferSize);
	SerializeHashTable<Port_Table, in_port_t, uint32_t>  (buf, &offset, m_PortTCPTable, 0, bufferSize);
	SerializeHashTable<Port_Table, in_port_t, uint32_t>  (buf, &offset, m_PortUDPTable, 0, bufferSize);

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


	for(Port_Table::iterator it = m_PortTCPTable.begin(); it != m_PortTCPTable.end(); it++)
	{
		if(it->second)
		{
			count++;
		}
	}

	for(Port_Table::iterator it = m_PortUDPTable.begin(); it != m_PortUDPTable.end(); it++)
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
	uint32_t offset = 0;

	//Temporary variables to store and track data during deserialization
	uint32_t temp = 0;


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

	DeserializeChunk(buf, &offset, (char*)&temp, sizeof m_rstCount, bufferSize);
	m_rstCount += temp;

	DeserializeChunk(buf, &offset, (char*)&temp, sizeof m_ackCount, bufferSize);
	m_ackCount += temp;

	DeserializeChunk(buf, &offset, (char*)&temp, sizeof m_synCount, bufferSize);
	m_synCount += temp;

	DeserializeChunk(buf, &offset, (char*)&temp, sizeof m_finCount, bufferSize);
	m_finCount += temp;

	DeserializeChunk(buf, &offset, (char*)&temp, sizeof m_synAckCount, bufferSize);
	m_synAckCount += temp;

	DeserializeChunk(buf, &offset, (char*)&temp, sizeof m_tcpPacketCount, bufferSize);
	m_tcpPacketCount += temp;

	/***************************************************************************************************
	* For all of these tables we extract, the key (bin identifier) followed by the data (packet count)
	*  i += the # of packets in the bin, if we haven't reached packet count we know there's another item
	****************************************************************************************************/
	DeserializeHashTable<Interval_Table, time_t, uint32_t> (buf, &offset, m_intervalTable, bufferSize);
	DeserializeHashTable<Packet_Table, uint16_t, uint32_t> (buf, &offset, m_packTable, bufferSize);
	DeserializeHashTable<IP_Table, uint32_t, uint32_t>     (buf, &offset, m_IPTable, bufferSize);
	DeserializeHashTable<Port_Table, in_port_t, uint32_t>  (buf, &offset, m_PortTCPTable, bufferSize);
	DeserializeHashTable<Port_Table, in_port_t, uint32_t>  (buf, &offset, m_PortUDPTable, bufferSize);

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
	if (m_tcpPacketCount != rhs.m_tcpPacketCount)
	{
		return false;
	}
	if (m_ackCount != rhs.m_ackCount)
	{
		return false;
	}
	if (m_synCount != rhs.m_synCount)
	{
		return false;
	}
	if (m_finCount != rhs.m_finCount)
	{
		return false;
	}
	if (m_rstCount != rhs.m_rstCount)
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
