//============================================================================
// Name        : nodeconfig.h
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
// Description :
//============================================================================
#ifndef NOVACONFIG_H
#define NOVACONFIG_H

#include "ui_novaconfig.h"
#include "stdlib.h"
#include "novagui.h"

#define HAYSTACK_MENU_INDEX 5
#define NODE_INDEX 0
#define PROFILE_INDEX 1
#define FROM_NOVA_CONFIG false
#define DELETE_PROFILE true
#define UPDATE_PROFILE false
#define ADD_NODE 0
#define CLONE_NODE 1
#define EDIT_NODE 2

typedef google::dense_hash_map<uint, string, tr1::hash<uint>, eqint> MACToVendorTable;
typedef google::dense_hash_map<string, vector<uint> *,  tr1::hash<string>, eqstr > VendorToMACTable;
using namespace std;

class NovaConfig : public QMainWindow
{
    Q_OBJECT

public:
    //Value set by dialog windows
    string retVal;

    bool editingPorts;
    bool editingNodes;
    string homePath;

    SubnetTable subnets;
    NodeTable nodes;
    ProfileTable profiles;
    PortTable ports;
    ScriptTable scripts;

    MACToVendorTable MACVendorTable;
    VendorToMACTable VendorMACTable;
    vector<pair<string, string> > nmapPersonalities;

    string group;

    NovaConfig(QWidget *parent = 0, string homePath = "");

    ~NovaConfig();

    //Loads the configuration options from NOVAConfig.txt
    void loadPreferences();
    //Display MAC vendor prefix's
    bool displayMACPrefixWindow();
    //Load Personality choices from nmap fingerprints file
    void displayNmapPersonalityTree();
    //Resolve the first 3 bytes of a MAC Address to a MAC vendor that owns the range, returns the vendor string
    string resolveMACVendor(uint MACPrefix);
    //Load MAC vendor prefix choices from nmap mac prefix file
    void loadMACPrefixs();
    //Load nmap personalities from the nmap-os-db file
    void loadNmapPersonalities();

    //Randomly selects one of the ranges associated with vendor and generates the remainder of the MAC address
    // *note conflicts are only checked for locally, weird things may happen if the address is already being used.
    string generateUniqueMACAddr(string vendor);

    // Creates a new item for the featureList
    //		name - Name to be displayed, e.g. "Ports Contacted"
    //		enabled - '1' for enabled, all other disabled
    //	Returns: A Feature List Item
    QListWidgetItem* getFeatureListItem(QString name, char enabled);

    // Updates the color and text of a featureList Item
    //		newFeatureEntry - Pointer to the Feature List Item
    //		enabled - '1' for enabled, all other disabled
    void updateFeatureListItem(QListWidgetItem* newFeatureEntry, char enabled);

    // Advances the currently selected item to the next one in the featureList
    void advanceFeatureSelection();

    //Draws the current honeyd configuration for haystack and doppelganger
    void loadHaystack();

    //Updates the 'current' keys, not really pointers just used to access current selections
    //This is called on start up and by child windows that push modified data
    void updatePointers();

    //Populates the window with the select profile's information
    void loadProfile();
    //loads the inheritance values from the profiles, called by loadProfile
    void loadInherited();
    //Populates the profile heirarchy in the tree widget
    void loadAllProfiles();

    //Creates a tree widget item for the profile based on it's parent
    //If no parent it is created as a root node
    void createProfileItem(profile *p);
    //Creates a ptree for a profile with current values
    void createProfileTree(string name);

    //Temporarily stores changed configuration options when selection is changed
    // to permanently store these changes call pushData();
    void saveProfile();
    //saves the inheritance values of the profiles, called by saveProfile
    void saveInherited();
    //Removes a profile, all of it's children and any nodes that currently use it
    void deleteProfile(string name);

    //Recreates the profile tree of all ancestors
    //This needs to be called after adding, deleting or storing changes to a profile.
    void updateProfileTree(string name);

    //Takes a ptree and loads and sub profiles (used in clone to extract children)
    void loadProfilesFromTree(string parent);
    //set profile configurations (only called in loadProfilesFromTree)
    void loadProfileSet(ptree *ptr, profile *p);
    //add ports or subsystems (only called in loadProfilesFromTree)
    void loadProfileAdd(ptree *ptr, profile *p);
    //recursive descent down profile tree (only called in loadProfilesFromTree)
    void loadSubProfiles(string parent);

    //Function called on a delete signal to delete a node or subnet
    void deleteNodes();
    //Deletes a single node, called from deleteNodes();
    void deleteNode(node *n);
    //Populates the tree widget with the current node configuration
    void loadAllNodes();

    //If a profile is edited, this function updates the changes for the rest of the GUI
    void updateProfile(bool deleteProfile, profile *p);

    //Action to do when the window closes.
    void closeEvent(QCloseEvent * e);

    //Pushes the current configuration to novagui (Apply or Ok signal)
    void pushData();
    //Pulls the configuration stored in novagui
    void pullData();

    // Saves the configuration to the config file, returns true if success
    bool saveConfigurationToFile();

public slots:
//When an item in the port tree widget changes
void on_portTreeWidget_itemChanged(QTreeWidgetItem *item);

protected:
    void contextMenuEvent(QContextMenuEvent *event);


private slots:

// Right click action on a feature, we manually connect it so no need for proper prefix
void onFeatureClick(const QPoint & pos);

//Which menu item is selected
void on_treeWidget_itemSelectionChanged();
//General Preferences Buttons
void on_cancelButton_clicked();
void on_okButton_clicked();
void on_defaultsButton_clicked();
void on_applyButton_clicked();

//Browse dialogs for file paths
void on_pcapButton_clicked();
void on_dataButton_clicked();
void on_hsConfigButton_clicked();
void on_dmConfigButton_clicked();

void on_msgTypeListWidget_currentRowChanged();
void on_defaultActionListWidget_currentRowChanged();

//Check Box signal for enabling pcap settings group box
void on_pcapCheckBox_stateChanged(int state);
//Combo box signal for enabling or disabling dhcp IP resolution
void on_dhcpComboBox_currentIndexChanged(int index);
//Combo box signal for changing the uptime behavior
void on_uptimeBehaviorComboBox_currentIndexChanged(int index);

//GUI Signals for Profile settings
void on_cloneButton_clicked();
void on_addButton_clicked();
void on_deleteButton_clicked();
void on_profileTreeWidget_itemSelectionChanged();
void on_profileEdit_editingFinished();

//GUI Signals for Node settings
void on_nodeEditButton_clicked();
void on_nodeCloneButton_clicked();
void on_nodeAddButton_clicked();
void on_nodeDeleteButton_clicked();
void on_nodeEnableButton_clicked();
void on_nodeDisableButton_clicked();
void on_setEthernetButton_clicked();
void on_setPersonalityButton_clicked();
void on_nodeTreeWidget_itemSelectionChanged();
void on_dropRateSlider_valueChanged();

//GUI Signals for Feature addition/removal
void on_featureEnableButton_clicked();
void on_featureDisableButton_clicked();

//Inheritance Check boxes
void on_ipModeCheckBox_stateChanged();
void on_ethernetCheckBox_stateChanged();
void on_uptimeCheckBox_stateChanged();
void on_personalityCheckBox_stateChanged();
void on_dropRateCheckBox_stateChanged();
void on_tcpCheckBox_stateChanged();
void on_udpCheckBox_stateChanged();
void on_icmpCheckBox_stateChanged();

//Port inheritance menu toggle
void on_actionToggle_Inherited_triggered();
void on_actionDeletePort_triggered();
void on_actionAddPort_triggered();

// Profile right click menu
void on_actionProfileAdd_triggered();
void on_actionProfileDelete_triggered();
void on_actionProfileClone_triggered();


private:
	void setInputValidators();
    Ui::NovaConfigClass ui;
};

class TreeItemComboBox : public QComboBox
{
	Q_OBJECT

public:

	TreeItemComboBox(NovaConfig * parent = 0, QTreeWidgetItem* buddy = 0)
	{
		this->parent = parent;
		this->buddy = buddy;
		connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(setCurrentIndex(int)));
	}
	~TreeItemComboBox(){}

	NovaConfig * parent;
	QTreeWidgetItem * buddy;

public slots:
	void setCurrentIndex(int index)
	{
		QComboBox::setCurrentIndex(index);
		emit notifyParent(buddy);
	}


	signals:
	void notifyParent(QTreeWidgetItem *item);

};
#endif // NOVACONFIG_H
