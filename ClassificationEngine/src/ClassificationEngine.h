//============================================================================
// Name        : ClassificationEngine.h
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Classifies suspects as either hostile or benign then takes appropriate action
//============================================================================

#ifndef CLASSIFICATIONENGINE_H_
#define CLASSIFICATIONENGINE_H_

#include <NovaUtil.h>
#include <Suspect.h>

//Mode to knock on the silent alarm port
#define OPEN true
#define CLOSE false

//Used in classification algorithm. Store it here so we only need to calculate it once
double sqrtDIM;

//Hash table for current list of suspects
typedef google::dense_hash_map<in_addr_t, Suspect*, tr1::hash<in_addr_t>, eqaddr > SuspectHashTable;

namespace Nova{
namespace ClassificationEngine{

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

///Knocks on the port of the neighboring nova instance to open or close it
bool knockPort(bool mode);

///Receive featureData from another local component.
/// This is a blocking function. If nothing is received, then wait on this thread for an answer
bool ReceiveSuspectData();

/// Receives input commands from the GUI
void ReceiveGUICommand();

//Sends output to the UI
void SendToUI(Suspect *suspect);

//Loads configuration variables from NOVAConfig_CE.txt or specified config file
void LoadConfig(char * input);

// Dump the suspect information to a file
void saveSuspectsToFile(string filename);

}
}
#endif /* CLASSIFICATIONENGINE_H_ */
