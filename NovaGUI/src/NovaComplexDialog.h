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

#include "ui_NovaComplexDialog.h"
#include "novaconfig.h"

enum whichDialog {MACDialog, PersonalityDialog};

class NovaComplexDialog : public QDialog
{
    Q_OBJECT

public:
    std::string *m_retVal;
    //Default constructor, shouldn't be used
    NovaComplexDialog(QWidget *parent = 0);
    //Standard constructor, specify which dialog type you wish to create
    NovaComplexDialog(whichDialog type, std::string *retValue, QWidget *parent = 0, std::string filter = "");
    ~NovaComplexDialog();

    //Draws the parsed personalities in the widget
    void DrawPersonalities(std::string filterStr);
    //Sets the help message displayed at the bottom of the window to provide the user hints on usage.
    void SetHelpMsg(const QString& str);
    //Returns the help message displayed at the bottom of the window to provide the user hints on usage.
    QString GetHelpMsg();
    //Sets the description at the top on the window to inform the user of the window's purpose.
    void SetInfoMsg(const QString& str);
    //Returns the description at the top on the window to inform the user of the window's purpose.
    QString GetInfoMsg();
    //Returns the object's 'type' enum
    whichDialog GetType();

private:

    NovaConfig *m_novaParent;
    whichDialog m_type;
    Ui::NovaComplexDialogClass ui;

private Q_SLOTS:

	void on_searchEdit_textChanged();
	void on_cancelButton_clicked();
	void on_selectButton_clicked();
	void on_searchButton_clicked();
};

#endif // NOVACOMPLEXDIALOG_H
