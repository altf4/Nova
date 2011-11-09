#ifndef NOVACONFIG_H
#define NOVACONFIG_H

#include <QtGui/QMainWindow>
#include <QtGui/QTreeWidget>
#include "ui_novaconfig.h"
#include <tr1/unordered_map>

using namespace std;

#define HAYSTACK_MENU_INDEX 4
#define NODE_INDEX 0
#define PROFILE_INDEX 1
#define FROM_NOVA_CONFIG false
#define DELETE_PROFILE true
#define PROFILE_NAME_CHANGE false
#define ADD_NODE 0
#define CLONE_NODE 1
#define EDIT_NODE 2

/*********************************************************************
 - Structs and Tables for quick item access through pointers -
**********************************************************************/

//used to maintain information about a port, it's type and behavior
struct port
{
	QTreeWidgetItem * item;
	QTreeWidgetItem * portItem;
	string portNum;
	string type;
	string behavior;
};


//used to keep track of subnet gui items and allow for easy access
struct subnet
{
	QTreeWidgetItem * item;
	QTreeWidgetItem * nodeItem;
	string address;
	bool enabled;
	vector<struct node *> nodes;
};

//container for the subnet pairs
typedef std::tr1::unordered_map<string, subnet> SubnetTable;


//used to keep track of haystack profile gui items and allow for easy access
struct profile
{
	QTreeWidgetItem * item;
	QTreeWidgetItem * profileItem;
	string name;
	string personality;
	string ethernet;
	string tcpAction;
	string uptimeBehvaior; //TODO Once we have a settings file we can implement
	string uptime;
	vector<struct port> ports;
};

//Container for accessing profile item pairs
typedef std::tr1::unordered_map<string, profile> ProfileTable;


//used to keep track of haystack node gui items and allow for easy access
struct node
{
	QTreeWidgetItem * item;
	QTreeWidgetItem * nodeItem;
	struct subnet * sub;
	struct profile * pfile;
	string address;
	string pname;
	bool enabled;
};

//Container for accessing node item pairs
typedef std::tr1::unordered_map<string, node> NodeTable;


class NovaConfig : public QMainWindow
{
    Q_OBJECT

public:
    bool editingPorts;
    bool editingNodes;

    SubnetTable subnets;
    NodeTable nodes;
    ProfileTable profiles;

    NovaConfig(QWidget *parent = 0);

    ~NovaConfig();

    void loadPreferences();
    void loadHaystack();

    //Updates the 'current' pointers incase the data was modified
    void updatePointers();

    void loadProfile();
    void loadAllProfiles();
    void saveProfile();

    void deleteNode();
    void loadAllNodes();

    //If a profile is edited, this function updates the changes for the rest of the GUI
    void updateProfile(bool deleteProfile);

    //Action to do when the window closes.
    void closeEvent(QCloseEvent * e);

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
