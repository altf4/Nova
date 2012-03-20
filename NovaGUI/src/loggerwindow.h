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
    void InitializeLoggingWindow();
    void UpdateLoggingWindow();
    void HideSelected(QString level);
    void ShowSelected(QString level);
    void AdjustColumnWidths();
    void UpdateLogDisplay();
    bool ShouldBeVisible(QString level);
    QTreeWidgetItem * GenerateLogTabEntry(QString line);
    QTreeWidgetItem * LogFileNotFound();

private:
    Ui::LoggerWindowClass ui;
    bool m_isBasic;
    bool m_settingsBoxShowing;
    uint16_t m_showNumberOfLogs;
    QString m_viewLevels;

private Q_SLOTS:
	void on_settingsButton_clicked();
	void on_clearButton_clicked();
	void on_closeButton_clicked();
	void on_applyFilter_clicked();
	void on_linesBox_currentIndexChanged(const QString & text);
};

#endif // LOGGERWINDOW_H
