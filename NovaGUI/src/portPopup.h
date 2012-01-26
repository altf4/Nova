//============================================================================
// Name        : portPopup.h
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
