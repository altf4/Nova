//============================================================================
// Name        : loggerwindow.h
// Copyright   : DataSoft Corporation 2011-2012
//	Nova is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   Nova is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with Nova.  If not, see <http://www.gnu.org/licenses/>.
// Description : functions for the Log viewing window
//============================================================================

#ifndef LOGGERWINDOW_H
#define LOGGERWINDOW_H

#include <QtGui/QMainWindow>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QLineEdit>
#include <QRegExp>
#include <QValidator>
#include <stdint.h>
#include <iostream>
#include "Logger.h"
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
    void InitializeRecipientsList();
    void HideSelected(QString level);
    void ShowSelected(QString level);
    void AdjustColumnWidths();
    void UpdateLogDisplay();
    bool ShouldBeVisible(QString level);
    QTreeWidgetItem *GenerateLogTabEntry(QString line);
    QTreeWidgetItem *LogFileNotFound();

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
	//void on_applyFilter_clicked();
	void on_setEmailsButton_clicked();
	void on_appendEmailsButton_clicked();
	void on_removeEmailsButton_clicked();
	void on_clearEditButton_clicked();
	void on_emailsList_itemSelectionChanged();
	void on_checkDebug_stateChanged();
	void on_checkInfo_stateChanged();
	void on_checkNotice_stateChanged();
	void on_checkWarning_stateChanged();
	void on_checkError_stateChanged();
	void on_checkCritical_stateChanged();
	void on_checkAlert_stateChanged();
	void on_checkEmergency_stateChanged();
	void on_linesBox_currentIndexChanged(const QString & text);
};

#endif // LOGGERWINDOW_H
