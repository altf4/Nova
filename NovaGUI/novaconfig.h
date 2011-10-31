#ifndef NOVACONFIG_H
#define NOVACONFIG_H

#include <QtGui/QMainWindow>
#include "ui_novaconfig.h"

class NovaConfig : public QMainWindow
{
    Q_OBJECT

public:
    NovaConfig(QWidget *parent = 0);

    ~NovaConfig();

    void LoadPreferences();

private slots:

//Which Preferences group is selected
void on_treeWidget_itemClicked(QTreeWidgetItem * item, int column);

//Preferences Buttons
void on_cancelButton_clicked();
void on_okButton_clicked();
void on_defaultsButton_clicked();
void on_applyButton_clicked();

//Browse dialogs for file paths
void on_pcapButton_clicked();
void on_dataButton_clicked();
void on_hsConfigButton_clicked();
void on_dmConfigButton_clicked();

//Check Box signal for enabling group box
void on_pcapCheckBox_stateChanged(int state);




private:
    Ui::NovaConfigClass ui;
};

#endif // NOVACONFIG_H
