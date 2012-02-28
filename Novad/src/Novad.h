//============================================================================
// Name        : Novad.h
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
// Description : Nova Daemon to perform network anti-reconnaissance
//============================================================================

#ifndef NOVAD_H_
#define NOVAD_H_

#include "HashMapStructs.h"
#include "Defines.h"
#include "Suspect.h"
#include <arpa/inet.h>

//Mode to knock on the silent alarm port
#define OPEN true
#define CLOSE false


//Hash table for current list of suspects
typedef google::dense_hash_map<in_addr_t, ANNpoint, tr1::hash<in_addr_t>, eqaddr > lastPointHash;

enum normalizationType {
	NONORM, 		// Does no data normalization. Feature must already be in a range from 0 to 1
	LINEAR,			// Simple linear normalization by dividing data by the max
	LINEAR_SHIFT, 	// Shifts min value to 0 before doing linear normalization
	LOGARITHMIC		// Logarithmic normalization, larger outlier value will have less of an effect
};

namespace Nova{

// Start routine for the GUI command listening thread
//		ptr - Required for pthread start routines
void *CE_GUILoop(void *ptr);

// Start routine for a separate thread which infinite loops, periodically
// updating all the classifications for all the current suspects
//		prt - Required for pthread start routines
void *ClassificationLoop(void *ptr);

// Start routine for thread that calculates training data, and used for writing to file.
//		prt - Required for pthread start routines
void *TrainingLoop(void *ptr);

// Startup routine for thread that listens for Silent Alarms from other Nova instances
//		prt - Required for pthread start routines
void *SilentAlarmLoop(void *ptr);

// Performs classification on given suspect
//		suspect - suspect to classify based on current evidence
// Note: this updates the classification of the suspect in dataPtsWithClass as well as it's isHostile variable
void Classify(Suspect *suspect);

// Normalized a single value
double Normalize(normalizationType type, double value, double min, double max);

// Calculates normalized data points and stores into 'normalizedDataPts'
void NormalizeDataPoints();

// Forms the normalized kd tree, called once on start up
// Will be called again if the a suspect's max value for a feature exceeds the current maximum for normalization
void FormKdTree();

// Prints a single ANN point, p, to stream, out
//		out - steam to print to
//		p 	- ANN point to print
void PrintPt(ostream &out, ANNpoint p);

// Reads into the list of suspects from a file specified by inFilePath
//		inFilePath - path to input file, should contain Feature dimensions
//					 followed by hostile classification (0 or 1), all space separated
void LoadDataPointsFromFile(string inFilePath);

// Writes the list of suspects out to a file specified by outFilePath
//		outFilePath - path to output file
void WriteDataPointsToFile(string outFilePath, ANNkd_tree* tree);

// Send a silent alarm
//		suspect - Suspect to send alarm about
void SilentAlarm(Suspect *suspect);

// Knocks on the port of the neighboring nova instance to open or close it
//		mode - true for OPEN, false for CLOSE
bool KnockPort(bool mode);

// Receive featureData from another local component.
// This is a blocking function. If nothing is received, then wait on this thread for an answer
// Returns: false if any sort of error
bool Start_Packet_Handler();

// Receives input commands from the GUI
// This is a blocking function. If nothing is received, then wait on this thread for an answer
void CEReceiveGUICommand();

// Sends output to the UI
//	suspect - suspect to serialize GUI data and send
void SendToUI(Suspect *suspect);

// Loads configuration variables
//		configFilePath - Location of configuration file
void LoadConfiguration();

// Dump the suspect information to a file
//		filename - Path to file to write to
void SaveSuspectsToFile(string filename);

// Append to state file
void AppendToStateFile();

// Cleans up the state file
//	- Removes old entries
//	- Removes cleared suspects
void RefreshStateFile();

// Appends all entries in the state file to the suspect table
void LoadStateFile();

// Runs appendToStateFile before exiting
void saveAndExit(int param);

// Force a reload of NOVAConfig/Data.txt while running.
// This will reclassify all the suspects based on the new data.
void Reload();

// Parse through the honeyd config file and get the list of IP addresses used
//		honeyDConfigPath - path to honeyd configuration file
// Returns: vector containing IP addresses of all honeypots
vector <string> GetHaystackAddresses(string honeyDConfigPath);
vector <string> GetHaystackDhcpAddresses(string honeyDConfigPath);

void *UpdateIPFilter(void *ptr);
string ConstructFilterString();

// Callback function that is passed to pcap_loop(..) and called each time a packet is received
//		useless - Unused
//		pkthdr - pcap packet header
//		packet - packet data
void Packet_Handler(u_char *useless,const struct pcap_pkthdr* pkthdr,const u_char* packet);

// Startup rotuine for thread periodically checking for TCP timeout.
// IE: Not all TCP sessions get torn down properly. Sometimes they just end midstram
// This thread looks for old tcp sessions and declares them terminated
//		ptr - Required for pthread start routines
void *TCPTimeout( void *ptr );

// Updates a suspect with evidence to be processed later
//		packet : Packet headers to used for the evidence
void UpdateSuspect(Packet packet);


}

#endif /* NOVAD_H_ */
