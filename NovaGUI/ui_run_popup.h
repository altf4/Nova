/********************************************************************************
** Form generated from reading UI file 'run_popup.ui'
**
** Created: Mon Oct 31 11:37:11 2011
**      by: Qt User Interface Compiler version 4.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_RUN_POPUP_H
#define UI_RUN_POPUP_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QGroupBox>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QMainWindow>
#include <QtGui/QPushButton>
#include <QtGui/QTextEdit>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Run_PopupClass
{
public:
    QWidget *centralwidget;
    QCheckBox *trainingCheckBox;
    QCheckBox *pcapCheckBox;
    QGroupBox *pcapGroupBox;
    QTextEdit *pcapEdit;
    QLabel *pcapLabel;
    QPushButton *pcapButton;
    QCheckBox *liveCapCheckBox;
    QPushButton *startButton;
    QPushButton *cancelButton;

    void setupUi(QMainWindow *Run_PopupClass)
    {
        if (Run_PopupClass->objectName().isEmpty())
            Run_PopupClass->setObjectName(QString::fromUtf8("Run_PopupClass"));
        Run_PopupClass->resize(379, 304);
        centralwidget = new QWidget(Run_PopupClass);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        trainingCheckBox = new QCheckBox(centralwidget);
        trainingCheckBox->setObjectName(QString::fromUtf8("trainingCheckBox"));
        trainingCheckBox->setGeometry(QRect(20, 20, 291, 20));
        trainingCheckBox->setLayoutDirection(Qt::LeftToRight);
        pcapCheckBox = new QCheckBox(centralwidget);
        pcapCheckBox->setObjectName(QString::fromUtf8("pcapCheckBox"));
        pcapCheckBox->setGeometry(QRect(20, 60, 321, 20));
        pcapCheckBox->setLayoutDirection(Qt::LeftToRight);
        pcapGroupBox = new QGroupBox(centralwidget);
        pcapGroupBox->setObjectName(QString::fromUtf8("pcapGroupBox"));
        pcapGroupBox->setEnabled(true);
        pcapGroupBox->setGeometry(QRect(20, 90, 341, 171));
        pcapEdit = new QTextEdit(pcapGroupBox);
        pcapEdit->setObjectName(QString::fromUtf8("pcapEdit"));
        pcapEdit->setEnabled(false);
        pcapEdit->setGeometry(QRect(20, 60, 248, 20));
        pcapLabel = new QLabel(pcapGroupBox);
        pcapLabel->setObjectName(QString::fromUtf8("pcapLabel"));
        pcapLabel->setGeometry(QRect(80, 30, 151, 20));
        pcapButton = new QPushButton(pcapGroupBox);
        pcapButton->setObjectName(QString::fromUtf8("pcapButton"));
        pcapButton->setGeometry(QRect(250, 57, 80, 26));
        liveCapCheckBox = new QCheckBox(pcapGroupBox);
        liveCapCheckBox->setObjectName(QString::fromUtf8("liveCapCheckBox"));
        liveCapCheckBox->setEnabled(true);
        liveCapCheckBox->setGeometry(QRect(20, 110, 301, 20));
        liveCapCheckBox->setLayoutDirection(Qt::LeftToRight);
        startButton = new QPushButton(centralwidget);
        startButton->setObjectName(QString::fromUtf8("startButton"));
        startButton->setGeometry(QRect(280, 270, 80, 25));
        cancelButton = new QPushButton(centralwidget);
        cancelButton->setObjectName(QString::fromUtf8("cancelButton"));
        cancelButton->setGeometry(QRect(20, 270, 80, 25));
        Run_PopupClass->setCentralWidget(centralwidget);

        retranslateUi(Run_PopupClass);

        QMetaObject::connectSlotsByName(Run_PopupClass);
    } // setupUi

    void retranslateUi(QMainWindow *Run_PopupClass)
    {
        Run_PopupClass->setWindowTitle(QApplication::translate("Run_PopupClass", "Run as...", 0, QApplication::UnicodeUTF8));
        trainingCheckBox->setText(QApplication::translate("Run_PopupClass", "Run in Training Mode", 0, QApplication::UnicodeUTF8));
        pcapCheckBox->setText(QApplication::translate("Run_PopupClass", "Read packets from file", 0, QApplication::UnicodeUTF8));
        pcapGroupBox->setTitle(QApplication::translate("Run_PopupClass", "Packet File Options", 0, QApplication::UnicodeUTF8));
        pcapLabel->setText(QApplication::translate("Run_PopupClass", "Packet Capture File", 0, QApplication::UnicodeUTF8));
        pcapButton->setText(QApplication::translate("Run_PopupClass", "Browse", 0, QApplication::UnicodeUTF8));
        liveCapCheckBox->setText(QApplication::translate("Run_PopupClass", "Go to live capture after reading", 0, QApplication::UnicodeUTF8));
        startButton->setText(QApplication::translate("Run_PopupClass", "Start", 0, QApplication::UnicodeUTF8));
        cancelButton->setText(QApplication::translate("Run_PopupClass", "Cancel", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class Run_PopupClass: public Ui_Run_PopupClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_RUN_POPUP_H
