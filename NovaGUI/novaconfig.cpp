#include "novaconfig.h"
#include "novagui.h"
#include "portPopup.h"
#include "nodePopup.h"
#include <sys/un.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <QtGui>
#include <QApplication>
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

struct profile * currentProfile = NULL;
struct node * currentNode = NULL;
struct subnet * currentSubnet = NULL;

portPopup * portwindow;
nodePopup * nodewindow;
//flag to avoid GUI signal conflicts
bool loadingItems, editingItems = false;
bool selectedSubnet = false;

LoggerPtr n_logger(Logger::getLogger("main"));

/************************************************
 * Construct and Initialize GUI
 ************************************************/

NovaConfig::NovaConfig(QWidget *parent)
    : QMainWindow(parent)
{
	ui.setupUi(this);
	DOMConfigurator::configure("Config/Log4cxxConfig.xml");
	editingPorts = false;
	loadPreferences();
	loadHaystack();
	ui.treeWidget->expandAll();
	ui.hsNodeTreeWidget->expandAll();
}

NovaConfig::~NovaConfig()
{

}

void NovaConfig::closeEvent(QCloseEvent * e)
{
	e = e;
	if(editingNodes && (nodewindow != NULL))
	{
		nodewindow->remoteCall = true;
		nodewindow->close();
	}
	if(editingPorts && (portwindow != NULL))
	{
		portwindow->remoteCall = true;
		portwindow->close();
	}
}

/************************************************
 * Loading preferences from configuration files
 ************************************************/

void NovaConfig::loadPreferences()
{
	string line;
	string prefix;

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

			prefix = "IS_TRAINING";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				ui.trainingCheckBox->setChecked(atoi(line.c_str()));
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
				ui.tcpTimeoutEdit->setText((QString)line.c_str());
				continue;
			}

			prefix = "TCP_CHECK_FREQ";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
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
		LOG4CXX_ERROR(n_logger, "Error loading from Classification Engine config file.");
		this->close();
	}
	config.close();
}


void NovaConfig::loadHaystack()
{
	QTreeWidgetItem * item = NULL;
	string subnetStr, nodeStr, profileStr, line, prefix;

	//Reads the Haystack config file.
	ifstream * config = new ifstream(ui.hsConfigEdit->toPlainText().toStdString().c_str());
	if(config->is_open())
	{
		ui.hsNodeTreeWidget->clear();
		ui.hsProfileTreeWidget->clear();
		ui.nodeTreeWidget->clear();
		ui.profileTreeWidget->clear();
		subnets.clear();
		nodes.clear();
		profiles.clear();
		while(config->good())
		{
			getline(*config, line);

			//If it's a node
			prefix = "bind";
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				//If we found a node in the config remove the prefix
				line = line.substr(prefix.size()+1, line.size());
				//Extract the subnet, profile and ip address of the node
				nodeStr = line.substr(0, line.find(' ', 0));
				subnetStr = nodeStr.substr(0, nodeStr.find_last_of('.', nodeStr.size()));
				profileStr = line.substr(line.find(' ', 0)+1, line.size());

				//Not a real profile so skip it.
				if(!profileStr.compare("default")) continue;

				//Create the node item struct
				struct node n;

				//If this is the first of that subnet
				if(subnets.find(subnetStr) == subnets.end())
				{
					//Create the subnet item struct
					struct subnet sub;

					/*NOTE*/
					//These items don't need to be deleted because the clear function
					// and destructor of the tree widget does that already.

					//create the subnet item for the Haystack menu tree
					item = new QTreeWidgetItem(ui.hsNodeTreeWidget, 0);
					item->setText(0, (QString)subnetStr.c_str());
					sub.item = item;

					//create the subnet item for the node edit tree
					item = new QTreeWidgetItem(ui.nodeTreeWidget, 0);
					item->setText(0, (QString)subnetStr.c_str());
					sub.nodeItem = item;

					//Create the node item for the Haystack tree
					item = new QTreeWidgetItem(subnets[subnetStr].item, 0);
					item->setText(0, (QString)nodeStr.c_str());
					item->setText(1, (QString)profileStr.c_str());
					n.item = item;

					//Create the node item for the node edit tree
					item = new QTreeWidgetItem(subnets[subnetStr].nodeItem, 0);
					item->setText(0, (QString)nodeStr.c_str());
					item->setText(1, (QString)profileStr.c_str());
					n.nodeItem = item;

					//Store members
					n.pname = profileStr;
					n.pfile = &profiles[profileStr];
					n.address = nodeStr;
					n.sub = &subnets[subnetStr];
					sub.address = subnetStr;
					sub.enabled = true;
					n.enabled = true;


					//store the structs in our tables
					nodes[nodeStr] = n;
					subnets[subnetStr] = sub;
					subnets[subnetStr].nodes.push_back(&nodes[nodeStr]);
				}
				else
				{
					/*NOTE*/
					//These items don't need to be deleted because the clear function
					// and destructor of the tree widget does that already.

					//Create the node item for the Haystack tree
					item = new QTreeWidgetItem(subnets[subnetStr].item, 0);
					item->setText(0, (QString)nodeStr.c_str());
					item->setText(1, (QString)profileStr.c_str());
					n.item = item;

					//Create the node item for the node edit tree
					item = new QTreeWidgetItem(subnets[subnetStr].nodeItem, 0);
					item->setText(0, (QString)nodeStr.c_str());
					item->setText(1, (QString)profileStr.c_str());
					n.nodeItem = item;

					//Store members
					n.pname = profileStr;
					n.pfile = &profiles[profileStr];
					n.sub = &subnets[subnetStr];
					n.address = nodeStr;
					n.enabled = true;
					nodes[nodeStr] = n;
					subnets[subnetStr].nodes.push_back(&nodes[nodeStr]);
				}
				continue;
			}

			//If it's a profile
			prefix = "create";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				//Create the item structure for profiles
				struct profile p;

				//Create the profile item for the Haystack menu tree
				profileStr = line.substr(prefix.size()+1,line.size());

				//Not a real profile so skip it.
				if(!profileStr.compare("default")) continue;

				p.name =profileStr;

				profiles[profileStr] = p;
				continue;
			}

			//If we are setting profile parameters
			prefix = "set";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				profileStr = line.substr(0,line.find(' ', 0));

				//Not a real profile so skip it.
				if(!profileStr.compare("default")) continue;

				line = line.substr(profileStr.size()+1, line.size());
				//Create a pointer to access it's profile
				struct profile *p  = &profiles[profileStr];

				prefix = "uptime";
				if(!line.substr(0,prefix.size()).compare(prefix))
				{
					line = line.substr(prefix.size()+1,line.size());
					p->uptime = line;
					continue;
				}

				prefix = "ethernet";
				if(!line.substr(0,prefix.size()).compare(prefix))
				{
					line = line.substr(prefix.size()+2,line.size());
					line = line.substr(0,line.size()-1);
					p->ethernet = line;
					continue;
				}

				prefix = "personality";
				if(!line.substr(0,prefix.size()).compare(prefix))
				{
					line = line.substr(prefix.size()+2,line.size());
					line = line.substr(0,line.size()-1);
					p->personality = line;
					continue;
				}

				prefix = "default tcp action";
				if(!line.substr(0,prefix.size()).compare(prefix))
				{
					line = line.substr(prefix.size()+1,line.size());
					p->tcpAction = line;
					continue;
				}
				continue;
			}

			//If we are adding port behavior
			prefix = "add";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				profileStr = line.substr(0,line.find(' ', 0));

				//Not a real profile so skip it.
				if(!profileStr.compare("default")) continue;

				line = line.substr(profileStr.size()+1, line.size());
				//Create a pointer to access it's profile
				struct profile *p  = &profiles[profileStr];
				struct port pr;
				pr.type = line.substr(0,line.find(' ', 0));
				line = line.substr(pr.type.size()+6, line.size());
				prefix = line.substr(0,line.find(' ',0));
				pr.portNum = prefix;
				line = line.substr(prefix.size()+1, line.size());
				prefix = "open";
				if(!line.substr(0,prefix.size()).compare(prefix))
				{
					pr.behavior = prefix;
				}
				else
				{
					line = line.substr(1,line.size());
					line = line.substr(0, line.size()-1);
					pr.behavior = line;
				}
				p->ports.push_back(pr);
				continue;
			}
		}
	}
	else
	{
		config->close();
		delete config;
		config = NULL;
		LOG4CXX_ERROR(n_logger, "Error loading from Classification Engine config file.");
		this->close();
	}
	currentSubnet = &subnets.begin()->second;
	currentNode = currentSubnet->nodes.front();
	currentProfile = &profiles.begin()->second;
	loadAllProfiles();
	ui.nodeTreeWidget->expandAll();
	config->close();
	delete config;
	config = NULL;
}

void NovaConfig::updatePointers()
{
	//Resets the pointer if the subnet still exists
	if(subnets.find(currentSubnet->address) != subnets.end())
		currentSubnet = &subnets[currentSubnet->address];
	//If not it sets it to the front or NULL
	else if(subnets.size())
		currentSubnet = &subnets.begin()->second;
	else
		currentSubnet = NULL;

	//Resets the pointer if the node still exists
	if(nodes.find(currentNode->address) != nodes.end())
		currentNode = &nodes[currentNode->address];
	//If not it sets it to the front or NULL
	else if(nodes.size())
		currentNode = &nodes.begin()->second;
	else
		currentNode = NULL;

	//Resets the pointer if the profile still exists
	if(currentNode->pname.compare("Profile Deleted") && (currentNode->pname.compare("")))
	{
		if(profiles.find(currentProfile->name) != profiles.end())
			currentProfile = &profiles[currentProfile->name];
		//If not it sets it to the front or NULL
		else if(profiles.size())
			currentProfile = &profiles.begin()->second;
		else
			currentProfile = NULL;
	}
}
/************************************************
 * Browse file system dialog box signals
 ************************************************/

void NovaConfig::on_pcapButton_clicked()
{
	//Gets the current path location
	QDir path = QDir::currentPath();

	//Opens a cross-platform dialog box
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open Data File"),  path.path(), tr("Text Files (*.txt)"));

	//Gets the relative path using the absolute path in fileName and the current path
	if(fileName != NULL)
	{
		fileName = path.relativeFilePath(fileName);
		ui.pcapEdit->setText(fileName);
	}
}

void NovaConfig::on_dataButton_clicked()
{
	//Gets the current path location
	QDir path = QDir::currentPath();

	//Opens a cross-platform dialog box
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open Data File"),  path.path(), tr("Text Files (*.txt)"));

	//Gets the relative path using the absolute path in fileName and the current path
	if(fileName != NULL)
	{
		fileName = path.relativeFilePath(fileName);
		ui.dataEdit->setText(fileName);
	}
}

void NovaConfig::on_hsConfigButton_clicked()
{
	//Gets the current path location
	QDir path = QDir::currentPath();

	//Opens a cross-platform dialog box
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open Config File"),  path.path(), tr("Text Files (*.config)"));

	//Gets the relative path using the absolute path in fileName and the current path
	if(fileName != NULL)
	{
		fileName = path.relativeFilePath(fileName);
		ui.hsConfigEdit->setText(fileName);
	}
	loadHaystack();
}

void NovaConfig::on_dmConfigButton_clicked()
{
	//Gets the current path location
	QDir path = QDir::currentPath();

	//Opens a cross-platform dialog box
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open Config File"), path.path(), tr("Text Files (*.config)"));

	//Gets the relative path using the absolute path in fileName and the current path
	if(fileName != NULL)
	{
		fileName = path.relativeFilePath(fileName);
		ui.dmConfigEdit->setText(fileName);
	}
}

/************************************************
 * General Preferences GUI Signals
 ************************************************/

void NovaConfig::on_okButton_clicked()
{
	string line, prefix;

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
		if(ui.trainingCheckBox->isChecked())
		{
			config << "IS_TRAINING 1"<<endl;
		}
		else
		{
			config << "IS_TRAINING 0"<<endl;
		}
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
		if(ui.pcapCheckBox->isChecked())
		{
			config << "READ_PCAP 1"<<endl;
		}
		else
		{
			config << "READ_PCAP 0"<<endl;
		}
		config << "PCAP_FILE " << ui.pcapEdit->toPlainText().toStdString() << endl;
		if(ui.liveCapCheckBox->isChecked())
		{
			config << "GO_TO_LIVE 1" << endl;
		}
		else
		{
			config << "GO_TO_LIVE 0" << endl;
		}
		if(ui.terminalCheckBox->isChecked())
		{
			config << "USE_TERMINALS 1";
		}
		else
		{
			config << "USE_TERMINALS 0";
		}
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
	string line, prefix;

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
		if(ui.trainingCheckBox->isChecked())
		{
			config << "IS_TRAINING 1"<<endl;
		}
		else
		{
			config << "IS_TRAINING 0"<<endl;
		}
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
		if(ui.pcapCheckBox->isChecked())
		{
			config << "READ_PCAP 1"<<endl;
		}
		else
		{
			config << "READ_PCAP 0"<<endl;
		}
		config << "PCAP_FILE " << ui.pcapEdit->toPlainText().toStdString() << endl;
		if(ui.liveCapCheckBox->isChecked())
		{
			config << "GO_TO_LIVE 1";
		}
		else
		{
			config << "GO_TO_LIVE 0";
		}
		if(ui.terminalCheckBox->isChecked())
		{
			config << "USE_TERMINALS 1";
		}
		else
		{
			config << "USE_TERMINALS 0";
		}
	}
	else
	{
		LOG4CXX_ERROR(n_logger, "Error writing to Nova config file.");
		this->close();
	}
	config.close();
	//This call isn't needed but it helps to make sure the values have been written correctly during debugging.
	loadPreferences();
	loadHaystack();
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

	loadPreferences();
	loadHaystack();
}

void NovaConfig::on_treeWidget_itemSelectionChanged()
{
	QTreeWidgetItem * item = ui.treeWidget->selectedItems().first();

	//If last window was the profile window, save any changes
	if(editingItems && profiles.size()) saveProfile();
	editingItems = false;


	//If it's a top level item the page corresponds to their index in the tree
	//Any new top level item should be inserted at the corresponding index and the defines
	// for the lower level items will need to be adjusted appropriately.
	int i = ui.treeWidget->indexOfTopLevelItem(item);
	if(i != -1)
	{
		if(i != -1 )
		{
			ui.stackedWidget->setCurrentIndex(i);
		}
	}
	//If the item is a child of a top level item, find out what type of item it is
	else
	{
		//Find the parent and keep getting parents until we have a top level item
		QTreeWidgetItem * parent = item->parent();
		while(ui.treeWidget->indexOfTopLevelItem(parent) == -1)
		{
			parent = parent->parent();
		}
		if(ui.treeWidget->indexOfTopLevelItem(parent) == HAYSTACK_MENU_INDEX)
		{
			//If the 'Nodes' Item
			if(parent->child(NODE_INDEX) == item)
			{
				ui.stackedWidget->setCurrentIndex(ui.treeWidget->topLevelItemCount());
			}
			//If the 'Profiles' item
			else if(parent->child(PROFILE_INDEX) == item)
			{
				editingItems = true;
				ui.stackedWidget->setCurrentIndex(ui.treeWidget->topLevelItemCount()+1);
			}
		}
		else
		{
			LOG4CXX_ERROR(n_logger, "Unable to set stackedWidget page"
					" index from treeWidgetItem");
		}
	}
}


/************************************************
 * Preference Menu Specific GUI Signals
 ************************************************/


/*************************
 * Pcap Menu GUI Signals */

/* Enables or disables options specific for reading from pcap file */
void NovaConfig::on_pcapCheckBox_stateChanged(int state)
{
	ui.pcapGroupBox->setEnabled(state);
}


/******************************************
 * Profile Menu GUI Functions *************/

void NovaConfig::saveProfile()
{
	QTreeWidgetItem * item = NULL;
	struct port * pr = NULL;

	//If the name has changed we need to move it in the profile hash table and point all
	//nodes that use the profile to the new location.
	updateProfile(PROFILE_NAME_CHANGE);

	//Saves any modifications to the last selected profile object.
	if(profiles.find(currentProfile->name) != profiles.end())
	{
		//currentProfile->name is set in updateProfile
		currentProfile->ethernet = ui.ethernetEdit->toPlainText().toStdString();
		currentProfile->tcpAction = ui.tcpActionEdit->toPlainText().toStdString();
		currentProfile->uptime = ui.uptimeEdit->toPlainText().toStdString();
		currentProfile->personality = ui.personalityEdit->toPlainText().toStdString();
		//Save the port table
		for(int i = 0; i < ui.portTreeWidget->topLevelItemCount(); i++)
		{
			pr = &currentProfile->ports[i];
			item = ui.portTreeWidget->topLevelItem(i);
			pr->portNum = item->text(0).toStdString();
			pr->type = item->text(1).toStdString();
			pr->behavior = item->text(2).toStdString();
		}
	}
}

void NovaConfig::loadProfile()
{
	struct port * pr = NULL;
	QTreeWidgetItem * item = NULL;
	if(profiles.size() && (profiles.find(currentProfile->name) != profiles.end()))
	{

		//Clear the tree widget and load new selections
		ui.portTreeWidget->clear();

		//Set the variables of the profile
		ui.profileEdit->setText((QString)currentProfile->name.c_str());
		ui.ethernetEdit->setText((QString)currentProfile->ethernet.c_str());
		ui.tcpActionEdit->setText((QString)currentProfile->tcpAction.c_str());
		ui.uptimeEdit->setText((QString)currentProfile->uptime.c_str());
		ui.personalityEdit->setText((QString)currentProfile->personality.c_str());

		//Populate the port table
		for(uint i = 0; i < currentProfile->ports.size(); i++)
		{
			pr = &currentProfile->ports[i];

			//These don't need to be deleted because the clear function
			// and destructor of the tree widget does that already.
			item = new QTreeWidgetItem(0);
			item->setText(0,(QString)pr->portNum.c_str());
			item->setText(1,(QString)pr->type.c_str());
			item->setText(2,(QString)pr->behavior.c_str());
			ui.portTreeWidget->insertTopLevelItem(i, item);
			pr->item = item;
		}
		ui.profileEdit->setEnabled(true);
		ui.ethernetEdit->setEnabled(true);
		ui.tcpActionEdit->setEnabled(true);
		ui.uptimeEdit->setEnabled(true);
		ui.personalityEdit->setEnabled(true);
		ui.uptimeBehaviorComboBox->setEnabled(true);
	}
	else
	{
		LOG4CXX_ERROR(n_logger, profiles.size());
		ui.portTreeWidget->clear();

		//Set the variables of the profile
		ui.profileEdit->clear();
		ui.ethernetEdit->clear();
		ui.tcpActionEdit->clear();
		ui.uptimeEdit->clear();
		ui.personalityEdit->clear();
		ui.uptimeBehaviorComboBox->setCurrentIndex(0);
		ui.profileEdit->setEnabled(false);
		ui.ethernetEdit->setEnabled(false);
		ui.tcpActionEdit->setEnabled(false);
		ui.uptimeEdit->setEnabled(false);
		ui.personalityEdit->setEnabled(false);
		ui.uptimeBehaviorComboBox->setEnabled(false);
	}
}

void NovaConfig::loadAllProfiles()
{
	loadingItems = true;
	string profileStr;
	QTreeWidgetItem * item = NULL;
	ui.hsProfileTreeWidget->clear();
	ui.profileTreeWidget->clear();
	ui.hsProfileTreeWidget->sortByColumn(0,Qt::AscendingOrder);
	ui.profileTreeWidget->sortByColumn(0,Qt::AscendingOrder);

	if(profiles.size())
	{
		for(ProfileTable::iterator it = profiles.begin(); it != profiles.end(); it++)
		{
			profileStr = it->second.name;

			/*NOTE*/
			//These items don't need to be deleted because the clear function
			// and destructor of the tree widget does that already.
			item = new QTreeWidgetItem(ui.hsProfileTreeWidget,0);
			item->setText(0, (QString)profileStr.c_str());
			it->second.item = item;

			//Create the profile item for the profile tree
			item = new QTreeWidgetItem(ui.profileTreeWidget,0);
			item->setText(0, (QString)profileStr.c_str());
			it->second.profileItem = item;
		}
		ui.profileTreeWidget->setCurrentItem(currentProfile->profileItem);
	}
	loadProfile();
	loadingItems = false;
}

void NovaConfig::updateProfile(bool deleteProfile)
{
	//If the profile is being deleted
	if(deleteProfile)
	{
		QBrush rbrush(QColor(200, 80, 80, 255));
		rbrush.setStyle(Qt::SolidPattern);

		string pname = "Profile Deleted";
		//Find all nodes who use this profile and remove it
		for(NodeTable::iterator it = nodes.begin(); it != nodes.end(); it++)
		{
			if(it->second.pfile == currentProfile)
			{
				it->second.pfile = NULL;
				it->second.nodeItem->setText(1, (QString)pname.c_str());
				it->second.pname = pname;
				it->second.nodeItem->setBackground(1, rbrush);
				it->second.enabled = false;
			}
		}
		profiles.erase(currentProfile->name);
	}
	//If the profile's name has changed
	else
	{
		string pname = currentProfile->profileItem->text(0).toStdString();

		//If item text and profile name don't match, we need to update
		if(currentProfile->name.compare(pname))
		{
			//Set the profile to the correct name and put the profile in the table
			profiles[pname] = *currentProfile;
			profiles[pname].name = pname;

			//Find all nodes who use this profile and update to the new one
			for(NodeTable::iterator it = nodes.begin(); it != nodes.end(); it++)
			{
				if(it->second.pfile == currentProfile)
				{
					it->second.pfile = &profiles[pname];
					it->second.pname = pname;
					it->second.nodeItem->setText(1, (QString)pname.c_str());
				}
			}
			//Remove the old profile and update the currentProfile pointer
			profiles.erase(currentProfile->name);
			currentProfile = &profiles[pname];
		}
	}
}


/******************************************
 * Profile Menu GUI Signals ***************/


/* This loads the profile config menu for the profile selected */
void NovaConfig::on_profileTreeWidget_itemSelectionChanged()
{
	if(!loadingItems && profiles.size())
	{
		//Save old profile
		saveProfile();
		if(!ui.profileTreeWidget->selectedItems().isEmpty())
		{
			QTreeWidgetItem * item = ui.profileTreeWidget->selectedItems().first();
			currentProfile = &profiles[item->text(0).toStdString()];
		}
		loadingItems = true;
		loadProfile();
		loadingItems = false;
	}
}

void NovaConfig::on_deleteButton_clicked()
{
	if((!ui.profileTreeWidget->selectedItems().isEmpty()) && profiles.size())
	{
		QTreeWidgetItem * item = currentProfile->profileItem;
		uint i = ui.profileTreeWidget->indexOfTopLevelItem(item);
		ui.profileTreeWidget->removeItemWidget(item, 0);
		ui.hsProfileTreeWidget->removeItemWidget(currentProfile->item, 0);
		updateProfile(DELETE_PROFILE);
		if(profiles.size())
		{
			//If it was the bottom item select previous item
			if(i == profiles.size()) i--;
			//otherwise select next item.
			else i++;
			item = ui.profileTreeWidget->topLevelItem(i);
			currentProfile = &profiles[item->text(0).toStdString()];
		}
		else
		{
			currentProfile = NULL;
		}
		loadAllProfiles();
	}
}

void NovaConfig::on_addButton_clicked()
{
	struct profile temp;
	stringstream ss;
	temp.name = "New Profile";
	temp.ethernet = "Dell";
	temp.personality = "Microsoft Windows 2003 Server";
	temp.tcpAction = "reset";
	temp.uptime = "0";
	temp.ports.clear();
	int i = 1;

	while(profiles.find(temp.name) != profiles.end())
	{
		i++;
		ss.clear();
		ss << "(" << i << ")New Profile";
		getline(ss, temp.name);
	}

	profiles[temp.name] = temp;
	loadAllProfiles();
}
void NovaConfig::on_cloneButton_clicked()
{
	//Do nothing if no profiles
	if(profiles.size())
	{
		QTreeWidgetItem * item = ui.profileTreeWidget->selectedItems().first();
		string profileStr = item->text(0).toStdString();
		struct profile p = *currentProfile;
		int i = 1;
		stringstream ss;
		ss.clear();
		ss << "(" << i << ")" << profileStr;
		getline(ss, p.name);
		//Check for name in use, if so increase number
		while(profiles.find(p.name) != profiles.end())
		{
			ss.clear();
			i++;
			ss << "(" << i << ")" << profileStr;
			getline(ss, p.name);
		}
		//Change the profile name and put in the table, update the current profile
		profiles[p.name] = p;
		loadAllProfiles();
	}
}

void NovaConfig::on_editPortsButton_clicked()
{
	editingPorts = true;
	if(!ui.profileTreeWidget->selectedItems().isEmpty())
	{
		QTreeWidgetItem * item = ui.profileTreeWidget->selectedItems().first();
		string profileStr = item->text(0).toStdString();
		struct profile *p = &profiles[profileStr];
		portwindow = new portPopup(this, p, FROM_NOVA_CONFIG);
		loadAllNodes();
		portwindow->show();
	}
	else
	{
		LOG4CXX_ERROR(n_logger, "No profile selected when opening port edit window.");
	}
}

void NovaConfig::on_profileEdit_textChanged()
{
	if(!loadingItems && !profiles.empty())
	{
		currentProfile->item->setText(0,ui.profileEdit->toPlainText());
		currentProfile->profileItem->setText(0,ui.profileEdit->toPlainText());
	}
}

/******************************************
 * Node Menu GUI Functions ****************/

void NovaConfig::loadAllNodes()
{
	loadingItems = true;
	QBrush whitebrush(QColor(255, 255, 255, 255));
	QBrush greybrush(QColor(100, 100, 100, 255));
	greybrush.setStyle(Qt::SolidPattern);
	QBrush blackbrush(QColor(0, 0, 0, 255));
	blackbrush.setStyle(Qt::NoBrush);
	struct node * n = NULL;

	QTreeWidgetItem * item = NULL;
	ui.nodeTreeWidget->clear();
	ui.hsNodeTreeWidget->clear();


	for(SubnetTable::iterator it = subnets.begin(); it != subnets.end(); it++)
	{
		//create the subnet item for the Haystack menu tree
		item = new QTreeWidgetItem(ui.hsNodeTreeWidget, 0);
		item->setText(0, (QString)it->second.address.c_str());
		it->second.item = item;

		//create the subnet item for the node edit tree
		item = new QTreeWidgetItem(ui.nodeTreeWidget, 0);
		item->setText(0, (QString)it->second.address.c_str());
		it->second.nodeItem = item;

		if(this->isEnabled())
		{
			if(!it->second.enabled)
			{
				whitebrush.setStyle(Qt::NoBrush);
				it->second.nodeItem->setBackground(0,greybrush);
				it->second.nodeItem->setForeground(0,whitebrush);
				it->second.item->setBackground(0,greybrush);
				it->second.item->setForeground(0,whitebrush);
			}
		}

		for(uint i = 0; i < it->second.nodes.size(); i++)
		{

			n = it->second.nodes[i];

			//Create the node item for the Haystack tree
			item = new QTreeWidgetItem(it->second.item, 0);
			item->setText(0, (QString)n->address.c_str());
			item->setText(1, (QString)n->pname.c_str());
			n->item = item;

			//Create the node item for the node edit tree
			item = new QTreeWidgetItem(it->second.nodeItem, 0);
			item->setText(0, (QString)n->address.c_str());
			item->setText(1, (QString)n->pname.c_str());
			n->nodeItem = item;

			if(this->isEnabled())
			{
				if(!n->enabled)
				{
					whitebrush.setStyle(Qt::NoBrush);
					n->nodeItem->setBackground(0,greybrush);
					n->nodeItem->setForeground(0,whitebrush);
					n->item->setBackground(0,greybrush);
					n->item->setForeground(0,whitebrush);
				}
			}
		}
	}
	ui.nodeTreeWidget->expandAll();

	if(nodes.size()+subnets.size())
	{
		if(currentNode != NULL)
			ui.nodeTreeWidget->setCurrentItem(currentNode->nodeItem);
		else
			ui.nodeTreeWidget->setCurrentItem(currentSubnet->nodeItem);
	}

	loadingItems = false;
}

// Removes the node from item widgets and data structures.
void NovaConfig::deleteNode()
{
	ui.nodeTreeWidget->removeItemWidget(currentNode->nodeItem, 0);
	ui.hsNodeTreeWidget->removeItemWidget(currentNode->item, 0);
	LOG4CXX_INFO(n_logger, currentSubnet->address);

	for(uint i = 0; i < currentSubnet->nodes.size(); i++)
	{
		if(currentSubnet->nodes[i] == currentNode)
		{
			currentSubnet->nodes.erase(currentSubnet->nodes.begin()+i);
		}
	}
	LOG4CXX_INFO(n_logger, currentNode->address);
	nodes.erase(currentNode->address);
}

/******************************************
 * Node Menu GUI Signals ******************/

//The current selection in the node list
void NovaConfig::on_nodeTreeWidget_itemSelectionChanged()
{
	//If the user is changing the selection AND something is selected
	if(!loadingItems)
	{
		if(!ui.nodeTreeWidget->selectedItems().isEmpty())
		{
			QTreeWidgetItem * item = ui.nodeTreeWidget->selectedItems().first();
			//If it's not a top level item (which means it's a node)
			if(ui.nodeTreeWidget->indexOfTopLevelItem(item) == -1)
			{
				currentNode = &nodes[item->text(0).toStdString()];
				currentSubnet = currentNode->sub;
				selectedSubnet = false;
			}
			else //If it's a subnet
			{
				currentSubnet = &subnets[item->text(0).toStdString()];
				currentNode = currentSubnet->nodes.front();
				selectedSubnet = true;
			}
		}
	}
}

//Pops up the node edit window selecting the current node
void NovaConfig::on_nodeEditButton_clicked()
{
	if(nodes.size())
	{
		editingNodes = true;
		nodewindow = new nodePopup(this, currentNode, EDIT_NODE);
		loadAllNodes();
		nodewindow->show();
	}
}

//Creates a copy of the current node and pops up the edit window
void NovaConfig::on_nodeCloneButton_clicked()
{
	if(nodes.size())
	{
		editingNodes = true;
		nodewindow = new nodePopup(this, currentNode, CLONE_NODE);
		loadAllNodes();
		nodewindow->show();
	}
}

//Creates a new node and pops up the edit window
void NovaConfig::on_nodeAddButton_clicked()
{
	if(nodes.size())
	{
		editingNodes = true;
		nodewindow = new nodePopup(this, currentNode, ADD_NODE);
		loadAllNodes();
		nodewindow->show();
	}
}

//Gets item selection then calls the function(s) to remove the node(s)
void NovaConfig::on_nodeDeleteButton_clicked()
{
	QTreeWidgetItem * temp = NULL;
	string address;
	bool nextIsSubnet;

	loadingItems = true;

	//If the list isn't empty
	if(nodes.size()+subnets.size())
	{
		//If there is at least one other item
		if((nodes.size()+subnets.size()) > 1)
		{
			//Try to select the bottom item first
			temp = ui.nodeTreeWidget->itemBelow(ui.nodeTreeWidget->selectedItems().first());

			//If that fails, select the one above
			if(temp == NULL)
				temp = ui.nodeTreeWidget->itemAbove(ui.nodeTreeWidget->selectedItems().first());

			//Save the address since the item temp points to will change when we redraw the list
			address = temp->text(0).toStdString();

			//If the item isn't top level it is a node and will return -1
			if(ui.nodeTreeWidget->indexOfTopLevelItem(temp) == -1)
				nextIsSubnet = false; //Flag as node

			else nextIsSubnet = true; //Flag as subnet
		}

		//If we are deleteing a subnet, remove each node first then remove the subnet.
		if(selectedSubnet)
		{
			for(uint i = 0; i < currentSubnet->nodes.size(); i++)
			{
				currentNode = currentSubnet->nodes[i];
				deleteNode();
			}
			ui.nodeTreeWidget->removeItemWidget(currentSubnet->nodeItem, 0);
			ui.hsNodeTreeWidget->removeItemWidget(currentSubnet->item, 0);
			subnets.erase(currentSubnet->address);

			loadAllNodes();
		}
		//Else just delete the selected node
		else
		{
			deleteNode();
			loadAllNodes();
		}
		loadingItems = true;


		//If we have a node selected, set it as current item
		if(!nextIsSubnet)
		{
			currentNode = &nodes[address];
			currentSubnet = currentNode->sub;
			ui.nodeTreeWidget->setCurrentItem(currentNode->nodeItem);

		}
		//If we have a subnet selected
		else
		{
			currentSubnet = &subnets[address];

			//If there are nodes available, set that as current item
			if(currentSubnet->nodes.size())
			{
				currentNode = currentSubnet->nodes.front();
				ui.nodeTreeWidget->setCurrentItem(currentNode->nodeItem);
				selectedSubnet = false; //Change flag to indicate a node
			}

			else //Else select the subnet
			{
				currentNode = NULL;
				ui.nodeTreeWidget->setCurrentItem(currentSubnet->nodeItem);
			}
		}
	}
	//If there are no more items in the list make sure it is clear
	else
	{
		ui.nodeTreeWidget->clear();
		ui.hsNodeTreeWidget->clear();
	}
	loadingItems = false;
}

//Enables a node or a subnet in honeyd
//TODO Use this flag to comment out nodes whens writing to the honeyd config
//  loading from the file will need to recognize these
void NovaConfig::on_nodeEnableButton_clicked()
{
	if(selectedSubnet)
	{
		for(uint i = 0; i < currentSubnet->nodes.size(); i++)
		{
			currentSubnet->nodes[i]->enabled = true;

		}
		currentSubnet->enabled = true;
	}
	else
	{
		currentNode->enabled = true;
	}

	//Draw the nodes and restore selection
	loadingItems = true;
	loadAllNodes();
	if(selectedSubnet)
		ui.nodeTreeWidget->setCurrentItem(currentSubnet->nodeItem);
	else
		ui.nodeTreeWidget->setCurrentItem(currentNode->nodeItem);
	loadingItems = false;
}

//Disables a node or a subnet in honeyd
//TODO Use this flag to comment out nodes whens writing to the honeyd config
//  loading from the file will need to recognize these
void NovaConfig::on_nodeDisableButton_clicked()
{
	if(selectedSubnet)
	{
		for(uint i = 0; i < currentSubnet->nodes.size(); i++)
		{
			currentSubnet->nodes[i]->enabled = false;

		}
		currentSubnet->enabled = false;
	}
	else
	{
		currentNode->enabled = false;
	}

	//Draw the nodes and restore selection
	loadingItems = true;
	loadAllNodes();
	if(selectedSubnet)
		ui.nodeTreeWidget->setCurrentItem(currentSubnet->nodeItem);
	else
		ui.nodeTreeWidget->setCurrentItem(currentNode->nodeItem);
	loadingItems = false;
}
