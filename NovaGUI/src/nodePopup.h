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


class nodePopup : public QMainWindow
{
    Q_OBJECT

public:

    string homePath;

    nodePopup(QWidget *parent = 0, node *n  = NULL);
    ~nodePopup();

    //Saves the current configuration
    void saveNode();
    //Loads the last saved configuration
    void loadNode();
    //Copies the data from parent novaconfig and adjusts the pointers
    void pullData();
    //Copies the data to parent novaconfig and adjusts the pointers
    void pushData();

private Q_SLOTS:

	//General Button slots
	void on_okButton_clicked();
	void on_cancelButton_clicked();
	void on_restoreButton_clicked();
	void on_applyButton_clicked();

private:

    Ui::nodePopupClass ui;
};

#endif // NODEPOPUP_H
