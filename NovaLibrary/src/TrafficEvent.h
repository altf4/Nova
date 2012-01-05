//============================================================================
// Name        : TrafficEvent.h
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Information on a single or set of packets that allows for
//					identification and classification of the sender.
//============================================================================/*


#include "NovaUtil.h"

#ifndef TRAFFICEVENT_H_
#define TRAFFICEVENT_H_

//Size of the static elements: 44
//Rest allocated for IP_packet_sizes. One int for each packet
//TODO: Increase this value if proven necessary.
#define TRAFFIC_EVENT_MAX_SIZE 1028

using namespace std;
namespace Nova{
///An event which occurs on the wire.
class TrafficEvent
{
	public:

		//********************
		//* Member Variables *
		//********************

		///Timestamp of the begining of this event
		time_t start_timestamp;
		///Timestamp of the end of this event
		time_t end_timestamp;

		///Source IP address of this event
		struct in_addr src_IP;
		///Destination IP address of this event
		struct in_addr dst_IP;

		///Destination port of this event
		in_port_t dst_port;

		///Total amount of IP layer bytes sent to the victim
		uint IP_total_data_bytes;

		///Packets involved in this event
		uint packet_count;

		///	False for from the host machine
		bool from_haystack;

		///For training use. Is this a hostile Event?
		//bool isHostile;

		///A vector of the sizes of the IP layers of
		vector <int> IP_packet_sizes;
		///A vector of start times for all packets in a tcp session
		vector <time_t> packet_intervals;

		//********************
		//* Member Functions *
		//********************

		///Used in serialization
		TrafficEvent();

		///UDP Constructor
		// int component_source is one of FROM_HAYSTACK or FROM_LTM
		TrafficEvent(struct Packet packet, int component_source);

		///TCP Constructor
		// int component_source is one of FROM_HAYSTACK or FROM_LTM
		TrafficEvent( vector <struct Packet>  *list, int component_source);

		///Returns a string representation of the LogEntry for printing to screen
		string ToString();

};

///	A struct version of a packet, as received from libpcap
struct Packet
{
	///	Meta information about packet
	struct pcap_pkthdr pcap_header;
	///	Pointer to an IP header
	struct ip ip_hdr;
	/// Pointer to a TCP Header
	struct tcphdr tcp_hdr;
	/// Pointer to a UDP Header
	struct udphdr udp_hdr;
	/// Pointer to an ICMP Header
	struct icmphdr icmp_hdr;
};
}

#endif /* TRAFFICEVENT_H_ */
