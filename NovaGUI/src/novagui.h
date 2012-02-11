//============================================================================
// Name        : novagui.h
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
// Description : Header file for the main NovaGUI component
//============================================================================
#ifndef NOVAGUI_H
#define NOVAGUI_H

#include <QProcess>
#include <QChar>
#include <QtGui>
#include <QPoint>
#include <QString>
#include <Suspect.h>
#include <QMouseEvent>
#include <QApplication>
#include <sys/socket.h>
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "DialogPrompter.h"
#include "ui_novagui.h"
#include "run_popup.h"
#include "nova_manual.h"
#include "NovaUtil.h"

using namespace std;
using namespace Nova;
using boost::property_tree::ptree;


/*********************************************************************
 - Structs and Tables for quick item access through pointers -
**********************************************************************/
#define INHERITED_MAX 8

enum profileType { static_IP = 0, staticDHCP = 1, randomDHCP = 2, Doppelganger = 3};

enum profileIndex { TYPE = 0, TCP_ACTION = 1, UDP_ACTION = 2, ICMP_ACTION = 3,
	PERSONALITY = 4, ETHERNET = 5, UPTIME = 6, DROP_RATE = 7};

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
	profileType type;
	bool inherited[INHERITED_MAX];
	vector<pair<string, bool> > ports;
	string parentProfile;
	ptree tree;
};

//Container for accessing profile items
typedef google::dense_hash_map<string, profile, tr1::hash<string>, eqstr > ProfileTable;

//used to keep track of haystack node gui items and allow for easy access
struct node
{
	string name;
	QTreeWidgetItem * item;
	QTreeWidgetItem * nodeItem;
	string sub;
	string interface;
	string pfile;
	string IP;
	string MAC;
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

	//We need a second item because an item can only be in one list at a time
	QListWidgetItem * mainItem;
};

struct novaComponent
{
	string name;
	string terminalCommand;
	string noTerminalCommand;
	QProcess *process;
	bool shouldBeRunning;
};

typedef google::dense_hash_map<in_addr_t, suspectItem, tr1::hash<in_addr_t>, eqaddr > SuspectHashTable;

class NovaGUI : public QMainWindow
{
    Q_OBJECT

public:

    bool editingPreferences;
    bool isHelpUp;
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
    ptree nodesTree;
    ptree subnetTree;

    DialogPrompter *prompter;
    messageHandle CONFIG_READ_FAIL, CONFIG_WRITE_FAIL, HONEYD_READ_FAIL;
    messageHandle HONEYD_LOAD_FAIL, UNEXPECTED_ENTRY, HONEYD_INVALID_SUBNET;
    messageHandle LAUNCH_TRAINING_MERGE, NODE_LOAD_FAIL, CANNOT_DELETE_PORT;
    messageHandle NO_ANCESTORS, PROFILE_IN_USE, DOPP_EXISTS, NO_DOPP, CANNOT_INHERIT_PORT;

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
    void writeHoneyd();
    string profileToString(profile* p);

    void setFeatureDistances(Suspect* suspect);

protected:
    void contextMenuEvent(QContextMenuEvent *event);

private Q_SLOTS:

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
	void on_actionHelp_2_triggered();

	void on_actionSystemStatKill_triggered();
	void on_actionSystemStatStop_triggered();
	void on_actionSystemStatStart_triggered();
	void on_actionSystemStatReload_triggered();

	void on_actionTrainingData_triggered();
	void on_actionMakeDataFile_triggered();

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
	void on_systemStatusTable_itemSelectionChanged();

	//Suspect view widgets
	void on_clearSuspectsButton_clicked();
	void on_suspectList_itemSelectionChanged();

	//Custom Slots
    //Updates the UI with the latest suspect information
    void drawSuspect(in_addr_t suspectAddr);
    void updateSystemStatus();
    void initiateSystemStatus();

Q_SIGNALS:

	//Custom Signals
	void newSuspect(in_addr_t suspectAddr);
	void refreshSystemStatus();

private:
	const QIcon* greenIcon;
	const QIcon* redIcon;
	const QIcon* yellowIcon;
};

namespace Nova {

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
void stopNova();

//Starts the Nova processes
void startNova();

void reloadConfiguration();

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
}

//Some includes need to occur at the end of the header to fix some linking errors during compilation
#include "nodePopup.h"
#include "NovaComplexDialog.h"
#include "novaconfig.h"
#endif // NOVAGUI_H
