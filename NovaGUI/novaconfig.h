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

    void loadPreferences();
    void loadHaystack();

    //Updates the 'current' pointers incase the data was modified
    void updatePointers();

    void loadProfile();
    void loadAllProfiles();
    void createProfileItem(profile *p);
    void createProfileTree(profile *p);
    void saveProfile();
    void deleteProfile(string name);
    void updateProfileTree(string parent);
    //Takes a ptree and loads and sub profiles (used in clone)
    void loadProfilesFromTree(string parent);
    //set profile configurations
    void loadProfileSet(ptree *ptr, profile *p);
    //add ports or subsystems
    void loadProfileAdd(ptree *ptr, profile *p);
    //recursive descent down profile tree
    void loadSubProfiles(string parent);

    void deleteNodes();
    void deleteNode(node *n);
    void loadAllNodes();

    //If a profile is edited, this function updates the changes for the rest of the GUI
    void updateProfile(bool deleteProfile, profile *p);

    //Action to do when the window closes.
    void closeEvent(QCloseEvent * e);

    void pushData();
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
