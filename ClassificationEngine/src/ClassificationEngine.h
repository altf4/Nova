//============================================================================
// Name        : ClassificationEngine.h
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Classifies suspects as either hostile or benign then takes appropriate action
//============================================================================

#ifndef CLASSIFICATIONENGINE_H_
#define CLASSIFICATIONENGINE_H_

#include <TrafficEvent.h>
#include "Suspect.h"
#include <ANN/ANN.h>
#include <google/dense_hash_map>

//TODO: Make Nova create this file on startup or installation.
///	Filename of the file to be used as an IPC key
// See ticket #12

#define KEY_FILENAME "/keys/NovaIPCKey"
///	Filename of the file to be used as an Doppelganger IPC key
#define KEY_ALARM_FILENAME "/keys/NovaDoppIPCKey"
///	Filename of the file to be used as an Classification Engine IPC key
#define CE_FILENAME "/keys/CEKey"
/// File name of the file to be used as GUI Input IPC key.
#define GUI_FILENAME "/keys/GUI_CEKey"
//Sets the Initial Table size for faster operations
#define INITIAL_TABLESIZE 256
//Number of messages to queue in a listening socket before ignoring requests until the queue is open
#define SOCKET_QUEUE_SIZE 50


///The maximum message, as defined in /proc/sys/kernel/msgmax
#define MAX_MSG_SIZE 65535
//dimension
#define DIM 9
//Number of values read from the NOVAConfig file
#define CONFIG_FILE_LINE_COUNT 10
//Number of messages to queue in a listening socket before ignoring requests until the queue is open
#define SOCKET_QUEUE_SIZE 50

//Used in classification algorithm. Store it here so we only need to calculate it once
const double sqrtDIM = sqrt(DIM);

//Equality operator used by google's dense hash map
struct eq
{
  bool operator()(in_addr_t s1, in_addr_t s2) const
  {
    return (s1 == s2);
  }
};

namespace Nova{
namespace ClassificationEngine{

//Hash table for current list of suspects
typedef google::dense_hash_map<in_addr_t, Suspect*, tr1::hash<in_addr_t>, eq > SuspectHashTable;

// Thread for listening for GUI commands
void *GUILoop(void *ptr);

//Separate thread which infinite loops, periodically updating all the classifications
//	for all the current suspects
void *ClassificationLoop(void *ptr);

///Thread for calculating training data, and writing to file.
void *TrainingLoop(void *ptr);

///Thread for listening for Silent Alarms from other Nova instances
void *SilentAlarmLoop(void *ptr);

///Performs classification on given suspect
void Classify(Suspect *suspect);

///Calculates normalized data points and stores into 'normalizedDataPts'
void NormalizeDataPoints();

///Reforms the kd tree in the vent that a suspects' feature exceeds the current max value for normalization
void FormKdTree();

///Subroutine to copy the data points in 'suspects' to their respective ANN Points
void CopyDataToAnnPoints();

///Prints a single ANN point, p, to stream, out
void printPt(ostream &out, ANNpoint p);

///Reads into the list of suspects from a file specified by inFilePath
void LoadDataPointsFromFile(string inFilePath);

///Writes the list of suspects out to a file specified by outFilePath
void WriteDataPointsToFile(string outFilePath);

///Returns usage tips
string Usage();

///Returns a string representation of the given local device's IP address
string getLocalIP(const char *dev);

///Send a silent alarm about the argument suspect
void SilentAlarm(Suspect *suspect);

///Receive a TrafficEvent from another local component.
/// This is a blocking function. If nothing is received, then wait on this thread for an answer
bool ReceiveTrafficEvent();

/// Receives input commands from the GUI
void ReceiveGUICommand();

//Sends output to the UI
void SendToUI(Suspect *suspect);

//Loads configuration variables from NOVAConfig_CE.txt or specified config file
void LoadConfig(char * input);

}
}
#endif /* CLASSIFICATIONENGINE_H_ */
