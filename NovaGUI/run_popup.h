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
