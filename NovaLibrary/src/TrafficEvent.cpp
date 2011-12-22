//============================================================================
// Name        : TrafficEvent.cpp
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Information on a single or set of packets that allows for
//					identification and classification of the sender.
//============================================================================/*

#include "TrafficEvent.h"
#include <arpa/inet.h>
#include <stdio.h>

using namespace std;

namespace Nova{
//
//	Constructors from the wire
//
//ICMP and UDP Constructor
// int component_source is one of FROM_HAYSTACK or FROM_LTM
TrafficEvent::TrafficEvent(struct Packet packet, int component_source)
{
	struct ip *ip_hdr = &packet.ip_hdr;
	if(ip_hdr == NULL)
	{
		return;
	}
	//Start and end times are the same since this is a one packet event
	start_timestamp = packet.pcap_header.ts.tv_sec;
	end_timestamp = packet.pcap_header.ts.tv_sec;
	packet_count = 1;
	dst_IP = packet.ip_hdr.ip_dst;
	src_IP = packet.ip_hdr.ip_src;
	//If UDP
	if(packet.ip_hdr.ip_p == 17)
	{
		dst_port = ntohs(packet.udp_hdr.dest);
		IP_total_data_bytes = ntohs(packet.ip_hdr.ip_len);
	}
	//If ICMP
	else if(packet.ip_hdr.ip_p == 1)
	{
		IP_total_data_bytes = ntohs(packet.ip_hdr.ip_len);
		dst_port = -1;
	}
	IP_packet_sizes.push_back(ntohs(packet.ip_hdr.ip_len));
	packet_intervals.push_back(packet.pcap_header.ts.tv_sec);
	from_haystack = component_source;
}

//TCP Constructor
// int component_source is one of FROM_HAYSTACK or FROM_LTM
TrafficEvent::TrafficEvent( vector<struct Packet>  *list, int component_source)
{
	struct Packet *packet = &list->front();
	start_timestamp = list->front().pcap_header.ts.tv_sec;
	end_timestamp = list->back().pcap_header.ts.tv_sec;
	packet_count = list->size();
	dst_IP = packet->ip_hdr.ip_dst;
	src_IP = packet->ip_hdr.ip_src;
	//Really complicated casting which grabs the dst and src ports
	dst_port =  ntohs(packet->tcp_hdr.dest);

	IP_total_data_bytes = 0;
	for(uint i = 0; i < list->size(); i++)
	{
		IP_total_data_bytes += ntohs((*list)[i].ip_hdr.ip_len);
		IP_packet_sizes.push_back(ntohs((*list)[i].ip_hdr.ip_len));
		packet_intervals.push_back((*list)[i].pcap_header.ts.tv_sec);
		//cout << ntohs((*list)[i].packet->ip_hdr.ip_len) << "\n";
	}
	from_haystack = component_source;
}

//Used in serialization
TrafficEvent::TrafficEvent()
{
	start_timestamp = 0;
	end_timestamp = 0;
	src_IP.s_addr = 0;
	dst_IP.s_addr = 0;
	dst_port = 0;
	IP_total_data_bytes = 0;
	packet_count = 0;
	from_haystack = 0;
}

//Outputs a string representation of the LogEntry to the screen
string TrafficEvent::ToString()
{
	string output;
	char temp[64];
	output = "Timestamp: ";
	output += ctime(&(start_timestamp));
	output += "\tTo ";
	output += ctime(&(end_timestamp));
	if(from_haystack)
	{
		output += "\tFrom: Haystack\n";
	}
	else
	{
		output += "\tFrom: Host\n";
	}
	output += "\tSource IP:\t";
	output += inet_ntoa(src_IP);
	output += "\n";
	output += "\tDestination IP:\t";
	output += inet_ntoa(dst_IP);
	output += "\n";
	if(dst_port != -1)
	{
		output += "\tTarget port:\t";
		bzero(temp, sizeof(temp));
		snprintf(temp, sizeof(temp), "%d", dst_port);
		output += temp;
		output += "\n";
	}
	output += "\tPacket Count:\t";
	bzero(temp, sizeof(temp));
	snprintf(temp, sizeof(temp), "%d", packet_count);
	output += temp;
	output += "\n";
	output += "\tRx bytes:\t";
	bzero(temp, sizeof(temp));
	snprintf(temp, sizeof(temp), "%d", IP_total_data_bytes);
	output += temp;
	output += "\n";
	output += "\n";
	return output;
}

/*
//Stores the traffic event information into the buffer, retrieved using deserializeEvent
//	returns the number of bytes set in the buffer
uint TrafficEvent::serializeEvent(u_char * buf)
{
	uint size = sizeof(uint); // 4 bytes
	uint offset = 0;

	//Clears a chunk of the buffer
	bzero(buf, ((11+2*packet_count)*size)+2);

	//Copies the value and increases the offset
	memcpy(buf, &start_timestamp, size);
	offset+= size;
	memcpy(buf+offset, &end_timestamp, size);
	offset+= size;
	memcpy(buf+offset, &src_IP.s_addr, size);
	offset+= size;
	memcpy(buf+offset, &dst_IP.s_addr, size);
	offset+= size;
	memcpy(buf+offset, &src_port, size);
	offset+= size;
	memcpy(buf+offset, &dst_port, size);
	offset+= size;
	memcpy(buf+offset, &IP_total_data_bytes, size);
	offset+= size;
	memcpy(buf+offset, &IP_protocol, size);
	offset+= size;
	memcpy(buf+offset, &ICMP_type, size);
	offset+= size;
	memcpy(buf+offset, &ICMP_code,  size);
	offset+= size;
	memcpy(buf+offset, &packet_count, size);
	offset+= size;
	//Despite being treated like an integer booleans only use 1 byte.
	memcpy(buf+offset, &from_haystack, 1);
	offset++;
	memcpy(buf+offset, &isHostile, 1);
	offset++;

	for(uint i = 0; i < packet_count; i++)
	{
		memcpy(buf+offset, &IP_packet_sizes[i], size);
		offset+= size;
	}
	for(uint i = 0; i < packet_count; i++)
	{
		memcpy(buf+offset, &packet_intervals[i], size);
		offset+= size;
	}
	//The offset at the end of execution is also equal to the number of bytes set.
	return offset;
}

//Reads TrafficEvent information from a buffer originally populated by serializeEvent
//	returns the number of bytes read from the buffer
uint TrafficEvent::deserializeEvent(u_char * buf)
{
	uint size = sizeof(uint);
	uint offset = 0;

	//Copies the value and increases the offset
	memcpy(&start_timestamp, buf, size);
	offset+= size;
	memcpy(&end_timestamp, buf+offset, size);
	offset+= size;
	memcpy(&src_IP.s_addr, buf+offset, size);
	offset+= size;
	memcpy(&dst_IP.s_addr, buf+offset, size);
	offset+= size;
	memcpy(&src_port, buf+offset, size);
	offset+= size;
	memcpy(&dst_port, buf+offset, size);
	offset+= size;
	memcpy(&IP_total_data_bytes, buf+offset, size);
	offset+= size;
	memcpy(&IP_protocol, buf+offset, size);
	offset+= size;
	memcpy(&ICMP_type, buf+offset, size);
	offset+= size;
	memcpy(&ICMP_code, buf+offset, size);
	offset+= size;
	memcpy(&packet_count, buf+offset, size);
	offset+= size;
	//Despite being treated like an integer booleans only use 1 byte.
	memcpy(&from_haystack, buf+offset, 1);
	offset++;
	memcpy(&isHostile, buf+offset, 1);
	offset++;

	IP_packet_sizes.resize(packet_count);
	for(uint i = 0; i < packet_count; i++)
	{
		memcpy(&IP_packet_sizes[i], buf+offset, size);
		offset += size;
	}

	packet_intervals.resize(packet_count);
	for(uint i = 0; i < packet_count; i++)
	{
		memcpy(&packet_intervals[i], buf+offset, size);
		offset += size;
	}
	//The offset at the end of execution is also equal to the number of bytes read.
	return offset;
}*/
}
