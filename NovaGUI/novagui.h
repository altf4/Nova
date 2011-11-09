//============================================================================
// Name        : novagui.h
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Header file for the main NovaGUI component
//============================================================================/*

#ifndef NOVAGUI_H
#define NOVAGUI_H

#include <QtGui/QMainWindow>
#include "ui_novagui.h"
#include <tr1/unordered_map>
#include <sys/un.h>
#include <arpa/inet.h>
#include <Suspect.h>

/// File name of the file to be used as Traffic Event IPC key
#define KEY_FILENAME "/.nova/keys/NovaIPCKey"
///	Filename of the file to be used as an Doppelganger IPC key
#define KEY_ALARM_FILENAME "/.nova/keys/NovaDoppIPCKey"
///	Filename of the file to be used as an Classification Engine IPC key
#define CE_FILENAME "/.nova/keys/CEKey"
/// File name of the file to be used as CE Output IPC key.
#define CE_GUI_FILENAME "/.nova/keys/GUI_CEKey"
/// File name of the file to be used as CE Output IPC key.
#define DM_GUI_FILENAME "/.nova/keys/GUI_DMKey"
/// File name of the file to be used as CE Output IPC key.
#define HS_GUI_FILENAME "/.nova/keys/GUI_HSKey"
/// File name of the file to be used as CE Output IPC key.
#define LTM_GUI_FILENAME "/.nova/keys/GUI_LTMKey"

///The maximum message, as defined in /proc/sys/kernel/msgmax
#define MAX_MSG_SIZE 65535
//Number of messages to queue in a listening socket before ignoring requests until the queue is open
#define SOCKET_QUEUE_SIZE 50
using namespace Nova;
using namespace ClassificationEngine;

struct suspectItem
{
	//The suspect information
	Suspect * suspect;
	//The associated item for the suspect view list
	QListWidgetItem * item;
	//The associated item for the main view list
	//We need a second item because an item can only be in one list at a time
	QListWidgetItem * mainItem;
};

class NovaGUI : public QMainWindow
{
    Q_OBJECT

public:

    NovaGUI(QWidget *parent = 0);
    ~NovaGUI();
    Ui::NovaGUIClass ui;

    ///Receive a input from Classification Engine.
    bool receiveCE(int socket);

    ///Processes the recieved suspect in the suspect table
    void updateSuspect(suspectItem suspect);

    ///Calls clearSuspects first then draws the suspect tables from scratch
    void drawAllSuspects();

    ///Updates the UI with the latest suspect information
    void drawSuspects();

    ///Clears the suspect tables completely.
    void clearSuspectList();

    //Action to do when the window closes.
    void closeEvent(QCloseEvent * e);


private slots:

	//Menu actions
	void on_actionRunNovaAs_triggered();
	void on_actionRunNova_triggered();
	void on_actionStopNova_triggered();
	void on_actionConfigure_triggered();
	void on_actionExit_triggered();
	void on_actionHide_Old_Suspects_triggered();
	void on_actionShow_All_Suspects_triggered();

	//Main view buttons
	void on_mainButton_clicked();
	void on_suspectButton_clicked();
	void on_doppelButton_clicked();
	void on_haystackButton_clicked();
	void on_runButton_clicked();
	void on_stopButton_clicked();
	void on_clearSuspectsButton_clicked();

private:

};

typedef std::tr1::unordered_map<in_addr_t, suspectItem> SuspectHashTable;

/// This is a blocking function. If nothing is received, then wait on this thread for an answer
void *CEListen(void *ptr);
/// Updates the suspect list every so often.
void *CEDraw(void *ptr);

///Socket closing workaround for namespace issue.
void sclose(int sock);

//Opens the sockets for a message
void openSocket();

//Closes the Nova processes
void closeNova();

//Starts the Nova processes
void startNova();

//Saves the socket addresses for re-use.
void getSocketAddr();

//Sends the contents of global scope const char * 'data' to all Nova processes
void sendAll();
//Sends 'data' to Classification Engine
void sendToCE();
//Sends 'data' to Doppelganger Module
void sendToDM();
//Sends 'data' to Haystack
void sendToHS();
//Sends 'data' to Local Traffic Monitor
void sendToLTM();

//Deletes all Suspect information for the GUI and Nova
void clearSuspects();





#endif // NOVAGUI_H
