//============================================================================
// Name        : main.h
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Creates and runs the NovaGUI object
//============================================================================/*

#ifndef MAIN_H_
#define MAIN_H_

///Receive a input from Classification Engine.
/// This is a blocking function. If nothing is received, then wait on this thread for an answer
bool ReceiveCE(int socket, NovaGUI *w);

///Receive a input from Haystack Monitor.
/// This is a blocking function. If nothing is received, then wait on this thread for an answer
bool ReceiveHS(int socket, NovaGUI *w);

///Receive a input from Doppelganger Module.
/// This is a blocking function. If nothing is received, then wait on this thread for an answer
bool ReceiveDM(int socket, NovaGUI *w);

///Receive a input from Local Traffic Monitor.
/// This is a blocking function. If nothing is received, then wait on this thread for an answer
bool ReceiveLTM(int socket, NovaGUI *w);

#endif /* MAIN_H_ */
