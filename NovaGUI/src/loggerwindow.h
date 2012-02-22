#ifndef LOGGERWINDOW_H
#define LOGGERWINDOW_H

#include <QtGui/QMainWindow>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include "ui_loggerwindow.h"

class LoggerWindow : public QMainWindow
{
    Q_OBJECT

public:
    LoggerWindow(QWidget *parent = 0);
    ~LoggerWindow();

private:
    void setUpButtons();
    void initializeLoggingWindow();
    void updateLoggingWindow();
    void hideSelected(QString level);
    void showSelected(QString level);
    QTreeWidgetItem * generateLogTabEntry(QString line);
    QTreeWidgetItem * logFileNotFound();

private:
    Ui::LoggerWindowClass ui;
    bool isBasic;
    bool settingsBoxShowing;

private Q_SLOTS:
	void on_settingsButton_clicked();
	void on_clearButton_clicked();
	void on_closeButton_clicked();
	void on_logTabContainer_currentChanged();
	void on_checkDebug_stateChanged(int state);
	void on_checkInfo_stateChanged(int state);
	void on_checkNotice_stateChanged(int state);
	void on_checkWarning_stateChanged(int state);
	void on_checkError_stateChanged(int state);
	void on_checkCritical_stateChanged(int state);
	void on_checkAlert_stateChanged(int state);
	void on_checkEmergency_stateChanged(int state);
};

#endif // LOGGERWINDOW_H
