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

namespace Nova
{

class PacketCapture
{
	std::function packetCb;

public:
	void SetPacketCb(std::function<void(u_char *index,const struct pcap_pkthdr *pkthdr,const u_char *packet)> cb)
	{
		packetCb = cb;
	}

};


class PacketCaptureException : public std::exception {
public:
	std::string s;
	PacketCaptureException(std::string ss) : s(ss) {}

	const char* what() const throw()
	{
		return s.c_str();
	}
};

} /* namespace Nova */
#endif /* PACKETCAPTURE_H_ */
