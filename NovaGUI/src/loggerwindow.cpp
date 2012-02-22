#include "loggerwindow.h"

LoggerWindow::LoggerWindow(QWidget *parent)
    : QMainWindow(parent)
{
	ui.setupUi(this);
	isBasic = true;
	ui.settingsBox->hide();
	ui.settingsBoxAdv->hide();
	settingsBoxShowing = false;
	setUpButtons();
	initializeLoggingWindow();
	//create listening thread
	//reference NovaGUI CEListen, etc...
}

void LoggerWindow::setUpButtons()
{

}

void LoggerWindow::initializeLoggingWindow()
{
	QFile file("/usr/share/nova/Logs/Nova.log");

	if(!file.open(QIODevice::ReadOnly))
	{
		ui.logDisplay->addTopLevelItem(logFileNotFound());
	}
	else
	{
		QTextStream in(&file);
		while(!in.atEnd())
		{
			QString pass = in.readLine();
			if(pass > 0)
				ui.logDisplay->addTopLevelItem(generateLogTabEntry(pass));
		}
	}
	file.close();
}

QTreeWidgetItem * LoggerWindow::generateLogTabEntry(QString line)
{
	QTreeWidgetItem * ret = new QTreeWidgetItem();
	QStringList parse = line.split(" ", QString::SkipEmptyParts);
	QString time(parse[0] + " " + parse[1]+ " " + parse[2]);
	ret->setText(0, time);
	QString level(parse[5]);
	ret->setText(1, level);
	QString message(parse[6]);

	for(int i = 7; i < parse.size(); i++)
	{
		message.append(" ");
		message.append(parse[i]);
	}

	ret->setText(2, message);
	if(!level.compare("INFO"))
	{
		for(int i = 0; i < 4; i++)
		{
			ret->setBackgroundColor(i, "skyblue");
		}
	}
	else if(!level.compare("NOTICE"))
	{
		for(int i = 0; i < 4; i++)
		{
			ret->setBackgroundColor(i, "darkseagreen");
		}
	}
	else if(!level.compare("WARNING"))
	{
		for(int i = 0; i < 4; i++)
		{
			ret->setBackgroundColor(i, "yellow");
		}
	}
	else if(!level.compare("ERROR"))
	{
		for(int i = 0; i < 4; i++)
		{
			ret->setBackgroundColor(i, "tomato");
		}
	}
	else if(!level.compare("CRITICAL"))
	{
		for(int i = 0; i < 4; i++)
		{
			ret->setBackgroundColor(i, "orangered");
		}
	}
	else if(!level.compare("ALERT"))
	{
		for(int i = 0; i < 4; i++)
		{
			ret->setBackgroundColor(i, "red");
		}
	}
	else if(!level.compare("EMERGENCY"))
	{
		for(int i = 0; i < 4; i++)
		{
			ret->setBackgroundColor(i, "black");
			ret->setForeground(i, QBrush("white"));
		}
	}
	return ret;
}

QTreeWidgetItem * LoggerWindow::logFileNotFound()
{
	QTime time = QTime::currentTime();
	QTreeWidgetItem * ret = new QTreeWidgetItem();
	ret->setText(0, time.toString());
	ret->setText(1, "ERROR");
	ret->setText(2, "Log File not Found");
	return ret;
}

void LoggerWindow::on_settingsButton_clicked()
{
	if(ui.logTabContainer->currentIndex() == 0 && !settingsBoxShowing)
	{
		ui.settingsBox->show();
		settingsBoxShowing = true;
	}
	else if(ui.logTabContainer->currentIndex() == 0 && settingsBoxShowing)
	{
		ui.settingsBox->hide();
		settingsBoxShowing = false;
	}
	else if(ui.logTabContainer->currentIndex() == 1 && !settingsBoxShowing)
	{
		ui.settingsBoxAdv->show();
		settingsBoxShowing = true;
	}
	else if(ui.logTabContainer->currentIndex() == 1 && settingsBoxShowing)
	{
		ui.settingsBoxAdv->hide();
		settingsBoxShowing = false;
	}
}

/*void LoggerWindow::on_saveButton_clicked()
{
	if(ui.logTabContainer->currentIndex() == 0)
	{

	}
	else if(ui.logTabContainer->currentIndex() == 1)
	{

	}
}*/

void LoggerWindow::on_clearButton_clicked()
{
	if(ui.logTabContainer->currentIndex() == 0)
	{

	}
	else if(ui.logTabContainer->currentIndex() == 1)
	{
		ui.logDisplay->clear();
	}
}

void LoggerWindow::on_logTabContainer_currentChanged()
{
	if(settingsBoxShowing)
	{
		settingsBoxShowing = false;
		ui.settingsBox->hide();
		ui.settingsBoxAdv->hide();
	}
}

void LoggerWindow::on_checkDebug_stateChanged(int state)
{
	if(state == Qt::Checked)
	{
		showSelected("DEBUG");
	}
	else if(state == Qt::Unchecked)
	{
		hideSelected("DEBUG");
	}
}

void LoggerWindow::on_checkInfo_stateChanged(int state)
{
	if(state == Qt::Checked)
	{
		showSelected("INFO");
	}
	else if(state == Qt::Unchecked)
	{
		hideSelected("INFO");
	}
}

void LoggerWindow::on_checkNotice_stateChanged(int state)
{
	if(state == Qt::Checked)
	{
		showSelected("NOTICE");
	}
	else if(state == Qt::Unchecked)
	{
		hideSelected("NOTICE");
	}
}

void LoggerWindow::on_checkWarning_stateChanged(int state)
{
	if(state == Qt::Checked)
	{
		showSelected("WARNING");
	}
	else if(state == Qt::Unchecked)
	{
		hideSelected("WARNING");
	}
}

void LoggerWindow::on_checkError_stateChanged(int state)
{
	if(state == Qt::Checked)
	{
		showSelected("ERROR");
	}
	else if(state == Qt::Unchecked)
	{
		hideSelected("ERROR");
	}
}

void LoggerWindow::on_checkCritical_stateChanged(int state)
{
	if(state == Qt::Checked)
	{
		showSelected("CRITICAL");
	}
	else if(state == Qt::Unchecked)
	{
		hideSelected("CRITICAL");
	}
}

void LoggerWindow::on_checkAlert_stateChanged(int state)
{
	if(state == Qt::Checked)
	{
		showSelected("ALERT");
	}
	else if(state == Qt::Unchecked)
	{
		hideSelected("ALERT");
	}
}

void LoggerWindow::on_checkEmergency_stateChanged(int state)
{
	if(state == Qt::Checked)
	{
		showSelected("EMERGENCY");
	}
	else if(state == Qt::Unchecked)
	{
		hideSelected("EMERGENCY");
	}
}

void LoggerWindow::hideSelected(QString level)
{
	QTreeWidgetItemIterator it(ui.logDisplay);
	while (*it)
	{
		if((*it)->text(1) == level)
		(*it)->setHidden(true);
		++it;
	}
}

void LoggerWindow::showSelected(QString level)
{
	QTreeWidgetItemIterator it(ui.logDisplay);
	while (*it)
	{
		if((*it)->text(1) == level)
		(*it)->setHidden(false);
		++it;
	}
}

void LoggerWindow::on_closeButton_clicked()
{
	this->close();
}

LoggerWindow::~LoggerWindow()
{

}
