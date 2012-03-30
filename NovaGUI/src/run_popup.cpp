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
#include "novagui.h"
#include "nova_ui_core.h"

#include <QDir>
#include <QFileDialog>

using namespace std;
using namespace Nova;

Run_Popup::Run_Popup(QWidget *parent)
    : QMainWindow(parent)
{
	ui.setupUi(this);
	LoadPreferences();
}

Run_Popup::~Run_Popup()
{

}

void Run_Popup::LoadPreferences()
{
	ui.trainingCheckBox->setChecked(Config::Inst()->GetIsTraining());
	ui.pcapCheckBox->setChecked(Config::Inst()->GetReadPcap());
	ui.pcapGroupBox->setEnabled(ui.pcapCheckBox->isChecked());
	ui.pcapEdit->setText(Config::Inst()->GetPathPcapFile().c_str());
	ui.liveCapCheckBox->setChecked(Config::Inst()->GetGotoLive());
}

void Run_Popup::on_pcapCheckBox_stateChanged(int state)
{
	ui.pcapGroupBox->setEnabled(state);
}

void Run_Popup::on_startButton_clicked()
{
	if(SavePreferences())
	{
		StartNovad();
		((NovaGUI*)parent())->ConnectGuiToNovad();
		StartHaystack();
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

bool Run_Popup::SavePreferences()
{
	Config::Inst()->SetIsTraining(ui.trainingCheckBox->isChecked());
	Config::Inst()->SetReadPcap(ui.pcapCheckBox->isChecked());
	Config::Inst()->SetPathPcapFile(ui.pcapEdit->text().toStdString());
	Config::Inst()->SetGotoLive(ui.liveCapCheckBox->isChecked());
	return Config::Inst()->SaveConfig();
}
