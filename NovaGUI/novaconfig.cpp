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

NovaConfig::NovaConfig(QWidget *parent)
    : QMainWindow(parent)
{
	ui.setupUi(this);
}

NovaConfig::~NovaConfig()
{

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

