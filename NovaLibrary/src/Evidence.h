//============================================================================
// Name        : Evidence.h
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
// Description : Evidence object represents a preprocessed ip packet
//					for inclusion in a Suspect's Feature Set
//============================================================================/*

#ifndef EVIDENCE_H_
#define EVIDENCE_H_

#include <pcap.h>
#include <arpa/inet.h>

using namespace std;

namespace Nova
{

struct _evidencePacket //* Total of 18 bytes */
{
	uint16_t ip_len; 	//Length in bytes
	uint8_t ip_p;		//Ip protocol (UDP, TCP or ICMP)
	uint32_t ip_src;	//Source IPv4 address
	uint32_t ip_dst;	//Destination IPv4 address
	uint16_t dst_port;	//Destination Port (UDP or TCP Only)
	time_t ts;		//Arrival Timestamp (in seconds)
};

class Evidence
{

public:

	_evidencePacket m_evidencePacket;
	Evidence *m_next;

	Evidence(const u_char *packet_at_ip_header, const pcap_pkthdr *pkthdr);

	Evidence(Evidence *evidence);

	Evidence();

};

}

#endif /* EVIDENCE_H_ */
