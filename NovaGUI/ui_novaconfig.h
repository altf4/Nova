/********************************************************************************
** Form generated from reading UI file 'novaconfig.ui'
**
** Created: Mon Oct 31 11:34:59 2011
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
#include <QtGui/QCheckBox>
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
    QGroupBox *genGroupBox;
    QLabel *interfaceLabel;
    QGroupBox *saGroupBox;
    QLabel *saIPLabel;
    QLabel *saPortLabel;
    QTextEdit *saIPEdit;
    QTextEdit *saPortEdit;
    QTextEdit *interfaceEdit;
    QGroupBox *tcpGroupBox;
    QLabel *tcpFrequencyLabel;
    QLabel *tcpTimeoutLabel;
    QTextEdit *tcpFrequencyEdit;
    QTextEdit *tcpTimeoutEdit;
    QWidget *runPage;
    QCheckBox *pcapCheckBox;
    QCheckBox *trainingCheckBox;
    QGroupBox *pcapGroupBox;
    QTextEdit *pcapEdit;
    QLabel *pcapLabel;
    QPushButton *pcapButton;
    QCheckBox *liveCapCheckBox;
    QWidget *classificationPage;
    QGroupBox *ceGroupBox;
    QLabel *dataLabel;
    QGroupBox *ceConfigGroupBox;
    QLabel *ceIntensityLabel;
    QLabel *ceFrequencyLabel;
    QTextEdit *ceIntensityEdit;
    QTextEdit *ceFrequencyEdit;
    QLabel *ceThresholdLabel;
    QLabel *ceErrorLabel;
    QTextEdit *ceThresholdEdit;
    QTextEdit *ceErrorEdit;
    QTextEdit *dataEdit;
    QPushButton *dataButton;
    QWidget *doppelgangerPage;
    QGroupBox *dmGroupBox;
    QGroupBox *dmConfigGroupBox;
    QLabel *dmConfigLabel;
    QTextEdit *dmConfigEdit;
    QPushButton *dmConfigButton;
    QLabel *dmIPLabel;
    QTextEdit *dmIPEdit;
    QCheckBox *dmCheckBox;
    QWidget *haystackPage;
    QGroupBox *hsGroupBox;
    QGroupBox *hsConfigGroupBox;
    QLabel *hsConfigLabel;
    QTextEdit *hsConfigEdit;
    QPushButton *hsConfigButton;
    QFrame *buttonFrame;
    QPushButton *okButton;
    QPushButton *cancelButton;
    QPushButton *defaultsButton;
    QPushButton *applyButton;

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
        new QTreeWidgetItem(treeWidget);
        treeWidget->setObjectName(QString::fromUtf8("treeWidget"));
        treeWidget->setGeometry(QRect(0, 0, 150, 391));
        stackedWidget = new QStackedWidget(centralwidget);
        stackedWidget->setObjectName(QString::fromUtf8("stackedWidget"));
        stackedWidget->setGeometry(QRect(150, -20, 451, 411));
        generalPage = new QWidget();
        generalPage->setObjectName(QString::fromUtf8("generalPage"));
        genGroupBox = new QGroupBox(generalPage);
        genGroupBox->setObjectName(QString::fromUtf8("genGroupBox"));
        genGroupBox->setGeometry(QRect(10, 40, 431, 351));
        interfaceLabel = new QLabel(genGroupBox);
        interfaceLabel->setObjectName(QString::fromUtf8("interfaceLabel"));
        interfaceLabel->setGeometry(QRect(146, 50, 61, 20));
        saGroupBox = new QGroupBox(genGroupBox);
        saGroupBox->setObjectName(QString::fromUtf8("saGroupBox"));
        saGroupBox->setGeometry(QRect(10, 100, 401, 111));
        saIPLabel = new QLabel(saGroupBox);
        saIPLabel->setObjectName(QString::fromUtf8("saIPLabel"));
        saIPLabel->setGeometry(QRect(70, 30, 131, 20));
        saPortLabel = new QLabel(saGroupBox);
        saPortLabel->setObjectName(QString::fromUtf8("saPortLabel"));
        saPortLabel->setGeometry(QRect(160, 70, 41, 20));
        saIPEdit = new QTextEdit(saGroupBox);
        saIPEdit->setObjectName(QString::fromUtf8("saIPEdit"));
        saIPEdit->setGeometry(QRect(210, 30, 150, 20));
        QFont font;
        font.setKerning(true);
        saIPEdit->setFont(font);
        saPortEdit = new QTextEdit(saGroupBox);
        saPortEdit->setObjectName(QString::fromUtf8("saPortEdit"));
        saPortEdit->setGeometry(QRect(210, 70, 150, 20));
        interfaceEdit = new QTextEdit(genGroupBox);
        interfaceEdit->setObjectName(QString::fromUtf8("interfaceEdit"));
        interfaceEdit->setGeometry(QRect(220, 50, 150, 20));
        tcpGroupBox = new QGroupBox(genGroupBox);
        tcpGroupBox->setObjectName(QString::fromUtf8("tcpGroupBox"));
        tcpGroupBox->setGeometry(QRect(10, 230, 401, 111));
        tcpFrequencyLabel = new QLabel(tcpGroupBox);
        tcpFrequencyLabel->setObjectName(QString::fromUtf8("tcpFrequencyLabel"));
        tcpFrequencyLabel->setGeometry(QRect(30, 70, 171, 20));
        tcpTimeoutLabel = new QLabel(tcpGroupBox);
        tcpTimeoutLabel->setObjectName(QString::fromUtf8("tcpTimeoutLabel"));
        tcpTimeoutLabel->setGeometry(QRect(90, 30, 111, 20));
        tcpFrequencyEdit = new QTextEdit(tcpGroupBox);
        tcpFrequencyEdit->setObjectName(QString::fromUtf8("tcpFrequencyEdit"));
        tcpFrequencyEdit->setGeometry(QRect(210, 70, 150, 20));
        tcpTimeoutEdit = new QTextEdit(tcpGroupBox);
        tcpTimeoutEdit->setObjectName(QString::fromUtf8("tcpTimeoutEdit"));
        tcpTimeoutEdit->setGeometry(QRect(210, 30, 150, 20));
        stackedWidget->addWidget(generalPage);
        runPage = new QWidget();
        runPage->setObjectName(QString::fromUtf8("runPage"));
        pcapCheckBox = new QCheckBox(runPage);
        pcapCheckBox->setObjectName(QString::fromUtf8("pcapCheckBox"));
        pcapCheckBox->setGeometry(QRect(50, 110, 251, 20));
        pcapCheckBox->setLayoutDirection(Qt::LeftToRight);
        trainingCheckBox = new QCheckBox(runPage);
        trainingCheckBox->setObjectName(QString::fromUtf8("trainingCheckBox"));
        trainingCheckBox->setGeometry(QRect(50, 50, 251, 20));
        trainingCheckBox->setLayoutDirection(Qt::LeftToRight);
        pcapGroupBox = new QGroupBox(runPage);
        pcapGroupBox->setObjectName(QString::fromUtf8("pcapGroupBox"));
        pcapGroupBox->setEnabled(true);
        pcapGroupBox->setGeometry(QRect(20, 150, 411, 161));
        pcapEdit = new QTextEdit(pcapGroupBox);
        pcapEdit->setObjectName(QString::fromUtf8("pcapEdit"));
        pcapEdit->setEnabled(false);
        pcapEdit->setGeometry(QRect(27, 63, 301, 20));
        pcapLabel = new QLabel(pcapGroupBox);
        pcapLabel->setObjectName(QString::fromUtf8("pcapLabel"));
        pcapLabel->setGeometry(QRect(80, 30, 191, 20));
        pcapButton = new QPushButton(pcapGroupBox);
        pcapButton->setObjectName(QString::fromUtf8("pcapButton"));
        pcapButton->setGeometry(QRect(310, 60, 80, 26));
        liveCapCheckBox = new QCheckBox(pcapGroupBox);
        liveCapCheckBox->setObjectName(QString::fromUtf8("liveCapCheckBox"));
        liveCapCheckBox->setEnabled(true);
        liveCapCheckBox->setGeometry(QRect(30, 110, 301, 20));
        liveCapCheckBox->setLayoutDirection(Qt::LeftToRight);
        stackedWidget->addWidget(runPage);
        classificationPage = new QWidget();
        classificationPage->setObjectName(QString::fromUtf8("classificationPage"));
        ceGroupBox = new QGroupBox(classificationPage);
        ceGroupBox->setObjectName(QString::fromUtf8("ceGroupBox"));
        ceGroupBox->setGeometry(QRect(10, 40, 431, 351));
        dataLabel = new QLabel(ceGroupBox);
        dataLabel->setObjectName(QString::fromUtf8("dataLabel"));
        dataLabel->setGeometry(QRect(180, 30, 61, 20));
        ceConfigGroupBox = new QGroupBox(ceGroupBox);
        ceConfigGroupBox->setObjectName(QString::fromUtf8("ceConfigGroupBox"));
        ceConfigGroupBox->setGeometry(QRect(10, 110, 401, 231));
        ceIntensityLabel = new QLabel(ceConfigGroupBox);
        ceIntensityLabel->setObjectName(QString::fromUtf8("ceIntensityLabel"));
        ceIntensityLabel->setGeometry(QRect(119, 30, 71, 20));
        ceFrequencyLabel = new QLabel(ceConfigGroupBox);
        ceFrequencyLabel->setObjectName(QString::fromUtf8("ceFrequencyLabel"));
        ceFrequencyLabel->setGeometry(QRect(109, 80, 81, 20));
        ceIntensityEdit = new QTextEdit(ceConfigGroupBox);
        ceIntensityEdit->setObjectName(QString::fromUtf8("ceIntensityEdit"));
        ceIntensityEdit->setGeometry(QRect(190, 30, 150, 20));
        ceFrequencyEdit = new QTextEdit(ceConfigGroupBox);
        ceFrequencyEdit->setObjectName(QString::fromUtf8("ceFrequencyEdit"));
        ceFrequencyEdit->setGeometry(QRect(190, 80, 150, 20));
        ceThresholdLabel = new QLabel(ceConfigGroupBox);
        ceThresholdLabel->setObjectName(QString::fromUtf8("ceThresholdLabel"));
        ceThresholdLabel->setGeometry(QRect(109, 130, 71, 20));
        ceErrorLabel = new QLabel(ceConfigGroupBox);
        ceErrorLabel->setObjectName(QString::fromUtf8("ceErrorLabel"));
        ceErrorLabel->setGeometry(QRect(139, 180, 41, 20));
        ceThresholdEdit = new QTextEdit(ceConfigGroupBox);
        ceThresholdEdit->setObjectName(QString::fromUtf8("ceThresholdEdit"));
        ceThresholdEdit->setGeometry(QRect(190, 130, 150, 20));
        ceErrorEdit = new QTextEdit(ceConfigGroupBox);
        ceErrorEdit->setObjectName(QString::fromUtf8("ceErrorEdit"));
        ceErrorEdit->setGeometry(QRect(190, 180, 150, 20));
        dataEdit = new QTextEdit(ceGroupBox);
        dataEdit->setObjectName(QString::fromUtf8("dataEdit"));
        dataEdit->setEnabled(false);
        dataEdit->setGeometry(QRect(27, 60, 330, 20));
        dataButton = new QPushButton(ceGroupBox);
        dataButton->setObjectName(QString::fromUtf8("dataButton"));
        dataButton->setGeometry(QRect(320, 57, 70, 26));
        dataButton->setFlat(false);
        stackedWidget->addWidget(classificationPage);
        doppelgangerPage = new QWidget();
        doppelgangerPage->setObjectName(QString::fromUtf8("doppelgangerPage"));
        dmGroupBox = new QGroupBox(doppelgangerPage);
        dmGroupBox->setObjectName(QString::fromUtf8("dmGroupBox"));
        dmGroupBox->setGeometry(QRect(10, 40, 431, 351));
        dmConfigGroupBox = new QGroupBox(dmGroupBox);
        dmConfigGroupBox->setObjectName(QString::fromUtf8("dmConfigGroupBox"));
        dmConfigGroupBox->setGeometry(QRect(10, 100, 401, 241));
        dmConfigLabel = new QLabel(dmConfigGroupBox);
        dmConfigLabel->setObjectName(QString::fromUtf8("dmConfigLabel"));
        dmConfigLabel->setGeometry(QRect(130, 30, 121, 20));
        dmConfigEdit = new QTextEdit(dmConfigGroupBox);
        dmConfigEdit->setObjectName(QString::fromUtf8("dmConfigEdit"));
        dmConfigEdit->setEnabled(false);
        dmConfigEdit->setGeometry(QRect(20, 60, 320, 20));
        dmConfigButton = new QPushButton(dmConfigGroupBox);
        dmConfigButton->setObjectName(QString::fromUtf8("dmConfigButton"));
        dmConfigButton->setGeometry(QRect(320, 57, 70, 26));
        dmConfigButton->setFlat(false);
        dmIPLabel = new QLabel(dmConfigGroupBox);
        dmIPLabel->setObjectName(QString::fromUtf8("dmIPLabel"));
        dmIPLabel->setGeometry(QRect(69, 100, 81, 20));
        dmIPEdit = new QTextEdit(dmConfigGroupBox);
        dmIPEdit->setObjectName(QString::fromUtf8("dmIPEdit"));
        dmIPEdit->setGeometry(QRect(150, 100, 150, 20));
        dmCheckBox = new QCheckBox(dmGroupBox);
        dmCheckBox->setObjectName(QString::fromUtf8("dmCheckBox"));
        dmCheckBox->setGeometry(QRect(70, 50, 191, 20));
        dmCheckBox->setLayoutDirection(Qt::RightToLeft);
        stackedWidget->addWidget(doppelgangerPage);
        haystackPage = new QWidget();
        haystackPage->setObjectName(QString::fromUtf8("haystackPage"));
        hsGroupBox = new QGroupBox(haystackPage);
        hsGroupBox->setObjectName(QString::fromUtf8("hsGroupBox"));
        hsGroupBox->setGeometry(QRect(10, 40, 431, 351));
        hsConfigGroupBox = new QGroupBox(hsGroupBox);
        hsConfigGroupBox->setObjectName(QString::fromUtf8("hsConfigGroupBox"));
        hsConfigGroupBox->setGeometry(QRect(10, 80, 401, 261));
        hsConfigLabel = new QLabel(hsConfigGroupBox);
        hsConfigLabel->setObjectName(QString::fromUtf8("hsConfigLabel"));
        hsConfigLabel->setGeometry(QRect(100, 30, 131, 20));
        hsConfigEdit = new QTextEdit(hsConfigGroupBox);
        hsConfigEdit->setObjectName(QString::fromUtf8("hsConfigEdit"));
        hsConfigEdit->setEnabled(false);
        hsConfigEdit->setGeometry(QRect(30, 60, 320, 20));
        hsConfigButton = new QPushButton(hsConfigGroupBox);
        hsConfigButton->setObjectName(QString::fromUtf8("hsConfigButton"));
        hsConfigButton->setGeometry(QRect(320, 57, 70, 26));
        hsConfigButton->setFlat(false);
        stackedWidget->addWidget(haystackPage);
        buttonFrame = new QFrame(centralwidget);
        buttonFrame->setObjectName(QString::fromUtf8("buttonFrame"));
        buttonFrame->setGeometry(QRect(0, 390, 601, 51));
        buttonFrame->setFrameShape(QFrame::StyledPanel);
        buttonFrame->setFrameShadow(QFrame::Raised);
        okButton = new QPushButton(buttonFrame);
        okButton->setObjectName(QString::fromUtf8("okButton"));
        okButton->setGeometry(QRect(529, 10, 61, 25));
        cancelButton = new QPushButton(buttonFrame);
        cancelButton->setObjectName(QString::fromUtf8("cancelButton"));
        cancelButton->setGeometry(QRect(150, 10, 81, 25));
        defaultsButton = new QPushButton(buttonFrame);
        defaultsButton->setObjectName(QString::fromUtf8("defaultsButton"));
        defaultsButton->setGeometry(QRect(260, 10, 141, 25));
        applyButton = new QPushButton(buttonFrame);
        applyButton->setObjectName(QString::fromUtf8("applyButton"));
        applyButton->setGeometry(QRect(430, 10, 71, 25));
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
        ___qtreewidgetitem2->setText(0, QApplication::translate("NovaConfigClass", "Run Settings", 0, QApplication::UnicodeUTF8));
        QTreeWidgetItem *___qtreewidgetitem3 = treeWidget->topLevelItem(2);
        ___qtreewidgetitem3->setText(0, QApplication::translate("NovaConfigClass", "Classification", 0, QApplication::UnicodeUTF8));
        QTreeWidgetItem *___qtreewidgetitem4 = treeWidget->topLevelItem(3);
        ___qtreewidgetitem4->setText(0, QApplication::translate("NovaConfigClass", "Doppelganger", 0, QApplication::UnicodeUTF8));
        QTreeWidgetItem *___qtreewidgetitem5 = treeWidget->topLevelItem(4);
        ___qtreewidgetitem5->setText(0, QApplication::translate("NovaConfigClass", "Haystack", 0, QApplication::UnicodeUTF8));
        treeWidget->setSortingEnabled(__sortingEnabled);

        genGroupBox->setTitle(QApplication::translate("NovaConfigClass", "General Settings", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        interfaceLabel->setToolTip(QApplication::translate("NovaConfigClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<table border=\"0\" style=\"-qt-table-type: root; margin-top:4px; margin-bottom:4px; margin-left:4px; margin-right:4px;\">\n"
"<tr>\n"
"<td style=\"border: none;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">The name of the interface to run Nova on.</p></td></tr></table></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        interfaceLabel->setText(QApplication::translate("NovaConfigClass", "Interface", 0, QApplication::UnicodeUTF8));
        saGroupBox->setTitle(QApplication::translate("NovaConfigClass", "Silent Alarm", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        saIPLabel->setToolTip(QApplication::translate("NovaConfigClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<table border=\"0\" style=\"-qt-table-type: root; margin-top:4px; margin-bottom:4px; margin-left:4px; margin-right:4px;\">\n"
"<tr>\n"
"<td style=\"border: none;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">The IP address to send silent alarams from.</p></td></tr></table></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        saIPLabel->setText(QApplication::translate("NovaConfigClass", "Broadcast Address", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        saPortLabel->setToolTip(QApplication::translate("NovaConfigClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<table border=\"0\" style=\"-qt-table-type: root; margin-top:4px; margin-bottom:4px; margin-left:4px; margin-right:4px;\">\n"
"<tr>\n"
"<td style=\"border: none;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">The port to send silent alarams on.</p></td></tr></table></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        saPortLabel->setText(QApplication::translate("NovaConfigClass", "Port", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        saIPEdit->setToolTip(QApplication::translate("NovaConfigClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<table border=\"0\" style=\"-qt-table-type: root; margin-top:4px; margin-bottom:4px; margin-left:4px; margin-right:4px;\">\n"
"<tr>\n"
"<td style=\"border: none;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">The IP address to send silent alarams from.</p></td></tr></table></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        saPortEdit->setToolTip(QApplication::translate("NovaConfigClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<table border=\"0\" style=\"-qt-table-type: root; margin-top:4px; margin-bottom:4px; margin-left:4px; margin-right:4px;\">\n"
"<tr>\n"
"<td style=\"border: none;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">The port to send silent alarams on.</p></td></tr></table></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        interfaceEdit->setToolTip(QApplication::translate("NovaConfigClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<table border=\"0\" style=\"-qt-table-type: root; margin-top:4px; margin-bottom:4px; margin-left:4px; margin-right:4px;\">\n"
"<tr>\n"
"<td style=\"border: none;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">The name of the interface to run Nova on.</p></td></tr></table></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        tcpGroupBox->setTitle(QApplication::translate("NovaConfigClass", "TCP Timeout", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        tcpFrequencyLabel->setToolTip(QApplication::translate("NovaConfigClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<table border=\"0\" style=\"-qt-table-type: root; margin-top:4px; margin-bottom:4px; margin-left:4px; margin-right:4px;\">\n"
"<tr>\n"
"<td style=\"border: none;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">How long to wait (in seconds) between checking for timed out TCP sessions.</p></td></tr></table></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        tcpFrequencyLabel->setText(QApplication::translate("NovaConfigClass", "Timeout Check Frequency", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        tcpTimeoutLabel->setToolTip(QApplication::translate("NovaConfigClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<table border=\"0\" style=\"-qt-table-type: root; margin-top:4px; margin-bottom:4px; margin-left:4px; margin-right:4px;\">\n"
"<tr>\n"
"<td style=\"border: none;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">How long to wait (in seconds) before considering a TCP session as timed out.</p></td></tr></table></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        tcpTimeoutLabel->setText(QApplication::translate("NovaConfigClass", "Time till Timeout", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        tcpFrequencyEdit->setToolTip(QApplication::translate("NovaConfigClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<table border=\"0\" style=\"-qt-table-type: root; margin-top:4px; margin-bottom:4px; margin-left:4px; margin-right:4px;\">\n"
"<tr>\n"
"<td style=\"border: none;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">How long to wait (in seconds) between checking for timed out TCP sessions.</p></td></tr></table></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        tcpTimeoutEdit->setToolTip(QApplication::translate("NovaConfigClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<table border=\"0\" style=\"-qt-table-type: root; margin-top:4px; margin-bottom:4px; margin-left:4px; margin-right:4px;\">\n"
"<tr>\n"
"<td style=\"border: none;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">How long to wait (in seconds) before considering a TCP session as timed out.</p></td></tr></table></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        pcapCheckBox->setText(QApplication::translate("NovaConfigClass", "Read packets from file", 0, QApplication::UnicodeUTF8));
        trainingCheckBox->setText(QApplication::translate("NovaConfigClass", "Run in Training Mode", 0, QApplication::UnicodeUTF8));
        pcapGroupBox->setTitle(QApplication::translate("NovaConfigClass", "Packet File Options", 0, QApplication::UnicodeUTF8));
        pcapLabel->setText(QApplication::translate("NovaConfigClass", "Packet Capture File", 0, QApplication::UnicodeUTF8));
        pcapButton->setText(QApplication::translate("NovaConfigClass", "Browse", 0, QApplication::UnicodeUTF8));
        liveCapCheckBox->setText(QApplication::translate("NovaConfigClass", "Go to live capture after reading", 0, QApplication::UnicodeUTF8));
        ceGroupBox->setTitle(QApplication::translate("NovaConfigClass", "Classification Settings", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        dataLabel->setToolTip(QApplication::translate("NovaConfigClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<table border=\"0\" style=\"-qt-table-type: root; margin-top:4px; margin-bottom:4px; margin-left:4px; margin-right:4px;\">\n"
"<tr>\n"
"<td style=\"border: none;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">The Data File to read and write to when classifying suspects or training Nova.</p></td></tr></table></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        dataLabel->setText(QApplication::translate("NovaConfigClass", "Data File", 0, QApplication::UnicodeUTF8));
        ceConfigGroupBox->setTitle(QApplication::translate("NovaConfigClass", "Configuration", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        ceIntensityLabel->setToolTip(QApplication::translate("NovaConfigClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<table border=\"0\" style=\"-qt-table-type: root; margin-top:4px; margin-bottom:4px; margin-left:4px; margin-right:4px;\">\n"
"<tr>\n"
"<td style=\"border: none;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">How many similar points to find in classification. A higher value will increase accuracy but decrease performance.</p></td></tr></table></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        ceIntensityLabel->setText(QApplication::translate("NovaConfigClass", "Intensity", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        ceFrequencyLabel->setToolTip(QApplication::translate("NovaConfigClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<table border=\"0\" style=\"-qt-table-type: root; margin-top:4px; margin-bottom:4px; margin-left:4px; margin-right:4px;\">\n"
"<tr>\n"
"<td style=\"border: none;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">How long to wait (in seconds) before re-classifying suspects with new information.</p></td></tr></table></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        ceFrequencyLabel->setText(QApplication::translate("NovaConfigClass", "Frequency", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        ceIntensityEdit->setToolTip(QApplication::translate("NovaConfigClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<table border=\"0\" style=\"-qt-table-type: root; margin-top:4px; margin-bottom:4px; margin-left:4px; margin-right:4px;\">\n"
"<tr>\n"
"<td style=\"border: none;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">How many similar points to find in classification. A higher value will increase accuracy but decrease performance.</p></td></tr></table></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        ceFrequencyEdit->setToolTip(QApplication::translate("NovaConfigClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<table border=\"0\" style=\"-qt-table-type: root; margin-top:4px; margin-bottom:4px; margin-left:4px; margin-right:4px;\">\n"
"<tr>\n"
"<td style=\"border: none;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">How long to wait (in seconds) before re-classifying suspects with new information.</p></td></tr></table></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        ceThresholdLabel->setToolTip(QApplication::translate("NovaConfigClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<table border=\"0\" style=\"-qt-table-type: root; margin-top:4px; margin-bottom:4px; margin-left:4px; margin-right:4px;\">\n"
"<tr>\n"
"<td style=\"border: none;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">When to consider a suspect hostile. Range is between 0 and 1. 0 is least hostile, 1 is most hostile. Default is 0.5.</p></td></tr></table></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        ceThresholdLabel->setText(QApplication::translate("NovaConfigClass", "Threshold", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        ceErrorLabel->setToolTip(QApplication::translate("NovaConfigClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<table border=\"0\" style=\"-qt-table-type: root; margin-top:4px; margin-bottom:4px; margin-left:4px; margin-right:4px;\">\n"
"<tr>\n"
"<td style=\"border: none;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">A higher error may improve performance but will decrease accuracy. Default is 0.</p></td></tr></table></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        ceErrorLabel->setText(QApplication::translate("NovaConfigClass", "Error", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        ceThresholdEdit->setToolTip(QApplication::translate("NovaConfigClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<table border=\"0\" style=\"-qt-table-type: root; margin-top:4px; margin-bottom:4px; margin-left:4px; margin-right:4px;\">\n"
"<tr>\n"
"<td style=\"border: none;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">When to consider a suspect hostile. Range is between 0 and 1. 0 is least hostile, 1 is most hostile. Default is 0.5.</p></td></tr></table></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        ceErrorEdit->setToolTip(QApplication::translate("NovaConfigClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<table border=\"0\" style=\"-qt-table-type: root; margin-top:4px; margin-bottom:4px; margin-left:4px; margin-right:4px;\">\n"
"<tr>\n"
"<td style=\"border: none;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">A higher error may improve performance but will decrease accuracy. Default is 0.</p></td></tr></table></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        dataEdit->setToolTip(QApplication::translate("NovaConfigClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<table border=\"0\" style=\"-qt-table-type: root; margin-top:4px; margin-bottom:4px; margin-left:4px; margin-right:4px;\">\n"
"<tr>\n"
"<td style=\"border: none;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">The Data File to read and write to when classifying suspects or training Nova.</p></td></tr></table></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        dataButton->setToolTip(QApplication::translate("NovaConfigClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<table border=\"0\" style=\"-qt-table-type: root; margin-top:4px; margin-bottom:4px; margin-left:4px; margin-right:4px;\">\n"
"<tr>\n"
"<td style=\"border: none;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">Browse the file system for a data file.</p></td></tr></table></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        dataButton->setText(QApplication::translate("NovaConfigClass", "Browse", 0, QApplication::UnicodeUTF8));
        dmGroupBox->setTitle(QApplication::translate("NovaConfigClass", "Doppelganger Settings", 0, QApplication::UnicodeUTF8));
        dmConfigGroupBox->setTitle(QApplication::translate("NovaConfigClass", "Configure (HoneyD Todo)", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        dmConfigLabel->setToolTip(QApplication::translate("NovaConfigClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<table border=\"0\" style=\"-qt-table-type: root; margin-top:4px; margin-bottom:4px; margin-left:4px; margin-right:4px;\">\n"
"<tr>\n"
"<td style=\"border: none;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">The file to read the Doppelganger configuration from.</p></td></tr></table></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        dmConfigLabel->setText(QApplication::translate("NovaConfigClass", "Configuration File", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        dmConfigEdit->setToolTip(QApplication::translate("NovaConfigClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<table border=\"0\" style=\"-qt-table-type: root; margin-top:4px; margin-bottom:4px; margin-left:4px; margin-right:4px;\">\n"
"<tr>\n"
"<td style=\"border: none;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">The file to read the Doppelganger configuration from.</p></td></tr></table></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        dmConfigButton->setToolTip(QApplication::translate("NovaConfigClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<table border=\"0\" style=\"-qt-table-type: root; margin-top:4px; margin-bottom:4px; margin-left:4px; margin-right:4px;\">\n"
"<tr>\n"
"<td style=\"border: none;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">Browse the file system for a configuration file.</p></td></tr></table></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        dmConfigButton->setText(QApplication::translate("NovaConfigClass", "Browse", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        dmIPLabel->setToolTip(QApplication::translate("NovaConfigClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<table border=\"0\" style=\"-qt-table-type: root; margin-top:4px; margin-bottom:4px; margin-left:4px; margin-right:4px;\">\n"
"<tr>\n"
"<td style=\"border: none;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">The IP address the Doppelganger is assigned,  Must be available.</p></td></tr></table></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        dmIPLabel->setText(QApplication::translate("NovaConfigClass", "IP Address", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        dmIPEdit->setToolTip(QApplication::translate("NovaConfigClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<table border=\"0\" style=\"-qt-table-type: root; margin-top:4px; margin-bottom:4px; margin-left:4px; margin-right:4px;\">\n"
"<tr>\n"
"<td style=\"border: none;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">The IP address the Doppelganger is assigned,  Must be available.</p></td></tr></table></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        dmCheckBox->setToolTip(QApplication::translate("NovaConfigClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<table border=\"0\" style=\"-qt-table-type: root; margin-top:4px; margin-bottom:4px; margin-left:4px; margin-right:4px;\">\n"
"<tr>\n"
"<td style=\"border: none;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">Turning off the Doppelganger will mean that this host will be visible at all times even if the suspect is classified as hostile.</p></td></tr></table></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        dmCheckBox->setText(QApplication::translate("NovaConfigClass", "Enable Doppelganger", 0, QApplication::UnicodeUTF8));
        hsGroupBox->setTitle(QApplication::translate("NovaConfigClass", "Haystack Settings", 0, QApplication::UnicodeUTF8));
        hsConfigGroupBox->setTitle(QApplication::translate("NovaConfigClass", "Configure (HoneyD Todo)", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        hsConfigLabel->setToolTip(QApplication::translate("NovaConfigClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<table border=\"0\" style=\"-qt-table-type: root; margin-top:4px; margin-bottom:4px; margin-left:4px; margin-right:4px;\">\n"
"<tr>\n"
"<td style=\"border: none;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">The file to read the Haystack configuration from.</p></td></tr></table></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        hsConfigLabel->setText(QApplication::translate("NovaConfigClass", "Configuration File", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        hsConfigEdit->setToolTip(QApplication::translate("NovaConfigClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<table border=\"0\" style=\"-qt-table-type: root; margin-top:4px; margin-bottom:4px; margin-left:4px; margin-right:4px;\">\n"
"<tr>\n"
"<td style=\"border: none;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">The file to read the Haystack configuration from.</p></td></tr></table></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        hsConfigButton->setToolTip(QApplication::translate("NovaConfigClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<table border=\"0\" style=\"-qt-table-type: root; margin-top:4px; margin-bottom:4px; margin-left:4px; margin-right:4px;\">\n"
"<tr>\n"
"<td style=\"border: none;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">Browse the file system for a configuration file.</p></td></tr></table></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        hsConfigButton->setText(QApplication::translate("NovaConfigClass", "Browse", 0, QApplication::UnicodeUTF8));
        okButton->setText(QApplication::translate("NovaConfigClass", "Ok", 0, QApplication::UnicodeUTF8));
        cancelButton->setText(QApplication::translate("NovaConfigClass", "Cancel", 0, QApplication::UnicodeUTF8));
        defaultsButton->setText(QApplication::translate("NovaConfigClass", "Restore Defaults", 0, QApplication::UnicodeUTF8));
        applyButton->setText(QApplication::translate("NovaConfigClass", "Apply", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class NovaConfigClass: public Ui_NovaConfigClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_NOVACONFIG_H
