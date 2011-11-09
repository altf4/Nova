#ifndef NODEPOPUP_H
#define NODEPOPUP_H

#include <QtGui/QWidget>
#include "ui_nodePopup.h"
#include "novaconfig.h"

#define FROM_NODE_CONFIG true

class nodePopup : public QMainWindow
{
    Q_OBJECT

public:
    bool remoteCall;
    bool editingPorts;

    //Copies of node data to distinguish from saved preferences
    SubnetTable subnets;
    NodeTable nodes;
    ProfileTable profiles;

    nodePopup(QWidget *parent = 0, node * n = NULL, int type = -1);
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

private slots:

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
