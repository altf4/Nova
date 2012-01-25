#ifndef PORTPOPUP_H
#define PORTPOPUP_H

#include <QtGui/QWidget>
#include "ui_portPopup.h"
#include "novagui.h"

using namespace std;

class portPopup : public QMainWindow
{
    Q_OBJECT

public:
    bool remoteCall;
    string homePath;

    portPopup(QWidget *parent = 0, struct profile *profile = NULL, bool fromNode = false, string homePath = "");
    ~portPopup();

    //Re-enables the GUI before exiting
    void closeWindow();
    //Closes the port window if it's 'parent' closes
    void remoteClose();
    //loads selected port into gui edits
    void loadPort(struct port *prt);
    //saves values in gui edits to selected port
    void savePort(struct port *prt);
    //clears and creates the port list and calls loadPort on current port (first by default)
    void loadAllPorts();
    //Action to do when the window closes.
    void closeEvent(QCloseEvent * e);

private slots:

//General Button signals
void on_okButton_clicked();
void on_cancelButton_clicked();
void on_defaultsButton_clicked();
void on_applyButton_clicked();

//Which port is selected
void on_portTreeWidget_itemSelectionChanged();

//Port Specific Button Signals
void on_cloneButton_clicked();
void on_addButton_clicked();
void on_deleteButton_clicked();

//If the port number text field changes
void on_portNumberEdit_textChanged();

private:
    Ui::portPopupClass ui;
};

#endif // PORTPOPUP_H
