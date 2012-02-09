//============================================================================
// Name        : NovaComplexDialog.h
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
// Description : Provides dialogs for MAC and personality selection
//============================================================================
#ifndef NOVACOMPLEXDIALOG_H
#define NOVACOMPLEXDIALOG_H

#include "novagui.h"
#include "ui_NovaComplexDialog.h"

enum whichDialog {MACDialog, PersonalityDialog};

class NovaConfig;
class NovaComplexDialog : public QDialog
{
    Q_OBJECT

public:
    string * retVal;
    //Default constructor, shouldn't be used
    NovaComplexDialog(QWidget *parent = 0);
    //Standard constructor, specify which dialog type you wish to create
    NovaComplexDialog(whichDialog type, string* retValue, QWidget *parent = 0, string filter = "");
    ~NovaComplexDialog();

    //Draws the parsed personalities in the widget
    void drawPersonalities(string filterStr);
    //Sets the help message displayed at the bottom of the window to provide the user hints on usage.
    void setHelpMsg(const QString& str);
    //Returns the help message displayed at the bottom of the window to provide the user hints on usage.
    QString getHelpMsg();
    //Sets the description at the top on the window to inform the user of the window's purpose.
    void setInfoMsg(const QString& str);
    //Returns the description at the top on the window to inform the user of the window's purpose.
    QString getInfoMsg();
    //Returns the object's 'type' enum
    whichDialog getType();

private:

    NovaConfig * novaParent;
    whichDialog type;
    Ui::NovaComplexDialogClass ui;

private Q_SLOTS:

	void on_searchEdit_textChanged();
	void on_cancelButton_clicked();
	void on_selectButton_clicked();
	void on_searchButton_clicked();
};

#endif // NOVACOMPLEXDIALOG_H
