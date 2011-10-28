/********************************************************************************
** Form generated from reading UI file 'novagui.ui'
**
** Created: Fri Oct 28 15:35:07 2011
**      by: Qt User Interface Compiler version 4.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_NOVAGUI_H
#define UI_NOVAGUI_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QFrame>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QListWidget>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QPushButton>
#include <QtGui/QStackedWidget>
#include <QtGui/QStatusBar>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_NovaGUIClass
{
public:
    QAction *actionConfigure;
    QWidget *centralwidget;
    QStackedWidget *stackedWidget;
    QWidget *stackedWidgetPage1;
    QListWidget *benignList;
    QLabel *bListLabel;
    QLabel *hListLabel;
    QListWidget *hostileList;
    QWidget *stackedWidgetPage2;
    QPlainTextEdit *CETrainingTimeoutEdit;
    QLabel *CEInterfaceLabel;
    QLabel *CEIPCMaxConnectLabel;
    QPlainTextEdit *CEMaxFeatureValEdit;
    QLabel *CEConfigLabel;
    QPlainTextEdit *CEEnableTrainingEdit;
    QLabel *CEDatafileLabel;
    QFrame *CELine_2;
    QPlainTextEdit *CEMaxPtsEdit;
    QPlainTextEdit *CEClassificationTimeoutEdit;
    QPlainTextEdit *CEEPSEdit;
    QPlainTextEdit *CEBroadcastAddrEdit;
    QPlainTextEdit *CESilentAlarmPortEdit;
    QPlainTextEdit *CEKEdit;
    QPlainTextEdit *CEInterfaceEdit;
    QPlainTextEdit *CEIPCMaxConnectEdit;
    QPlainTextEdit *CEDatafileEdit;
    QLabel *CEBroadcastAddrLabel;
    QLabel *CESilentAlarmPortLabel;
    QLabel *CEKLabel;
    QLabel *CEEPSLabel;
    QLabel *CEMaxPtsLabel;
    QLabel *CEClassificationTimeoutLabel;
    QLabel *CETrainingTimeoutLabel;
    QLabel *CEMaxFeatureValLabel;
    QLabel *CEEnableTrainingLabel;
    QWidget *stackedWidgetPage3;
    QPlainTextEdit *DMInterfaceEdit;
    QFrame *DMLine;
    QLabel *DMIPLabel;
    QLabel *DMInterfaceLabel;
    QPlainTextEdit *DMIPEdit;
    QLabel *DMEnabledLabel;
    QPlainTextEdit *DMEnabledEdit;
    QLabel *DMConfigLabel;
    QWidget *stackedWidgetPage4;
    QLabel *HSConfigLabel;
    QPlainTextEdit *HSFreqEdit;
    QFrame *HSLine;
    QLabel *HSInterfaceLabel;
    QPlainTextEdit *HSTimeoutEdit;
    QPlainTextEdit *HSInterfaceEdit;
    QLabel *HSTimeoutLabel;
    QLabel *HSFreqLabel;
    QPlainTextEdit *HSHoneyDConfigEdit;
    QLabel *HSHoneyDConfigLabel;
    QWidget *stackedWidgetPage5;
    QPlainTextEdit *LTMInterfaceEdit;
    QPlainTextEdit *LTMTimeoutEdit;
    QPlainTextEdit *LTMFreqEdit;
    QLabel *LTMInterfaceLabel;
    QLabel *LTMTimeoutLabel;
    QLabel *LTMFreqLabel;
    QFrame *LTMLine;
    QLabel *LTMConfigLabel;
    QPushButton *SuspectButton;
    QPushButton *CEButton;
    QPushButton *DMButton;
    QPushButton *HSButton;
    QPushButton *LTMButton;
    QMenuBar *menubar;
    QMenu *menuFile;
    QMenu *menuEdit;
    QMenu *menuRun;
    QMenu *menuView;
    QMenu *menuHelp;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *NovaGUIClass)
    {
        if (NovaGUIClass->objectName().isEmpty())
            NovaGUIClass->setObjectName(QString::fromUtf8("NovaGUIClass"));
        NovaGUIClass->resize(800, 600);
        actionConfigure = new QAction(NovaGUIClass);
        actionConfigure->setObjectName(QString::fromUtf8("actionConfigure"));
        centralwidget = new QWidget(NovaGUIClass);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        stackedWidget = new QStackedWidget(centralwidget);
        stackedWidget->setObjectName(QString::fromUtf8("stackedWidget"));
        stackedWidget->setGeometry(QRect(100, -20, 691, 581));
        stackedWidgetPage1 = new QWidget();
        stackedWidgetPage1->setObjectName(QString::fromUtf8("stackedWidgetPage1"));
        benignList = new QListWidget(stackedWidgetPage1);
        benignList->setObjectName(QString::fromUtf8("benignList"));
        benignList->setGeometry(QRect(20, 90, 301, 471));
        bListLabel = new QLabel(stackedWidgetPage1);
        bListLabel->setObjectName(QString::fromUtf8("bListLabel"));
        bListLabel->setGeometry(QRect(100, 60, 121, 31));
        hListLabel = new QLabel(stackedWidgetPage1);
        hListLabel->setObjectName(QString::fromUtf8("hListLabel"));
        hListLabel->setGeometry(QRect(450, 60, 121, 31));
        hostileList = new QListWidget(stackedWidgetPage1);
        hostileList->setObjectName(QString::fromUtf8("hostileList"));
        hostileList->setGeometry(QRect(370, 90, 301, 471));
        stackedWidget->addWidget(stackedWidgetPage1);
        stackedWidgetPage2 = new QWidget();
        stackedWidgetPage2->setObjectName(QString::fromUtf8("stackedWidgetPage2"));
        CETrainingTimeoutEdit = new QPlainTextEdit(stackedWidgetPage2);
        CETrainingTimeoutEdit->setObjectName(QString::fromUtf8("CETrainingTimeoutEdit"));
        CETrainingTimeoutEdit->setGeometry(QRect(190, 340, 181, 21));
        CEInterfaceLabel = new QLabel(stackedWidgetPage2);
        CEInterfaceLabel->setObjectName(QString::fromUtf8("CEInterfaceLabel"));
        CEInterfaceLabel->setGeometry(QRect(110, 70, 71, 20));
        CEIPCMaxConnectLabel = new QLabel(stackedWidgetPage2);
        CEIPCMaxConnectLabel->setObjectName(QString::fromUtf8("CEIPCMaxConnectLabel"));
        CEIPCMaxConnectLabel->setGeometry(QRect(40, 130, 131, 20));
        CEMaxFeatureValEdit = new QPlainTextEdit(stackedWidgetPage2);
        CEMaxFeatureValEdit->setObjectName(QString::fromUtf8("CEMaxFeatureValEdit"));
        CEMaxFeatureValEdit->setGeometry(QRect(190, 370, 181, 21));
        CEConfigLabel = new QLabel(stackedWidgetPage2);
        CEConfigLabel->setObjectName(QString::fromUtf8("CEConfigLabel"));
        CEConfigLabel->setGeometry(QRect(250, 40, 67, 17));
        CEEnableTrainingEdit = new QPlainTextEdit(stackedWidgetPage2);
        CEEnableTrainingEdit->setObjectName(QString::fromUtf8("CEEnableTrainingEdit"));
        CEEnableTrainingEdit->setGeometry(QRect(190, 400, 181, 21));
        CEDatafileLabel = new QLabel(stackedWidgetPage2);
        CEDatafileLabel->setObjectName(QString::fromUtf8("CEDatafileLabel"));
        CEDatafileLabel->setGeometry(QRect(90, 100, 81, 21));
        CELine_2 = new QFrame(stackedWidgetPage2);
        CELine_2->setObjectName(QString::fromUtf8("CELine_2"));
        CELine_2->setGeometry(QRect(170, 70, 20, 351));
        CELine_2->setFrameShape(QFrame::VLine);
        CELine_2->setFrameShadow(QFrame::Sunken);
        CEMaxPtsEdit = new QPlainTextEdit(stackedWidgetPage2);
        CEMaxPtsEdit->setObjectName(QString::fromUtf8("CEMaxPtsEdit"));
        CEMaxPtsEdit->setGeometry(QRect(190, 280, 181, 21));
        CEClassificationTimeoutEdit = new QPlainTextEdit(stackedWidgetPage2);
        CEClassificationTimeoutEdit->setObjectName(QString::fromUtf8("CEClassificationTimeoutEdit"));
        CEClassificationTimeoutEdit->setGeometry(QRect(190, 310, 181, 21));
        CEEPSEdit = new QPlainTextEdit(stackedWidgetPage2);
        CEEPSEdit->setObjectName(QString::fromUtf8("CEEPSEdit"));
        CEEPSEdit->setGeometry(QRect(190, 250, 181, 21));
        CEBroadcastAddrEdit = new QPlainTextEdit(stackedWidgetPage2);
        CEBroadcastAddrEdit->setObjectName(QString::fromUtf8("CEBroadcastAddrEdit"));
        CEBroadcastAddrEdit->setGeometry(QRect(190, 160, 181, 21));
        CESilentAlarmPortEdit = new QPlainTextEdit(stackedWidgetPage2);
        CESilentAlarmPortEdit->setObjectName(QString::fromUtf8("CESilentAlarmPortEdit"));
        CESilentAlarmPortEdit->setGeometry(QRect(190, 190, 181, 21));
        CEKEdit = new QPlainTextEdit(stackedWidgetPage2);
        CEKEdit->setObjectName(QString::fromUtf8("CEKEdit"));
        CEKEdit->setGeometry(QRect(190, 220, 181, 21));
        CEInterfaceEdit = new QPlainTextEdit(stackedWidgetPage2);
        CEInterfaceEdit->setObjectName(QString::fromUtf8("CEInterfaceEdit"));
        CEInterfaceEdit->setGeometry(QRect(190, 70, 181, 21));
        CEIPCMaxConnectEdit = new QPlainTextEdit(stackedWidgetPage2);
        CEIPCMaxConnectEdit->setObjectName(QString::fromUtf8("CEIPCMaxConnectEdit"));
        CEIPCMaxConnectEdit->setGeometry(QRect(190, 130, 181, 21));
        CEDatafileEdit = new QPlainTextEdit(stackedWidgetPage2);
        CEDatafileEdit->setObjectName(QString::fromUtf8("CEDatafileEdit"));
        CEDatafileEdit->setGeometry(QRect(190, 100, 181, 21));
        CEBroadcastAddrLabel = new QLabel(stackedWidgetPage2);
        CEBroadcastAddrLabel->setObjectName(QString::fromUtf8("CEBroadcastAddrLabel"));
        CEBroadcastAddrLabel->setGeometry(QRect(60, 160, 121, 20));
        CESilentAlarmPortLabel = new QLabel(stackedWidgetPage2);
        CESilentAlarmPortLabel->setObjectName(QString::fromUtf8("CESilentAlarmPortLabel"));
        CESilentAlarmPortLabel->setGeometry(QRect(70, 190, 101, 20));
        CEKLabel = new QLabel(stackedWidgetPage2);
        CEKLabel->setObjectName(QString::fromUtf8("CEKLabel"));
        CEKLabel->setGeometry(QRect(160, 220, 16, 20));
        CEEPSLabel = new QLabel(stackedWidgetPage2);
        CEEPSLabel->setObjectName(QString::fromUtf8("CEEPSLabel"));
        CEEPSLabel->setGeometry(QRect(150, 250, 21, 20));
        CEMaxPtsLabel = new QLabel(stackedWidgetPage2);
        CEMaxPtsLabel->setObjectName(QString::fromUtf8("CEMaxPtsLabel"));
        CEMaxPtsLabel->setGeometry(QRect(120, 280, 51, 20));
        CEClassificationTimeoutLabel = new QLabel(stackedWidgetPage2);
        CEClassificationTimeoutLabel->setObjectName(QString::fromUtf8("CEClassificationTimeoutLabel"));
        CEClassificationTimeoutLabel->setGeometry(QRect(40, 310, 131, 20));
        CETrainingTimeoutLabel = new QLabel(stackedWidgetPage2);
        CETrainingTimeoutLabel->setObjectName(QString::fromUtf8("CETrainingTimeoutLabel"));
        CETrainingTimeoutLabel->setGeometry(QRect(70, 340, 101, 20));
        CEMaxFeatureValLabel = new QLabel(stackedWidgetPage2);
        CEMaxFeatureValLabel->setObjectName(QString::fromUtf8("CEMaxFeatureValLabel"));
        CEMaxFeatureValLabel->setGeometry(QRect(60, 370, 111, 20));
        CEEnableTrainingLabel = new QLabel(stackedWidgetPage2);
        CEEnableTrainingLabel->setObjectName(QString::fromUtf8("CEEnableTrainingLabel"));
        CEEnableTrainingLabel->setGeometry(QRect(40, 400, 131, 20));
        stackedWidget->addWidget(stackedWidgetPage2);
        stackedWidgetPage3 = new QWidget();
        stackedWidgetPage3->setObjectName(QString::fromUtf8("stackedWidgetPage3"));
        DMInterfaceEdit = new QPlainTextEdit(stackedWidgetPage3);
        DMInterfaceEdit->setObjectName(QString::fromUtf8("DMInterfaceEdit"));
        DMInterfaceEdit->setGeometry(QRect(180, 70, 181, 21));
        DMLine = new QFrame(stackedWidgetPage3);
        DMLine->setObjectName(QString::fromUtf8("DMLine"));
        DMLine->setGeometry(QRect(160, 70, 16, 81));
        DMLine->setFrameShape(QFrame::VLine);
        DMLine->setFrameShadow(QFrame::Sunken);
        DMIPLabel = new QLabel(stackedWidgetPage3);
        DMIPLabel->setObjectName(QString::fromUtf8("DMIPLabel"));
        DMIPLabel->setGeometry(QRect(60, 100, 101, 21));
        DMInterfaceLabel = new QLabel(stackedWidgetPage3);
        DMInterfaceLabel->setObjectName(QString::fromUtf8("DMInterfaceLabel"));
        DMInterfaceLabel->setGeometry(QRect(100, 70, 71, 21));
        DMIPEdit = new QPlainTextEdit(stackedWidgetPage3);
        DMIPEdit->setObjectName(QString::fromUtf8("DMIPEdit"));
        DMIPEdit->setGeometry(QRect(180, 100, 181, 21));
        DMEnabledLabel = new QLabel(stackedWidgetPage3);
        DMEnabledLabel->setObjectName(QString::fromUtf8("DMEnabledLabel"));
        DMEnabledLabel->setGeometry(QRect(30, 130, 131, 20));
        DMEnabledEdit = new QPlainTextEdit(stackedWidgetPage3);
        DMEnabledEdit->setObjectName(QString::fromUtf8("DMEnabledEdit"));
        DMEnabledEdit->setGeometry(QRect(180, 130, 181, 21));
        DMConfigLabel = new QLabel(stackedWidgetPage3);
        DMConfigLabel->setObjectName(QString::fromUtf8("DMConfigLabel"));
        DMConfigLabel->setGeometry(QRect(220, 40, 67, 17));
        stackedWidget->addWidget(stackedWidgetPage3);
        stackedWidgetPage4 = new QWidget();
        stackedWidgetPage4->setObjectName(QString::fromUtf8("stackedWidgetPage4"));
        HSConfigLabel = new QLabel(stackedWidgetPage4);
        HSConfigLabel->setObjectName(QString::fromUtf8("HSConfigLabel"));
        HSConfigLabel->setGeometry(QRect(240, 80, 67, 17));
        HSFreqEdit = new QPlainTextEdit(stackedWidgetPage4);
        HSFreqEdit->setObjectName(QString::fromUtf8("HSFreqEdit"));
        HSFreqEdit->setGeometry(QRect(200, 220, 181, 21));
        HSLine = new QFrame(stackedWidgetPage4);
        HSLine->setObjectName(QString::fromUtf8("HSLine"));
        HSLine->setGeometry(QRect(180, 130, 20, 111));
        HSLine->setFrameShape(QFrame::VLine);
        HSLine->setFrameShadow(QFrame::Sunken);
        HSInterfaceLabel = new QLabel(stackedWidgetPage4);
        HSInterfaceLabel->setObjectName(QString::fromUtf8("HSInterfaceLabel"));
        HSInterfaceLabel->setGeometry(QRect(120, 130, 71, 21));
        HSTimeoutEdit = new QPlainTextEdit(stackedWidgetPage4);
        HSTimeoutEdit->setObjectName(QString::fromUtf8("HSTimeoutEdit"));
        HSTimeoutEdit->setGeometry(QRect(200, 190, 181, 21));
        HSInterfaceEdit = new QPlainTextEdit(stackedWidgetPage4);
        HSInterfaceEdit->setObjectName(QString::fromUtf8("HSInterfaceEdit"));
        HSInterfaceEdit->setGeometry(QRect(200, 130, 181, 21));
        HSTimeoutLabel = new QLabel(stackedWidgetPage4);
        HSTimeoutLabel->setObjectName(QString::fromUtf8("HSTimeoutLabel"));
        HSTimeoutLabel->setGeometry(QRect(100, 190, 91, 21));
        HSFreqLabel = new QLabel(stackedWidgetPage4);
        HSFreqLabel->setObjectName(QString::fromUtf8("HSFreqLabel"));
        HSFreqLabel->setGeometry(QRect(60, 220, 131, 20));
        HSHoneyDConfigEdit = new QPlainTextEdit(stackedWidgetPage4);
        HSHoneyDConfigEdit->setObjectName(QString::fromUtf8("HSHoneyDConfigEdit"));
        HSHoneyDConfigEdit->setGeometry(QRect(200, 160, 181, 21));
        HSHoneyDConfigLabel = new QLabel(stackedWidgetPage4);
        HSHoneyDConfigLabel->setObjectName(QString::fromUtf8("HSHoneyDConfigLabel"));
        HSHoneyDConfigLabel->setGeometry(QRect(30, 160, 151, 21));
        stackedWidget->addWidget(stackedWidgetPage4);
        stackedWidgetPage5 = new QWidget();
        stackedWidgetPage5->setObjectName(QString::fromUtf8("stackedWidgetPage5"));
        LTMInterfaceEdit = new QPlainTextEdit(stackedWidgetPage5);
        LTMInterfaceEdit->setObjectName(QString::fromUtf8("LTMInterfaceEdit"));
        LTMInterfaceEdit->setGeometry(QRect(220, 90, 181, 21));
        LTMTimeoutEdit = new QPlainTextEdit(stackedWidgetPage5);
        LTMTimeoutEdit->setObjectName(QString::fromUtf8("LTMTimeoutEdit"));
        LTMTimeoutEdit->setGeometry(QRect(220, 120, 181, 21));
        LTMFreqEdit = new QPlainTextEdit(stackedWidgetPage5);
        LTMFreqEdit->setObjectName(QString::fromUtf8("LTMFreqEdit"));
        LTMFreqEdit->setGeometry(QRect(220, 150, 181, 21));
        LTMInterfaceLabel = new QLabel(stackedWidgetPage5);
        LTMInterfaceLabel->setObjectName(QString::fromUtf8("LTMInterfaceLabel"));
        LTMInterfaceLabel->setGeometry(QRect(140, 90, 71, 21));
        LTMTimeoutLabel = new QLabel(stackedWidgetPage5);
        LTMTimeoutLabel->setObjectName(QString::fromUtf8("LTMTimeoutLabel"));
        LTMTimeoutLabel->setGeometry(QRect(120, 120, 91, 21));
        LTMFreqLabel = new QLabel(stackedWidgetPage5);
        LTMFreqLabel->setObjectName(QString::fromUtf8("LTMFreqLabel"));
        LTMFreqLabel->setGeometry(QRect(80, 150, 131, 20));
        LTMLine = new QFrame(stackedWidgetPage5);
        LTMLine->setObjectName(QString::fromUtf8("LTMLine"));
        LTMLine->setGeometry(QRect(200, 90, 16, 81));
        LTMLine->setFrameShape(QFrame::VLine);
        LTMLine->setFrameShadow(QFrame::Sunken);
        LTMConfigLabel = new QLabel(stackedWidgetPage5);
        LTMConfigLabel->setObjectName(QString::fromUtf8("LTMConfigLabel"));
        LTMConfigLabel->setGeometry(QRect(260, 60, 67, 17));
        stackedWidget->addWidget(stackedWidgetPage5);
        SuspectButton = new QPushButton(centralwidget);
        SuspectButton->setObjectName(QString::fromUtf8("SuspectButton"));
        SuspectButton->setGeometry(QRect(0, 0, 101, 111));
        CEButton = new QPushButton(centralwidget);
        CEButton->setObjectName(QString::fromUtf8("CEButton"));
        CEButton->setGeometry(QRect(0, 110, 101, 111));
        CEButton->setAutoRepeat(false);
        DMButton = new QPushButton(centralwidget);
        DMButton->setObjectName(QString::fromUtf8("DMButton"));
        DMButton->setGeometry(QRect(0, 220, 101, 111));
        HSButton = new QPushButton(centralwidget);
        HSButton->setObjectName(QString::fromUtf8("HSButton"));
        HSButton->setGeometry(QRect(0, 330, 101, 111));
        LTMButton = new QPushButton(centralwidget);
        LTMButton->setObjectName(QString::fromUtf8("LTMButton"));
        LTMButton->setGeometry(QRect(0, 440, 101, 111));
        NovaGUIClass->setCentralWidget(centralwidget);
        menubar = new QMenuBar(NovaGUIClass);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 800, 20));
        menuFile = new QMenu(menubar);
        menuFile->setObjectName(QString::fromUtf8("menuFile"));
        menuEdit = new QMenu(menubar);
        menuEdit->setObjectName(QString::fromUtf8("menuEdit"));
        menuRun = new QMenu(menubar);
        menuRun->setObjectName(QString::fromUtf8("menuRun"));
        menuView = new QMenu(menubar);
        menuView->setObjectName(QString::fromUtf8("menuView"));
        menuHelp = new QMenu(menubar);
        menuHelp->setObjectName(QString::fromUtf8("menuHelp"));
        NovaGUIClass->setMenuBar(menubar);
        statusbar = new QStatusBar(NovaGUIClass);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        NovaGUIClass->setStatusBar(statusbar);

        menubar->addAction(menuFile->menuAction());
        menubar->addAction(menuEdit->menuAction());
        menubar->addAction(menuRun->menuAction());
        menubar->addAction(menuView->menuAction());
        menubar->addAction(menuHelp->menuAction());
        menuFile->addAction(actionConfigure);

        retranslateUi(NovaGUIClass);

        stackedWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(NovaGUIClass);
    } // setupUi

    void retranslateUi(QMainWindow *NovaGUIClass)
    {
        NovaGUIClass->setWindowTitle(QApplication::translate("NovaGUIClass", "MainWindow", 0, QApplication::UnicodeUTF8));
        actionConfigure->setText(QApplication::translate("NovaGUIClass", "Preferences", 0, QApplication::UnicodeUTF8));
        bListLabel->setText(QApplication::translate("NovaGUIClass", "Benign Suspects", 0, QApplication::UnicodeUTF8));
        hListLabel->setText(QApplication::translate("NovaGUIClass", " Hostile Suspects", 0, QApplication::UnicodeUTF8));
        CEInterfaceLabel->setText(QApplication::translate("NovaGUIClass", " Interface", 0, QApplication::UnicodeUTF8));
        CEIPCMaxConnectLabel->setText(QApplication::translate("NovaGUIClass", "Max IPC Connections", 0, QApplication::UnicodeUTF8));
        CEConfigLabel->setText(QApplication::translate("NovaGUIClass", "Config", 0, QApplication::UnicodeUTF8));
        CEDatafileLabel->setText(QApplication::translate("NovaGUIClass", "Data File Path", 0, QApplication::UnicodeUTF8));
        CEBroadcastAddrLabel->setText(QApplication::translate("NovaGUIClass", "Broadcast Address", 0, QApplication::UnicodeUTF8));
        CESilentAlarmPortLabel->setText(QApplication::translate("NovaGUIClass", "Silent Alarm Port", 0, QApplication::UnicodeUTF8));
        CEKLabel->setText(QApplication::translate("NovaGUIClass", "K", 0, QApplication::UnicodeUTF8));
        CEEPSLabel->setText(QApplication::translate("NovaGUIClass", "EPS", 0, QApplication::UnicodeUTF8));
        CEMaxPtsLabel->setText(QApplication::translate("NovaGUIClass", "Max PTS", 0, QApplication::UnicodeUTF8));
        CEClassificationTimeoutLabel->setText(QApplication::translate("NovaGUIClass", "Classification Timeout", 0, QApplication::UnicodeUTF8));
        CETrainingTimeoutLabel->setText(QApplication::translate("NovaGUIClass", "Training Timeout", 0, QApplication::UnicodeUTF8));
        CEMaxFeatureValLabel->setText(QApplication::translate("NovaGUIClass", "Max Feature Value", 0, QApplication::UnicodeUTF8));
        CEEnableTrainingLabel->setText(QApplication::translate("NovaGUIClass", "Enable Training Mode", 0, QApplication::UnicodeUTF8));
        DMIPLabel->setText(QApplication::translate("NovaGUIClass", "Doppelganger IP", 0, QApplication::UnicodeUTF8));
        DMInterfaceLabel->setText(QApplication::translate("NovaGUIClass", " Interface", 0, QApplication::UnicodeUTF8));
        DMEnabledLabel->setText(QApplication::translate("NovaGUIClass", "Enable Doppelganger", 0, QApplication::UnicodeUTF8));
        DMConfigLabel->setText(QApplication::translate("NovaGUIClass", "Config", 0, QApplication::UnicodeUTF8));
        HSConfigLabel->setText(QApplication::translate("NovaGUIClass", "Config", 0, QApplication::UnicodeUTF8));
        HSInterfaceLabel->setText(QApplication::translate("NovaGUIClass", " Interface", 0, QApplication::UnicodeUTF8));
        HSTimeoutLabel->setText(QApplication::translate("NovaGUIClass", "TCP Timeout", 0, QApplication::UnicodeUTF8));
        HSFreqLabel->setText(QApplication::translate("NovaGUIClass", "Timeout Frequency", 0, QApplication::UnicodeUTF8));
        HSHoneyDConfigLabel->setText(QApplication::translate("NovaGUIClass", " HoneyD Config File Path", 0, QApplication::UnicodeUTF8));
        LTMInterfaceLabel->setText(QApplication::translate("NovaGUIClass", " Interface", 0, QApplication::UnicodeUTF8));
        LTMTimeoutLabel->setText(QApplication::translate("NovaGUIClass", "TCP Timeout", 0, QApplication::UnicodeUTF8));
        LTMFreqLabel->setText(QApplication::translate("NovaGUIClass", "Timeout Frequency", 0, QApplication::UnicodeUTF8));
        LTMConfigLabel->setText(QApplication::translate("NovaGUIClass", "Config", 0, QApplication::UnicodeUTF8));
        SuspectButton->setText(QApplication::translate("NovaGUIClass", "Suspects", 0, QApplication::UnicodeUTF8));
        CEButton->setText(QApplication::translate("NovaGUIClass", "Classification\n"
"Engine", 0, QApplication::UnicodeUTF8));
        DMButton->setText(QApplication::translate("NovaGUIClass", "Doppelganger\n"
"Module", 0, QApplication::UnicodeUTF8));
        HSButton->setText(QApplication::translate("NovaGUIClass", "Haystack\n"
"Monitor", 0, QApplication::UnicodeUTF8));
        LTMButton->setText(QApplication::translate("NovaGUIClass", "Local\n"
"Traffic\n"
"Monitor", 0, QApplication::UnicodeUTF8));
        menuFile->setTitle(QApplication::translate("NovaGUIClass", "File", 0, QApplication::UnicodeUTF8));
        menuEdit->setTitle(QApplication::translate("NovaGUIClass", "Edit", 0, QApplication::UnicodeUTF8));
        menuRun->setTitle(QApplication::translate("NovaGUIClass", "Run", 0, QApplication::UnicodeUTF8));
        menuView->setTitle(QApplication::translate("NovaGUIClass", "View", 0, QApplication::UnicodeUTF8));
        menuHelp->setTitle(QApplication::translate("NovaGUIClass", "Help", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class NovaGUIClass: public Ui_NovaGUIClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_NOVAGUI_H
