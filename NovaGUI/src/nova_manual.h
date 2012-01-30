#ifndef NOVA_MANUAL_H
#define NOVA_MANUAL_H

#include <QtGui/QMainWindow>
#include <QStringList>
#include <QString>
#include "ui_nova_manual.h"
#include "novagui.h"

class Nova_Manual : public QMainWindow
{
    Q_OBJECT

public:
    Nova_Manual(QWidget *parent);
    ~Nova_Manual();
    void closeEvent(QCloseEvent * e);
    void setToSelected();
    void setPaths();

private:
    Ui::Nova_ManualClass ui;
    QStringList *paths;

private slots:
	void on_helpTree_itemSelectionChanged();
};

#endif // NOVA_MANUAL_H
