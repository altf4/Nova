/********************************************************************************
** Form generated from reading UI file 'InterfaceForm.ui'
**
** Created: Tue Jun 26 17:38:15 2012
**      by: Qt User Interface Compiler version 4.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_INTERFACEFORM_H
#define UI_INTERFACEFORM_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHeaderView>
#include <QtGui/QSpacerItem>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_InterfaceFormClass
{
public:
    QGridLayout *interfaceGridLayout;
    QGroupBox *interfaceGroupBox;
    QGridLayout *interfaceGroupBoxGridLayout;
    QSpacerItem *interfacesVSpacer;

    void setupUi(QWidget *InterfaceFormClass)
    {
        if (InterfaceFormClass->objectName().isEmpty())
            InterfaceFormClass->setObjectName(QString::fromUtf8("InterfaceFormClass"));
        InterfaceFormClass->resize(400, 300);
        interfaceGridLayout = new QGridLayout(InterfaceFormClass);
        interfaceGridLayout->setSpacing(0);
        interfaceGridLayout->setContentsMargins(0, 0, 0, 0);
        interfaceGridLayout->setObjectName(QString::fromUtf8("interfaceGridLayout"));
        interfaceGroupBox = new QGroupBox(InterfaceFormClass);
        interfaceGroupBox->setObjectName(QString::fromUtf8("interfaceGroupBox"));
        interfaceGroupBox->setEnabled(true);
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(interfaceGroupBox->sizePolicy().hasHeightForWidth());
        interfaceGroupBox->setSizePolicy(sizePolicy);
        interfaceGroupBox->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
        interfaceGroupBoxGridLayout = new QGridLayout(interfaceGroupBox);
        interfaceGroupBoxGridLayout->setSpacing(6);
        interfaceGroupBoxGridLayout->setContentsMargins(11, 11, 11, 11);
        interfaceGroupBoxGridLayout->setObjectName(QString::fromUtf8("interfaceGroupBoxGridLayout"));
        interfaceGroupBoxGridLayout->setHorizontalSpacing(0);
        interfaceGroupBoxGridLayout->setVerticalSpacing(6);
        interfaceGroupBoxGridLayout->setContentsMargins(0, 6, 0, 0);

        interfaceGridLayout->addWidget(interfaceGroupBox, 0, 0, 1, 1);

        interfacesVSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        interfaceGridLayout->addItem(interfacesVSpacer, 1, 0, 1, 1);


        retranslateUi(InterfaceFormClass);

        QMetaObject::connectSlotsByName(InterfaceFormClass);
    } // setupUi

    void retranslateUi(QWidget *InterfaceFormClass)
    {
        InterfaceFormClass->setWindowTitle(QApplication::translate("InterfaceFormClass", "InterfaceForm", 0, QApplication::UnicodeUTF8));
        interfaceGroupBox->setTitle(QApplication::translate("InterfaceFormClass", "Interfaces", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class InterfaceFormClass: public Ui_InterfaceFormClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_INTERFACEFORM_H
