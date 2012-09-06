//============================================================================
// Name        : PacketCapture.cpp
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
// Description : 
//============================================================================/*

#include "PacketCapture.h"
#include "Logger.h"

#include <pthread.h>
#include <signal.h>

using namespace Nova;
using namespace std;

PacketCapture::PacketCapture()
{
	m_handle = NULL;
	m_packetCb = NULL;
}

void PacketCapture::SetPacketCb(void (*cb)(unsigned char *index, const struct pcap_pkthdr *pkthdr, const unsigned char *packet))
{
	m_packetCb = cb;
}

void PacketCapture::SetFilter(string filter)
{
	struct bpf_program fp;

	if(pcap_compile(m_handle, &fp, filter.c_str(), 0, PCAP_NETMASK_UNKNOWN) == -1)
	{
		throw PacketCaptureException("Couldn't parse filter: "+filter+ " " + pcap_geterr(m_handle) +".");
	}

	if(pcap_setfilter(m_handle, &fp) == -1)
	{
		throw PacketCaptureException("Couldn't install filter: "+filter+ " " + pcap_geterr(m_handle) +".");
	}

	pcap_freecode(&fp);

}

int PacketCapture::GetDroppedPackets()
{
	pcap_stat captureStats;
	int result = pcap_stats(m_handle, &captureStats);

	if (result != 0)
	{
		return -1;
	}
	else
	{
		return captureStats.ps_drop;
	}
}

bool PacketCapture::StartCapture()
{
	LOG(DEBUG, "Starting packet capture on: " + identifier, "");
	return (pthread_create(&m_thread, NULL, InternalThreadEntryFunc, this) == 0);
}

bool PacketCapture::StartCaptureBlocking()
{
	LOG(DEBUG, "Starting packet capture on: " + identifier, "");
	return (pcap_loop(m_handle, -1, m_packetCb, 0) == 0);
}

void PacketCapture::StopCapture()
{
	// Kill and wait for the child thread to exit
	pcap_breakloop(m_handle);
	pthread_join(m_thread, NULL);

	pcap_close(m_handle);
	m_handle = NULL;
}


void PacketCapture::InternalThreadEntry()
{
	pcap_loop(m_handle, -1, m_packetCb, 0);
	LOG(DEBUG, "Dropped out of pcap loop", "");
}


