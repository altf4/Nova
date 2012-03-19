#include "loggerwindow.h"

using namespace Nova;

//TODO: add support and GUI elements for viewing honeyd logs

LoggerWindow::LoggerWindow(QWidget *parent)
    : QMainWindow(parent)
{
	ui.setupUi(this);
	m_isBasic = true;
	ui.settingsBoxAdv->hide();
	m_settingsBoxShowing = false;
	m_showNumberOfLogs = 0;
	InitializeLoggingWindow();
	this->setFocus();
	//create listening thread
	//reference NovaGUI CEListen, etc...
}

void LoggerWindow::InitializeLoggingWindow()
{
	QFile file("/usr/share/nova/Logs/Nova.log");

	if(!file.open(QIODevice::ReadOnly))
	{
		ui.logDisplay->addTopLevelItem(LogFileNotFound());
	}
	else
	{
		QTextStream in(&file);
		while(!in.atEnd())
		{
			QString pass = in.readLine();
			if(pass > 0)
				ui.logDisplay->addTopLevelItem(GenerateLogTabEntry(pass));
		}

		AdjustColumnWidths();
	}
	//TODO: Add icons for checkboxes
	file.close();

	ui.applyFilter->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_F));
	ui.applyFilter->setToolTip("Shortcut: Shift+F");
	ui.closeButton->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
	ui.closeButton->setToolTip("Shortcut: Ctrl+Q");
	ui.settingsButton->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_S));
	ui.settingsButton->setToolTip("Shortcut: Shift+S");
}

void LoggerWindow::UpdateLogDisplay()
{
	QFile file("/usr/share/nova/Logs/Nova.log");
	uint16_t i = 0;

	if(!file.open(QIODevice::ReadOnly))
	{
		ui.logDisplay->addTopLevelItem(LogFileNotFound());
	}
	else
	{
		ui.logDisplay->clear();
		QTextStream in(&file);
		if(m_showNumberOfLogs > 0)
		{
			ui.showLabel->setText("Last");
			while(!in.atEnd() && i < m_showNumberOfLogs)
			{
				QString pass = in.readLine();
				if(pass > 0)
				{
					ui.logDisplay->addTopLevelItem(GenerateLogTabEntry(pass));
					i++;
				}
			}
			while(!in.atEnd())
			{
				QString pass = in.readLine();
				if(pass > 0)
				{
					ui.logDisplay->takeTopLevelItem(0);
					ui.logDisplay->addTopLevelItem(GenerateLogTabEntry(pass));
				}
			}
		}
		else
		{
			ui.showLabel->setText("Show");
			while(!in.atEnd())
			{
				QString pass = in.readLine();
				if(pass > 0)
				{
					ui.logDisplay->addTopLevelItem(GenerateLogTabEntry(pass));
				}
			}
		}

		AdjustColumnWidths();
	}
	file.close();
}

QTreeWidgetItem * LoggerWindow::GenerateLogTabEntry(QString line)
{
	QTreeWidgetItem * ret = new QTreeWidgetItem();
	QStringList parse = line.split(" ", QString::SkipEmptyParts);
	QString time(parse[0] + " " + parse[1]+ " " + parse[2]);
	ret->setText(0, time);
	QString process(parse[5].mid(0, (parse[5].size() - 1)));
	ret->setText(1, process);
	QString level(parse[6]);
	ret->setText(2, level);
	QString message(parse[7]);

	for(int i = 8; i < parse.size(); i++)
	{
		message.append(" ");
		message.append(parse[i]);
	}

	ret->setText(3, message);
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

QTreeWidgetItem * LoggerWindow::LogFileNotFound()
{
	QTime time = QTime::currentTime();
	QTreeWidgetItem * ret = new QTreeWidgetItem();
	ret->setText(0, time.toString());
	ret->setText(1, "ERROR");
	ret->setText(3, "Log File not Found");
	return ret;
}

void LoggerWindow::on_settingsButton_clicked()
{
	if(!m_settingsBoxShowing)
	{
		ui.settingsBoxAdv->show();
		m_settingsBoxShowing = true;
	}
	else if(m_settingsBoxShowing)
	{
		ui.settingsBoxAdv->hide();
		m_settingsBoxShowing = false;
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
	ui.logDisplay->clear();
}

void LoggerWindow::on_applyFilter_clicked()
{
 // ME for filter options enforced here, find a way to manage checkbox toggle permissions

	QTreeWidgetItemIterator it(ui.logDisplay);
	while(*it)
	{
		if(ShouldBeVisible((*it)->text(2), (*it)->text(1)))
		{
			(*it)->setHidden(false);
		}
		else
		{
			(*it)->setHidden(true);
		}
		++it;
	}
}

bool LoggerWindow::ShouldBeVisible(QString level, QString process)
{
	bool show = false;

	if(process == "ClassificationEngine" && ui.checkClassification->isChecked())
	{
		show = true;
	}

	else if(process == "LocalTrafficMonitor" && ui.checkLocalTraffic->isChecked())
	{
		show = true;
	}

	else if(process == "Haystack" && ui.checkHaystack->isChecked())
	{
		show = true;
	}

	else if(process == "DoppelgangerModule" && ui.checkDoppelganger->isChecked())
	{
		show = true;
	}

	else
	{
		return false;
	}

	if(level == "DEBUG" && ui.checkDebug->isChecked())
	{
		show = true;
	}

	else if(level == "INFO" && ui.checkInfo->isChecked())
	{
		show = true;
	}

	else if(level == "NOTICE" && ui.checkNotice->isChecked())
	{
		show = true;
	}

	else if(level == "WARNING" && ui.checkWarning->isChecked())
	{
		show = true;
	}

	else if(level == "ERROR" && ui.checkError->isChecked())
	{
		show = true;
	}

	else if(level == "CRITICAL" && ui.checkCritical->isChecked())
	{
		show = true;
	}

	else if(level == "ALERT" && ui.checkAlert->isChecked())
	{
		show = true;
	}

	else if(level == "EMERGENCY" && ui.checkEmergency->isChecked())
	{
		show = true;
	}

	else
	{
		show = false;
	}

	return show;
}

void LoggerWindow::on_linesBox_currentIndexChanged(const QString & text)
{
	m_showNumberOfLogs = text.toInt();
	UpdateLogDisplay();
}

void LoggerWindow::HideSelected(QString level, bool isProcess)
{
	QTreeWidgetItemIterator it(ui.logDisplay);
	if(isProcess)
	{
		while(*it)
		{
			if((*it)->text(1) == level)
			(*it)->setHidden(true);
			++it;
		}
	}
	else
	{
		while(*it)
		{
			if((*it)->text(2) == level && !((*it)->isHidden()))
			(*it)->setHidden(true);
			++it;
		}
	}
}

void LoggerWindow::ShowSelected(QString level, bool isProcess)
{
	QTreeWidgetItemIterator it(ui.logDisplay);
	if(isProcess)
	{
		while(*it)
		{
			if((*it)->text(1) == level)
			(*it)->setHidden(false);
			++it;
		}
	}
	else
	{
		while(*it)
		{
			if((*it)->text(2) == level && ((*it)->isHidden()))
			(*it)->setHidden(false);
			++it;
		}
	}
}

void LoggerWindow::AdjustColumnWidths()
{
	ui.logDisplay->resizeColumnToContents(0);
	ui.logDisplay->resizeColumnToContents(1);
	ui.logDisplay->resizeColumnToContents(2);
}

void LoggerWindow::on_closeButton_clicked()
{
	this->close();
}

LoggerWindow::~LoggerWindow()
{

}
