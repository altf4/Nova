//============================================================================
// Name        : PacketCapture.h
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

#ifndef PACKETCAPTURE_H_
#define PACKETCAPTURE_H_

#include <string>
#include <pcap.h>

namespace Nova
{

class PacketCapture
{
public:
	PacketCapture();

	void SetPacketCb(void (*cb)(unsigned char *index, const struct pcap_pkthdr *pkthdr, const unsigned char *packet));
	void SetFilter(std::string filter);

	pcap_t* GetPcapHandle();

	bool StartCapture();
	bool StartCaptureBlocking();

	void StopCapture();

	int GetDroppedPackets();

protected:
	std::string m_identifier;
	pcap_t *m_handle;

	// This is so we can run blocking pcap_loop in it's own thread
	pthread_t m_thread;
	char m_errorbuf[PCAP_ERRBUF_SIZE];

	void InternalThreadEntry();

	// Work around for conversion of class method to C style function pointer for pcap
	void (*m_packetCb)(unsigned char *index, const struct pcap_pkthdr *pkthdr, const unsigned char *packet);
	static void * InternalThreadEntryFunc(void * This) {
		((PacketCapture*)This)->InternalThreadEntry();
		return NULL;
	}

};


class PacketCaptureException : public std::exception {
public:
	std::string s;
	PacketCaptureException(std::string ss) : s(ss) {}
	~PacketCaptureException() throw () {}

	const char* what() const throw()
	{
		return s.c_str();
	}
};

} /* namespace Nova */
#endif /* PACKETCAPTURE_H_ */
