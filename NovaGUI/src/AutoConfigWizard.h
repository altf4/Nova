#ifndef AUTOCONFIGWIZARD_H
#define AUTOCONFIGWIZARD_H

#include <QtGui/QMainWindow>
#include "ui_AutoConfigWizard.h"

class AutoConfigWizard : public QMainWindow
{
    Q_OBJECT

public:
    AutoConfigWizard(QWidget *parent = 0);
    ~AutoConfigWizard();

private:
    Ui::AutoConfigWizardClass ui;
};

#endif // AUTOCONFIGWIZARD_H
