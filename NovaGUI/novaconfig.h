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

private slots:

void on_treeWidget_itemClicked(QTreeWidgetItem * item, int column);

private:
    Ui::NovaConfigClass ui;
};

#endif // NOVACONFIG_H
