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

string currentProfile = "";
string currentNode = "";
string currentSubnet = "";

portPopup * portwindow;
nodePopup * nodewindow;
NovaGUI * mainwindow;

//flag to avoid GUI signal conflicts
bool loadingItems, editingItems = false;
bool selectedSubnet = false;

LoggerPtr n_logger(Logger::getLogger("main"));

/************************************************
 * Construct and Initialize GUI
 ************************************************/

NovaConfig::NovaConfig(QWidget *parent, string home)
    : QMainWindow(parent)
{
	//store current directory / base path for Nova
	homePath = home;

	//Initialize hash tables
	subnets.set_empty_key("");
	nodes.set_empty_key("");
	profiles.set_empty_key("");
	ports.set_empty_key("");
	scripts.set_empty_key("");

	//Store parent and load UI
	mainwindow = (NovaGUI*)parent;
	group = mainwindow->group;
	ui.setupUi(this);
	DOMConfigurator::configure("Config/Log4cxxConfig.xml");
	editingPorts = false;

	//Read NOVAConfig, pull honeyd info from parent, populate GUI
	loadPreferences();
	pullData();
	loadHaystack();

	ui.treeWidget->expandAll();
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
	mainwindow->editingPreferences = false;

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
	updatePointers();
	loadAllNodes();
	//Load profiles
	loadAllProfiles();
	ui.nodeTreeWidget->expandAll();
	ui.hsNodeTreeWidget->expandAll();
}

void NovaConfig::pushData()
{
	//Clears the tables
	mainwindow->subnets.clear_no_resize();
	mainwindow->nodes.clear_no_resize();
	mainwindow->profiles.clear_no_resize();
	mainwindow->ports.clear_no_resize();
	mainwindow->scripts.clear_no_resize();

	//Copies the tables
	mainwindow->scripts = scripts;
	mainwindow->subnets = subnets;
	mainwindow->nodes = nodes;
	mainwindow->ports = ports;
	mainwindow->profiles = profiles;

	//Saves the current configuration in XML files
	mainwindow->saveAll();
}

void NovaConfig::pullData()
{
	//Clears the tables
	subnets.clear_no_resize();
	nodes.clear_no_resize();
	profiles.clear_no_resize();
	ports.clear_no_resize();
	scripts.clear_no_resize();

	//Copies the tables
	scripts = mainwindow->scripts;
	subnets = mainwindow->subnets;
	nodes = mainwindow->nodes;
	ports = mainwindow->ports;
	profiles = mainwindow->profiles;
}

void NovaConfig::updatePointers()
{
	if(selectedSubnet)
	{
		//Asserts the subnet still exists
		if(subnets.find(currentSubnet) != subnets.end());
		//If not it sets it to the front or NULL
		else if(subnets.size())
			currentSubnet = subnets.begin()->second.address;
		else
			currentSubnet = "";
		currentNode = "";
	}
	else
	{
		//Asserts the node still exists
		if(nodes.find(currentNode) != nodes.end());
		//If not it sets it to the front or NULL
		else if(nodes.size())
			currentNode = nodes.begin()->second.address;
		else
			currentNode = "";
		currentSubnet = "";
	}

	//Asserts the profile still exists
	if(profiles.find(currentProfile) != profiles.end());
	//If not it sets it to the front or NULL
	else if(profiles.size())
		currentProfile = profiles.begin()->second.name;
	else
		currentProfile = "";
}
/************************************************
 * Browse file system dialog box signals
 ************************************************/

void NovaConfig::on_pcapButton_clicked()
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

void NovaConfig::on_dataButton_clicked()
{
	//Gets the current path location
	QDir path = QDir::current();
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
	QDir path = QDir::current();
	//Opens a cross-platform dialog box
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open Haystack Config File"),  path.path(), tr("Text Files (*.config)"));

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
	QDir path = QDir::current();

	//Opens a cross-platform dialog box
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open Doppelganger Config File"), path.path(), tr("Text Files (*.config)"));

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
	pushData();
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
	pushData();
	loadingItems = true;
	loadHaystack();
	loadingItems = false;
}

void NovaConfig::on_cancelButton_clicked()
{
	this->close();
}

void NovaConfig::on_defaultsButton_clicked() //TODO
{
	//Currently just restores to last save changes
	//We should really identify default values and write those while maintaining
	//Critical values that might cause nova to break if changed.

	loadPreferences();
	mainwindow->loadAll();
	pullData();
	loadingItems = true;
	loadHaystack();
	loadingItems = false;
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
	updateProfile(UPDATE_PROFILE, &profiles[currentProfile]);

	//Saves any modifications to the last selected profile object.
	if(profiles.find(currentProfile) != profiles.end())
	{
		profile * p = &profiles[currentProfile];
		//currentProfile->name is set in updateProfile
		p->ethernet = ui.ethernetEdit->toPlainText().toStdString();
		p->tcpAction = ui.tcpActionEdit->toPlainText().toStdString();
		p->uptime = ui.uptimeEdit->toPlainText().toStdString();
		p->personality = ui.personalityEdit->toPlainText().toStdString();
		//Save the port table
		for(int i = 0; i < ui.portTreeWidget->topLevelItemCount(); i++)
		{
			pr = &ports[p->ports[i]];
			item = ui.portTreeWidget->topLevelItem(i);
			pr->portNum = item->text(0).toStdString();
			pr->type = item->text(1).toStdString();
			pr->behavior = item->text(2).toStdString();
		}
	}
}

void NovaConfig::deleteProfile(string name)
{
	for(ProfileTable::iterator it = profiles.begin(); it != profiles.end(); it++)
	{
		if(!it->second.parentProfile.compare(name))
		{
			deleteProfile(it->second.name);
		}
	}
	profile  p = profiles[name];
	if(!name.compare(currentProfile))
	{
		QTreeWidgetItem * item = NULL, *temp = NULL;
		if(profiles.size() > 1)
		{
			item = profiles[currentProfile].profileItem;
			temp = ui.profileTreeWidget->itemBelow(item);
			if(temp == NULL)
			{
				item = ui.profileTreeWidget->itemAbove(item);
			}
			else item = temp;
		}
		ui.profileTreeWidget->removeItemWidget(p.profileItem, 0);
		ui.hsProfileTreeWidget->removeItemWidget(p.item, 0);

		profiles[name].tree.clear();
		updateProfile(DELETE_PROFILE, &profiles[name]);

		if(p.parentProfile.compare(""))
		{
			profile parent = profiles[p.parentProfile];
			ptree * pt = &parent.tree.get_child("profiles");
			pt->clear();
			for(ProfileTable::iterator it = profiles.begin(); it != profiles.end(); it++)
			{
				if(!it->second.parentProfile.compare(parent.name))
				{
					pt->add_child("profile", it->second.tree);
				}
			}
			ptree * treePtr = &profiles[p.parentProfile].tree.get_child("profiles");
			treePtr->clear();
			*treePtr	= *pt;
			updateProfileTree(p.parentProfile);
		}
		if((item != NULL) && (!name.compare(currentProfile)))
		{
			currentProfile = item->text(0).toStdString();
		}
		else if(!name.compare(currentProfile))
		{
			currentProfile = "";
		}
		loadHaystack();
	}
	else
	{
		updateProfile(DELETE_PROFILE, &profiles[name]);
	}
}

void NovaConfig::loadProfile()
{
	struct port * pr = NULL;
	QTreeWidgetItem * item = NULL;
	if(profiles.size() && (profiles.find(currentProfile) != profiles.end()))
	{

		//Clear the tree widget and load new selections
		ui.portTreeWidget->clear();

		profile * p = &profiles[currentProfile];
		//Set the variables of the profile
		ui.profileEdit->setText((QString)p->name.c_str());
		ui.ethernetEdit->setText((QString)p->ethernet.c_str());
		ui.tcpActionEdit->setText((QString)p->tcpAction.c_str());
		ui.uptimeEdit->setText((QString)p->uptime.c_str());
		ui.personalityEdit->setText((QString)p->personality.c_str());

		//Populate the port table
		for(uint i = 0; i < p->ports.size(); i++)
		{
			pr = &ports[p->ports[i]];

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

void NovaConfig::updateProfileTree(string name)
{
	if(profiles[name].parentProfile.compare(""))
	{
		string parent = profiles[name].parentProfile;
		ptree pt;
		pt.clear();
		for(ProfileTable::iterator it = profiles.begin(); it != profiles.end(); it++)
		{
			if(!it->second.parentProfile.compare(parent))
			{
				pt.add_child("profile", it->second.tree);
			}
		}
		profiles[parent].tree.put_child("profiles", pt);
		updateProfileTree(parent);
	}
}

void NovaConfig::loadProfilesFromTree(string parent)
{
	using boost::property_tree::ptree;
	ptree * ptr, pt = profiles[parent].tree;
	try
	{
		BOOST_FOREACH(ptree::value_type &v, pt.get_child("profiles"))
		{
			//Generic profile, essentially a honeyd template
			if(!string(v.first.data()).compare("profile"))
			{
				profile p;
				//Root profile has no parent
				p.parentProfile = parent;
				p.tree = v.second;

				//Name required, DCHP boolean intialized (set in loadProfileSet)
				p.name = v.second.get<std::string>("name");

				string profileStr = p.name;
				stringstream ss;
				int i = 0;

				while(profiles.find(p.name) != profiles.end())
				{
					ss.str("");
					i++;
					ss << profileStr << "-" << i;
					p.name = ss.str();
				}

				p.DHCP = false;
				p.ports.clear();

				try //Conditional: has "set" values
				{
					ptr = &v.second.get_child("set");
					//pass 'set' subset and pointer to this profile
					loadProfileSet(ptr, &p);
				}
				catch(...){}

				try //Conditional: has "add" values
				{
					ptr = &v.second.get_child("add");
					//pass 'add' subset and pointer to this profile
					loadProfileAdd(ptr, &p);
				}
				catch(...){}

				//Save the profile
				profiles[p.name] = p;

				try //Conditional: has children profiles
				{
					ptr = &v.second.get_child("profiles");

					//start recurisive descent down profile tree with this profile as the root
					//pass subtree and pointer to parent
					loadSubProfiles(p.name);
				}
				catch(...){}

			}

			//Honeyd's implementation of switching templates based on conditions
			else if(!string(v.first.data()).compare("dynamic"))
			{
				//TODO
			}
			else
			{
				LOG4CXX_ERROR(n_logger, "Invalid XML Path" +string(v.first.data()));
			}
		}
	}
	catch(std::exception &e)
	{
		LOG4CXX_ERROR(n_logger, "Problem loading Profiles: "+ string(e.what()));
	}
}

//Sets the configuration of 'set' values for profile that called it
void NovaConfig::loadProfileSet(ptree *ptr, profile *p)
{
	string prefix;
	try
	{
		BOOST_FOREACH(ptree::value_type &v, ptr->get_child(""))
		{
			prefix = "TCP";
			if(!string(v.first.data()).compare(prefix))
			{
				p->tcpAction = v.second.data();
				continue;
			}
			prefix = "UDP";
			if(!string(v.first.data()).compare(prefix))
			{
				p->udpAction = v.second.data();
				continue;
			}
			prefix = "ICMP";
			if(!string(v.first.data()).compare(prefix))
			{
				p->icmpAction = v.second.data();
				continue;
			}
			prefix = "personality";
			if(!string(v.first.data()).compare(prefix))
			{
				p->personality = v.second.data();
				continue;
			}
			prefix = "ethernet";
			if(!string(v.first.data()).compare(prefix))
			{
				p->ethernet = v.second.data();
				continue;
			}
			prefix = "uptime";
			if(!string(v.first.data()).compare(prefix))
			{
				p->uptime = v.second.data();
				continue;
			}
			prefix = "uptimeRange";
			if(!string(v.first.data()).compare(prefix))
			{
				p->uptimeRange = v.second.data();
				continue;
			}
			prefix = "dropRate";
			if(!string(v.first.data()).compare(prefix))
			{
				p->dropRate = v.second.data();
				continue;
			}
			prefix = "DHCP";
			if(!string(v.first.data()).compare(prefix))
			{
				p->DHCP = true;
				continue;
			}
		}
	}
	catch(std::exception &e)
	{
		LOG4CXX_ERROR(n_logger, "Problem loading profile set parameters: "+ string(e.what()));
	}
}

//Adds specified ports and subsystems
// removes any previous port with same number and type to avoid conflicts
void NovaConfig::loadProfileAdd(ptree *ptr, profile *p)
{
	string prefix;
	port * prt;

	try
	{
		BOOST_FOREACH(ptree::value_type &v, ptr->get_child(""))
		{
			//Checks for ports
			prefix = "ports";
			if(!string(v.first.data()).compare(prefix))
			{
				//Iterates through the ports
				BOOST_FOREACH(ptree::value_type &v2, ptr->get_child("ports"))
				{
					prt = &ports[v2.second.data()];

					//Checks inherited ports for conflicts
					for(uint i = 0; i < p->ports.size(); i++)
					{
						//Erase inherited port if a conflict is found
						if(!prt->portNum.compare(ports[p->ports[i]].portNum) && !prt->type.compare(ports[p->ports[i]].type))
						{
							p->ports.erase(p->ports.begin()+i);
						}
					}
					//Add specified port
					p->ports.push_back(prt->portName);
				}
				continue;
			}

			//Checks for a subsystem
			prefix = "subsystem"; //TODO
			if(!string(v.first.data()).compare(prefix))
			{
				continue;
			}
		}
	}
	catch(std::exception &e)
	{
		LOG4CXX_ERROR(n_logger, "Problem loading profile add parameters: "+ string(e.what()));
	}
}

//Recurisve descent down a profile tree, inherits parent, sets values and continues if not leaf.
void NovaConfig::loadSubProfiles(string parent)
{
	ptree ptr = profiles[parent].tree;
	try
	{
		BOOST_FOREACH(ptree::value_type &v, ptr.get_child("profiles"))
		{
			ptree *ptr2;

			//Inherits parent,
			profile prof = profiles[parent];
			prof.tree = v.second;
			prof.parentProfile = parent;

			//Gets name, initializes DHCP
			prof.name = v.second.get<std::string>("name");

			string profileStr = prof.name;
			stringstream ss;
			uint i = 0;

			while(profiles.find(prof.name) != profiles.end())
			{
				ss.str("");
				i++;
				ss << profileStr << "-" << i;
				prof.name = ss.str();
			}

			prof.DHCP = false;

			try //Conditional: If profile has set configurations different from parent
			{
				ptr2 = &v.second.get_child("set");
				loadProfileSet(ptr2, &prof);
			}
			catch(...){}

			try //Conditional: If profile has port or subsystems different from parent
			{
				ptr2 = &v.second.get_child("add");
				loadProfileAdd(ptr2, &prof);
			}
			catch(...){}

			//Saves the profile
			profiles[prof.name] = prof;


			try //Conditional: if profile has children (not leaf)
			{
				loadSubProfiles(prof.name);
			}
			catch(...){}
		}
	}
	catch(std::exception &e)
	{
		LOG4CXX_ERROR(n_logger, "Problem loading sub profiles: "+ string(e.what()));
	}
}
void NovaConfig::loadAllProfiles()
{
	loadingItems = true;
	ui.hsProfileTreeWidget->clear();
	ui.profileTreeWidget->clear();
	ui.hsProfileTreeWidget->sortByColumn(0,Qt::AscendingOrder);
	ui.profileTreeWidget->sortByColumn(0,Qt::AscendingOrder);

	if(profiles.size())
	{
		for(ProfileTable::iterator it = profiles.begin(); it != profiles.end(); it++)
		{
			it->second.item = NULL;
			it->second.profileItem = NULL;
		}
		for(ProfileTable::iterator it = profiles.begin(); it != profiles.end(); it++)
		{
			createProfileItem(&it->second);
		}
		ui.profileTreeWidget->setCurrentItem(profiles[currentProfile].profileItem);
		loadProfile();
		ui.hsProfileTreeWidget->expandAll();
		ui.profileTreeWidget->expandAll();
	}
	else
	{
		currentProfile = "";
	}
	loadingItems = false;
}

void NovaConfig::createProfileItem(profile *p)
{
	if(p->item == NULL)
	{
		QTreeWidgetItem * item = NULL;
		string profileStr = p->name;

		if(p->parentProfile == "")
		{
			/*NOTE*/
			//These items don't need to be deleted because the clear function
			// and destructor of the tree widget does that already.
			item = new QTreeWidgetItem(ui.hsProfileTreeWidget,0);
			item->setText(0, (QString)profileStr.c_str());
			p->item = item;

			//Create the profile item for the profile tree
			item = new QTreeWidgetItem(ui.profileTreeWidget,0);
			item->setText(0, (QString)profileStr.c_str());
			p->profileItem = item;
		}
		else
		{
			if(profiles.find(p->parentProfile) != profiles.end())
			{
				profile * parent = &profiles[p->parentProfile];

				if(parent->item == NULL)
				{
					createProfileItem(parent);
				}
				//*NOTE*
				//These items don't need to be deleted because the clear function
				// and destructor of the tree widget does that already.
				item = new QTreeWidgetItem(parent->item,0);
				item->setText(0, (QString)profileStr.c_str());
				p->item = item;

				//Create the profile item for the profile tree
				item = new QTreeWidgetItem(parent->profileItem,0);
				item->setText(0, (QString)profileStr.c_str());
				p->profileItem = item;
			}
		}
	}
}

void NovaConfig::createProfileTree(profile *p)
{
	ptree temp;
	if(p->name.compare(""))
		temp.put<std::string>("name", p->name);
	if(p->tcpAction.compare(""))
		temp.put<std::string>("set.TCP", p->tcpAction);
	if(p->udpAction.compare(""))
		temp.put<std::string>("set.UDP", p->udpAction);
	if(p->icmpAction.compare(""))
		temp.put<std::string>("set.ICMP", p->icmpAction);
	if(p->personality.compare(""))
		temp.put<std::string>("set.personality", p->personality);
	if(p->ethernet.compare(""))
		temp.put<std::string>("set.ethernet", p->ethernet);
	if(p->uptime.compare(""))
		temp.put<std::string>("set.uptime", p->uptime);
	if(p->uptimeRange.compare(""))
		temp.put<std::string>("set.uptimeRange", p->uptimeRange);
	if(p->dropRate.compare(""))
		temp.put<std::string>("set.dropRate", p->dropRate);
	temp.put<bool>("set.DHCP", p->DHCP);

	ptree pt;
	if(p->ports.size())
	{
		for(uint i = 0; i < p->ports.size(); i++)
		{
			temp.add<std::string>("add.ports.port", p->ports[i]);
		}
	}
	else
	{
		temp.put_child("add.ports",pt);
	}
	temp.put_child("profiles", pt);
	p->tree = temp;
	updateProfileTree(p->name);
}

void NovaConfig::updateProfile(bool deleteProfile, profile * p)
{
	//If the profile is being deleted
	if(deleteProfile)
	{
		for(NodeTable::iterator it = nodes.begin(); it != nodes.end(); it++)
		{
			if(!it->second.pfile.compare(p->name))
			{
				deleteNode(&it->second);
			}
		}
		profiles.erase(p->name);
	}
	//If the profile needs to be updated
	else
	{
		string pfile = p->profileItem->text(0).toStdString();

		//If item text and profile name don't match, we need to update
		if(p->name.compare(pfile))
		{
			//Set the profile to the correct name and put the profile in the table
			profiles[pfile] = *p;
			profiles[pfile].name = pfile;

			//Find all nodes who use this profile and update to the new one
			for(NodeTable::iterator it = nodes.begin(); it != nodes.end(); it++)
			{
				if(it->second.pfile == p->name)
				{
					it->second.pfile = pfile;
					it->second.nodeItem->setText(1, (QString)pfile.c_str());
				}
			}
			//Remove the old profile and update the currentProfile pointer
			profiles.erase(p->name);
			p = &profiles[pfile];
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
			currentProfile = item->text(0).toStdString();
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
		deleteProfile(currentProfile);
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
		ss.str("");
		ss << "New Profile-" << i;
		temp.name = ss.str();
	}
	if(profiles.find(currentProfile) != profiles.end())
		temp.parentProfile = currentProfile;
	else
	{
		temp.parentProfile = "";
		currentProfile = temp.name;
	}
	profiles[temp.name] = temp;
	createProfileTree(&profiles[temp.name]);
	loadAllProfiles();
}
void NovaConfig::on_cloneButton_clicked()
{
	//Do nothing if no profiles
	if(profiles.size())
	{
		QTreeWidgetItem * item = ui.profileTreeWidget->selectedItems().first();
		string profileStr = item->text(0).toStdString();
		profile p = profiles[currentProfile];
		int i = 1;
		stringstream ss;
		ss.str("");
		ss << profileStr << "-" << i;
		p.name = ss.str();
		//Check for name in use, if so increase number
		while(profiles.find(p.name) != profiles.end())
		{
			ss.str("");
			i++;
			ss << profileStr << "-" << i;
			p.name = ss.str();
		}
		//Change the profile name and put in the table, update the current profile
		profiles[p.name] = p;
		loadProfilesFromTree(p.name);
		createProfileTree(&profiles[p.name]);
		loadAllProfiles();
	}
}

void NovaConfig::on_editPortsButton_clicked()
{
	/*editingPorts = true;
	if(!ui.profileTreeWidget->selectedItems().isEmpty())
	{
		QTreeWidgetItem * item = ui.profileTreeWidget->selectedItems().first();
		string profileStr = item->text(0).toStdString();
		struct profile *p = &profiles[profileStr];
		portwindow = new portPopup(this, p, FROM_NOVA_CONFIG, homePath);
		loadAllNodes();
		portwindow->show();
	}
	else
	{
		LOG4CXX_ERROR(n_logger, "No profile selected when opening port edit window.");
	}*/
}

void NovaConfig::on_profileEdit_textChanged()
{
	if(!loadingItems && !profiles.empty())
	{
		profiles[currentProfile].item->setText(0,ui.profileEdit->toPlainText());
		profiles[currentProfile].profileItem->setText(0,ui.profileEdit->toPlainText());
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

			n = &nodes[it->second.nodes[i]];

			//Create the node item for the Haystack tree
			item = new QTreeWidgetItem(it->second.item, 0);
			item->setText(0, (QString)n->address.c_str());
			item->setText(1, (QString)n->pfile.c_str());
			n->item = item;

			//Create the node item for the node edit tree
			item = new QTreeWidgetItem(it->second.nodeItem, 0);
			item->setText(0, (QString)n->address.c_str());
			item->setText(1, (QString)n->pfile.c_str());
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
		if(nodes.find(currentNode) != nodes.end())
			ui.nodeTreeWidget->setCurrentItem(nodes[currentNode].nodeItem);
		else if(subnets.find(currentSubnet) != subnets.end())
			ui.nodeTreeWidget->setCurrentItem(subnets[currentSubnet].nodeItem);
	}
	loadingItems = false;
}

//Function called when delete button
void NovaConfig::deleteNodes()
{
	QTreeWidgetItem * temp = NULL;
	string address = "";
	bool nextIsSubnet;

	loadingItems = true;

	//If a subnet is selected and there's another to select
	if((subnets.size() > 1) && selectedSubnet)
	{
		//Get current subnet index and pre-select another one preferring lower item first
		int tempI = ui.nodeTreeWidget->indexOfTopLevelItem(subnets[currentSubnet].nodeItem);

		//If the current subnet is at the bottom of the list
		if((tempI + 1) == ui.nodeTreeWidget->topLevelItemCount())
		{
			tempI--;
			//Select the subnet above it
			temp = ui.nodeTreeWidget->topLevelItem(tempI);
		}
		else //Select the item below it
		{
			tempI++;
			temp = ui.nodeTreeWidget->topLevelItem(tempI);
		}
		//Save the address since the item will change when we redraw the list
		address = temp->text(0).toStdString();

		//Flag as subnet
		nextIsSubnet = true;
	}
	//If there is at least one other item and we have a node selected
	else if(((nodes.size()+subnets.size()) > 1) && !selectedSubnet)
	{
		//Try to select the bottom item first
		temp = ui.nodeTreeWidget->itemBelow(ui.nodeTreeWidget->selectedItems().first());

		//If that fails select the one above
		if(temp == NULL)
			temp = ui.nodeTreeWidget->itemAbove(ui.nodeTreeWidget->selectedItems().first());

		//Save the address since the item temp points to will change when we redraw the list
		address = temp->text(0).toStdString();

		//If the item isn't top level it is a node and will return -1
		if(ui.nodeTreeWidget->indexOfTopLevelItem(temp) == -1)
			nextIsSubnet = false; //Flag as node

		else nextIsSubnet = true; //Flag as subnet
	}
	//If there are no more items in the list make sure it is clear then return.
	else
	{
		ui.nodeTreeWidget->clear();
		ui.hsNodeTreeWidget->clear();
		subnets.clear_no_resize();
		nodes.clear_no_resize();
		currentSubnet = "";
		currentNode = "";
		return;
	}
	//If we are deleteing a subnet, remove each node first then remove the subnet.
	if(selectedSubnet && currentSubnet.compare(""))
	{
		subnet * s = &subnets[currentSubnet];
		//Get initial size
		uint nodesSize = s->nodes.size();
		//Delete front and get new front until empty
		for(uint i = 0; i < nodesSize; i++)
		{
			currentNode = s->nodes.front();
			if(currentNode.compare(""))
				deleteNode(&nodes[currentNode]);
		}
		//Remove the subnet from the list and delete from table
		ui.nodeTreeWidget->removeItemWidget(s->nodeItem, 0);
		ui.hsNodeTreeWidget->removeItemWidget(s->item, 0);
		subnets.erase(currentSubnet);
	}
	//Delete the selected node
	else
	{
		deleteNode(&nodes[currentNode]);
	}
	loadingItems = true;


	//If we have a node as our new selection, set it as current item
	if(!nextIsSubnet)
	{
		currentNode = address;
		currentSubnet = nodes[currentNode].sub;
		ui.nodeTreeWidget->setCurrentItem(nodes[currentNode].nodeItem);
	}
	//If we have a subnet selected
	else
	{
		currentSubnet = address;
		ui.nodeTreeWidget->setCurrentItem(subnets[currentSubnet].nodeItem);
	}
	loadAllNodes();
	loadingItems = false;
}

// Removes the node from item widgets and data structures.
void NovaConfig::deleteNode(node *n)
{
	ui.nodeTreeWidget->removeItemWidget(n->nodeItem, 0);
	ui.hsNodeTreeWidget->removeItemWidget(n->item, 0);
	subnet * s = &subnets[n->sub];
	for(uint i = 0; i < s->nodes.size(); i++)
	{
		if(s->nodes[i] == n->address)
		{
			s->nodes.erase(s->nodes.begin()+i);
		}
	}
	nodes.erase(n->address);
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
				currentNode = item->text(0).toStdString();
				currentSubnet = "";
				selectedSubnet = false;
			}
			else //If it's a subnet
			{
				currentSubnet = item->text(0).toStdString();
				currentNode = "";
				selectedSubnet = true;
			}
		}
	}
}

//Pops up the node edit window selecting the current node
void NovaConfig::on_nodeEditButton_clicked()
{
	/*if(nodes.size())
	{
		editingNodes = true;
		nodewindow = new nodePopup(this, &nodes[currentNode], EDIT_NODE, homePath);
		loadAllNodes();
		nodewindow->show();
	}*/
}

//Creates a copy of the current node and pops up the edit window
void NovaConfig::on_nodeCloneButton_clicked()
{
	/*if(nodes.size())
	{
		editingNodes = true;
		nodewindow = new nodePopup(this, &nodes[currentNode], CLONE_NODE, homePath);
		loadAllNodes();
		nodewindow->show();
	}*/
}

//Creates a new node and pops up the edit window
void NovaConfig::on_nodeAddButton_clicked()
{
	/*if(nodes.size())
	{
		editingNodes = true;
		nodewindow = new nodePopup(this, &nodes[currentNode], ADD_NODE, homePath);
		loadAllNodes();
		nodewindow->show();
	}*/
}

//Gets item selection then calls the function(s) to remove the node(s)
void NovaConfig::on_nodeDeleteButton_clicked()
{
	if(subnets.size() || nodes.size())
		deleteNodes();
}

//Enables a node or a subnet in honeyd
//TODO Use this flag to comment out nodes whens writing to the honeyd config
//  loading from the file will need to recognize these
void NovaConfig::on_nodeEnableButton_clicked()
{
	if(selectedSubnet)
	{
		subnet * s = &subnets[currentSubnet];
		for(uint i = 0; i < s->nodes.size(); i++)
		{
			nodes[s->nodes[i]].enabled = true;

		}
		s->enabled = true;
	}
	else
	{
		nodes[currentNode].enabled = true;
	}

	//Draw the nodes and restore selection
	loadingItems = true;
	loadAllNodes();
	if(selectedSubnet)
		ui.nodeTreeWidget->setCurrentItem(subnets[currentSubnet].nodeItem);
	else
		ui.nodeTreeWidget->setCurrentItem(nodes[currentNode].nodeItem);
	loadingItems = false;
}

//Disables a node or a subnet in honeyd
//TODO Use this flag to comment out nodes whens writing to the honeyd config
//  loading from the file will need to recognize these
void NovaConfig::on_nodeDisableButton_clicked()
{
	if(selectedSubnet)
	{
		subnet * s = &subnets[currentSubnet];
		for(uint i = 0; i < s->nodes.size(); i++)
		{
			nodes[s->nodes[i]].enabled = false;

		}
		s->enabled = false;
	}
	else
	{
		nodes[currentNode].enabled = false;
	}

	//Draw the nodes and restore selection
	loadingItems = true;
	loadAllNodes();
	if(selectedSubnet)
		ui.nodeTreeWidget->setCurrentItem(subnets[currentSubnet].nodeItem);
	else
		ui.nodeTreeWidget->setCurrentItem(nodes[currentNode].nodeItem);
	loadingItems = false;
}
