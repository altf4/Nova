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
		exit(1);
	}
	config.close();
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
		cout << "Not a top level item.\n";
	}
}

