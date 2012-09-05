//============================================================================
// Name        : InterfacePacketCapture.h
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

#ifndef INTERFACEPACKETCAPTURE_H_
#define INTERFACEPACKETCAPTURE_H_

#include <string>
#include <pcap.h>

namespace Nova
{

class InterfacePacketCapture : public PacketCapture
{
public:
	InterfacePacketCapture(std::string interface, std::string filter);
	void Setfilter(std::string filter);
	bool StartCapture();
	void Init();

private:
	static void * InternalThreadEntryFunc(void * This) {
		((InterfacePacketCapture*)This)->InternalThreadEntry();
		return NULL;
	}

	void InternalThreadEntry(void *ptr);

	char m_errorbuf[PCAP_ERRBUF_SIZE];
	pcap_t m_handle;
	int m_dropCount;

	uint m_interfaceIp;

	pthread_t m_thread;

	std::string m_interface;
	std::string m_filter;
};

} /* namespace Nova */
#endif /* INTERFACEPACKETCAPTURE_H_ */
