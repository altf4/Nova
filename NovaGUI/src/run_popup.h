//============================================================================
// Name        : run_popup.h
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
// Description : This provides a run dialog where the user can override the configuration
//	options provided in the config file and set things like training mode/pcap file.
//============================================================================
#ifndef RUN_POPUP_H
#define RUN_POPUP_H

#include <QtGui/QMainWindow>
#include "ui_run_popup.h"

using namespace std;

class Run_Popup : public QMainWindow
{
    Q_OBJECT

public:
    string homePath;

    Run_Popup(QWidget *parent = 0, string homePath = "");
    ~Run_Popup();

    void closeEvent(QCloseEvent * e);

    void loadPreferences();
    bool savePreferences();

private slots:

//Main buttons
void on_cancelButton_clicked();
void on_startButton_clicked();

//Opens browse dialog for pcap file
void on_pcapButton_clicked();
//Check Box signal for enabling group box
void on_pcapCheckBox_stateChanged(int state);

private:
    Ui::Run_PopupClass ui;
};

#endif // RUN_POPUP_H
