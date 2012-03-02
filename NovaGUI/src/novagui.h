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

#include "DialogPrompter.h"
#include "NovaGuiTypes.h"
#include "ui_novagui.h"
#include "Suspect.h"
#include "loggerwindow.h"
#include "NOVAConfiguration.h"
#include "HoneydConfiguration.h"

#include <QProcess>

using namespace std;
using namespace Nova;

struct suspectItem
{
	//The suspect information
	Suspect * suspect;

	//The associated item for the suspect view list
	QListWidgetItem * item;

	//We need a second item because an item can only be in one list at a time
	QListWidgetItem * mainItem;
};
typedef google::dense_hash_map<in_addr_t, suspectItem, tr1::hash<in_addr_t>, eqaddr > SuspectGUIHashTable;


struct novaComponent
{
	string name;
	string terminalCommand;
	string noTerminalCommand;
	QProcess *process;
	bool shouldBeRunning;
};

class NovaGUI : public QMainWindow
{
    Q_OBJECT

public:

    bool m_isHelpUp;
    HoneydConfiguration *honeydConfig;
    NOVAConfiguration *configuration;

    DialogPrompter *prompter;
    messageHandle CONFIG_READ_FAIL, CONFIG_WRITE_FAIL, HONEYD_READ_FAIL;
    messageHandle HONEYD_LOAD_FAIL, UNEXPECTED_ENTRY, HONEYD_INVALID_SUBNET;
    messageHandle LAUNCH_TRAINING_MERGE, NODE_LOAD_FAIL, CANNOT_DELETE_PORT, CANNOT_DELETE_ITEM;
    messageHandle NO_ANCESTORS, PROFILE_IN_USE, NO_DOPP, CANNOT_INHERIT_PORT;

    NovaGUI(QWidget *parent = 0);
    ~NovaGUI();
    Ui::NovaGUIClass ui;

    ///Receive a input from Classification Engine.
    bool ReceiveSuspectFromNovad(int socket);

    ///Processes the recieved suspect in the suspect table
    void ProcessReceivedSuspect(suspectItem suspect);

    void SystemStatusRefresh();

    //Calls clearSuspects first then draws the suspect tables from scratch
    void DrawAllSuspects();
    //Updates various Suspect-based widgets, called when suspect information changes
    void UpdateSuspectWidgets();
    //Removes an individual suspect from display until it's information is updated
    void HideSuspect(in_addr_t addr);

    // Tells CE to save suspects to file
    void SaveAllSuspects();

    //Clears the suspect tables completely.
    void ClearSuspectList();

    //Action to do when the window closes.
    void closeEvent(QCloseEvent * e);

    //Get preliminary config information
    void InitConfiguration();
    void InitPaths();
    void InitNovadCommands();

    void SetFeatureDistances(Suspect* suspect);

    void LoadNovadConfiguration();

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
	void on_actionLogger_triggered();

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
    void DrawSuspect(in_addr_t suspectAddr);
    void UpdateSystemStatus();
    void InitiateSystemStatus();

Q_SIGNALS:

	//Custom Signals
	void newSuspect(in_addr_t suspectAddr);
	void refreshSystemStatus();

private:
	const QIcon* m_greenIcon;
	const QIcon* m_redIcon;
	const QIcon* m_yellowIcon;

	QMenu * m_suspectMenu;
	QMenu * m_systemStatMenu;

	bool m_featureEnabled[DIM];
	bool m_editingSuspectList;

	//Configuration variables
	char * m_pathsFile;
};

namespace Nova {

/// This is a blocking function. If nothing is received, then wait on this thread for an answer
void *NovadListenLoop(void *ptr);

void *StatusUpdate(void *ptr);

///Socket closing workaround for namespace issue.
void CloseSocket(int sock);

//Closes the Nova processes
void StopNova();

//Starts the Nova processes
void StartNova();

// Start one component of Nova
void StartComponent(novaComponent *component);

//Saves the socket addresses for re-use.
void InitSocketAddresses();

//Sends the GUIMsg object serialized inside the 'data' buffer with size n
void SendToNovad(u_char * data, int size);

//Deletes all Suspect information for the GUI and Nova
void ClearAllSuspects();

//Removes all information on a suspect
void ClearSuspect(string suspectStr);

}
#endif // NOVAGUI_H
