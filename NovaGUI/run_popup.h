#ifndef RUN_POPUP_H
#define RUN_POPUP_H

#include <QtGui/QMainWindow>
#include "ui_run_popup.h"

class Run_Popup : public QMainWindow
{
    Q_OBJECT

public:
    Run_Popup(QWidget *parent = 0);
    ~Run_Popup();

    void LoadPreferences();

private slots:

//Main buttons
void on_cancelButton_clicked();

//Check Box signal for enabling group box
void on_pcapCheckBox_stateChanged(int state);

private:
    Ui::Run_PopupClass ui;
};

#endif // RUN_POPUP_H
