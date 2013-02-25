//============================================================================
// Name        : FeatureSet.cpp
// Copyright   : DataSoft Corporation 2011-2013
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

#include <time.h>
#include <math.h>
#include <sstream>
#include <sys/un.h>

using namespace std;

namespace Nova
{

string FeatureSet::m_featureNames[] =
{
	"IP Traffic Distribution",
	"Port Traffic Distribution",
	"Packet Size Mean",
	"Packet Size Deviation",
	"Protected IPs Contacted",
	"Distinct TCP Ports Contacted",
	"Distinct UDP Ports Contacted",
	"Average TCP Ports Per Host",
	"Average UDP Ports Per Host",
	"TCP Percent SYN",
	"TCP Percent FIN",
	"TCP Percent RST",
	"TCP Percent SYN ACK",
	"Haystack Percent Contacted",
};

FeatureSet::FeatureSet()
{
	m_numberOfHaystackNodesContacted = 0;

	//Temp variables
	m_startTime = 2147483647; //2^31 - 1 (IE: Y2.038K bug) (IE: The largest standard Unix time value)
	m_endTime = 0;
	m_totalInterval = 0;

	m_rstCount = 0;
	m_ackCount = 0;
	m_synCount = 0;
	m_finCount = 0;
	m_synAckCount = 0;

	m_packetCount = 0;
	m_tcpPacketCount = 0;
	m_udpPacketCount = 0;
	m_icmpPacketCount = 0;
	m_otherPacketCount = 0;
	m_bytesTotal = 0;
	m_lastTime = 0;

	//Features
	for(int i = 0; i < DIM; i++)
	{
		m_features[i] = 0;
	}
}

FeatureSet::~FeatureSet()
{
}

string FeatureSet::toString()
{
	stringstream ss;

	time_t start = m_startTime;
	time_t end = m_endTime;
	ss << "First packet seen at: " << ctime(&start) << endl;
	ss << "Last packet seen at: " << ctime(&end) << endl;
	ss << endl;
	ss << "Total bytes in IP packets: " << m_bytesTotal << endl;
	ss << "Packets seen: " << m_packetCount << endl;
	ss << "TCP Packets Seen: " << m_tcpPacketCount << endl;
	ss << "UDP Packets Seen: " << m_udpPacketCount << endl;
	ss << "ICMP Packets Seen: " << m_icmpPacketCount << endl;
	ss << "Other protocol Packets Seen: " << m_otherPacketCount << endl;
	ss << endl;
	ss << "TCP RST Packets: " << m_rstCount << endl;
	ss << "TCP ACK Packets: " << m_ackCount << endl;
	ss << "TCP SYN Packets: " << m_synCount << endl;
	ss << "TCP FIN Packets: " << m_finCount << endl;
	ss << "TCP SYN ACK Packets: " << m_synAckCount << endl;
	ss << endl;

	ss << "Contacted " << m_numberOfHaystackNodesContacted << " honeypot IP addresses out of " << m_HaystackIPTable.size() << endl;
	for (IP_Table::iterator it = m_HaystackIPTable.begin(); it != m_HaystackIPTable.end(); it++)
	{
		in_addr t;
		t.s_addr = ntohl(it->first);
		ss << "Contacted honeypot " << inet_ntoa(t) << "    " << boolalpha << (bool)it->second << endl;
	}
	ss << endl;

	ss << "IPs contacted and number of packets to IP: " << endl;
	for (IP_Table::iterator it = m_IPTable.begin(); it != m_IPTable.end(); it++)
	{
		in_addr t;
		t.s_addr = ntohl(it->first);
		ss << "    " << inet_ntoa(t) << "    " << it->second << endl;
	}
	ss << endl;

	ss << "TCP Ports contacted and number of packets to port: " << endl;
	for (Port_Table::iterator it = m_PortTCPTable.begin(); it != m_PortTCPTable.end(); it++)
	{
		ss << "    " << it->first << "    " << it->second << endl;
	}
	ss << endl;

	ss << "UDP Ports contacted and number of packets to port: " << endl;
	for (Port_Table::iterator it = m_PortUDPTable.begin(); it != m_PortUDPTable.end(); it++)
	{
		ss << "    " << it->first << "    " << it->second << endl;
	}
	ss << endl;

	return ss.str();
}

void FeatureSet::CalculateAll()
{
	CalculateTimeInterval();
	Calculate(IP_TRAFFIC_DISTRIBUTION);
	Calculate(PORT_TRAFFIC_DISTRIBUTION);
	Calculate(PACKET_SIZE_MEAN);
	Calculate(PACKET_SIZE_MEAN);
	Calculate(PACKET_SIZE_DEVIATION);
	Calculate(DISTINCT_IPS);
	Calculate(DISTINCT_TCP_PORTS);
	Calculate(DISTINCT_UDP_PORTS);
	Calculate(AVG_TCP_PORTS_PER_HOST);
	Calculate(AVG_UDP_PORTS_PER_HOST);
	Calculate(TCP_PERCENT_SYN);
	Calculate(TCP_PERCENT_FIN);
	Calculate(TCP_PERCENT_RST);
	Calculate(TCP_PERCENT_SYNACK);
	Calculate(HAYSTACK_PERCENT_CONTACTED);
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

			if (m_IPTable.size() == 0)
			{
				m_features[IP_TRAFFIC_DISTRIBUTION] = 0;
			}
			else
			{
				m_features[IP_TRAFFIC_DISTRIBUTION] = m_features[IP_TRAFFIC_DISTRIBUTION] / (double)m_IPTable.size();
			}

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
				portDivisor = portDivisor *((double)m_PortTCPTable.size() + (double)m_PortUDPTable.size());
				long long unsigned int temp = 0;
				for(Port_Table::iterator it = m_PortTCPTable.begin() ; it != m_PortTCPTable.end(); it++)
				{
					temp += it->second;
				}

				for(Port_Table::iterator it = m_PortUDPTable.begin() ; it != m_PortUDPTable.end(); it++)
				{
					temp += it->second;
				}

				if (portDivisor == 0)
				{
					m_features[PORT_TRAFFIC_DISTRIBUTION] = 0;
				}
				else
				{
					m_features[PORT_TRAFFIC_DISTRIBUTION] = ((double)temp)/portDivisor;
				}
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
		case DISTINCT_TCP_PORTS:
		{
			m_features[DISTINCT_TCP_PORTS] =  m_PortTCPTable.size();
			break;
		}
		/// Number of distinct ports contacted
		case DISTINCT_UDP_PORTS:
		{
			m_features[DISTINCT_UDP_PORTS] =  m_PortUDPTable.size();
			break;
		}
		case AVG_TCP_PORTS_PER_HOST:
		{
			if (m_tcpPortsContactedForIP.size() == 0)
			{
				m_features[AVG_TCP_PORTS_PER_HOST] = 0;
			}
			else
			{
				double acc = 0;
				for (IP_Table::iterator it = m_tcpPortsContactedForIP.begin(); it != m_tcpPortsContactedForIP.end(); it++)
				{
					acc += it->second;
				}
				m_features[AVG_TCP_PORTS_PER_HOST] = acc / m_tcpPortsContactedForIP.size();
			}
			break;
		}
		case AVG_UDP_PORTS_PER_HOST:
		{
			if (m_udpPortsContactedForIP.size() == 0)
			{
				m_features[AVG_UDP_PORTS_PER_HOST] = 0;
			}
			else
			{
				double acc = 0;
				for (IP_Table::iterator it = m_udpPortsContactedForIP.begin(); it != m_udpPortsContactedForIP.end(); it++)
				{
					acc += it->second;
				}

				m_features[AVG_UDP_PORTS_PER_HOST] = acc / m_udpPortsContactedForIP.size();
			}
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
			//cout << "TCP stats: synCount: " << synCount << " synAckCount: " << synAckCount << " ackCount: "
			//	<< ackCount << " finCount: " << finCount << " rstCount" << rstCount << endl;
			m_features[TCP_PERCENT_SYNACK] = ((double)m_synAckCount)/((double)m_tcpPacketCount + 1);
			break;
		}
		case HAYSTACK_PERCENT_CONTACTED:
		{
			if(m_HaystackIPTable.size())
			{
				m_features[HAYSTACK_PERCENT_CONTACTED] = (double)m_numberOfHaystackNodesContacted/(double)m_HaystackIPTable.size();
			}
			else
			{
				m_features[HAYSTACK_PERCENT_CONTACTED] = 0;
			}
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

void FeatureSet::UpdateEvidence(const Evidence &evidence)
{
	// Ensure our assumptions about valid packet fields are true
	if(evidence.m_evidencePacket.ip_dst == 0)
	{
		LOG(DEBUG, "Got packet with invalid source IP address of 0. Skipping.", "");
		return;
	}
	switch(evidence.m_evidencePacket.ip_p)
	{
		//If UDP
		case 17:
		{
			m_udpPacketCount++;
			m_PortUDPTable[evidence.m_evidencePacket.dst_port]++;

			IpPortCombination t;
			t.m_ip = evidence.m_evidencePacket.ip_dst;
			t.m_port = evidence.m_evidencePacket.dst_port;
			if (!m_hasUdpPortIpBeenContacted.keyExists(t))
			{
				m_hasUdpPortIpBeenContacted[t] = true;
				if (m_udpPortsContactedForIP.keyExists(t.m_ip))
				{
					m_udpPortsContactedForIP[t.m_ip]++;
				}
				else
				{
					m_udpPortsContactedForIP[t.m_ip] = 1;
				}
			}

			m_IPTable[evidence.m_evidencePacket.ip_dst]++;
			break;
		}
		//If TCP
		case 6:
		{
			m_tcpPacketCount++;

			// Only count as an IP/port contacted if it looks like a scan (SYN or NULL packet)
			if ((evidence.m_evidencePacket.tcp_hdr.syn && !evidence.m_evidencePacket.tcp_hdr.ack)
					|| (!evidence.m_evidencePacket.tcp_hdr.syn && !evidence.m_evidencePacket.tcp_hdr.ack
							&& !evidence.m_evidencePacket.tcp_hdr.rst))
			{
				m_IPTable[evidence.m_evidencePacket.ip_dst]++;

				m_PortTCPTable[evidence.m_evidencePacket.dst_port]++;

				IpPortCombination t;
				t.m_ip = evidence.m_evidencePacket.ip_dst;
				t.m_port = evidence.m_evidencePacket.dst_port;
				if (!m_hasTcpPortIpBeenContacted.keyExists(t))
				{
					m_hasTcpPortIpBeenContacted[t] = true;
					if (m_tcpPortsContactedForIP.keyExists(t.m_ip))
					{
						m_tcpPortsContactedForIP[t.m_ip]++;
					}
					else
					{
						m_tcpPortsContactedForIP[t.m_ip] = 1;
					}
				}
			}

			if(evidence.m_evidencePacket.tcp_hdr.syn && evidence.m_evidencePacket.tcp_hdr.ack)
			{
				m_synAckCount++;
			}
			else if(evidence.m_evidencePacket.tcp_hdr.syn)
			{
				m_synCount++;
			}
			else if(evidence.m_evidencePacket.tcp_hdr.ack)
			{
				m_ackCount++;
			}

			if(evidence.m_evidencePacket.tcp_hdr.rst)
			{
				m_rstCount++;
			}

			if(evidence.m_evidencePacket.tcp_hdr.fin)
			{
				m_finCount++;
			}

			break;
		}
		//If ICMP
		case 1:
		{
			m_icmpPacketCount++;
			m_IPTable[evidence.m_evidencePacket.ip_dst]++;
			break;
		}
		//If untracked IP protocol or error case ignore it
		default:
		{
			m_otherPacketCount++;
			m_IPTable[evidence.m_evidencePacket.ip_dst]++;
			break;
		}
	}

	m_packetCount++;
	m_bytesTotal += evidence.m_evidencePacket.ip_len;

	if(m_HaystackIPTable.keyExists(evidence.m_evidencePacket.ip_dst))
	{
		if (m_HaystackIPTable[evidence.m_evidencePacket.ip_dst] == 0)
		{
			m_numberOfHaystackNodesContacted++;
			m_HaystackIPTable[evidence.m_evidencePacket.ip_dst]++;
		}
	}

	m_packTable[evidence.m_evidencePacket.ip_len]++;
	m_lastTime = evidence.m_evidencePacket.ts;

	//Accumulate to find the lowest Start time and biggest end time.
	if(evidence.m_evidencePacket.ts < m_startTime)
	{
		m_startTime = evidence.m_evidencePacket.ts;
	}
	if(evidence.m_evidencePacket.ts > m_endTime)
	{
		m_endTime =  evidence.m_evidencePacket.ts;
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

	for(IP_Table::iterator it = rhs.m_tcpPortsContactedForIP.begin(); it != rhs.m_tcpPortsContactedForIP.end(); it++)
	{
		m_tcpPortsContactedForIP[it->first] += rhs.m_tcpPortsContactedForIP[it->first];
	}

	for(IP_Table::iterator it = rhs.m_udpPortsContactedForIP.begin(); it != rhs.m_udpPortsContactedForIP.end(); it++)
	{
		m_udpPortsContactedForIP[it->first] += rhs.m_udpPortsContactedForIP[it->first];
	}

	for (IpPortTable::iterator it = rhs.m_hasTcpPortIpBeenContacted.begin(); it != rhs.m_hasTcpPortIpBeenContacted.end(); it++)
	{
		m_hasTcpPortIpBeenContacted[it->first] = true;
	}

	for (IpPortTable::iterator it = rhs.m_hasUdpPortIpBeenContacted.begin(); it != rhs.m_hasUdpPortIpBeenContacted.end(); it++)
	{
		m_hasUdpPortIpBeenContacted[it->first] = true;
	}

	m_HaystackIPTable.clear();
	for(IP_Table::iterator it = rhs.m_HaystackIPTable.begin(); it != rhs.m_HaystackIPTable.end(); it++)
	{
			m_HaystackIPTable[it->first] += rhs.m_HaystackIPTable[it->first];
	}
	m_numberOfHaystackNodesContacted = rhs.m_numberOfHaystackNodesContacted;

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

	m_synCount += rhs.m_synCount;
	m_ackCount += rhs.m_ackCount;
	m_finCount += rhs.m_finCount;
	m_rstCount += rhs.m_rstCount;
	m_synAckCount += rhs.m_synAckCount;

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
	SerializeChunk(buf, &offset, (char*)&m_udpPacketCount, sizeof m_udpPacketCount, bufferSize);
	SerializeChunk(buf, &offset, (char*)&m_icmpPacketCount, sizeof m_icmpPacketCount, bufferSize);
	SerializeChunk(buf, &offset, (char*)&m_otherPacketCount, sizeof m_otherPacketCount, bufferSize);
	SerializeChunk(buf, &offset, (char*)&m_numberOfHaystackNodesContacted, sizeof m_numberOfHaystackNodesContacted, bufferSize);

	SerializeHashTable<Packet_Table, uint16_t, uint64_t> (buf, &offset, m_packTable, bufferSize);
	SerializeHashTable<IP_Table, uint32_t, uint64_t>     (buf, &offset, m_IPTable, bufferSize);
	SerializeHashTable<IP_Table, uint32_t, uint64_t>     (buf, &offset, m_HaystackIPTable, bufferSize);
	SerializeHashTable<Port_Table, in_port_t, uint64_t>  (buf, &offset, m_PortTCPTable, bufferSize);
	SerializeHashTable<Port_Table, in_port_t, uint64_t>  (buf, &offset, m_PortUDPTable, bufferSize);

	SerializeHashTable<IP_Table, uint32_t, uint64_t>  (buf, &offset, m_tcpPortsContactedForIP, bufferSize);
	SerializeHashTable<IP_Table, uint32_t, uint64_t>  (buf, &offset, m_udpPortsContactedForIP, bufferSize);
	SerializeHashTable<IpPortTable, IpPortCombination, uint8_t>  (buf, &offset, m_hasTcpPortIpBeenContacted, bufferSize);
	SerializeHashTable<IpPortTable, IpPortCombination, uint8_t>  (buf, &offset, m_hasUdpPortIpBeenContacted,bufferSize);

	return offset;
}

uint32_t FeatureSet::GetFeatureDataLength()
{
	uint32_t out = 0;

	//Vars we need to send useable Data
	out += sizeof(m_totalInterval)
			+ sizeof(m_packetCount)
			+ sizeof(m_bytesTotal)
			+ sizeof(m_startTime)
			+ sizeof(m_endTime)
			+ sizeof(m_lastTime);

	out += sizeof m_rstCount
			+ sizeof m_ackCount
			+ sizeof m_synCount
			+ sizeof m_finCount
			+ sizeof m_synAckCount
			+ sizeof m_tcpPacketCount
			+ sizeof m_udpPacketCount
			+ sizeof m_icmpPacketCount
			+ sizeof m_otherPacketCount
			+ sizeof m_numberOfHaystackNodesContacted;

	out += GetSerializeHashTableLength<Packet_Table, uint16_t, uint64_t> (m_packTable);
	out += GetSerializeHashTableLength<IP_Table, uint32_t, uint64_t>     (m_IPTable);
	out += GetSerializeHashTableLength<IP_Table, uint32_t, uint64_t>     (m_HaystackIPTable);
	out += GetSerializeHashTableLength<Port_Table, in_port_t, uint64_t>  (m_PortTCPTable);
	out += GetSerializeHashTableLength<Port_Table, in_port_t, uint64_t>  (m_PortUDPTable);
	out += GetSerializeHashTableLength<IP_Table, uint32_t, uint64_t> (m_tcpPortsContactedForIP);
	out += GetSerializeHashTableLength<IP_Table, uint32_t, uint64_t> (m_udpPortsContactedForIP);
	out += GetSerializeHashTableLength<IpPortTable, IpPortCombination, uint8_t> (m_hasTcpPortIpBeenContacted);
	out += GetSerializeHashTableLength<IpPortTable, IpPortCombination, uint8_t> (m_hasUdpPortIpBeenContacted);

	return out;
}


uint32_t FeatureSet::DeserializeFeatureData(u_char *buf, uint32_t bufferSize)
{
	uint32_t offset = 0;

	//Temporary variables to store and track data during deserialization
	uint64_t temp = 0;

	//Required, individual variables for calculation
	DeserializeChunk(buf, &offset, (char*)&m_totalInterval, sizeof m_totalInterval, bufferSize);
	DeserializeChunk(buf, &offset, (char*)&m_packetCount, sizeof m_packetCount, bufferSize);
	DeserializeChunk(buf, &offset, (char*)&m_bytesTotal, sizeof m_bytesTotal, bufferSize);
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

	DeserializeChunk(buf, &offset, (char*)&m_rstCount, sizeof m_rstCount, bufferSize);
	DeserializeChunk(buf, &offset, (char*)&m_ackCount, sizeof m_ackCount, bufferSize);
	DeserializeChunk(buf, &offset, (char*)&m_synCount, sizeof m_synCount, bufferSize);
	DeserializeChunk(buf, &offset, (char*)&m_finCount, sizeof m_finCount, bufferSize);
	DeserializeChunk(buf, &offset, (char*)&m_synAckCount, sizeof m_synAckCount, bufferSize);

	DeserializeChunk(buf, &offset, (char*)&m_tcpPacketCount, sizeof m_tcpPacketCount, bufferSize);
	DeserializeChunk(buf, &offset, (char*)&m_udpPacketCount, sizeof m_udpPacketCount, bufferSize);
	DeserializeChunk(buf, &offset, (char*)&m_icmpPacketCount, sizeof m_icmpPacketCount, bufferSize);
	DeserializeChunk(buf, &offset, (char*)&m_otherPacketCount, sizeof m_otherPacketCount, bufferSize);

	DeserializeChunk(buf, &offset, (char*)&temp, sizeof m_numberOfHaystackNodesContacted, bufferSize);
	m_numberOfHaystackNodesContacted = temp;

	/***************************************************************************************************
	 For all of these tables we extract, the key (bin identifier) followed by the data (packet count)
	 i += the # of packets in the bin, if we haven't reached packet count we know there's another item
	****************************************************************************************************/
	DeserializeHashTable<Packet_Table, uint16_t, uint64_t> (buf, &offset, m_packTable, bufferSize);
	DeserializeHashTable<IP_Table, uint32_t, uint64_t>     (buf, &offset, m_IPTable, bufferSize);
	DeserializeHashTable<IP_Table, uint32_t, uint64_t>     (buf, &offset, m_HaystackIPTable, bufferSize);
	DeserializeHashTable<Port_Table, in_port_t, uint64_t>  (buf, &offset, m_PortTCPTable, bufferSize);
	DeserializeHashTable<Port_Table, in_port_t, uint64_t>  (buf, &offset, m_PortUDPTable, bufferSize);

	DeserializeHashTable<IP_Table, uint32_t, uint64_t>  (buf, &offset, m_tcpPortsContactedForIP, bufferSize);
	DeserializeHashTable<IP_Table, uint32_t, uint64_t>  (buf, &offset, m_udpPortsContactedForIP, bufferSize);
	DeserializeHashTable<IpPortTable, IpPortCombination, uint8_t>  (buf, &offset, m_hasTcpPortIpBeenContacted, bufferSize);
	DeserializeHashTable<IpPortTable, IpPortCombination, uint8_t>  (buf, &offset, m_hasUdpPortIpBeenContacted, bufferSize);

	return offset;
}

void FeatureSet::SetHaystackNodes(std::vector<uint32_t> nodes)
{
	// TODO DTC
	// We could possibly do something a little more advanced here. If an IP was in
	// the old list and also is in the new list, we could update the data instead of just
	// deleting all of our old data. Not worrying about it right now though.
	m_HaystackIPTable.clear();
	m_numberOfHaystackNodesContacted = 0;

	for (uint i = 0; i < nodes.size(); i++) {
		m_HaystackIPTable[nodes.at(i)] = 0;
	}
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
	if(m_tcpPacketCount != rhs.m_tcpPacketCount)
	{
		return false;
	}
	if(m_ackCount != rhs.m_ackCount)
	{
		return false;
	}
	if(m_synCount != rhs.m_synCount)
	{
		return false;
	}
	if(m_finCount != rhs.m_finCount)
	{
		return false;
	}
	if(m_rstCount != rhs.m_rstCount)
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

	int size1 = m_PortTCPTable.size();
	int size2 = rhs.m_PortTCPTable.size();
	if (size1 != size2)
	{
		return false;
	}

	size1 = m_HaystackIPTable.size();
	size2 = rhs.m_HaystackIPTable.size();
	if (size1 != size2)
	{
		return false;
	}

	if (m_numberOfHaystackNodesContacted != rhs.m_numberOfHaystackNodesContacted)
	{
		return false;
	}
	return true;
}

bool FeatureSet::operator !=(const FeatureSet &rhs) const
{
	return !(*this == rhs);
}

}
