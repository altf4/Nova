//============================================================================
// Name        : TrafficEvent.h
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Information on a single or set of packets that allows for
//					identification and classification of the sender.
//============================================================================/*

#include <time.h>
#include <string.h>
#include <string>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <netinet/if_ether.h>
#include <vector>
#include <pcap.h>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <tr1/unordered_map>
#include <boost/serialization/vector.hpp>

#ifndef TRAFFICEVENT_H_
#define TRAFFICEVENT_H_

#define TRAFFIC_EVENT_MTYPE 2
#define SCAN_EVENT_MTYPE 3

//Size of the static elements: 44
//Rest allocated for IP_packet_sizes. One int for each packet
//TODO: Increase this value if proven necessary.
#define TRAFFIC_EVENT_MAX_SIZE 1028

//From the Haystack or Doppelganger Module
#define FROM_HAYSTACK_DP	1
//From the Local Traffic Monitor
#define FROM_LTM			0

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

		///Source port of this event
		in_port_t src_port;
		///Destination port of this event
		in_port_t dst_port;

		///Total amount of IP layer bytes sent to the victim
		uint IP_total_data_bytes;

		///A vector of the sizes of the IP layers of
		vector <int> IP_packet_sizes;

		///The IP proto type number
		///	IE: 6 == TCP, 17 == UDP, 1 == ICMP, etc...
		uint IP_protocol;

		///ICMP specific values
		///IE:	0,0 = Ping reply
		///		8,0 = Ping request
		///		3,3 = Destination port unreachable
		int ICMP_type;
		///IE:	0,0 = Ping reply
		///		8,0 = Ping request
		///		3,3 = Destination port unreachable
		int ICMP_code;

		///Packets involved in this event
		uint packet_count;

		///Did this event originate from the Haystack?
		///	False for from the host machine
		bool from_haystack;

		///For training use. Is this a hostile Event?
		bool isHostile;

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

		///Returns true if the majority of packets are flagged as hostile
		/// Else, false
		bool ArePacketsHostile( vector<struct Packet>  *list);

		//Copies the contents of this TrafficEvent to the parameter TrafficEvent
		void copyTo(TrafficEvent *toEvent);

	private:
		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive & ar, const unsigned int version)
		{
			ar & start_timestamp;
			ar & end_timestamp;
			ar & src_IP.s_addr;
			ar & dst_IP.s_addr;
			ar & src_port;
			ar & dst_port;
			ar & IP_total_data_bytes;
			ar & IP_packet_sizes;
			ar & IP_protocol;
			ar & ICMP_type;
			ar & ICMP_code;
			ar & packet_count;
			ar & from_haystack;
			ar & isHostile;
		}
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
