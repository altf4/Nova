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
// Description : NOVA preferences/configuration window
//============================================================================
#ifndef NOVACONFIG_H
#define NOVACONFIG_H

#include "ui_novaconfig.h"
#include "novagui.h"
#include "nodePopup.h"

#include <QMutex>
#include <QWheelEvent>


// TODO: Clean this up. At a minimum add some descriptive comments, maybe replace with enums
#define HAYSTACK_MENU_INDEX 4
#define NODE_INDEX 0
#define PROFILE_INDEX 1

class NovaConfig : public QMainWindow
{
	Q_OBJECT

public:

	Nova::HoneydConfiguration *m_honeydConfig;

    QMutex * m_loading;

    NovaGUI * m_mainwindow;

    NovaConfig(QWidget *parent = 0, std::string homePath = "");

    ~NovaConfig();

    //Loads the configuration options from NOVAConfig.txt
    void LoadNovadPreferences();
    //Display MAC vendor prefix's
    bool DisplayMACPrefixWindow();
    //Load Personality choices from nmap fingerprints file
    void DisplayNmapPersonalityWindow();

    // Creates a new item for the featureList
    //		name - Name to be displayed, e.g. "Ports Contacted"
    //		enabled - '1' for enabled, all other disabled
    //	Returns: A Feature List Item
    QListWidgetItem* GetFeatureListItem(QString name, char enabled);

    // Updates the color and text of a featureList Item
    //		newFeatureEntry - Pointer to the Feature List Item
    //		enabled - '1' for enabled, all other disabled
    void UpdateFeatureListItem(QListWidgetItem* newFeatureEntry, char enabled);

    // Advances the currently selected item to the next one in the featureList
    void AdvanceFeatureSelection();

    //Draws the current honeyd configuration for haystack and doppelganger
    void LoadHaystackConfiguration();

    //Populates the window with the select profile's information
    void LoadProfileSettings();
    //loads the inheritance values from the profiles, called by loadProfile
    void LoadInheritedProfileSettings();
    //Populates the profile heirarchy in the tree widget
    void LoadAllProfiles();

    //Creates a tree widget item for the profile based on it's parent
    //If no parent it is created as a root node
    void CreateProfileItem(std::string pstr);


    //Temporarily stores changed configuration options when selection is changed
    // to permanently store these changes call pushData();
    void SaveProfileSettings();
    //saves the inheritance values of the profiles, called by saveProfile
    void SaveInheritedProfileSettings();

    //Removes a profile, all of it's children and any nodes that currently use it
    void DeleteProfile(std::string name);

    //Takes a ptree and loads and sub profiles (used in clone to extract children)
    void LoadProfilesFromTree(std::string parent);
    //set profile configurations (only called in LoadProfilesFromTree)
    void LoadProfileSettings(boost::property_tree::ptree *ptr, Nova::profile *p);
    //add ports or subsystems (only called in LoadProfilesFromTree)
    void LoadProfileServices(boost::property_tree::ptree *ptr, Nova::profile *p);
    //recursive descent down profile tree (only called in LoadProfilesFromTree)
    void LoadProfileChildren(std::string parent);

    //Function called on a delete signal to delete a node or subnet
    void DeleteNodes();

    //Populates the tree widget with the current node configuration
    void LoadAllNodes();

    // Checks the value in the spin boxes to see if it's a used IP or not
    bool IsDoppelIPValid();

    // Saves the configuration to the config file, returns true if success
    bool SaveConfigurationToFile();

    void SetSelectedNode(std::string node);

protected:
    void contextMenuEvent(QContextMenuEvent *event);


private Q_SLOTS:

void interfaceCheckBoxes_buttonClicked(QAbstractButton * button);

// Right click action on a feature, we manually connect it so no need for proper prefix
void onFeatureClick(const QPoint & pos);

//Empty action slot to do nothing on certain action prompts
void on_actionNo_Action_triggered();

//Which menu item is selected
void on_menuTreeWidget_itemSelectionChanged();
//General Preferences Buttons
void on_cancelButton_clicked();
void on_okButton_clicked();
void on_defaultsButton_clicked();
void on_applyButton_clicked();

//Browse dialogs for file paths
void on_pcapButton_clicked();
void on_dataButton_clicked();
void on_hsConfigButton_clicked();
void on_msgTypeListWidget_currentRowChanged();
void on_defaultActionListWidget_currentRowChanged();

//Enable DM checkbox, action syncs Node displayed in haystack as disabled/enabled
void on_dmCheckBox_stateChanged(int state);
//Check Box signal for enabling pcap settings group box
void on_pcapCheckBox_stateChanged(int state);
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


// Node right click menu
void on_actionNodeAdd_triggered();
void on_actionNodeDelete_triggered();
void on_actionNodeClone_triggered();
void on_actionNodeEdit_triggered();
void on_actionNodeCustomizeProfile_triggered();
void on_actionNodeEnable_triggered();
void on_actionNodeDisable_triggered();
void on_actionSubnetAdd_triggered();

//When text box in the tree is edited
void on_portTreeWidget_itemChanged(QTreeWidgetItem *item);

//Custom signal for combo box items in the tree changing
void portTreeWidget_comboBoxChanged(QTreeWidgetItem *item, bool edited);
//Custom signal for combo box items in the tree changing
void nodeTreeWidget_comboBoxChanged(QTreeWidgetItem *item, bool edited);

//Doppelganger IP Address Spin boxes
void on_dmIPSpinBox_0_valueChanged();
void on_dmIPSpinBox_1_valueChanged();
void on_dmIPSpinBox_2_valueChanged();
void on_dmIPSpinBox_3_valueChanged();

//Haystack storage type combo box
void on_hsSaveTypeComboBox_currentIndexChanged(int index);

void on_useAllIfCheckBox_stateChanged();
void on_useAnyLoopbackCheckBox_stateChanged();

private:
	void SetInputValidators();

    // These replace the QT pointers that were in the data structures,
    // they all return a pointer to the QTreeWidgetItem corresponding to a table key
    QTreeWidgetItem * GetProfileTreeWidgetItem(std::string profileName);
    QTreeWidgetItem * GetProfileHsTreeWidgetItem(std::string profileName);
    QTreeWidgetItem * GetSubnetTreeWidgetItem(std::string subnetName);
    QTreeWidgetItem * GetSubnetHsTreeWidgetItem(std::string subnetName);
    QTreeWidgetItem * GetNodeTreeWidgetItem(std::string nodeName);
    QTreeWidgetItem * GetNodeHsTreeWidgetItem(std::string nodeName);
    bool IsPortTreeWidgetItem(std::string port, QTreeWidgetItem* item);

    Ui::NovaConfigClass ui;

    //Keys used to maintain and lookup current selections
    std::string m_currentProfile;
    std::string m_currentPort;
    std::string m_currentNode;
    std::string m_currentSubnet;

    nodePopup * m_nodewindow;
    QMenu * m_portMenu;
    QMenu * m_profileTreeMenu;
    QMenu * m_nodeTreeMenu;

    QButtonGroup * m_radioButtons;
    QButtonGroup * m_interfaceCheckBoxes;

    //flag to avoid GUI signal conflicts
    bool m_editingItems;
    bool m_selectedSubnet;
    bool m_loadingDefaultActions;

    //Value set by dialog windows
    std::string m_retVal;
    std::string m_homePath;

	// Reference to the dialog prompter initialized in novagui
	DialogPrompter *m_prompter;
};

class TreeItemComboBox : public QComboBox
{
	Q_OBJECT

public:

	TreeItemComboBox(NovaConfig * parent = 0, QTreeWidgetItem* buddy = 0)
	{
		this->m_parent = parent;
		this->m_buddy = buddy;
		this->setFocusPolicy(Qt::ClickFocus);
		this->setContextMenuPolicy(Qt::NoContextMenu);
		this->setAutoFillBackground(true);
		connect(this, SIGNAL(activated(int)), this, SLOT(setCurrentIndex(int)));
	}
	~TreeItemComboBox(){}

	NovaConfig * m_parent;
	QTreeWidgetItem * m_buddy;

public Q_SLOTS:
	void setCurrentIndex(int index)
	{
		QComboBox::setCurrentIndex(index);
		Q_EMIT notifyParent(m_buddy, true);
	}

	Q_SIGNALS:
	void notifyParent(QTreeWidgetItem *item, bool edited);

protected:
	void wheelEvent(QWheelEvent * e)
	{
		e->ignore();
	}
	void focusInEvent(QFocusEvent * e)
	{
		e->ignore();
		Q_EMIT notifyParent(m_buddy, false);
	}

};
#endif // NOVACONFIG_H
