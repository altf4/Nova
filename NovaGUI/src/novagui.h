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
#include "Config.h"
#include "HoneydConfiguration.h"

#include <QProcess>

struct suspectItem
{
	//The suspect information
	Nova::Suspect * suspect;

	//The associated item for the suspect view list
	QListWidgetItem * item;

	//We need a second item because an item can only be in one list at a time
	QListWidgetItem * mainItem;
};
typedef Nova::HashMap<uint64_t, suspectItem, std::tr1::hash<uint64_t>, eqaddr > SuspectGUIHashTable;

class NovaGUI : public QMainWindow
{
    Q_OBJECT

public:

    bool m_isHelpUp;

    DialogPrompter *m_prompter;
    messageHandle CONFIG_READ_FAIL, CONFIG_WRITE_FAIL, HONEYD_READ_FAIL;
    messageHandle HONEYD_LOAD_FAIL, UNEXPECTED_ENTRY, HONEYD_INVALID_SUBNET;
    messageHandle LAUNCH_TRAINING_MERGE, NODE_LOAD_FAIL, CANNOT_DELETE_PORT, CANNOT_DELETE_ITEM;
    messageHandle NO_ANCESTORS, PROFILE_IN_USE, NO_DOPP, CANNOT_INHERIT_PORT;

    NovaGUI(QWidget *parent = 0);
    ~NovaGUI();
    Ui::NovaGUIClass ui;

    ///Processes the recieved suspect in the suspect table
    	// initialization - if initializing suspects, don't overwrite existing ones and don't use the qsignal for drawing
    void ProcessReceivedSuspect(suspectItem suspect, bool initialization);

    void SystemStatusRefresh();

    //Calls clearSuspects first then draws the suspect tables from scratch
    void DrawAllSuspects();
    //Updates various Suspect-based widgets, called when suspect information changes
    void UpdateSuspectWidgets();
    //Removes an individual suspect from display until it's information is updated
    void HideSuspect(in_addr_t addr);

    //Clears the suspect tables completely.
    void ClearSuspectList();

    //Action to do when the window closes.
    void closeEvent();

    //Get preliminary config information
    void InitConfiguration();
    void InitPaths();

    void SetFeatureDistances(Nova::Suspect* suspect);

    void LoadNovadConfiguration();

    // Connect to novad and make the callback socket
    bool ConnectGuiToNovad();

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
	void on_systemStatusTable_itemSelectionChanged();

	//Suspect view widgets
	void on_clearSuspectsButton_clicked();
	void on_suspectList_currentItemChanged(QListWidgetItem * current, QListWidgetItem * previous);

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

	QMenu * m_suspectMenu;
	QMenu * m_systemStatMenu;

	bool m_featureEnabled[DIM];
	bool m_editingSuspectList;

	//Configuration variables
	char * m_pathsFile;
};

namespace Nova
{

//Listen on IPC for messages from Novad
//	NOTE: This is the only callback function you need to call
//	Takes in the GUI object (IE: "this" from the calling context)
//	returns - true if server set successfully, false on error
bool StartCallbackLoop(void *ptr);

/// This is a blocking function. If nothing is received, then wait on this thread for an answer
void *CallbackLoop(void *ptr);

void *StatusUpdate(void *ptr);

}
#endif // NOVAGUI_H
