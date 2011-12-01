#ifndef NOVACONFIG_H
#define NOVACONFIG_H

#include <QtGui/QMainWindow>
#include <QtGui/QTreeWidget>
#include "ui_novaconfig.h"
#include "novagui.h"

using namespace std;

#define HAYSTACK_MENU_INDEX 4
#define NODE_INDEX 0
#define PROFILE_INDEX 1
#define FROM_NOVA_CONFIG false
#define DELETE_PROFILE true
#define UPDATE_PROFILE false
#define ADD_NODE 0
#define CLONE_NODE 1
#define EDIT_NODE 2

class NovaConfig : public QMainWindow
{
    Q_OBJECT

public:
    bool editingPorts;
    bool editingNodes;
    string homePath;

    SubnetTable subnets;
    NodeTable nodes;
    ProfileTable profiles;
    PortTable ports;
    ScriptTable scripts;

    string group;

    NovaConfig(QWidget *parent = 0, string homePath = "");

    ~NovaConfig();

    //Loads the configuration options from NOVAConfig.txt
    void loadPreferences();
    //Draws the current honeyd configuration for haystack and doppelganger
    void loadHaystack();

    //Updates the 'current' keys, not really pointers just used to access current selections
    //This is called on start up and by child windows that push modified data
    void updatePointers();

    //Populates the window with the select profile's information
    void loadProfile();
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

private slots:

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

//Check Box signal for enabling pcap settings group box
void on_pcapCheckBox_stateChanged(int state);

//GUI Signals for Profile settings
void on_editPortsButton_clicked();
void on_cloneButton_clicked();
void on_addButton_clicked();
void on_deleteButton_clicked();
void on_profileTreeWidget_itemSelectionChanged();
void on_profileEdit_textChanged();

//GUI Signals for Node settings
void on_nodeEditButton_clicked();
void on_nodeCloneButton_clicked();
void on_nodeAddButton_clicked();
void on_nodeDeleteButton_clicked();
void on_nodeEnableButton_clicked();
void on_nodeDisableButton_clicked();
void on_nodeTreeWidget_itemSelectionChanged();




private:

    Ui::NovaConfigClass ui;
};

#endif // NOVACONFIG_H
