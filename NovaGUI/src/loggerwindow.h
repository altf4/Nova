#ifndef LOGGERWINDOW_H
#define LOGGERWINDOW_H

#include <QtGui/QMainWindow>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <stdint.h>
#include "ui_loggerwindow.h"
#include "NovaUtil.h"

class LoggerWindow : public QMainWindow
{
    Q_OBJECT

public:
    LoggerWindow(QWidget *parent = 0);
    ~LoggerWindow();

private:
    void initializeLoggingWindow();
    void updateLoggingWindow();
    void hideSelected(QString level, bool isProcess);
    void showSelected(QString level, bool isProcess);
    void adjustColumnWidths();
    void updateLogDisplay();
    bool shouldBeVisible(QString level, QString process);
    QTreeWidgetItem * generateLogTabEntry(QString line);
    QTreeWidgetItem * logFileNotFound();

private:
    Ui::LoggerWindowClass ui;
    bool isBasic;
    bool settingsBoxShowing;
    uint16_t showNumberOfLogs;
    QString viewLevels;

private Q_SLOTS:
	void on_settingsButton_clicked();
	void on_clearButton_clicked();
	void on_closeButton_clicked();
	void on_applyFilter_clicked();
	void on_linesBox_currentIndexChanged(const QString & text);
};

#endif // LOGGERWINDOW_H
