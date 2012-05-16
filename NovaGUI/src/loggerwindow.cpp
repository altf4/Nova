//============================================================================
// Name        : novagui.cpp
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
// Description : Class to show the Log viewing window and handle the GUI logic
//============================================================================

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
	InitializeRecipientsList();
	this->setFocus();
	//create listening thread
	//reference NovaGUI CEListen, etc...
}

void LoggerWindow::InitializeRecipientsList()
{
	//QRegExp rx("\\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\\.[A-Z]{2,4}\\b\\b\\,[A-Z0-9._%+-]+@[A-Z0-9.-]+\\.[A-Z]{2,4}\\b");
	//QValidator *validator = new QRegExpValidator(rx, this);
	bool empty = false;
	ui.setEmailsButton->setDisabled(true);
	ui.appendEmailsButton->setDisabled(true);
	ui.removeEmailsButton->setDisabled(true);
	ui.emailEdit->setPlaceholderText("Enter a comma separated list of emails here...");
	//ui.emailEdit->setValidator(validator);

	QListWidgetItem *item = new QListWidgetItem("Emails file is empty, or does not exist...");
	item->setFlags(Qt::NoItemFlags);
	QFile file("/usr/share/nova/emails");

	if(!file.open(QIODevice::ReadOnly))
	{
		ui.emailsList->addItem(item);
		ui.setEmailsButton->setDisabled(false);
	}
	else
	{
		QTextStream in(&file);

		do
		{
			QString pass = in.readLine();

			if(pass == 0)
			{
				ui.emailsList->addItem(item);
				ui.setEmailsButton->setDisabled(false);
				empty = true;
			}
			else if(pass > 0)
			{
				ui.emailsList->addItem(pass);
			}
		}while(!in.atEnd() && !empty);
	}
	if(!empty)
	{
		ui.appendEmailsButton->setDisabled(false);
		ui.removeEmailsButton->setDisabled(false);
		ui.removeEmailsButton->setText("Clear Emails");
	}

	file.close();
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

	file.close();

	ui.checkDebug->setChecked(false);
	ui.checkInfo->setChecked(false);
	ui.checkNotice->setChecked(false);

	HideSelected("DEBUG");
	HideSelected("INFO");
	HideSelected("NOTICE");

	ui.logDisplay->sortItems(0, Qt::DescendingOrder);

	//ui.applyFilter->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_F));
	//ui.applyFilter->setToolTip("Shortcut: Shift+F");
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

QTreeWidgetItem *LoggerWindow::GenerateLogTabEntry(QString line)
{
	QTreeWidgetItem *ret = new QTreeWidgetItem();
	QStringList parse = line.split(" ", QString::SkipEmptyParts);

	if(parse[4].compare("last") != 0)
	{
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
	else
	{
		QString time(parse[0] + " " + parse[1]+ " " + parse[2]);
		ret->setText(0, time);
		QString level("INFO");
		ret->setText(1, level);
		QString message(parse[4]);
		for(int i = 5; i < parse.size(); i++)
		{
			message.append(" ");
			message.append(parse[i]);
		}
		ret->setText(2, message);
		for(int i = 0; i < 4; i++)
		{
			ret->setBackgroundColor(i, "skyblue");
		}
		return ret;
	}
}

QTreeWidgetItem *LoggerWindow::LogFileNotFound()
{
	QTime time = QTime::currentTime();
	QTreeWidgetItem *ret = new QTreeWidgetItem();
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

void LoggerWindow::on_setEmailsButton_clicked()
{
	if(ui.emailEdit->text().isEmpty())
	{
		//If there's nothing there to parse, do nothing
	}
	else
	{
		/*QString *valid = new QString(ui.emailEdit->text());

		if(ui.emailEdit->validator()->validate(valid, 0) == QValidator::Acceptable)
		{
			ui.emailsList->addItem("switch worked");
		}*/

		std::vector<std::string> recs;

		QString *text = new QString(ui.emailEdit->text());
		QStringList parse = text->split(", ", QString::SkipEmptyParts);

		ui.emailsList->clear();

		for(int i = 0; i < parse.size(); i++)
		{
			ui.emailsList->addItem(parse[i]);
			recs.push_back(parse[i].toStdString());
		}

		Logger::Inst()->SetEmailRecipients(recs);

		ui.emailEdit->clear();
		ui.setEmailsButton->setDisabled(true);
		ui.appendEmailsButton->setDisabled(false);
		ui.removeEmailsButton->setDisabled(false);
		ui.removeEmailsButton->setText("Clear Emails");
	}
}

void LoggerWindow::on_appendEmailsButton_clicked()
{
	bool add = true;

	if(ui.emailEdit->text().isEmpty())
	{
		//If there's nothing there to parse, do nothing
	}
	else
	{
		/*QString *valid = new QString(ui.emailEdit->text());

		if(ui.emailEdit->validator()->validate(valid, 0) == QValidator::Acceptable)
		{
			ui.emailsList->addItem("switch worked");
		}*/

		std::vector<std::string> recs;

		QString *text = new QString(ui.emailEdit->text());
		QStringList parse = text->split(", ", QString::SkipEmptyParts);

		for(int i = 0; i < parse.size(); i++)
		{
			add = true;

			for(int j = 0; j < ui.emailsList->count() and add; j++)
			{
				if(!ui.emailsList->item(j)->text().compare(parse[i]))
				{
					add = false;
				}
			}

			if(add)
			{
				ui.emailsList->addItem(parse[i]);
				recs.push_back(parse[i].toStdString());
			}
		}

		Logger::Inst()->AppendEmailRecipients(recs);
		ui.emailEdit->clear();
	}
}

void LoggerWindow::on_removeEmailsButton_clicked()
{
	if(!ui.removeEmailsButton->text().compare("Clear Emails"))
	{
		ui.emailsList->clear();
		Logger::Inst()->ClearEmailRecipients();
		ui.removeEmailsButton->setDisabled(true);
		ui.appendEmailsButton->setDisabled(true);
		ui.setEmailsButton->setDisabled(false);
	}
	else
	{
		std::vector<std::string> recs;
		//throw in some debug prints to see what's in this QList
		QList<QListWidgetItem *> selected = ui.emailsList->selectedItems();

		for(uint16_t i = 0; i < selected.size(); i++)
		{
			recs.push_back(selected[i]->text().toStdString());
			delete selected[i];
		}

		Logger::Inst()->RemoveEmailRecipients(recs);

		if(ui.emailsList->count() == 0)
		{
			ui.removeEmailsButton->setDisabled(true);
			ui.appendEmailsButton->setDisabled(true);
			ui.setEmailsButton->setDisabled(false);
		}
	}
}

void LoggerWindow::on_clearEditButton_clicked()
{
	ui.emailEdit->clear();
}

void LoggerWindow::on_emailsList_itemSelectionChanged()
{
	bool change = true;

	if(!ui.removeEmailsButton->text().compare("Clear Emails"))
	{
		ui.removeEmailsButton->setText("Remove Emails");
	}
	else
	{
		for(uint16_t i = 0; i < ui.emailsList->count(); i++)
		{
			if(ui.emailsList->item(i)->isSelected())
			{
				change = false;
			}
		}
		if(change)
		{
			ui.removeEmailsButton->setText("Clear Emails");
		}
	}
}

void LoggerWindow::on_clearButton_clicked()
{
	ui.logDisplay->clear();
}

/*void LoggerWindow::on_applyFilter_clicked()
{
	QTreeWidgetItemIterator it(ui.logDisplay);
	while(*it)
	{
		if(ShouldBeVisible((*it)->text(1)))
		{
			(*it)->setHidden(false);
		}
		else
		{
			(*it)->setHidden(true);
		}
		++it;
	}
}*/

void LoggerWindow::on_checkDebug_stateChanged()
{
	if(ui.checkDebug->checkState() == Qt::Checked)
	{
		ShowSelected("DEBUG");
	}
	else
	{
		HideSelected("DEBUG");
	}
}

void LoggerWindow::on_checkInfo_stateChanged()
{
	if(ui.checkInfo->checkState() == Qt::Checked)
	{
		ShowSelected("INFO");
	}
	else
	{
		HideSelected("INFO");
	}
}

void LoggerWindow::on_checkNotice_stateChanged()
{
	if(ui.checkNotice->checkState() == Qt::Checked)
	{
		ShowSelected("NOTICE");
	}
	else
	{
		HideSelected("NOTICE");
	}
}

void LoggerWindow::on_checkWarning_stateChanged()
{
	if(ui.checkWarning->checkState() == Qt::Checked)
	{
		ShowSelected("WARNING");
	}
	else
	{
		HideSelected("WARNING");
	}
}

void LoggerWindow::on_checkError_stateChanged()
{
	if(ui.checkError->checkState() == Qt::Checked)
	{
		ShowSelected("ERROR");
	}
	else
	{
		HideSelected("ERROR");
	}
}

void LoggerWindow::on_checkCritical_stateChanged()
{
	if(ui.checkCritical->checkState() == Qt::Checked)
	{
		ShowSelected("CRITICAL");
	}
	else
	{
		HideSelected("CRITICAL");
	}
}

void LoggerWindow::on_checkAlert_stateChanged()
{
	if(ui.checkAlert->checkState() == Qt::Checked)
	{
		ShowSelected("ALERT");
	}
	else
	{
		HideSelected("ALERT");
	}
}

void LoggerWindow::on_checkEmergency_stateChanged()
{
	if(ui.checkEmergency->checkState() == Qt::Checked)
	{
		ShowSelected("EMERGENCY");
	}
	else
	{
		HideSelected("EMERGENCY");
	}
}

bool LoggerWindow::ShouldBeVisible(QString level)
{
	bool show = false;

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

void LoggerWindow::HideSelected(QString level)
{
	QTreeWidgetItemIterator it(ui.logDisplay);

	while(*it)
	{
		if((*it)->text(1) == level && !((*it)->isHidden()))
		(*it)->setHidden(true);
		++it;
	}
}

void LoggerWindow::ShowSelected(QString level)
{
	QTreeWidgetItemIterator it(ui.logDisplay);

	while(*it)
	{
		if((*it)->text(1) == level && ((*it)->isHidden()))
		(*it)->setHidden(false);
		++it;
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
