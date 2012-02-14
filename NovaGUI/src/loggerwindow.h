#ifndef LOGGERWINDOW_H
#define LOGGERWINDOW_H

#include <QtGui/QMainWindow>
#include "ui_loggerwindow.h"

class LoggerWindow : public QMainWindow
{
    Q_OBJECT

public:
    LoggerWindow(QWidget *parent = 0);
    ~LoggerWindow();

private:
    Ui::LoggerWindowClass ui;
};

#endif // LOGGERWINDOW_H
