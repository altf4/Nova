#include "novaconfig.h"
#include <sys/un.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <QtGui>
#include <QApplication>
#include "novagui.h"
#include <sstream>
#include <QString>
#include <QChar>
#include <fstream>
#include <log4cxx/xml/domconfigurator.h>
#include <errno.h>

using namespace std;
using namespace Nova;
using namespace ClassificationEngine;
using namespace log4cxx;
using namespace log4cxx::xml;

LoggerPtr n_logger(Logger::getLogger("main"));


NovaConfig::NovaConfig(QWidget *parent)
    : QMainWindow(parent)
{
	ui.setupUi(this);
	DOMConfigurator::configure("Config/Log4cxxConfig_CGUI.xml");
	LoadPreferences();
}

NovaConfig::~NovaConfig()
{

}

void NovaConfig::LoadPreferences()
{
	string line;
	string prefix;
	string interface;
	string tcpTimeout;
	string tcpFreq;

	//Read from CE Config
	ifstream config("Config/NOVAConfig.txt");

	if(config.is_open())
	{
		while(config.good())
		{
			getline(config,line);

			prefix = "INTERFACE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				interface = line;
				ui.interfaceEdit->setText((QString)line.c_str());
				continue;
			}

			prefix = "DATAFILE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				ui.dataEdit->setText((QString)line.c_str());
				continue;
			}

			prefix = "BROADCAST_ADDR";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				ui.saIPEdit->setText((QString)line.c_str());
				continue;
			}

			prefix = "SILENT_ALARM_PORT";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				ui.saPortEdit->setText((QString)line.c_str());
				continue;
			}

			prefix = "K";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				ui.ceIntensityEdit->setText((QString)line.c_str());
				continue;
			}

			prefix = "EPS";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				ui.ceErrorEdit->setText((QString)line.c_str());
				continue;
			}

			prefix = "CLASSIFICATION_TIMEOUT";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				ui.ceFrequencyEdit->setText((QString)line.c_str());
				continue;
			}

			prefix = "CLASSIFICATION_THRESHOLD";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				ui.ceThresholdEdit->setText((QString)line.c_str());
				continue;
			}

			prefix = "HS_HONEYD_CONFIG";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				ui.hsConfigEdit->setText((QString)line.c_str());
				continue;
			}

			prefix = "TCP_TIMEOUT";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				tcpTimeout = line;
				ui.tcpTimeoutEdit->setText((QString)line.c_str());
				continue;
			}

			prefix = "TCP_CHECK_FREQ";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				tcpFreq = line;
				ui.tcpFrequencyEdit->setText((QString)line.c_str());
				continue;
			}

			prefix = "DM_HONEYD_CONFIG";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				ui.dmConfigEdit->setText((QString)line.c_str());
				continue;
			}

			prefix = "DOPPELGANGER_IP";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				ui.dmIPEdit->setText((QString)line.c_str());
				continue;
			}

			prefix = "DM_ENABLED";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				ui.dmCheckBox->setChecked(atoi(line.c_str()));
				continue;
			}
		}
	}
	else
	{
		LOG4CXX_ERROR(n_logger, "Error loading from Classification Engine config file.");
		this->close();
	}
	config.close();
}

void NovaConfig::on_dataButton_clicked()
{
	//Gets the current path location
	QDir path = QDir::currentPath();

	//Opens a cross-platform dialog box
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open Data File"),  path.path(), tr("Text Files (*.txt)"));

	//Gets the relative path using the absolute path in fileName and the current path
	fileName = path.relativeFilePath(fileName);
	ui.dataEdit->setText(fileName);
}

void NovaConfig::on_hsConfigButton_clicked()
{
	//Gets the current path location
	QDir path = QDir::currentPath();

	//Opens a cross-platform dialog box
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open Config File"),  path.path(), tr("Text Files (*.config)"));

	//Gets the relative path using the absolute path in fileName and the current path
	fileName = path.relativeFilePath(fileName);
	ui.hsConfigEdit->setText(fileName);
}

void NovaConfig::on_dmConfigButton_clicked()
{
	//Gets the current path location
	QDir path = QDir::currentPath();

	//Opens a cross-platform dialog box
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open Config File"), path.path(), tr("Text Files (*.config)"));

	//Gets the relative path using the absolute path in fileName and the current path
	fileName = path.relativeFilePath(fileName);
	ui.dmConfigEdit->setText(fileName);
}

void NovaConfig::on_okButton_clicked()
{
	string line, prefix, isTraining, readPcap, pcapFile;

	//Save some values that aren't set in preferences
	ifstream preConfig("Config/NOVAConfig.txt");
	if(preConfig.is_open())
	{
		while(preConfig.good())
		{
			getline(preConfig,line);
			prefix = "IS_TRAINING";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				isTraining = line;
				continue;
			}
			prefix = "READ_PCAP";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				readPcap = line;
				continue;
			}
			prefix = "PCAP_FILE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				pcapFile = line;
				continue;
			}
		}
	}
	else
	{
		LOG4CXX_ERROR(n_logger, "Error reading from Nova config file.");
		this->close();
	}
	preConfig.close();

	//Rewrite the config file with the new settings
	ofstream config("Config/NOVAConfig.txt");
	if(config.is_open())
	{
		config << "INTERFACE " << this->ui.interfaceEdit->toPlainText().toStdString()<< endl;
		config << "DATAFILE " << this->ui.dataEdit->toPlainText().toStdString()<< endl;
		config << "BROADCAST_ADDR " << this->ui.saIPEdit->toPlainText().toStdString() << endl;
		config << "SILENT_ALARM_PORT " << this->ui.saPortEdit->toPlainText().toStdString() << endl;
		config << "K " << this->ui.ceIntensityEdit->toPlainText().toStdString() << endl;
		config << "EPS " << this->ui.ceErrorEdit->toPlainText().toStdString() << endl;
		config << "CLASSIFICATION_TIMEOUT " << this->ui.ceFrequencyEdit->toPlainText().toStdString() << endl;
		config << "IS_TRAINING " << isTraining << endl;
		config << "CLASSIFICATION_THRESHOLD " << this->ui.ceThresholdEdit->toPlainText().toStdString() << endl;
		config << "DM_HONEYD_CONFIG " << this->ui.dmConfigEdit->toPlainText().toStdString() << endl;

		config << "DOPPELGANGER_IP " << this->ui.dmIPEdit->toPlainText().toStdString() << endl;
		if(ui.dmCheckBox->isChecked())
		{
			config << "DM_ENABLED 1"<<endl;
		}
		else
		{
			config << "DM_ENABLED 0"<<endl;
		}
		config << "HS_HONEYD_CONFIG " << this->ui.hsConfigEdit->toPlainText().toStdString() << endl;
		config << "TCP_TIMEOUT " << this->ui.tcpTimeoutEdit->toPlainText().toStdString() << endl;
		config << "TCP_CHECK_FREQ " << this->ui.tcpFrequencyEdit->toPlainText().toStdString() << endl;
		config << "READ_PCAP " << readPcap << endl;
		config << "PCAP_FILE " << pcapFile;
	}
	else
	{
		LOG4CXX_ERROR(n_logger, "Error writing to Nova config file.");
		this->close();
	}
	config.close();
	this->close();
}

void NovaConfig::on_applyButton_clicked()
{
	string line, prefix, isTraining, readPcap, pcapFile;

	//Save some values that aren't set in preferences
	ifstream preConfig("Config/NOVAConfig.txt");
	if(preConfig.is_open())
	{
		while(preConfig.good())
		{
			getline(preConfig,line);
			prefix = "IS_TRAINING";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				isTraining = line;
				continue;
			}
			prefix = "READ_PCAP";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				readPcap = line;
				continue;
			}
			prefix = "PCAP_FILE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				pcapFile = line;
				continue;
			}
		}
	}
	else
	{
		LOG4CXX_ERROR(n_logger, "Error reading from Nova config file.");
		this->close();
	}
	preConfig.close();

	//Rewrite the config file with the new settings
	ofstream config("Config/NOVAConfig.txt");
	if(config.is_open())
	{
		config << "INTERFACE " << this->ui.interfaceEdit->toPlainText().toStdString()<< endl;
		config << "DATAFILE " << this->ui.dataEdit->toPlainText().toStdString()<< endl;
		config << "BROADCAST_ADDR " << this->ui.saIPEdit->toPlainText().toStdString() << endl;
		config << "SILENT_ALARM_PORT " << this->ui.saPortEdit->toPlainText().toStdString() << endl;
		config << "K " << this->ui.ceIntensityEdit->toPlainText().toStdString() << endl;
		config << "EPS " << this->ui.ceErrorEdit->toPlainText().toStdString() << endl;
		config << "CLASSIFICATION_TIMEOUT " << this->ui.ceFrequencyEdit->toPlainText().toStdString() << endl;
		config << "IS_TRAINING " << isTraining << endl;
		config << "CLASSIFICATION_THRESHOLD " << this->ui.ceThresholdEdit->toPlainText().toStdString() << endl;
		config << "DM_HONEYD_CONFIG " << this->ui.dmConfigEdit->toPlainText().toStdString() << endl;

		config << "DOPPELGANGER_IP " << this->ui.dmIPEdit->toPlainText().toStdString() << endl;
		if(ui.dmCheckBox->isChecked())
		{
			config << "DM_ENABLED 1"<<endl;
		}
		else
		{
			config << "DM_ENABLED 0"<<endl;
		}
		config << "HS_HONEYD_CONFIG " << this->ui.hsConfigEdit->toPlainText().toStdString() << endl;
		config << "TCP_TIMEOUT " << this->ui.tcpTimeoutEdit->toPlainText().toStdString() << endl;
		config << "TCP_CHECK_FREQ " << this->ui.tcpFrequencyEdit->toPlainText().toStdString() << endl;
		config << "READ_PCAP " << readPcap << endl;
		config << "PCAP_FILE " << pcapFile;
	}
	else
	{
		LOG4CXX_ERROR(n_logger, "Error writing to Nova config file.");
		this->close();
	}
	config.close();
	//This call isn't needed but it helps to make sure the values have been written correctly during debugging.
	LoadPreferences();
}

void NovaConfig::on_cancelButton_clicked()
{
	this->close();
}

void NovaConfig::on_defaultsButton_clicked()
{
	//Currently just loads the preferences window with what's in the .txt file
	//We should really identify default values and write those while maintaining
	//Critical values that might cause nova to break if changed.

	//The below code will write default values but until we decide
	//exactly what to write, it will remain unimplemented.

	/*ofstream config("Config/NOVAConfig.txt");
	if(config.is_open())
	{
		config << "INTERFACE eth0" << endl;
		config << "BROADCAST_ADDR 255.255.255.255" << endl;
		config << "SILENT_ALARM_PORT 4242" << endl;
		config << "K 3" << endl;
		config << "EPS 0" << endl;
		config << "CLASSIFICATION_TIMEOUT 5" << endl;
		config << "IS_TRAINING 1" << endl;
		config << "CLASSIFICATION_THRESHOLD .5" << endl;
		config << "DM_HONEYD_CONFIG Config/doppelganger.config" << endl;
		config << "DOPPELGANGER_IP 10.0.0.1" << endl;
		config << "DM_ENABLED 1"<<endl;
		config << "HS_HONEYD_CONFIG Config/haystack.config" << endl;
		config << "TCP_TIMEOUT 3" << endl;
		config << "TCP_CHECK_FREQ 3" << endl;
		config << "READ_PCAP 0" << endl;
		config << "PCAP_FILE Config/pcapfile";
	}
	else
	{
		LOG4CXX_ERROR(n_logger, "Error writing to Nova config file.");
		this->close();
	}
	config.close();*/

	LoadPreferences();
}

void NovaConfig::on_treeWidget_itemClicked(QTreeWidgetItem * item, int column)
{
	int i = ui.treeWidget->indexOfTopLevelItem(item);
	int c = column;
	c = 0;

	if(i != -1 )
	{
		ui.stackedWidget->setCurrentIndex(i);
	}
	else
	{
		LOG4CXX_ERROR(n_logger, "Attempted to change widget index from a lower level item.");
	}
}

