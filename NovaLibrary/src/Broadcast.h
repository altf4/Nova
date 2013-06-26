#ifndef BROADCAST_H
#define BROADCAST_H

#include <string>

namespace Nova
{

class Broadcast {
public:
	Broadcast();

	int m_srcPort;
	int m_dstPort;
	std::string m_script;

	int GetSrcPort() { return m_srcPort;}
	int GetDstPort() { return m_dstPort;}
	std::string GetScript() { return m_script; }
};


}

#endif
