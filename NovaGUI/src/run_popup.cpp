//============================================================================
// Name        : run_popup.cpp
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
// Description : This provides a run dialog where the user can override the configuration
//	options provided in the config file and set things like training mode/pcap file.
//============================================================================
#include "run_popup.h"
#include <fstream>
#include <sstream>
#include "novagui.h"
#include <QString>
#include <QtGui>
#include <QApplication>


using namespace std;

NovaGUI * parentwindow;

Run_Popup::Run_Popup(QWidget *parent, string home)
    : QMainWindow(parent)
{
	homePath = home;
	parentwindow = (NovaGUI*)parent;
	ui.setupUi(this);
	loadPreferences();
}

Run_Popup::~Run_Popup()
{

}

void Run_Popup::closeEvent(QCloseEvent * e)
{
	e = e;
	parentwindow->runAsWindowUp = false;
}

void Run_Popup::loadPreferences()
{
	string line, prefix;

	//Read from CE Config
	ifstream config("Config/NOVAConfig.txt");

	openlog("NovaGUI", OPEN_SYSL, LOG_AUTHPRIV);

	// TODO: Just pass the NovaConfig object in here.. or make a new one, no need to read file manually here
	if(config.is_open())
	{
		while(config.good())
		{
			getline(config,line);

			prefix = "IS_TRAINING";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				ui.trainingCheckBox->setChecked(atoi(line.c_str()));
				continue;
			}

			prefix = "READ_PCAP";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				ui.pcapCheckBox->setChecked(atoi(line.c_str()));
				ui.pcapGroupBox->setEnabled(ui.pcapCheckBox->isChecked());
				continue;
			}

			prefix = "PCAP_FILE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				ui.pcapEdit->setText(line.c_str());
				continue;
			}

			prefix = "GO_TO_LIVE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				ui.liveCapCheckBox->setChecked(atoi(line.c_str()));
				continue;
			}

			prefix = "USE_TERMINALS";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				ui.terminalCheckBox->setChecked(atoi(line.c_str()));
				continue;
			}
		}
	}
	else
	{
		syslog(SYSL_ERR, "File: %s Line: %d Unable to open Config file.", __FILE__, __LINE__);
		this->close();
	}
}

void Run_Popup::on_pcapCheckBox_stateChanged(int state)
{
	ui.pcapGroupBox->setEnabled(state);
}

void Run_Popup::on_startButton_clicked()
{
	if(savePreferences())
	{
		startNova();
	}
	this->close();
}

void Run_Popup::on_cancelButton_clicked()
{
	this->close();
}

void Run_Popup::on_pcapButton_clicked()
{
	//Gets the current path location
	QDir path = QDir::current();
	//Opens a cross-platform dialog box
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open Packet Capture File"),  path.path(), tr("Pcap Files (*)"));

	//Gets the relative path using the absolute path in fileName and the current path
	if(fileName != NULL)
	{
		fileName = path.relativeFilePath(fileName);
		ui.pcapEdit->setText(fileName);
	}
}

bool Run_Popup::savePreferences()
{
	string line, prefix;

	system("cp Config/NOVAConfig.txt Config/.NOVAConfig.tmp");
	ifstream *in = new ifstream("Config/.NOVAConfig.tmp");
	ofstream *out = new ofstream("Config/NOVAConfig.txt");


	if(in->is_open() && out->is_open())
	{
		while(in->good())
		{
			getline(*in, line);

			prefix = "IS_TRAINING";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << "IS_TRAINING " << this->ui.trainingCheckBox->isChecked() << endl;
				continue;
			}

			prefix = "READ_PCAP";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << "READ_PCAP " << this->ui.pcapCheckBox->isChecked() << endl;
				continue;
			}

			prefix = "PCAP_FILE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << "PCAP_FILE " << this->ui.pcapEdit->toPlainText().toStdString() << endl;
				continue;
			}

			prefix = "GO_TO_LIVE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << "GO_TO_LIVE " << this->ui.liveCapCheckBox->isChecked() << endl;
				continue;
			}

			prefix = "USE_TERMINALS";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << "USE_TERMINALS " << this->ui.terminalCheckBox->isChecked() << endl;
				continue;
			}

			*out << line << endl;
		}
	}
	else
	{
		in->close();
		out->close();
		delete out;
		delete in;
		system("rm Config/.NOVAConfig.tmp");
		syslog(SYSL_ERR, "File: %s Line: %d Unable to open Config file.", __FILE__, __LINE__);
		return false;
	}
	in->close();
	out->close();
	delete in;
	delete out;
	system("rm Config/.NOVAConfig.tmp");
	return true;
}
