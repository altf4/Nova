//============================================================================
// Name        : novagui.h
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Header file for the main NovaGUI component
//============================================================================/*

#ifndef NOVAGUI_H
#define NOVAGUI_H

#include <QtGui/QMainWindow>
#include <QtGui/QTreeWidget>
#include "ui_novagui.h"
#include <tr1/unordered_map>
#include <boost/property_tree/ptree.hpp>
#include <sys/un.h>
#include <arpa/inet.h>
#include <Suspect.h>
#include <GUIMsg.h>

/// File name of the file to be used as Traffic Event IPC key
#define KEY_FILENAME "/keys/NovaIPCKey"
///	Filename of the file to be used as an Doppelganger IPC key
#define KEY_ALARM_FILENAME "/keys/NovaDoppIPCKey"
///	Filename of the file to be used as an Classification Engine IPC key
#define CE_FILENAME "/keys/CEKey"
/// File name of the file to be used as CE Output IPC key.
#define CE_GUI_FILENAME "/keys/GUI_CEKey"
/// File name of the file to be used as CE Output IPC key.
#define DM_GUI_FILENAME "/keys/GUI_DMKey"
/// File name of the file to be used as CE Output IPC key.
#define HS_GUI_FILENAME "/keys/GUI_HSKey"
/// File name of the file to be used as CE Output IPC key.
#define LTM_GUI_FILENAME "/keys/GUI_LTMKey"

///The maximum message, as defined in /proc/sys/kernel/msgmax
#define MAX_MSG_SIZE 65535
//Number of messages to queue in a listening socket before ignoring requests until the queue is open
#define SOCKET_QUEUE_SIZE 50

using namespace std;
using namespace Nova;
using namespace ClassificationEngine;
using boost::property_tree::ptree;

/*********************************************************************
 - Structs and Tables for quick item access through pointers -
**********************************************************************/

//used to maintain information on imported scripts
struct script
{
	string name;
	string path;
	ptree * treePtr;
};
typedef std::tr1::unordered_map<string, script> ScriptTable;

//used to maintain information about a port, it's type and behavior
struct port
{
	QTreeWidgetItem * item;
	QTreeWidgetItem * portItem;
	string portName;
	string portNum;
	string type;
	string behavior;
	string scriptName;
	string proxyIP;
	string proxyPort;
	script * scriptPtr;
	ptree * treePtr;
};
typedef std::tr1::unordered_map<string, port> PortTable;


//used to keep track of subnet gui items and allow for easy access
struct subnet
{
	QTreeWidgetItem * item;
	QTreeWidgetItem * nodeItem;
	string name;
	string address;
	string mask;
	int maskBits;
	in_addr_t base;
	in_addr_t max;
	bool enabled;
	vector<struct node *> nodes;
	ptree * treePtr;
};

//container for the subnet pairs
typedef std::tr1::unordered_map<string, subnet> SubnetTable;


//used to keep track of haystack profile gui items and allow for easy access
struct profile
{
	QTreeWidgetItem * item;
	QTreeWidgetItem * profileItem;
	string name;
	string tcpAction;
	string udpAction;
	string icmpAction;
	string personality;
	string ethernet;
	string uptime;
	string uptimeRange;
	string dropRate;
	bool DHCP;
	vector<struct port> ports;
	profile * parentProfile;
	ptree * ptreePtr;
};

//Container for accessing profile item pairs
typedef std::tr1::unordered_map<string, profile> ProfileTable;


//used to keep track of haystack node gui items and allow for easy access
struct node
{
	QTreeWidgetItem * item;
	QTreeWidgetItem * nodeItem;
	struct subnet * sub;
	string interface;
	struct profile * pfile;
	string address;
	in_addr_t realIP;
	string pname;
	bool enabled;
	ptree * treePtr;
};

//Container for accessing node item pairs
typedef std::tr1::unordered_map<string, node> NodeTable;

//Used to store the current doppelganger
struct doppelganger
{
	string interface;
	struct profile * pfile;
	string address;
	in_addr_t realIP;
	string pname;
	bool enabled;
	ptree * treePtr;
};

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

    bool editingPreferences;
    bool runAsWindowUp;

    string group;
    doppelganger dm;

    SubnetTable subnets;
    PortTable ports;
    ProfileTable profiles;
    NodeTable nodes;
    ScriptTable scripts;

    //Storing these trees allow for easy modification and writing of the XML files
    //Without having to reconstruct the tree from scratch.
    ptree groupTree;
    ptree portTree;
    ptree profileTree;
    ptree scriptTree;
    ptree *nodesTree;
    ptree *doppTree;
    ptree *subnetTree;

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

    //Displays the topology of the honeyd configuration
    void drawNodes();

    ///Clears the suspect tables completely.
    void clearSuspectList();

    //Action to do when the window closes.
    void closeEvent(QCloseEvent * e);

    //Get preliminary config information
    void getInfo();
    void getPaths();
    void getSettings();

    //XML Read Functions

    //calls main load functions
    void loadAll();
    //load all scripts
    void loadScripts();
    //load all ports
    void loadPorts();

    //load all profiles
    void loadProfiles();
    //set profile configurations
    void loadProfileSet(ptree *ptr, profile *p);
    //add ports or subsystems
    void loadProfileAdd(ptree *ptr, profile *p);
    //recursive descent down profile tree
    void loadSubProfiles(ptree *ptr, profile *p);

    //load current honeyd configuration group
    void loadGroup();
    void loadSubnets(ptree *ptr);
    void loadDoppelganger(ptree *ptr);
    void loadNodes(ptree *ptr);


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

//Gets number of bits used in the mask
int getMaskBits(in_addr_t range);





#endif // NOVAGUI_H
