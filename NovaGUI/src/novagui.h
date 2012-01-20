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
 #include <QProcess>
#include <QMouseEvent>
#include "ui_novagui.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp>
#include <Suspect.h>
#include <NovaUtil.h>
#include "dialogPrompter.h"
#include "NOVAConfiguration.h"

using namespace std;
using namespace Nova;
using boost::property_tree::ptree;


/*********************************************************************
 - Structs and Tables for quick item access through pointers -
**********************************************************************/


//used to maintain information on imported scripts
struct script
{
	string name;
	string path;
	ptree tree;
};

//Container for accessing script items
typedef google::dense_hash_map<string, script, tr1::hash<string>, eqstr > ScriptTable;

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
	ptree tree;
};
//Container for accessing port items
typedef google::dense_hash_map<string, port, tr1::hash<string>, eqstr > PortTable;

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
	vector<string> nodes;
	ptree tree;
};

//Container for accessing subnet items
typedef google::dense_hash_map<string, subnet, tr1::hash<string>, eqstr > SubnetTable;


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
	vector<string> ports;
	string  parentProfile;
	ptree tree;
};

//Container for accessing profile items
typedef google::dense_hash_map<string, profile, tr1::hash<string>, eqstr > ProfileTable;

//used to keep track of haystack node gui items and allow for easy access
struct node
{
	QTreeWidgetItem * item;
	QTreeWidgetItem * nodeItem;
	string sub;
	string interface;
	string pfile;
	string address;
	in_addr_t realIP;
	bool enabled;
	ptree tree;
};

//Container for accessing node items
typedef google::dense_hash_map<string, node, tr1::hash<string>, eqstr > NodeTable;

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

struct novaComponent
{
	string name;
	string terminalCommand;
	string noTerminalCommand;
	QProcess *process;
};

typedef google::dense_hash_map<in_addr_t, suspectItem, tr1::hash<in_addr_t>, eqaddr > SuspectHashTable;

class NovaGUI : public QMainWindow
{
    Q_OBJECT

public:

    bool editingPreferences;
    bool runAsWindowUp;

    string group;

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

    dialogPrompter *prompter;

    NovaGUI(QWidget *parent = 0);
    ~NovaGUI();
    Ui::NovaGUIClass ui;

    ///Receive a input from Classification Engine.
    bool receiveCE(int socket);

    ///Processes the recieved suspect in the suspect table
    void updateSuspect(suspectItem suspect);

    void emitSystemStatusRefresh();

    //Calls clearSuspects first then draws the suspect tables from scratch
    void drawAllSuspects();
    //Updates various Suspect-based widgets, called when suspect information changes
    void updateSuspectWidgets();
    //Removes an individual suspect from display until it's information is updated
    void hideSuspect(in_addr_t addr);

    // Tells CE to save suspects to file
    void saveSuspects();

    //Displays the topology of the honeyd configuration
    void drawNodes();

    //Clears the suspect tables completely.
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
    void loadSubProfiles(string parent);

    //load current honeyd configuration group
    void loadGroup();
    void loadSubnets(ptree *ptr);
    void loadNodes(ptree *ptr);

    //Saves the current configuration information to XML files
    void saveAll();
    //Writes the current configuration to honeyd configs
    void writeHoneyd(); //TODO

protected:
    void contextMenuEvent(QContextMenuEvent *event);

private slots:

	//Menu actions
	void on_actionRunNovaAs_triggered();
	void on_actionRunNova_triggered();
	void on_actionStopNova_triggered();
	void on_actionConfigure_triggered();
	void on_actionExit_triggered();
	void on_actionSave_Suspects_triggered();
	void on_actionHide_Old_Suspects_triggered();
	void on_actionShow_All_Suspects_triggered();
	void on_actionClear_All_Suspects_triggered();
	void on_actionClear_Suspect_triggered();
	void on_actionHide_Suspect_triggered();

	//Global Widgets
	void on_mainButton_clicked();
	void on_suspectButton_clicked();
	void on_doppelButton_clicked();
	void on_haystackButton_clicked();

	//Main view widgets
	void on_runButton_clicked();
	void on_stopButton_clicked();

	//System Status widgets
	void on_systemStatStartButton_clicked();
	void on_systemStatStopButton_clicked();
	void on_systemStatKillButton_clicked();

	//Suspect view widgets
	void on_clearSuspectsButton_clicked();
	void on_suspectList_itemSelectionChanged();

	//Custom Slots
    //Updates the UI with the latest suspect information
    void drawSuspect(in_addr_t suspectAddr);
    void updateSystemStatus();
    void initiateSystemStatus();

signals:

	//Custom Signals
	void newSuspect(in_addr_t suspectAddr);
	void refreshSystemStatus();

private:
	QIcon* greenIcon;
	QIcon* redIcon;
	QIcon* yellowIcon;
};

/// This is a blocking function. If nothing is received, then wait on this thread for an answer
void *CEListen(void *ptr);
/// Updates the suspect list every so often.
void *CEDraw(void *ptr);

void *StatusUpdate(void *ptr);

///Socket closing workaround for namespace issue.
void sclose(int sock);

//Opens the sockets for a message
void openSocket();

//Closes the Nova processes
void closeNova();

//Starts the Nova processes
void startNova();

// Start one component of Nova
void startComponent(novaComponent *component);

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
//Removes all information on a suspect
void clearSuspect(string suspectStr);

#endif // NOVAGUI_H
