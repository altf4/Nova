#include "nodePopup.h"
#include "portPopup.h"
#include "novaconfig.h"
#include "nodePopup.h"
#include <fstream>
#include <signal.h>
#include <log4cxx/xml/domconfigurator.h>

using namespace log4cxx;
using namespace log4cxx::xml;
using namespace std;

//Parent window pointers, allows us to call functions from the parent
NovaConfig * novaParent;
portPopup * portwind;

//Global pointer to the current node
node * gnode = NULL;
subnet * gnet = NULL;
string address;

//Prevents signal clashing/conflicts and resultant seg faults
bool load;
bool subnetSel = false;

LoggerPtr node_logger(Logger::getLogger("main"));

/************************************************
 * Construct and Initialize GUI
 ************************************************/


nodePopup::nodePopup(QWidget *parent, node *n, int type, string home)
    : QMainWindow(parent)
{
	homePath = home;
	ui.setupUi(this);
	DOMConfigurator::configure("Config/Log4cxxConfig.xml");
	novaParent = (NovaConfig *)parent;
	novaParent->setEnabled(false);
	this->setEnabled(true);
	remoteCall = false;

	address = n->address;
	gnode = n;
	pullData();
	gnode = &nodes[address];

	if(type == EDIT_NODE)
	{
	}

	else if(type == ADD_NODE)
	{
	}

	else if(type == CLONE_NODE)
	{
	}
	else
	{
		LOG4CXX_ERROR(node_logger, "Invalid type given in constructor");
		this->close();
	}
	loadAllNodes();
	loadAllNodeProfiles();
}

nodePopup::~nodePopup()
{

}

/************************************************
 * Closing functions
 ************************************************/

void nodePopup::closeEvent(QCloseEvent * e)
{
	e = e;

	if(editingPorts && (portwind != NULL))
	{
		portwind->remoteCall = true;
		portwind->close();
	}

	if(remoteCall) remoteClose();
	else closeWindow();
}

void nodePopup::closeWindow()
{
	novaParent->setEnabled(true);
	novaParent->loadAllProfiles();
	novaParent->loadAllNodes();
	novaParent = NULL;
	gnode = NULL;
}

void nodePopup::remoteClose()
{
	gnode = NULL;
	novaParent = NULL;
}

/************************************************
 * Load and Save changes to the node's profile
 ************************************************/

//saves the changes to a node
void nodePopup::saveNodeProfile()
{
	if(!subnetSel && profiles.size())
	{
		profile * currentProfile = gnode->pfile;
		port * pr;
		QTreeWidgetItem * item;

		//Saves any modifications to the last selected object.
		if(profiles.find(currentProfile->name) != profiles.end())
		{
			gnode->pname = ui.profileEdit->toPlainText().toStdString();
			currentProfile->name = ui.profileEdit->toPlainText().toStdString();
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
}

//loads the selected node's options
void nodePopup::loadNodeProfile()
{
	bool temp = load;
	load = true;
	QTreeWidgetItem *item = NULL;

	if(gnode->pfile != NULL)
	{

		//If subnet is selected
		if(subnetSel)
		{
			ui.profileEdit->clear();
			ui.personalityEdit->clear();
			ui.ethernetEdit->clear();
			ui.ipEdit->clear();
			ui.tcpActionEdit->clear();
			ui.uptimeEdit->clear();
			ui.portTreeWidget->clear();
		}
		//If node is selected
		else
		{
			ui.profileEdit->setText((QString)gnode->pname.c_str());
			ui.personalityEdit->setText((QString)gnode->pfile->personality.c_str());
			ui.ethernetEdit->setText((QString)gnode->pfile->ethernet.c_str());
			ui.ipEdit->setText((QString)gnode->address.c_str());
			ui.tcpActionEdit->setText((QString)gnode->pfile->tcpAction.c_str());
			ui.uptimeEdit->setText((QString)gnode->pfile->uptime.c_str());
			ui.profileTreeWidget->setCurrentItem(gnode->pfile->item);

			ui.portTreeWidget->clear();

			if(!gnode->pfile->ports.empty())
			{
				struct port * prt;
				//Populate the port table
				for(uint i = 0; i < gnode->pfile->ports.size(); i++)
				{
					prt = &gnode->pfile->ports[i];
					item = new QTreeWidgetItem(0);
					item->setText(0,(QString)prt->portNum.c_str());
					item->setText(1,(QString)prt->type.c_str());
					item->setText(2,(QString)prt->behavior.c_str());
					ui.portTreeWidget->insertTopLevelItem(i, item);
					prt->portItem = item;
				}
			}
		}
	}
	load = temp;
}

//reloads all information
void nodePopup::loadAllNodes()
{
	load = true;
	QBrush whitebrush(QColor(255, 255, 255, 255));
	QBrush greybrush(QColor(100, 100, 100, 255));
	greybrush.setStyle(Qt::SolidPattern);
	QBrush blackbrush(QColor(0, 0, 0, 255));
	blackbrush.setStyle(Qt::NoBrush);
	node * n = NULL;

	QTreeWidgetItem * item = NULL;
	ui.nodeTreeWidget->clear();

	for(SubnetTable::iterator it = subnets.begin(); it != subnets.end(); it++)
	{
		//create the subnet item for the node  tree
		item = new QTreeWidgetItem(ui.nodeTreeWidget, 0);
		item->setText(0, (QString)it->second.address.c_str());
		it->second.item = item;
		if(!it->second.enabled && this->isEnabled())
		{
			whitebrush.setStyle(Qt::NoBrush);
			it->second.item->setBackground(0,greybrush);
			it->second.item->setForeground(0,whitebrush);
		}
		for(uint i = 0; i < it->second.nodes.size(); i++)
		{

			n = it->second.nodes[i];

			//Create the node item for the Haystack tree
			item = new QTreeWidgetItem(it->second.item, 0);
			item->setText(0, (QString)n->address.c_str());
			n->item = item;


			if(!n->enabled && this->isEnabled())
			{
				whitebrush.setStyle(Qt::NoBrush);
				n->item->setBackground(0,greybrush);
				n->item->setForeground(0,whitebrush);
			}
		}
	}
	gnode = &nodes[address];
	ui.nodeTreeWidget->expandAll();
	ui.nodeTreeWidget->setCurrentItem(gnode->item);
	load = false;
}

//Loads all the profile information
void nodePopup::loadAllNodeProfiles()
{
	load = true;
	string profileStr;
	QTreeWidgetItem * item = NULL;
	ui.profileTreeWidget->clear();

	if(profiles.size())
	{
		for(ProfileTable::iterator it = profiles.begin(); it != profiles.end(); it++)
		{
			profileStr = it->second.name;
			item = new QTreeWidgetItem(ui.profileTreeWidget,0);
			item->setText(0, (QString)profileStr.c_str());
			it->second.item = item;
		}
		ui.profileTreeWidget->setCurrentItem(gnode->pfile->item);
		loadNodeProfile();
	}
	load = false;
}
/************************************************
 * Data Transfer Functions
 ************************************************/

//Copies the data from parent novaconfig and adjusts the pointers
void nodePopup::pullData()
{
	subnets.clear();
	nodes.clear();
	profiles.clear();
	if(novaParent->subnets.size()) subnets = novaParent->subnets;
	if(novaParent->nodes.size()) nodes = novaParent->nodes;
	if(novaParent->profiles.size()) profiles = novaParent->profiles;

	//Adjust subnet pointers
	for(SubnetTable::iterator it = subnets.begin(); it != subnets.end(); it++)
	{

		//Adjust pointers for
		for(uint i = 0; i < it->second.nodes.size(); i++)
		{
			//point to new memory location of nodes
			it->second.nodes[i] = &nodes[it->second.nodes[i]->address];
		}
	}

	//Adjust node pointers
	for(NodeTable::iterator it = nodes.begin(); it != nodes.end(); it++)
	{
		//point each node back to it's subnet and profile
		it->second.sub = &subnets[it->second.sub->address];
		it->second.pfile = &profiles[it->second.pname];
	}

	//Adjust profile pointers
	for(ProfileTable::iterator it = profiles.begin(); it != profiles.end(); it++)
	{

	}

	//adjusts current node pointer
}

//Copies the data to parent novaconfig and adjusts the pointers
void nodePopup::pushData()
{
	novaParent->subnets.clear();
	novaParent->nodes.clear();
	novaParent->profiles.clear();
	if(subnets.size()) novaParent->subnets = subnets;
	if(nodes.size()) novaParent->nodes = nodes;
	if(profiles.size()) novaParent->profiles = profiles;

	//Adjust subnet pointers
	for(SubnetTable::iterator it = novaParent->subnets.begin(); it != novaParent->subnets.end(); it++)
	{

		//Adjust vector of node pointers
		for(uint i = 0; i < it->second.nodes.size(); i++)
		{
			//point to new memory location of nodes
			it->second.nodes[i] = &novaParent->nodes[it->second.nodes[i]->address];
		}
	}

	//Adjust node pointers
	for(NodeTable::iterator it = novaParent->nodes.begin(); it != novaParent->nodes.end(); it++)
	{
		//point each node back to it's subnet and profile
		it->second.sub = &novaParent->subnets[it->second.sub->address];
		it->second.pfile = &novaParent->profiles[it->second.pname];
	}

	//Adjust profile pointers
	for(ProfileTable::iterator it = novaParent->profiles.begin(); it != novaParent->profiles.end(); it++)
	{

	}
}

/************************************************
 * General GUI Signal Handling
 ************************************************/

//Changes the current node selection
void nodePopup::on_nodeTreeWidget_itemSelectionChanged()
{
	if(nodes.size() && !load)
	{
		if(!ui.nodeTreeWidget->selectedItems().isEmpty())
		{
			saveNodeProfile();
			QTreeWidgetItem * item = ui.nodeTreeWidget->selectedItems().first();
			address = item->text(0).toStdString();
			//If item is a node
			if(ui.nodeTreeWidget->indexOfTopLevelItem(item) == -1)
			{
				subnetSel = false;
				gnode = &nodes[address];
				gnet = gnode->sub;
				load = true;
				loadNodeProfile();
				load = false;
			}
			else
			{
				gnet = &subnets[address];
				if(gnet->nodes.size())
					gnode = gnet->nodes.front();
				else gnode = NULL;
				subnetSel = true;
				load = true;
				loadNodeProfile();
				load = false;

			}
		}
	}
}
//Changes the profile selection
void nodePopup::on_profileTreeWidget_itemSelectionChanged()
{
	if(profiles.size() && !load)
	{
		if(!ui.profileTreeWidget->selectedItems().isEmpty() && !subnetSel)
		{
			QTreeWidgetItem * item = ui.profileTreeWidget->selectedItems().first();
			string name = item->text(0).toStdString();
			gnode->pname = name;
			gnode->pfile = &profiles[name];
			load = true;
			loadNodeProfile();
			load = false;
		}
	}
}

//Closes the window without saving any changes
void nodePopup::on_cancelButton_clicked()
{
	//Enable the parent window and show any previously saved changes
	this->close();
}

//Saves the modified ports to the profile
void nodePopup::on_okButton_clicked()
{
	//Save the recent changes in window then push all changes
	if(!nodes.empty()) saveNodeProfile();
	pushData();
	novaParent->updatePointers();

	//Enable the parent window and show all changes*/
	this->close();
}

void nodePopup::on_defaultsButton_clicked()
{
	pullData();
	loadAllNodes();
	loadAllNodeProfiles();
}

void nodePopup::on_applyButton_clicked()
{
	//Save the recent changes in window then push all changes
	if(!nodes.empty()) saveNodeProfile();
	pushData();
	novaParent->updatePointers();
}

/************************************************
 * Node GUI Button Handling
 ************************************************/

void nodePopup::on_addButton_clicked()
{

}

void nodePopup::on_deleteButton_clicked()
{

}

void nodePopup::on_enableButton_clicked()
{
	if(subnetSel)
	{
		for(uint i = 0; i < gnet->nodes.size(); i++)
		{
			gnet->nodes[i]->enabled = true;

		}
		gnet->enabled = true;
	}
	else
	{
		gnode->enabled = true;
	}

	//Draw the nodes and restore selection
	load = true;
	loadAllNodes();
	if(subnetSel)
		ui.nodeTreeWidget->setCurrentItem(gnet->nodeItem);
	else
		ui.nodeTreeWidget->setCurrentItem(gnode->nodeItem);
	load = false;
}

void nodePopup::on_disableButton_clicked()
{
	if(subnetSel)
	{
		for(uint i = 0; i < gnet->nodes.size(); i++)
		{
			gnet->nodes[i]->enabled = false;

		}
		gnet->enabled = false;
	}
	else
	{
		gnode->enabled = false;
	}

	//Draw the nodes and restore selection
	load = true;
	loadAllNodes();
	if(subnetSel)
		ui.nodeTreeWidget->setCurrentItem(gnet->nodeItem);
	else
		ui.nodeTreeWidget->setCurrentItem(gnode->nodeItem);
	load = false;
}

void nodePopup::on_cloneButton_clicked()
{

}

void nodePopup::on_editPortsButton_clicked()
{
	editingPorts = true;
	if(!ui.profileTreeWidget->selectedItems().isEmpty() && !subnetSel)
	{
		portwind = new portPopup(this, gnode->pfile, FROM_NODE_CONFIG, homePath);
		loadAllNodes();
		portwind->show();
	}
	else
	{
		LOG4CXX_ERROR(node_logger, "No profile selected when opening port edit window.");
	}
}
