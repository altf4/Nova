/********************************************************************************
** Form generated from reading UI file 'novagui.ui'
**
** Created: Fri Oct 14 18:26:36 2011
**      by: Qt User Interface Compiler version 4.7.4
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_NOVAGUI_H
#define UI_NOVAGUI_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QListWidget>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
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
    QWidget *CETab;
    QPushButton *CEUpdate;
    QListWidget *benignList;
    QLabel *bListLabel;
    QLabel *hListLabel;
    QListWidget *hostileList;
    QWidget *DMTab;
    QLabel *DMLabel;
    QWidget *HSTab;
    QLabel *HSLabel;
    QWidget *LTMTab;
    QLabel *LTMLabel;
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
        tabWidget->setGeometry(QRect(-8, -1, 811, 561));
        CETab = new QWidget();
        CETab->setObjectName(QString::fromUtf8("CETab"));
        CEUpdate = new QPushButton(CETab);
        CEUpdate->setObjectName(QString::fromUtf8("CEUpdate"));
        CEUpdate->setGeometry(QRect(690, 480, 111, 41));
        benignList = new QListWidget(CETab);
        benignList->setObjectName(QString::fromUtf8("benignList"));
        benignList->setGeometry(QRect(10, 50, 301, 471));
        bListLabel = new QLabel(CETab);
        bListLabel->setObjectName(QString::fromUtf8("bListLabel"));
        bListLabel->setGeometry(QRect(90, 20, 111, 31));
        hListLabel = new QLabel(CETab);
        hListLabel->setObjectName(QString::fromUtf8("hListLabel"));
        hListLabel->setGeometry(QRect(440, 20, 111, 31));
        hostileList = new QListWidget(CETab);
        hostileList->setObjectName(QString::fromUtf8("hostileList"));
        hostileList->setGeometry(QRect(360, 50, 301, 471));
        tabWidget->addTab(CETab, QString());
        DMTab = new QWidget();
        DMTab->setObjectName(QString::fromUtf8("DMTab"));
        DMLabel = new QLabel(DMTab);
        DMLabel->setObjectName(QString::fromUtf8("DMLabel"));
        DMLabel->setGeometry(QRect(30, 10, 761, 521));
        DMLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
        DMLabel->setWordWrap(true);
        tabWidget->addTab(DMTab, QString());
        HSTab = new QWidget();
        HSTab->setObjectName(QString::fromUtf8("HSTab"));
        HSLabel = new QLabel(HSTab);
        HSLabel->setObjectName(QString::fromUtf8("HSLabel"));
        HSLabel->setGeometry(QRect(30, 10, 761, 521));
        HSLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
        HSLabel->setWordWrap(true);
        tabWidget->addTab(HSTab, QString());
        LTMTab = new QWidget();
        LTMTab->setObjectName(QString::fromUtf8("LTMTab"));
        LTMLabel = new QLabel(LTMTab);
        LTMLabel->setObjectName(QString::fromUtf8("LTMLabel"));
        LTMLabel->setGeometry(QRect(30, 10, 761, 521));
        LTMLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
        LTMLabel->setWordWrap(true);
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

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(NovaGUIClass);
    } // setupUi

    void retranslateUi(QMainWindow *NovaGUIClass)
    {
        NovaGUIClass->setWindowTitle(QApplication::translate("NovaGUIClass", "MainWindow", 0, QApplication::UnicodeUTF8));
        actionTodo->setText(QApplication::translate("NovaGUIClass", "Todo", 0, QApplication::UnicodeUTF8));
        actionTodo_2->setText(QApplication::translate("NovaGUIClass", "Todo", 0, QApplication::UnicodeUTF8));
        actionTodo_3->setText(QApplication::translate("NovaGUIClass", "Todo", 0, QApplication::UnicodeUTF8));
        actionTodo_4->setText(QApplication::translate("NovaGUIClass", "Todo", 0, QApplication::UnicodeUTF8));
        CEUpdate->setText(QApplication::translate("NovaGUIClass", "Update", 0, QApplication::UnicodeUTF8));
        bListLabel->setText(QApplication::translate("NovaGUIClass", "Benign Suspects", 0, QApplication::UnicodeUTF8));
        hListLabel->setText(QApplication::translate("NovaGUIClass", " Hostile Suspects", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(CETab), QApplication::translate("NovaGUIClass", "Classification Engine", 0, QApplication::UnicodeUTF8));
        DMLabel->setText(QApplication::translate("NovaGUIClass", "stuff", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(DMTab), QApplication::translate("NovaGUIClass", "Doppelganger Module", 0, QApplication::UnicodeUTF8));
        HSLabel->setText(QApplication::translate("NovaGUIClass", "stuff", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(HSTab), QApplication::translate("NovaGUIClass", "Haystack Monitor", 0, QApplication::UnicodeUTF8));
        LTMLabel->setText(QApplication::translate("NovaGUIClass", "stuff", 0, QApplication::UnicodeUTF8));
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
