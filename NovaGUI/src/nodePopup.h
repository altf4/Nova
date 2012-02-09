//============================================================================
// Name        : nodePopup.h
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
// Description : Popup for creating and editing nodes
//============================================================================
#ifndef NODEPOPUP_H
#define NODEPOPUP_H

#include "novagui.h"
#include "ui_nodePopup.h"

#define FROM_NODE_CONFIG true

class nodePopup : public QMainWindow
{
    Q_OBJECT

public:
    bool remoteCall;
    bool editingPorts;
    string homePath;

    //Copies of node data to distinguish from saved preferences
    SubnetTable subnets;
    NodeTable nodes;
    ProfileTable profiles;

    nodePopup(QWidget *parent = 0, node * n = NULL, int type = -1, string homePath = "");
    ~nodePopup();

    //Action to do when the window closes.
    void closeEvent(QCloseEvent * e);
    //Re-enables the GUI before exiting
    void closeWindow();
    //Closes the port window if it's 'parent' closes
    void remoteClose();

    //Saves the current configuration
    void saveNodeProfile(); //* Required by port popup
    //Loads the last saved configuration
    void loadNodeProfile(); //* Required by port popup
    //Loads the node list
    void loadAllNodes();
    //Loads the profile list
    void loadAllNodeProfiles();
    //Copies the data from parent novaconfig and adjusts the pointers
    void pullData();
    //Copies the data to parent novaconfig and adjusts the pointers
    void pushData();

private Q_SLOTS:

	//General Button signals
	void on_okButton_clicked();
	void on_cancelButton_clicked();
	void on_defaultsButton_clicked();
	void on_applyButton_clicked();

	//Item Selection signals
	void on_nodeTreeWidget_itemSelectionChanged();
	void on_profileTreeWidget_itemSelectionChanged();

	//Node GUI Button signals
	void on_addButton_clicked();
	void on_deleteButton_clicked();
	void on_cloneButton_clicked();
	void on_editPortsButton_clicked();
	void on_enableButton_clicked();
	void on_disableButton_clicked();

private:

    Ui::nodePopupClass ui;
};

#endif // NODEPOPUP_H
