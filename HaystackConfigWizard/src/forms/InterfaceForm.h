#ifndef INTERFACEFORM_H
#define INTERFACEFORM_H

#include <QtGui/QWidget>
#include "ui_InterfaceForm.h"

class InterfaceForm : public QWidget
{
    Q_OBJECT

public:
    InterfaceForm(QWidget *parent = 0);
    ~InterfaceForm();

private:
    Ui::InterfaceFormClass ui;
    QButtonGroup *m_interfaceCheckBoxes;
};

#endif // INTERFACEFORM_H
