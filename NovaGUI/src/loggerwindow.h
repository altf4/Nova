#ifndef LOGGERWINDOW_H
#define LOGGERWINDOW_H

#include <QtGui/QMainWindow>
#include <QFile>
#include <QTextStream>
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
    void on_pushButton_clicked();
    QTreeWidgetItem * generateLogTabEntry(QString line);

private:
    Ui::LoggerWindowClass ui;
};

#endif // LOGGERWINDOW_H
