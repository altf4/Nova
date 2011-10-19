//============================================================================
// Name        : TrafficEvent.cpp
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Information on a single or set of packets that allows for
//					identification and classification of the sender.
//============================================================================/*

#include "TrafficEvent.h"
#include <arpa/inet.h>
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
	this->start_timestamp = packet.pcap_header.ts.tv_sec;
	this->end_timestamp = packet.pcap_header.ts.tv_sec;
	this->packet_count = 1;
	this->dst_IP = packet.ip_hdr.ip_dst;
	this->src_IP = packet.ip_hdr.ip_src;
	this->IP_protocol = packet.ip_hdr.ip_p;
	//If UDP
	if(packet.ip_hdr.ip_p == 17)
	{
		this->dst_port = ntohs(packet.udp_hdr.dest);
		this->src_port = ntohs(packet.udp_hdr.source);
		this->IP_total_data_bytes = ntohs(packet.ip_hdr.ip_len);
		this->ICMP_type = -1;
		this->ICMP_code = -1;
	}
	//If ICMP
	else if(packet.ip_hdr.ip_p == 1)
	{
		this->ICMP_type = packet.icmp_hdr.type;
		this->ICMP_code = packet.icmp_hdr.code;
		this->IP_total_data_bytes = ntohs(packet.ip_hdr.ip_len);
		this->dst_port = -1;
		this->src_port = -1;
	}
	this->IP_packet_sizes.push_back(ntohs(packet.ip_hdr.ip_len));
	this->from_haystack = component_source;
	//Set known hostility, only used in training
	this->isHostile = component_source;
}

//TCP Constructor
// int component_source is one of FROM_HAYSTACK or FROM_LTM
TrafficEvent::TrafficEvent( vector<struct Packet>  *list, int component_source)
{
	struct Packet *packet = &list->front();
	this->start_timestamp = list->front().pcap_header.ts.tv_sec;
	this->end_timestamp = list->back().pcap_header.ts.tv_sec;
	this->packet_count = list->size();
	this->dst_IP = packet->ip_hdr.ip_dst;
	this->src_IP = packet->ip_hdr.ip_src;
	//Really complicated casting which grabs the dst and src ports
	this->dst_port =  ntohs(packet->tcp_hdr.dest);
	this->src_port =  ntohs(packet->tcp_hdr.source);
	this->IP_protocol = packet->ip_hdr.ip_p;
	if( this->IP_protocol != 6 )
	{
		cout << "Error with TCP protocol in list.\n";
		return;
	}
	this->IP_total_data_bytes = 0;
	for(uint i = 0; i < list->size(); i++)
	{
		this->IP_total_data_bytes += ntohs((*list)[i].ip_hdr.ip_len);
		this->IP_packet_sizes.push_back(ntohs((*list)[i].ip_hdr.ip_len));
		//cout << ntohs((*list)[i].packet->ip_hdr.ip_len) << "\n";
	}
	this->from_haystack = component_source;
	//Set known hostility, only used in training
	this->isHostile = component_source;
}

//Used in serialization
TrafficEvent::TrafficEvent()
{
	start_timestamp = 0;
	end_timestamp = 0;
	src_IP.s_addr = 0;
	dst_IP.s_addr = 0;
	src_port = 0;
	dst_port = 0;
	IP_total_data_bytes = 0;
	IP_protocol = 0;
	ICMP_type = 0;
	ICMP_code = 0;
	packet_count = 0;
	from_haystack = 0;
	isHostile = 0;
}

//Outputs a string representation of the LogEntry to the screen
string TrafficEvent::ToString()
{
	string output;
	char temp[64];
	output = "Timestamp: ";
	output += ctime(&(this->start_timestamp));
	output += "\tTo ";
	output += ctime(&(this->end_timestamp));
	if(this->from_haystack)
	{
		output += "\tFrom: Haystack\n";
	}
	else
	{
		output += "\tFrom: Host\n";
	}
	if(this->isHostile)
	{
		output += "\tIs Evil\n";
	}
	else
	{
		output += "\tIs Benign\n";
	}
	output += "\tIP Proto Type:\t";
	bzero(temp, sizeof(temp));
	snprintf(temp, sizeof(temp), "%d", this->IP_protocol);
	output += temp;
	output += "\n";
	output += "\tSource IP:\t";
	output += inet_ntoa(this->src_IP);
	output += "\n";
	output += "\tDestination IP:\t";
	output += inet_ntoa(this->dst_IP);
	output += "\n";
	if(this->IP_protocol == 1)
	{
		output += "\tICMP Type:\t";
		bzero(temp, sizeof(temp));
		snprintf(temp, sizeof(temp), "%d", this->ICMP_type);
		output += temp;
		output += "\n";
		output += "\tICMP Code:\t";
		bzero(temp, sizeof(temp));
		snprintf(temp, sizeof(temp), "%d", this->ICMP_code);
		output += temp;
		output += "\n";
	}
	//TCP or UDP
	else if(this->IP_protocol == 6 || this->IP_protocol == 17)
	{
		output += "\tSource port:\t";
		bzero(temp, sizeof(temp));
		snprintf(temp, sizeof(temp), "%d", this->src_port);
		output += temp;
		output += "\n";
		output += "\tTarget port:\t";
		bzero(temp, sizeof(temp));
		snprintf(temp, sizeof(temp), "%d", this->dst_port);
		output += temp;
		output += "\n";
	}
	output += "\tPacket Count:\t";
	bzero(temp, sizeof(temp));
	snprintf(temp, sizeof(temp), "%d", this->packet_count);
	output += temp;
	output += "\n";
	output += "\tRx bytes:\t";
	bzero(temp, sizeof(temp));
	snprintf(temp, sizeof(temp), "%d", this->IP_total_data_bytes);
	output += temp;
	output += "\n";
	output += "\n";
	return output;
}

//Returns true if the majority of packets are flagged as hostile
// Else, false
bool TrafficEvent::ArePacketsHostile( vector<struct Packet>  *list)
{
	uint sum = 0;
	for(uint i = 0; i < list->size(); i++)
	{
		if((((*list)[i].ip_hdr.ip_off) ^ IP_RF) == 0) //Evil bit
		{
			sum++;
		}
	}
	if(sum > (list->size() / 2))
	{
		return true;
	}
	else
	{
		return false;
	}
}

void TrafficEvent::copyTo(TrafficEvent *toEvent)
{
	toEvent->start_timestamp = this->start_timestamp;
	toEvent->end_timestamp = this->end_timestamp;
	toEvent->src_IP.s_addr = this->src_IP.s_addr;
	toEvent->dst_IP.s_addr = this->dst_IP.s_addr;
	toEvent->src_port = this->src_port;
	toEvent->dst_port = this->dst_port;
	toEvent->IP_total_data_bytes = this->IP_total_data_bytes;
	for(uint i = 0; i < this->IP_packet_sizes.size(); i++)
	{
		toEvent->IP_packet_sizes.push_back(this->IP_packet_sizes[i]);
	}
	toEvent->IP_protocol = this->IP_protocol;
	toEvent->ICMP_type = this->ICMP_type;
	toEvent->ICMP_code = this->ICMP_code;
	toEvent->packet_count = this->packet_count;
	toEvent->from_haystack = this->from_haystack;
	toEvent->isHostile = this->isHostile;
}
}
