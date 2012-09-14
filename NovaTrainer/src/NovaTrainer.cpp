//============================================================================
// Name        : NovaTrainer.cpp
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
// Description : Command line interface for Nova
//============================================================================

#include "InterfacePacketCapture.h"
#include "ClassificationEngine.h"
#include "FilePacketCapture.h"
#include "HaystackControl.h"
#include "EvidenceTable.h"
#include "SuspectTable.h"
#include "NovaTrainer.h"
#include "TrainingDump.h"
#include "Evidence.h"
#include "Logger.h"

#include <netinet/if_ether.h>
#include <unistd.h>
#include <vector>
#include <errno.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <signal.h>


using namespace std;
using namespace Nova;

// Maintains a list of suspects and information on network activity
SuspectTable suspects;

ClassificationEngine *engine;
EvidenceTable suspectEvidence;

pcap_dumper_t *pcapDumpStream;

ofstream trainingFileStream;

trainingMode mode;
string captureFolder;


int main(int argc, const char *argv[])
{
	if(argc < 2)
	{
		PrintUsage();
	}

    signal(SIGKILL, SaveAndExit);
    signal(SIGINT, SaveAndExit);
    signal(SIGTERM, SaveAndExit);
    signal(SIGPIPE, SIG_IGN);


	if (string(argv[1]) == "--capture")
	{
		mode = trainingMode_capture;
		captureFolder = string(argv[2]);
		string interface = string(argv[3]);

		CaptureData(captureFolder, interface);
	}
	else if (string(argv[1]) == "--convert")
	{
		mode = trainingMode_convert;
		captureFolder = string(argv[2]);

		ConvertCaptureToDump(captureFolder);
	}
	else if (string(argv[1]) == "--save")
	{
		mode = trainingMode_save;
		captureFolder = string(argv[2]);
		string databaseFile = string(argv[3]);

		SaveToDatabaseFile(captureFolder, databaseFile);
	}
	else
	{
		PrintUsage();
	}

	return EXIT_SUCCESS;
}

namespace Nova
{

void PrintUsage()
{
	cout << "Usage:" << endl;
	// TODO
	cout << "  novatrainer --writeToDatabase novaCaptureFolder databaseFile.db" << endl;
	cout << "  novatrainer --convert novaCaptureFolder" << endl;
	cout << "  novatrainer --capture novaCaptureFolder interface" << endl;
	cout << endl;

	exit(EXIT_FAILURE);
}

void SaveAndExit(int param)
{
	if (mode == trainingMode_capture)
	{
		pcap_dump_close(pcapDumpStream);
	}

	exit(EXIT_SUCCESS);
}

void HandleTrainingPacket(u_char *index,const struct pcap_pkthdr *pkthdr,const u_char *packet)
{
	if(packet == NULL)
	{
		LOG(ERROR, "Failed to capture packet!","");
		return;
	}
	switch(ntohs(*(uint16_t *)(packet+12)))
	{
		//IPv4, currently the only handled case
		case ETHERTYPE_IP:
		{
			//Prepare Packet structure
			Evidence evidencePacket(packet + sizeof(struct ether_header), pkthdr);
			uint32_t ipSrc = evidencePacket.m_evidencePacket.ip_src;

			suspects.ProcessEvidence(&evidencePacket, true);
			update(ipSrc);

			return;
		}
		default:
		{
			return;
		}
	}
}

void update(const in_addr_t& key)
{
	in_addr_t foo = key;
	suspects.ClassifySuspect(foo);

	//Check that we updated correctly
	Suspect suspectCopy = suspects.GetSuspect(foo);

	if(suspects.IsEmptySuspect(&suspectCopy))
	{
		cout << "Got an empty suspect when trying to classify" << endl;
		return;
	}

	//Store in training file if needed
	trainingFileStream << suspectCopy.GetIpString() << " ";

	FeatureSet fs = suspectCopy.GetFeatureSet(MAIN_FEATURES);
	if(fs.m_features[0] != fs.m_features[0] )
	{
		cout << "This can't be good..." << endl;
	}
	for(int j = 0; j < DIM; j++)
	{
		trainingFileStream << fs.m_features[j] << " ";
	}
	trainingFileStream << "\n";
}


void UpdateHaystackFeatures(string haystackFilePath)
{
	vector<string> haystackAddresses = Config::GetIpAddresses(haystackFilePath);

	vector<uint32_t> haystackNodes;
	for(uint i = 0; i < haystackAddresses.size(); i++)
	{
		cout << haystackAddresses[i] << " has been set as a haystack address" << endl;
		haystackNodes.push_back(htonl(inet_addr(haystackAddresses[i].c_str())));
	}

	suspects.SetHaystackNodes(haystackNodes);
}


void ConvertCaptureToDump(std::string captureFolder)
{
	if(chdir(Config::Inst()->GetPathHome().c_str()) == -1)
	{
		LOG(CRITICAL, "Unable to change folder to " + Config::Inst()->GetPathHome(), "");
	}


	engine = new ClassificationEngine(suspects);
	engine->LoadDataPointsFromFile(Config::Inst()->GetPathTrainingFile());


	string dumpFile = captureFolder + "/nova.dump";
	string pcapFile = captureFolder + "/capture.pcap";

   	string haystackFile = captureFolder + "/haystackIps.txt";
	UpdateHaystackFeatures(haystackFile);


	trainingFileStream.open(dumpFile);
	if(!trainingFileStream.is_open())
	{
		LOG(CRITICAL, "Unable to open the training capture file.", "Unable to open training capture file at: "+dumpFile);
	}

	FilePacketCapture capture(pcapFile);
	capture.SetPacketCb(HandleTrainingPacket);
	capture.Init();
	capture.StartCaptureBlocking();


	LOG(DEBUG, "Done processing PCAP file.", "");

	trainingFileStream.close();
}

void CaptureData(std::string captureFolder, std::string interface)
{
	LOG(DEBUG, "Starting data capture", "");

    // Write out the state of the haystack at capture
    if(IsHaystackUp())
    {
    	string haystackFile = captureFolder + "/haystackIps.txt";
        vector<string> haystackAddresses = Config::GetHaystackAddresses(Config::Inst()->GetPathHome() + "/config/" + Config::Inst()->GetPathConfigHoneydHS());
        vector<string> haystackDhcpAddresses = Config::GetIpAddresses("/var/log/honeyd/ipList");

        ofstream haystackIpStream(haystackFile);
        for(uint i = 0; i < haystackDhcpAddresses.size(); i++)
        {
            haystackIpStream << haystackDhcpAddresses.at(i) << endl;
        }
        for(uint i = 0; i < haystackAddresses.size(); i++)
        {
            haystackIpStream << haystackAddresses.at(i) << endl;
        }

        haystackIpStream.close();
    }

    // Prepare for packet capture
	string trainingCapFile = captureFolder + "/capture.pcap";

    if(system(string("mkdir " + captureFolder).c_str()))
    {
        // Not really an problem, throws compiler warning if we don't catch the system call though
    }

    InterfacePacketCapture *capture = new InterfacePacketCapture(interface);
    capture->Init();
    capture->SetPacketCb(SavePacket);

    pcap_t *handle = capture->GetPcapHandle();
    pcapDumpStream = pcap_dump_open(handle, trainingCapFile.c_str());

    capture->StartCaptureBlocking();
}

void SavePacket(u_char *index,const struct pcap_pkthdr *pkthdr,const u_char *packet)
{
	 pcap_dump((u_char *)pcapDumpStream, pkthdr, packet);
}

void SaveToDatabaseFile(string captureFolder, string databaseFile)
{
	string hostilesFile = captureFolder + "/hostiles.txt";
	vector<string> hostileIps = Config::GetIpAddresses(hostilesFile);


	string dumpFile = captureFolder + "/nova.dump";

	TrainingDump dump;
	dump.LoadCaptureFile(dumpFile);
	dump.SetAllIsHostile(false);
	dump.SetAllIsIncluded(true);

	for (vector<string>::iterator it = hostileIps.begin(); it != hostileIps.end(); it++)
	{
		if (!dump.SetIsHostile(*it, true))
		{
			LOG(ERROR, "hostiles.txt specifies IP " + (*it) + " as hostile, but it does not exist in the capture file.", "");
		}
	}

	dump.MergeIPs(hostileIps, "Hostile IPs from capture: " + captureFolder);
	dump.MergeBenign("Benign IPs from capture: " + captureFolder);
	dump.SaveToDb(databaseFile);
}


}
