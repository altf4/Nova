#ifndef NOVA_MANUAL_H
#define NOVA_MANUAL_H

#include <QtGui/QMainWindow>
#include "ui_nova_manual.h"

class Nova_Manual : public QMainWindow
{
    Q_OBJECT

public:
    Nova_Manual(QWidget *parent = 0);
    ~Nova_Manual();

private:
    Ui::Nova_ManualClass ui;
};

#endif // NOVA_MANUAL_H
