/********************************************************************************
** Form generated from reading UI file 'novagui.ui'
**
** Created: Tue Oct 25 12:39:03 2011
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
#include <QtGui/QStatusBar>
#include <QtGui/QTabWidget>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_NovaGUIClass
{
public:
    QAction *actionTodo;
    QAction *actionTodo_2;
    QAction *actionTodo_3;
    QAction *actionTodo_4;
    QWidget *centralwidget;
    QTabWidget *tabWidget;
    QWidget *Suspects;
    QListWidget *benignList;
    QLabel *bListLabel;
    QLabel *hListLabel;
    QListWidget *hostileList;
    QWidget *CETab;
    QPushButton *CELoadButton;
    QPlainTextEdit *CETrainingTimeoutEdit;
    QLabel *CEInterfaceLabel;
    QLabel *CEIPCMaxConnectLabel;
    QPushButton *CESaveButton;
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
    QWidget *DMTab;
    QPlainTextEdit *DMInterfaceEdit;
    QFrame *DMLine;
    QLabel *DMIPLabel;
    QLabel *DMInterfaceLabel;
    QPlainTextEdit *DMIPEdit;
    QLabel *DMEnabledLabel;
    QPlainTextEdit *DMEnabledEdit;
    QPushButton *DMSaveButton;
    QLabel *DMConfigLabel;
    QPushButton *DMLoadButton;
    QWidget *HSTab;
    QLabel *HSConfigLabel;
    QPlainTextEdit *HSFreqEdit;
    QPushButton *HSLoadButton;
    QFrame *HSLine;
    QLabel *HSInterfaceLabel;
    QPlainTextEdit *HSTimeoutEdit;
    QPushButton *HSSaveButton;
    QPlainTextEdit *HSInterfaceEdit;
    QLabel *HSTimeoutLabel;
    QLabel *HSFreqLabel;
    QPlainTextEdit *HSHoneyDConfigEdit;
    QLabel *HSHoneyDConfigLabel;
    QWidget *LTMTab;
    QPlainTextEdit *LTMInterfaceEdit;
    QPlainTextEdit *LTMTimeoutEdit;
    QPlainTextEdit *LTMFreqEdit;
    QLabel *LTMInterfaceLabel;
    QLabel *LTMTimeoutLabel;
    QLabel *LTMFreqLabel;
    QFrame *LTMLine;
    QPushButton *LTMSaveButton;
    QLabel *LTMConfigLabel;
    QPushButton *LTMLoadButton;
    QMenuBar *menubar;
    QMenu *menuClassification_Engine;
    QMenu *menuDoppelganger_Module;
    QMenu *menuHaystack_Monitor;
    QMenu *menuLocal_Traffic_Monitor;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *NovaGUIClass)
    {
        if (NovaGUIClass->objectName().isEmpty())
            NovaGUIClass->setObjectName(QString::fromUtf8("NovaGUIClass"));
        NovaGUIClass->resize(800, 600);
        actionTodo = new QAction(NovaGUIClass);
        actionTodo->setObjectName(QString::fromUtf8("actionTodo"));
        actionTodo_2 = new QAction(NovaGUIClass);
        actionTodo_2->setObjectName(QString::fromUtf8("actionTodo_2"));
        actionTodo_3 = new QAction(NovaGUIClass);
        actionTodo_3->setObjectName(QString::fromUtf8("actionTodo_3"));
        actionTodo_4 = new QAction(NovaGUIClass);
        actionTodo_4->setObjectName(QString::fromUtf8("actionTodo_4"));
        centralwidget = new QWidget(NovaGUIClass);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        tabWidget = new QTabWidget(centralwidget);
        tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
        tabWidget->setGeometry(QRect(0, 0, 811, 561));
        Suspects = new QWidget();
        Suspects->setObjectName(QString::fromUtf8("Suspects"));
        benignList = new QListWidget(Suspects);
        benignList->setObjectName(QString::fromUtf8("benignList"));
        benignList->setGeometry(QRect(10, 50, 301, 471));
        bListLabel = new QLabel(Suspects);
        bListLabel->setObjectName(QString::fromUtf8("bListLabel"));
        bListLabel->setGeometry(QRect(90, 20, 111, 31));
        hListLabel = new QLabel(Suspects);
        hListLabel->setObjectName(QString::fromUtf8("hListLabel"));
        hListLabel->setGeometry(QRect(440, 20, 111, 31));
        hostileList = new QListWidget(Suspects);
        hostileList->setObjectName(QString::fromUtf8("hostileList"));
        hostileList->setGeometry(QRect(360, 50, 301, 471));
        tabWidget->addTab(Suspects, QString());
        CETab = new QWidget();
        CETab->setObjectName(QString::fromUtf8("CETab"));
        CELoadButton = new QPushButton(CETab);
        CELoadButton->setObjectName(QString::fromUtf8("CELoadButton"));
        CELoadButton->setGeometry(QRect(600, 500, 81, 27));
        CETrainingTimeoutEdit = new QPlainTextEdit(CETab);
        CETrainingTimeoutEdit->setObjectName(QString::fromUtf8("CETrainingTimeoutEdit"));
        CETrainingTimeoutEdit->setGeometry(QRect(610, 410, 181, 21));
        CEInterfaceLabel = new QLabel(CETab);
        CEInterfaceLabel->setObjectName(QString::fromUtf8("CEInterfaceLabel"));
        CEInterfaceLabel->setGeometry(QRect(530, 140, 71, 20));
        CEIPCMaxConnectLabel = new QLabel(CETab);
        CEIPCMaxConnectLabel->setObjectName(QString::fromUtf8("CEIPCMaxConnectLabel"));
        CEIPCMaxConnectLabel->setGeometry(QRect(460, 200, 131, 20));
        CESaveButton = new QPushButton(CETab);
        CESaveButton->setObjectName(QString::fromUtf8("CESaveButton"));
        CESaveButton->setGeometry(QRect(700, 500, 81, 27));
        CEMaxFeatureValEdit = new QPlainTextEdit(CETab);
        CEMaxFeatureValEdit->setObjectName(QString::fromUtf8("CEMaxFeatureValEdit"));
        CEMaxFeatureValEdit->setGeometry(QRect(610, 440, 181, 21));
        CEConfigLabel = new QLabel(CETab);
        CEConfigLabel->setObjectName(QString::fromUtf8("CEConfigLabel"));
        CEConfigLabel->setGeometry(QRect(670, 110, 67, 17));
        CEEnableTrainingEdit = new QPlainTextEdit(CETab);
        CEEnableTrainingEdit->setObjectName(QString::fromUtf8("CEEnableTrainingEdit"));
        CEEnableTrainingEdit->setGeometry(QRect(610, 470, 181, 21));
        CEDatafileLabel = new QLabel(CETab);
        CEDatafileLabel->setObjectName(QString::fromUtf8("CEDatafileLabel"));
        CEDatafileLabel->setGeometry(QRect(510, 170, 81, 21));
        CELine_2 = new QFrame(CETab);
        CELine_2->setObjectName(QString::fromUtf8("CELine_2"));
        CELine_2->setGeometry(QRect(590, 140, 20, 351));
        CELine_2->setFrameShape(QFrame::VLine);
        CELine_2->setFrameShadow(QFrame::Sunken);
        CEMaxPtsEdit = new QPlainTextEdit(CETab);
        CEMaxPtsEdit->setObjectName(QString::fromUtf8("CEMaxPtsEdit"));
        CEMaxPtsEdit->setGeometry(QRect(610, 350, 181, 21));
        CEClassificationTimeoutEdit = new QPlainTextEdit(CETab);
        CEClassificationTimeoutEdit->setObjectName(QString::fromUtf8("CEClassificationTimeoutEdit"));
        CEClassificationTimeoutEdit->setGeometry(QRect(610, 380, 181, 21));
        CEEPSEdit = new QPlainTextEdit(CETab);
        CEEPSEdit->setObjectName(QString::fromUtf8("CEEPSEdit"));
        CEEPSEdit->setGeometry(QRect(610, 320, 181, 21));
        CEBroadcastAddrEdit = new QPlainTextEdit(CETab);
        CEBroadcastAddrEdit->setObjectName(QString::fromUtf8("CEBroadcastAddrEdit"));
        CEBroadcastAddrEdit->setGeometry(QRect(610, 230, 181, 21));
        CESilentAlarmPortEdit = new QPlainTextEdit(CETab);
        CESilentAlarmPortEdit->setObjectName(QString::fromUtf8("CESilentAlarmPortEdit"));
        CESilentAlarmPortEdit->setGeometry(QRect(610, 260, 181, 21));
        CEKEdit = new QPlainTextEdit(CETab);
        CEKEdit->setObjectName(QString::fromUtf8("CEKEdit"));
        CEKEdit->setGeometry(QRect(610, 290, 181, 21));
        CEInterfaceEdit = new QPlainTextEdit(CETab);
        CEInterfaceEdit->setObjectName(QString::fromUtf8("CEInterfaceEdit"));
        CEInterfaceEdit->setGeometry(QRect(610, 140, 181, 21));
        CEIPCMaxConnectEdit = new QPlainTextEdit(CETab);
        CEIPCMaxConnectEdit->setObjectName(QString::fromUtf8("CEIPCMaxConnectEdit"));
        CEIPCMaxConnectEdit->setGeometry(QRect(610, 200, 181, 21));
        CEDatafileEdit = new QPlainTextEdit(CETab);
        CEDatafileEdit->setObjectName(QString::fromUtf8("CEDatafileEdit"));
        CEDatafileEdit->setGeometry(QRect(610, 170, 181, 21));
        CEBroadcastAddrLabel = new QLabel(CETab);
        CEBroadcastAddrLabel->setObjectName(QString::fromUtf8("CEBroadcastAddrLabel"));
        CEBroadcastAddrLabel->setGeometry(QRect(480, 230, 121, 20));
        CESilentAlarmPortLabel = new QLabel(CETab);
        CESilentAlarmPortLabel->setObjectName(QString::fromUtf8("CESilentAlarmPortLabel"));
        CESilentAlarmPortLabel->setGeometry(QRect(490, 260, 101, 20));
        CEKLabel = new QLabel(CETab);
        CEKLabel->setObjectName(QString::fromUtf8("CEKLabel"));
        CEKLabel->setGeometry(QRect(580, 290, 16, 20));
        CEEPSLabel = new QLabel(CETab);
        CEEPSLabel->setObjectName(QString::fromUtf8("CEEPSLabel"));
        CEEPSLabel->setGeometry(QRect(570, 320, 21, 20));
        CEMaxPtsLabel = new QLabel(CETab);
        CEMaxPtsLabel->setObjectName(QString::fromUtf8("CEMaxPtsLabel"));
        CEMaxPtsLabel->setGeometry(QRect(540, 350, 51, 20));
        CEClassificationTimeoutLabel = new QLabel(CETab);
        CEClassificationTimeoutLabel->setObjectName(QString::fromUtf8("CEClassificationTimeoutLabel"));
        CEClassificationTimeoutLabel->setGeometry(QRect(460, 380, 131, 20));
        CETrainingTimeoutLabel = new QLabel(CETab);
        CETrainingTimeoutLabel->setObjectName(QString::fromUtf8("CETrainingTimeoutLabel"));
        CETrainingTimeoutLabel->setGeometry(QRect(490, 410, 101, 20));
        CEMaxFeatureValLabel = new QLabel(CETab);
        CEMaxFeatureValLabel->setObjectName(QString::fromUtf8("CEMaxFeatureValLabel"));
        CEMaxFeatureValLabel->setGeometry(QRect(480, 440, 111, 20));
        CEEnableTrainingLabel = new QLabel(CETab);
        CEEnableTrainingLabel->setObjectName(QString::fromUtf8("CEEnableTrainingLabel"));
        CEEnableTrainingLabel->setGeometry(QRect(460, 470, 131, 20));
        tabWidget->addTab(CETab, QString());
        DMTab = new QWidget();
        DMTab->setObjectName(QString::fromUtf8("DMTab"));
        DMInterfaceEdit = new QPlainTextEdit(DMTab);
        DMInterfaceEdit->setObjectName(QString::fromUtf8("DMInterfaceEdit"));
        DMInterfaceEdit->setGeometry(QRect(610, 410, 181, 21));
        DMLine = new QFrame(DMTab);
        DMLine->setObjectName(QString::fromUtf8("DMLine"));
        DMLine->setGeometry(QRect(590, 410, 16, 81));
        DMLine->setFrameShape(QFrame::VLine);
        DMLine->setFrameShadow(QFrame::Sunken);
        DMIPLabel = new QLabel(DMTab);
        DMIPLabel->setObjectName(QString::fromUtf8("DMIPLabel"));
        DMIPLabel->setGeometry(QRect(490, 440, 101, 21));
        DMInterfaceLabel = new QLabel(DMTab);
        DMInterfaceLabel->setObjectName(QString::fromUtf8("DMInterfaceLabel"));
        DMInterfaceLabel->setGeometry(QRect(530, 410, 71, 21));
        DMIPEdit = new QPlainTextEdit(DMTab);
        DMIPEdit->setObjectName(QString::fromUtf8("DMIPEdit"));
        DMIPEdit->setGeometry(QRect(610, 440, 181, 21));
        DMEnabledLabel = new QLabel(DMTab);
        DMEnabledLabel->setObjectName(QString::fromUtf8("DMEnabledLabel"));
        DMEnabledLabel->setGeometry(QRect(460, 470, 131, 20));
        DMEnabledEdit = new QPlainTextEdit(DMTab);
        DMEnabledEdit->setObjectName(QString::fromUtf8("DMEnabledEdit"));
        DMEnabledEdit->setGeometry(QRect(610, 470, 181, 21));
        DMSaveButton = new QPushButton(DMTab);
        DMSaveButton->setObjectName(QString::fromUtf8("DMSaveButton"));
        DMSaveButton->setGeometry(QRect(700, 500, 81, 27));
        DMConfigLabel = new QLabel(DMTab);
        DMConfigLabel->setObjectName(QString::fromUtf8("DMConfigLabel"));
        DMConfigLabel->setGeometry(QRect(650, 380, 67, 17));
        DMLoadButton = new QPushButton(DMTab);
        DMLoadButton->setObjectName(QString::fromUtf8("DMLoadButton"));
        DMLoadButton->setGeometry(QRect(600, 500, 81, 27));
        tabWidget->addTab(DMTab, QString());
        HSTab = new QWidget();
        HSTab->setObjectName(QString::fromUtf8("HSTab"));
        HSConfigLabel = new QLabel(HSTab);
        HSConfigLabel->setObjectName(QString::fromUtf8("HSConfigLabel"));
        HSConfigLabel->setGeometry(QRect(650, 330, 67, 17));
        HSFreqEdit = new QPlainTextEdit(HSTab);
        HSFreqEdit->setObjectName(QString::fromUtf8("HSFreqEdit"));
        HSFreqEdit->setGeometry(QRect(610, 470, 181, 21));
        HSLoadButton = new QPushButton(HSTab);
        HSLoadButton->setObjectName(QString::fromUtf8("HSLoadButton"));
        HSLoadButton->setGeometry(QRect(600, 500, 81, 27));
        HSLine = new QFrame(HSTab);
        HSLine->setObjectName(QString::fromUtf8("HSLine"));
        HSLine->setGeometry(QRect(590, 380, 20, 111));
        HSLine->setFrameShape(QFrame::VLine);
        HSLine->setFrameShadow(QFrame::Sunken);
        HSInterfaceLabel = new QLabel(HSTab);
        HSInterfaceLabel->setObjectName(QString::fromUtf8("HSInterfaceLabel"));
        HSInterfaceLabel->setGeometry(QRect(530, 380, 71, 21));
        HSTimeoutEdit = new QPlainTextEdit(HSTab);
        HSTimeoutEdit->setObjectName(QString::fromUtf8("HSTimeoutEdit"));
        HSTimeoutEdit->setGeometry(QRect(610, 440, 181, 21));
        HSSaveButton = new QPushButton(HSTab);
        HSSaveButton->setObjectName(QString::fromUtf8("HSSaveButton"));
        HSSaveButton->setGeometry(QRect(700, 500, 81, 27));
        HSInterfaceEdit = new QPlainTextEdit(HSTab);
        HSInterfaceEdit->setObjectName(QString::fromUtf8("HSInterfaceEdit"));
        HSInterfaceEdit->setGeometry(QRect(610, 380, 181, 21));
        HSTimeoutLabel = new QLabel(HSTab);
        HSTimeoutLabel->setObjectName(QString::fromUtf8("HSTimeoutLabel"));
        HSTimeoutLabel->setGeometry(QRect(510, 440, 91, 21));
        HSFreqLabel = new QLabel(HSTab);
        HSFreqLabel->setObjectName(QString::fromUtf8("HSFreqLabel"));
        HSFreqLabel->setGeometry(QRect(470, 470, 131, 20));
        HSHoneyDConfigEdit = new QPlainTextEdit(HSTab);
        HSHoneyDConfigEdit->setObjectName(QString::fromUtf8("HSHoneyDConfigEdit"));
        HSHoneyDConfigEdit->setGeometry(QRect(610, 410, 181, 21));
        HSHoneyDConfigLabel = new QLabel(HSTab);
        HSHoneyDConfigLabel->setObjectName(QString::fromUtf8("HSHoneyDConfigLabel"));
        HSHoneyDConfigLabel->setGeometry(QRect(440, 410, 151, 21));
        tabWidget->addTab(HSTab, QString());
        LTMTab = new QWidget();
        LTMTab->setObjectName(QString::fromUtf8("LTMTab"));
        LTMInterfaceEdit = new QPlainTextEdit(LTMTab);
        LTMInterfaceEdit->setObjectName(QString::fromUtf8("LTMInterfaceEdit"));
        LTMInterfaceEdit->setGeometry(QRect(610, 410, 181, 21));
        LTMTimeoutEdit = new QPlainTextEdit(LTMTab);
        LTMTimeoutEdit->setObjectName(QString::fromUtf8("LTMTimeoutEdit"));
        LTMTimeoutEdit->setGeometry(QRect(610, 440, 181, 21));
        LTMFreqEdit = new QPlainTextEdit(LTMTab);
        LTMFreqEdit->setObjectName(QString::fromUtf8("LTMFreqEdit"));
        LTMFreqEdit->setGeometry(QRect(610, 470, 181, 21));
        LTMInterfaceLabel = new QLabel(LTMTab);
        LTMInterfaceLabel->setObjectName(QString::fromUtf8("LTMInterfaceLabel"));
        LTMInterfaceLabel->setGeometry(QRect(530, 410, 71, 21));
        LTMTimeoutLabel = new QLabel(LTMTab);
        LTMTimeoutLabel->setObjectName(QString::fromUtf8("LTMTimeoutLabel"));
        LTMTimeoutLabel->setGeometry(QRect(510, 440, 91, 21));
        LTMFreqLabel = new QLabel(LTMTab);
        LTMFreqLabel->setObjectName(QString::fromUtf8("LTMFreqLabel"));
        LTMFreqLabel->setGeometry(QRect(470, 470, 131, 20));
        LTMLine = new QFrame(LTMTab);
        LTMLine->setObjectName(QString::fromUtf8("LTMLine"));
        LTMLine->setGeometry(QRect(590, 410, 16, 81));
        LTMLine->setFrameShape(QFrame::VLine);
        LTMLine->setFrameShadow(QFrame::Sunken);
        LTMSaveButton = new QPushButton(LTMTab);
        LTMSaveButton->setObjectName(QString::fromUtf8("LTMSaveButton"));
        LTMSaveButton->setGeometry(QRect(700, 500, 81, 27));
        LTMConfigLabel = new QLabel(LTMTab);
        LTMConfigLabel->setObjectName(QString::fromUtf8("LTMConfigLabel"));
        LTMConfigLabel->setGeometry(QRect(650, 380, 67, 17));
        LTMLoadButton = new QPushButton(LTMTab);
        LTMLoadButton->setObjectName(QString::fromUtf8("LTMLoadButton"));
        LTMLoadButton->setGeometry(QRect(600, 500, 81, 27));
        tabWidget->addTab(LTMTab, QString());
        NovaGUIClass->setCentralWidget(centralwidget);
        menubar = new QMenuBar(NovaGUIClass);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 800, 20));
        menuClassification_Engine = new QMenu(menubar);
        menuClassification_Engine->setObjectName(QString::fromUtf8("menuClassification_Engine"));
        menuDoppelganger_Module = new QMenu(menubar);
        menuDoppelganger_Module->setObjectName(QString::fromUtf8("menuDoppelganger_Module"));
        menuHaystack_Monitor = new QMenu(menubar);
        menuHaystack_Monitor->setObjectName(QString::fromUtf8("menuHaystack_Monitor"));
        menuLocal_Traffic_Monitor = new QMenu(menubar);
        menuLocal_Traffic_Monitor->setObjectName(QString::fromUtf8("menuLocal_Traffic_Monitor"));
        NovaGUIClass->setMenuBar(menubar);
        statusbar = new QStatusBar(NovaGUIClass);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        NovaGUIClass->setStatusBar(statusbar);

        menubar->addAction(menuClassification_Engine->menuAction());
        menubar->addAction(menuDoppelganger_Module->menuAction());
        menubar->addAction(menuHaystack_Monitor->menuAction());
        menubar->addAction(menuLocal_Traffic_Monitor->menuAction());
        menuClassification_Engine->addAction(actionTodo);
        menuDoppelganger_Module->addAction(actionTodo_2);
        menuHaystack_Monitor->addAction(actionTodo_3);
        menuLocal_Traffic_Monitor->addAction(actionTodo_4);

        retranslateUi(NovaGUIClass);

        tabWidget->setCurrentIndex(3);


        QMetaObject::connectSlotsByName(NovaGUIClass);
    } // setupUi

    void retranslateUi(QMainWindow *NovaGUIClass)
    {
        NovaGUIClass->setWindowTitle(QApplication::translate("NovaGUIClass", "MainWindow", 0, QApplication::UnicodeUTF8));
        actionTodo->setText(QApplication::translate("NovaGUIClass", "Todo", 0, QApplication::UnicodeUTF8));
        actionTodo_2->setText(QApplication::translate("NovaGUIClass", "Todo", 0, QApplication::UnicodeUTF8));
        actionTodo_3->setText(QApplication::translate("NovaGUIClass", "Todo", 0, QApplication::UnicodeUTF8));
        actionTodo_4->setText(QApplication::translate("NovaGUIClass", "Todo", 0, QApplication::UnicodeUTF8));
        bListLabel->setText(QApplication::translate("NovaGUIClass", "Benign Suspects", 0, QApplication::UnicodeUTF8));
        hListLabel->setText(QApplication::translate("NovaGUIClass", " Hostile Suspects", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(Suspects), QApplication::translate("NovaGUIClass", "Suspects", 0, QApplication::UnicodeUTF8));
        CELoadButton->setText(QApplication::translate("NovaGUIClass", "Load", 0, QApplication::UnicodeUTF8));
        CEInterfaceLabel->setText(QApplication::translate("NovaGUIClass", " Interface", 0, QApplication::UnicodeUTF8));
        CEIPCMaxConnectLabel->setText(QApplication::translate("NovaGUIClass", "Max IPC Connections", 0, QApplication::UnicodeUTF8));
        CESaveButton->setText(QApplication::translate("NovaGUIClass", "Save", 0, QApplication::UnicodeUTF8));
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
        tabWidget->setTabText(tabWidget->indexOf(CETab), QApplication::translate("NovaGUIClass", "Classification Engine", 0, QApplication::UnicodeUTF8));
        DMIPLabel->setText(QApplication::translate("NovaGUIClass", "Doppelganger IP", 0, QApplication::UnicodeUTF8));
        DMInterfaceLabel->setText(QApplication::translate("NovaGUIClass", " Interface", 0, QApplication::UnicodeUTF8));
        DMEnabledLabel->setText(QApplication::translate("NovaGUIClass", "Enable Doppelganger", 0, QApplication::UnicodeUTF8));
        DMSaveButton->setText(QApplication::translate("NovaGUIClass", "Save", 0, QApplication::UnicodeUTF8));
        DMConfigLabel->setText(QApplication::translate("NovaGUIClass", "Config", 0, QApplication::UnicodeUTF8));
        DMLoadButton->setText(QApplication::translate("NovaGUIClass", "Load", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(DMTab), QApplication::translate("NovaGUIClass", "Doppelganger Module", 0, QApplication::UnicodeUTF8));
        HSConfigLabel->setText(QApplication::translate("NovaGUIClass", "Config", 0, QApplication::UnicodeUTF8));
        HSLoadButton->setText(QApplication::translate("NovaGUIClass", "Load", 0, QApplication::UnicodeUTF8));
        HSInterfaceLabel->setText(QApplication::translate("NovaGUIClass", " Interface", 0, QApplication::UnicodeUTF8));
        HSSaveButton->setText(QApplication::translate("NovaGUIClass", "Save", 0, QApplication::UnicodeUTF8));
        HSTimeoutLabel->setText(QApplication::translate("NovaGUIClass", "TCP Timeout", 0, QApplication::UnicodeUTF8));
        HSFreqLabel->setText(QApplication::translate("NovaGUIClass", "Timeout Frequency", 0, QApplication::UnicodeUTF8));
        HSHoneyDConfigLabel->setText(QApplication::translate("NovaGUIClass", " HoneyD Config File Path", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(HSTab), QApplication::translate("NovaGUIClass", "Haystack Monitor", 0, QApplication::UnicodeUTF8));
        LTMInterfaceLabel->setText(QApplication::translate("NovaGUIClass", " Interface", 0, QApplication::UnicodeUTF8));
        LTMTimeoutLabel->setText(QApplication::translate("NovaGUIClass", "TCP Timeout", 0, QApplication::UnicodeUTF8));
        LTMFreqLabel->setText(QApplication::translate("NovaGUIClass", "Timeout Frequency", 0, QApplication::UnicodeUTF8));
        LTMSaveButton->setText(QApplication::translate("NovaGUIClass", "Save", 0, QApplication::UnicodeUTF8));
        LTMConfigLabel->setText(QApplication::translate("NovaGUIClass", "Config", 0, QApplication::UnicodeUTF8));
        LTMLoadButton->setText(QApplication::translate("NovaGUIClass", "Load", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(LTMTab), QApplication::translate("NovaGUIClass", "Local Traffic Monitor", 0, QApplication::UnicodeUTF8));
        menuClassification_Engine->setTitle(QApplication::translate("NovaGUIClass", "Classification Engine", 0, QApplication::UnicodeUTF8));
        menuDoppelganger_Module->setTitle(QApplication::translate("NovaGUIClass", "Doppelganger Module", 0, QApplication::UnicodeUTF8));
        menuHaystack_Monitor->setTitle(QApplication::translate("NovaGUIClass", "Haystack Monitor", 0, QApplication::UnicodeUTF8));
        menuLocal_Traffic_Monitor->setTitle(QApplication::translate("NovaGUIClass", "Local Traffic Monitor", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class NovaGUIClass: public Ui_NovaGUIClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_NOVAGUI_H
