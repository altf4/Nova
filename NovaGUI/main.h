/*
 * main.h
 *
 *  Created on: Oct 13, 2011
 *      Author: root
 */

#ifndef MAIN_H_
#define MAIN_H_

///	Filename of the file to be used as an Classification Engine IPC key
#define CE_FILENAME "/etc/CEKey"

///	Filename of the file to be used as an Haystack Monitor IPC key
#define HS_FILENAME "Haystack Monitor"

///	Filename of the file to be used as an Doppelganger Module IPC key
#define DM_FILENAME "Doppelganger Module"

///	Filename of the file to be used as an Local Traffic Monitor IPC key
#define LTM_FILENAME "Local Traffic Monitor"

///The maximum message, as defined in /proc/sys/kernel/msgmax
#define MAX_MSG_SIZE 65535

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
