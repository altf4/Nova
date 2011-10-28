/********************************************************************************
** Form generated from reading UI file 'novaconfig.ui'
**
** Created: Fri Oct 28 09:38:50 2011
**      by: Qt User Interface Compiler version 4.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_NOVACONFIG_H
#define UI_NOVACONFIG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QFrame>
#include <QtGui/QGroupBox>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QMainWindow>
#include <QtGui/QPushButton>
#include <QtGui/QStackedWidget>
#include <QtGui/QTextEdit>
#include <QtGui/QTreeWidget>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_NovaConfigClass
{
public:
    QWidget *centralwidget;
    QTreeWidget *treeWidget;
    QStackedWidget *stackedWidget;
    QWidget *generalPage;
    QGroupBox *groupBox;
    QLabel *label;
    QGroupBox *groupBox_2;
    QLabel *label_2;
    QLabel *label_3;
    QTextEdit *textEdit;
    QTextEdit *textEdit_2;
    QTextEdit *textEdit_3;
    QGroupBox *groupBox_3;
    QLabel *label_4;
    QLabel *label_5;
    QTextEdit *textEdit_4;
    QTextEdit *textEdit_5;
    QWidget *classificationPage;
    QWidget *doppelgangerPage;
    QWidget *haystackPage;
    QFrame *frame;
    QPushButton *pushButton;
    QPushButton *pushButton_2;
    QPushButton *pushButton_3;
    QPushButton *pushButton_4;

    void setupUi(QMainWindow *NovaConfigClass)
    {
        if (NovaConfigClass->objectName().isEmpty())
            NovaConfigClass->setObjectName(QString::fromUtf8("NovaConfigClass"));
        NovaConfigClass->setWindowModality(Qt::NonModal);
        NovaConfigClass->resize(600, 441);
        centralwidget = new QWidget(NovaConfigClass);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        treeWidget = new QTreeWidget(centralwidget);
        new QTreeWidgetItem(treeWidget);
        new QTreeWidgetItem(treeWidget);
        new QTreeWidgetItem(treeWidget);
        new QTreeWidgetItem(treeWidget);
        treeWidget->setObjectName(QString::fromUtf8("treeWidget"));
        treeWidget->setGeometry(QRect(0, 0, 150, 391));
        stackedWidget = new QStackedWidget(centralwidget);
        stackedWidget->setObjectName(QString::fromUtf8("stackedWidget"));
        stackedWidget->setGeometry(QRect(149, -11, 451, 401));
        generalPage = new QWidget();
        generalPage->setObjectName(QString::fromUtf8("generalPage"));
        groupBox = new QGroupBox(generalPage);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        groupBox->setGeometry(QRect(10, 20, 431, 321));
        label = new QLabel(groupBox);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(150, 40, 57, 20));
        groupBox_2 = new QGroupBox(groupBox);
        groupBox_2->setObjectName(QString::fromUtf8("groupBox_2"));
        groupBox_2->setGeometry(QRect(10, 90, 401, 91));
        label_2 = new QLabel(groupBox_2);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(80, 20, 121, 20));
        label_3 = new QLabel(groupBox_2);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setGeometry(QRect(170, 60, 31, 20));
        textEdit = new QTextEdit(groupBox_2);
        textEdit->setObjectName(QString::fromUtf8("textEdit"));
        textEdit->setGeometry(QRect(210, 20, 150, 20));
        textEdit_2 = new QTextEdit(groupBox_2);
        textEdit_2->setObjectName(QString::fromUtf8("textEdit_2"));
        textEdit_2->setGeometry(QRect(210, 60, 150, 20));
        textEdit_3 = new QTextEdit(groupBox);
        textEdit_3->setObjectName(QString::fromUtf8("textEdit_3"));
        textEdit_3->setGeometry(QRect(220, 40, 150, 20));
        groupBox_3 = new QGroupBox(groupBox);
        groupBox_3->setObjectName(QString::fromUtf8("groupBox_3"));
        groupBox_3->setGeometry(QRect(10, 210, 401, 101));
        label_4 = new QLabel(groupBox_3);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setGeometry(QRect(40, 70, 161, 20));
        label_5 = new QLabel(groupBox_3);
        label_5->setObjectName(QString::fromUtf8("label_5"));
        label_5->setGeometry(QRect(100, 30, 101, 20));
        textEdit_4 = new QTextEdit(groupBox_3);
        textEdit_4->setObjectName(QString::fromUtf8("textEdit_4"));
        textEdit_4->setGeometry(QRect(210, 70, 150, 20));
        textEdit_5 = new QTextEdit(groupBox_3);
        textEdit_5->setObjectName(QString::fromUtf8("textEdit_5"));
        textEdit_5->setGeometry(QRect(210, 30, 150, 20));
        stackedWidget->addWidget(generalPage);
        classificationPage = new QWidget();
        classificationPage->setObjectName(QString::fromUtf8("classificationPage"));
        stackedWidget->addWidget(classificationPage);
        doppelgangerPage = new QWidget();
        doppelgangerPage->setObjectName(QString::fromUtf8("doppelgangerPage"));
        stackedWidget->addWidget(doppelgangerPage);
        haystackPage = new QWidget();
        haystackPage->setObjectName(QString::fromUtf8("haystackPage"));
        stackedWidget->addWidget(haystackPage);
        frame = new QFrame(centralwidget);
        frame->setObjectName(QString::fromUtf8("frame"));
        frame->setGeometry(QRect(0, 390, 601, 51));
        frame->setFrameShape(QFrame::StyledPanel);
        frame->setFrameShadow(QFrame::Raised);
        pushButton = new QPushButton(frame);
        pushButton->setObjectName(QString::fromUtf8("pushButton"));
        pushButton->setGeometry(QRect(510, 10, 80, 25));
        pushButton_2 = new QPushButton(frame);
        pushButton_2->setObjectName(QString::fromUtf8("pushButton_2"));
        pushButton_2->setGeometry(QRect(150, 10, 80, 25));
        pushButton_3 = new QPushButton(frame);
        pushButton_3->setObjectName(QString::fromUtf8("pushButton_3"));
        pushButton_3->setGeometry(QRect(260, 10, 111, 25));
        pushButton_4 = new QPushButton(frame);
        pushButton_4->setObjectName(QString::fromUtf8("pushButton_4"));
        pushButton_4->setGeometry(QRect(400, 10, 80, 25));
        NovaConfigClass->setCentralWidget(centralwidget);

        retranslateUi(NovaConfigClass);

        stackedWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(NovaConfigClass);
    } // setupUi

    void retranslateUi(QMainWindow *NovaConfigClass)
    {
        NovaConfigClass->setWindowTitle(QApplication::translate("NovaConfigClass", "Preferences", 0, QApplication::UnicodeUTF8));
        QTreeWidgetItem *___qtreewidgetitem = treeWidget->headerItem();
        ___qtreewidgetitem->setText(0, QApplication::translate("NovaConfigClass", "Options", 0, QApplication::UnicodeUTF8));

        const bool __sortingEnabled = treeWidget->isSortingEnabled();
        treeWidget->setSortingEnabled(false);
        QTreeWidgetItem *___qtreewidgetitem1 = treeWidget->topLevelItem(0);
        ___qtreewidgetitem1->setText(0, QApplication::translate("NovaConfigClass", "General", 0, QApplication::UnicodeUTF8));
        QTreeWidgetItem *___qtreewidgetitem2 = treeWidget->topLevelItem(1);
        ___qtreewidgetitem2->setText(0, QApplication::translate("NovaConfigClass", "Classification", 0, QApplication::UnicodeUTF8));
        QTreeWidgetItem *___qtreewidgetitem3 = treeWidget->topLevelItem(2);
        ___qtreewidgetitem3->setText(0, QApplication::translate("NovaConfigClass", "Doppelganger", 0, QApplication::UnicodeUTF8));
        QTreeWidgetItem *___qtreewidgetitem4 = treeWidget->topLevelItem(3);
        ___qtreewidgetitem4->setText(0, QApplication::translate("NovaConfigClass", "Haystack", 0, QApplication::UnicodeUTF8));
        treeWidget->setSortingEnabled(__sortingEnabled);

        groupBox->setTitle(QApplication::translate("NovaConfigClass", "General Settings", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("NovaConfigClass", "Interface", 0, QApplication::UnicodeUTF8));
        groupBox_2->setTitle(QApplication::translate("NovaConfigClass", "Silent Alarm", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("NovaConfigClass", "Broadcast Address", 0, QApplication::UnicodeUTF8));
        label_3->setText(QApplication::translate("NovaConfigClass", "Port", 0, QApplication::UnicodeUTF8));
        groupBox_3->setTitle(QApplication::translate("NovaConfigClass", "TCP Timeout", 0, QApplication::UnicodeUTF8));
        label_4->setText(QApplication::translate("NovaConfigClass", "Timeout Check Frequency", 0, QApplication::UnicodeUTF8));
        label_5->setText(QApplication::translate("NovaConfigClass", "Time till Timeout", 0, QApplication::UnicodeUTF8));
        pushButton->setText(QApplication::translate("NovaConfigClass", "Ok", 0, QApplication::UnicodeUTF8));
        pushButton_2->setText(QApplication::translate("NovaConfigClass", "Cancel", 0, QApplication::UnicodeUTF8));
        pushButton_3->setText(QApplication::translate("NovaConfigClass", "Restore Defaults", 0, QApplication::UnicodeUTF8));
        pushButton_4->setText(QApplication::translate("NovaConfigClass", "Apply", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class NovaConfigClass: public Ui_NovaConfigClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_NOVACONFIG_H
