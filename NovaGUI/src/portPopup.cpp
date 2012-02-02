//============================================================================
// Name        : portPopup.cpp
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
// Description : 
//============================================================================
#include "portPopup.h"
#include "novaconfig.h"
#include "nodePopup.h"
#include <fstream>
#include <signal.h>

using namespace std;


/************************************************
 * Global Variables
 ************************************************/

//Global copy of parent's ports
// - used to temporarily store changes until the user commits them
vector<string> ports;

//Global port - used to store the currently selected port
struct port * gprt = NULL;

//Global profile - allows us access to the parent profile
struct profile * p = NULL;

//Parent window pointers, allows us to call functions from the parent
NovaConfig * profileMenu;
nodePopup * nodeMenu;

//Global item - Used to store the currently selected item
QTreeWidgetItem *citem;

//When the port number text is edited, we update the edit in real time in the item.
//This flag prevents that signal from executing when we load the port information
// from local storage which causes a conflict and results in a segfault.
bool loading = true;

//Flag to distinguish which type of window is the parent so proper casting can take place
bool fromNodeMenu;


/************************************************
 * Construct and Initialize GUI
 ************************************************/

portPopup::portPopup(QWidget *parent, struct profile *profile, bool fromNode, string home)
    : QMainWindow(parent)
{
	//Configure Logging and set up window
	homePath = home;
	ui.setupUi(this);
	openlog("NovaGUI", OPEN_SYSL, LOG_AUTHPRIV);
	remoteCall = false;

	fromNodeMenu = fromNode;

	//Cast the parent point to the correct object
	if(fromNodeMenu)nodeMenu = (nodePopup *)parent;
	else profileMenu = (NovaConfig *)parent;

	try
	{
		//save any modifications before the port window opened
		if(fromNodeMenu && (nodeMenu != NULL))
		{
			nodeMenu->saveNodeProfile();
			nodeMenu->setEnabled(false);
			this->setEnabled(true);
		}
		else if(profileMenu != NULL)
		{
			profileMenu->saveProfile();
			profileMenu->setEnabled(false);
			this->setEnabled(true);
		}
		else
		{
			syslog(SYSL_ERR, "File: %s Line: %d Parent pointer is NULL", __FILE__, __LINE__);
			this->close();
		}
	}
	catch(...)
	{
		syslog(SYSL_ERR, "File: %s Line: %d Unable to cast parent pointer into expected object", __FILE__, __LINE__);
		this->close();
	}

	//assign the global profile
	p = profile;
	//Copy the ports
	//ports = p->ports;

	//Set up label
	string temp = "Ports for ";
	temp += p->name;
	ui.nameLabel->setText((QString)temp.c_str());

	if(!ports.empty())
	{
		//Display first port by default
		//gprt = ports[0];
		//Load all ports from profile
		loadAllPorts();
	}
}

portPopup::~portPopup()
{

}


/************************************************
 * Closing functions
 ************************************************/

void portPopup::closeEvent(QCloseEvent * e)
{
	e = e;
	if(remoteCall) remoteClose();
	else closeWindow();
}

void portPopup::closeWindow()
{
	if(fromNodeMenu)
	{
		nodeMenu->editingPorts = false;
		nodeMenu->setEnabled(true);
		nodeMenu->loadAllNodes();
		nodeMenu->loadAllNodeProfiles();
	}
	else
	{
		profileMenu->editingPorts = false;
		profileMenu->setEnabled(true);
		profileMenu->loadProfile();
	}
	profileMenu = NULL;
	nodeMenu = NULL;
	p = NULL;
}

void portPopup::remoteClose()
{
	profileMenu = NULL;
	nodeMenu = NULL;
	p = NULL;
}


/************************************************
 * Loading port preferences from local storage
 ************************************************/

//Restores the ports to their previous values
void portPopup::loadAllPorts()
{
	//Create items
	QTreeWidgetItem *item = NULL;
	if(!ports.empty())
	{
		ui.typeComboBox->setEnabled(true);
		//Loads the current selection then redraws the table
		loadPort(gprt);

		ui.portTreeWidget->clear();

		//Populate the port table
		struct port * prt;
		for(uint i = 0; i < ports.size(); i++)
		{
			//prt = ports[i];
			item = new QTreeWidgetItem(0);
			item->setText(0,(QString)prt->portNum.c_str());
			ui.portTreeWidget->insertTopLevelItem(i, item);
			//prt->portItem = item;
		}
		//set the current item
		//citem = gprt->portItem;
		ui.portTreeWidget->setCurrentItem(citem);
	}
	else
	{
		citem = NULL;
		gprt = NULL;
		ui.portNumberEdit->clear();
		ui.typeComboBox->setEnabled(false);
		ui.behaviorEdit->clear();
		ui.portTreeWidget->clear();
	}
}

//Loads from a port object
void portPopup::loadPort(struct port* prt)
{
	//Flag to prevent conflict by signals sent during loading of portNumber field
	loading = true;
	ui.portNumberEdit->setText((QString)prt->portNum.c_str());
	loading = false;

	ui.behaviorEdit->setText((QString)(prt->behavior.c_str()));

	//If type is tcp set index to tcp index, else set to udp index
	if(!prt->type.compare("tcp")) ui.typeComboBox->setCurrentIndex(0);
	else ui.typeComboBox->setCurrentIndex(1);
}

/************************************************
 * Save port preferences to local storage
 ************************************************/

//Saves changes to a port object
void portPopup::savePort(struct port* prt)
{
	gprt->portNum = ui.portNumberEdit->toPlainText().toStdString();
	prt->behavior = ui.behaviorEdit->toPlainText().toStdString();

	if(ui.typeComboBox->currentIndex() == 0) gprt->type = "tcp";
	else gprt->type = "udp";
}

/************************************************
 * General GUI Signal Handling
 ************************************************/

//Switches selections
void portPopup::on_portTreeWidget_itemSelectionChanged()
{
	if(!ui.portTreeWidget->selectedItems().isEmpty())
	{
		//save modifications to last selected port
		savePort(gprt);

		//Switch to new port
		citem = ui.portTreeWidget->selectedItems().first();
		//gprt = ports[ui.portTreeWidget->indexOfTopLevelItem(citem)];

		//Load port preferences
		loadPort(gprt);
	}
}

//Closes the window without saving any changes
void portPopup::on_cancelButton_clicked()
{
	//Enable the parent window and show any previously saved changes
	this->close();
}

//Saves the modified ports to the profile
void portPopup::on_okButton_clicked()
{
	//Save the recent changes in window then push all changes over
	if(!ports.empty()) savePort(gprt);
	p->ports.clear();
	//p->ports = ports;
	if(fromNodeMenu)
	{

	}
	else
	{
		ptree * pt = &p->tree.get_child("add.ports");
		pt->clear();
		for(uint i = 0; i < ports.size(); i++)
		{
			//pt->add("port", ports[i]->portName);
		}
		//profileMenu->updateProfileTree(p);
	}

	//Enable the parent window and show all changes
	this->close();
}

void portPopup::on_defaultsButton_clicked()
{
	// int i = 0;
	//Pull the original ports
	//ports = p->ports;

	// TODO: Is this code deprecated or being worked on?
	// Commented to eliminate some compiler warnings about unused vars
	/*
	//gets index of current item
	if(!ports.empty())
	{
		if(ui.portTreeWidget->topLevelItemCount() > 0)
		{
			i = ui.portTreeWidget->indexOfTopLevelItem(citem);
		}
		else
		{
			i = 0;
		}
		//gprt = ports[i];
	}
	*/
	loadAllPorts();
}

void portPopup::on_applyButton_clicked()
{
	//Save the recent changes in window then push all changes
	if(!ports.empty()) savePort(gprt);
	p->ports.clear();
	//p->ports = ports;
	if(fromNodeMenu)
	{
		nodeMenu->loadNodeProfile();
	}
	else
	{
		ptree * pt = &p->tree.get_child("add.ports");
		pt->clear();
		for(uint i = 0; i < ports.size(); i++)
		{
			//pt->add("port", ports[i]->portName);
		}
		//profileMenu->updateProfileTree(p);
		profileMenu->loadProfile();
	}
}


/************************************************
 * Port Specific GUI Signal Handling
 ************************************************/

//When the user changes the text edit it updates the item in the list.
//This checks the loading flag because this signal is activated when the program loads the field
//When this occurs it causes a seg fault due to the simultaneous access of data.
void portPopup::on_portNumberEdit_textChanged()
{
	if(!loading && !ports.empty()) citem->setText(0,ui.portNumberEdit->toPlainText());
}

void portPopup::on_deleteButton_clicked()
{
	//Do nothing if no ports
	if(!ports.empty())
	{
		//Get index of selected port and remove it
		uint i = ui.portTreeWidget->indexOfTopLevelItem(citem);
		ui.portTreeWidget->removeItemWidget(citem,0);
		//ports[i] = NULL;
		ports.erase(ports.begin()+i);

		//If ports still exist
		if(!ports.empty())
		{
			//If the port deleted was the bottom item, reduce index one
			if(i == ports.size()) i--;

			//Set global port
			//gprt = ports[i];
		}

		//Update the GUI
		loadAllPorts();
	}
}

void portPopup::on_addButton_clicked()
{
	struct port temp;
	temp.behavior = "open";
	temp.portNum = "0";
	temp.type = "tcp";

	if(!ports.empty())
	{
		//Get current index and insert copy just after it.
		uint i = ui.portTreeWidget->indexOfTopLevelItem(citem);
		i++;
		//ports.insert((ports.begin()+i),(&temp));
		//gprt = ports[i];
	}
	else
	{
		//ports.push_back(&temp);
		//gprt = ports[0];
	}
	loadAllPorts();
}
void portPopup::on_cloneButton_clicked()
{
	//Do nothing if no ports
	if(!ports.empty())
	{
		//Get current index and insert copy just after it.
		uint i = ui.portTreeWidget->indexOfTopLevelItem(citem);
		i++;
		//ports.insert((ports.begin()+i),(gprt));
		//gprt = ports[i];
		loadAllPorts();
	}
}
