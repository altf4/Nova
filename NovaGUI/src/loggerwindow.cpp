#include "loggerwindow.h"

LoggerWindow::LoggerWindow(QWidget *parent)
    : QMainWindow(parent)
{
	ui.setupUi(this);
	setUpButtons();
	initializeLoggingWindow();
}

void LoggerWindow::setUpButtons()
{
    connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(ui.pushButton, SIGNAL(clicked()), this, SLOT(click()));
}

void LoggerWindow::initializeLoggingWindow()
{
	QFile file("/usr/share/nova/Logs/Nova.log");

	if(!file.open(QIODevice::ReadOnly))
	{

	}
	else
	{
		QTextStream in(&file);
		while(!file.atEnd())
		{
			QString pass = in.readLine();
			if(pass > 0)
				ui.treeWidget_2->addTopLevelItem(generateLogTabEntry(pass));
		}
	}
	QString pass("doesn't matter");
	ui.treeWidget_2->addTopLevelItem(generateLogTabEntry(pass));
}

QTreeWidgetItem * LoggerWindow::generateLogTabEntry(QString line)
{
	QTreeWidgetItem * ret = new QTreeWidgetItem();
	ret->setText(0, "Time goes here");
	ret->setText(1, "Level goes here");
	ret->setText(2, "Parent Process goes here");
	ret->setText(3, line);
	return ret;
}

void LoggerWindow::on_pushButton_clicked()
{

}

LoggerWindow::~LoggerWindow()
{

}
