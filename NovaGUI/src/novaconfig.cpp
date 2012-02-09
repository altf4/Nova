//============================================================================
// Name        : novaconfig.cpp
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
// Description : NOVA preferences/configuration window
//============================================================================
#include "novaconfig.h"
#include <QtGui/QComboBox>

using namespace std;
using namespace Nova;

//Keys used to maintain and lookup current selections
string currentProfile = "";
string currentNode = "";
string currentSubnet = "";

nodePopup * nodewindow;
NovaGUI * mainwindow;
QMenu * portMenu;
QMenu * profileTreeMenu;
QMenu * nodeTreeMenu;

//flag to avoid GUI signal conflicts
bool loadingItems, editingItems = false;
bool selectedSubnet = false;
bool loadingDefaultActions = false;

/************************************************
 * Construct and Initialize GUI
 ************************************************/

NovaConfig::NovaConfig(QWidget *parent, string home)
    : QMainWindow(parent)
{
	portMenu = new QMenu();
	profileTreeMenu = new QMenu();
	nodeTreeMenu = new QMenu();

	//store current directory / base path for Nova
	homePath = home;

	//Initialize hash tables
	MACVendorTable.set_empty_key(16777216); //2^24, invalid MAC prefix.
	VendorMACTable.set_empty_key("");
	subnets.set_empty_key("");
	subnets.set_deleted_key("DELETED");
	nodes.set_empty_key("");
	nodes.set_deleted_key("DELETED");
	profiles.set_empty_key("");
	profiles.set_deleted_key("DELETED");
	ports.set_empty_key("");
	ports.set_deleted_key("DELETED");
	scripts.set_empty_key("");
	scripts.set_deleted_key("DELETED");

	//Store parent and load UI
	mainwindow = (NovaGUI*)parent;
	group = mainwindow->group;

	// Set up the GUI
	ui.setupUi(this);
	setInputValidators();

	editingPorts = false;

	//Read NOVAConfig, pull honeyd info from parent, populate GUI
	loadPreferences();
	pullData();
	loadHaystack();
	loadMACPrefixs();
	loadNmapPersonalities();

	// Populate the dialog menu
	for (uint i = 0; i < mainwindow->prompter->registeredMessageTypes.size(); i++)
	{
		ui.msgTypeListWidget->insertItem(i, new QListWidgetItem(QString::fromStdString(
				mainwindow->prompter->registeredMessageTypes[i].descriptionUID)));
	}

	ui.treeWidget->expandAll();

	ui.featureList->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui.featureList, SIGNAL(customContextMenuRequested(const QPoint &)), this,
			SLOT(onFeatureClick(const QPoint &)));
}

NovaConfig::~NovaConfig()
{

}

void NovaConfig::contextMenuEvent(QContextMenuEvent * event)
{
	if(ui.portTreeWidget->hasFocus() || ui.portTreeWidget->underMouse())
	{
		portMenu->clear();
		portMenu->addAction(ui.actionAddPort);
		if(ui.portTreeWidget->topLevelItemCount())
		{
			portMenu->addSeparator();
			portMenu->addAction(ui.actionToggle_Inherited);
			portMenu->addAction(ui.actionDeletePort);
		}
		QPoint globalPos = event->globalPos();
		portMenu->popup(globalPos);
	}
	else if (ui.profileTreeWidget->hasFocus() || ui.profileTreeWidget->underMouse())
	{
		profileTreeMenu->clear();
		profileTreeMenu->addAction(ui.actionProfileAdd);
		profileTreeMenu->addSeparator();
		profileTreeMenu->addAction(ui.actionProfileClone);
		profileTreeMenu->addSeparator();
		profileTreeMenu->addAction(ui.actionProfileDelete);

		QPoint globalPos = event->globalPos();
		profileTreeMenu->popup(globalPos);
	}
	else if (ui.nodeTreeWidget|| ui.nodeTreeWidget->underMouse())
	{
		nodeTreeMenu->clear();
		nodeTreeMenu->addAction(ui.actionNodeEdit);
		nodeTreeMenu->addSeparator();
		nodeTreeMenu->addAction(ui.actionNodeAdd);
		nodeTreeMenu->addAction(ui.actionNodeClone);
		nodeTreeMenu->addAction(ui.actionNodeDelete);
		nodeTreeMenu->addSeparator();
		nodeTreeMenu->addAction(ui.actionNodeEnable);
		nodeTreeMenu->addAction(ui.actionNodeDisable);

		QPoint globalPos = event->globalPos();
		nodeTreeMenu->popup(globalPos);
	}
}

void NovaConfig::on_actionNo_Action_triggered()
{
	return;
}
void NovaConfig::on_actionToggle_Inherited_triggered()
{
	if(!loadingItems && !ui.portTreeWidget->selectedItems().empty())
	{
		loadingItems = true;
		port * prt = NULL;
		for(PortTable::iterator it = ports.begin(); it != ports.end(); it++)
		{
			if(ui.portTreeWidget->currentItem() == it->second.item)
			{
				//iterators are copies not the actual items
				prt = &ports[it->second.portName];
				break;
			}
		}
		profile * p = &profiles[currentProfile];
		for(uint i = 0; i < p->ports.size(); i++)
		{
			if(!p->ports[i].first.compare(prt->portName))
			{
				//If the port is inherited we can just make it explicit
				if(p->ports[i].second)
					p->ports[i].second = false;

				//If the port isn't inherited and the profile has parents
				else if(p->parentProfile.compare(""))
				{
					profile * parent = &profiles[p->parentProfile];
					uint j = 0;
					//check for the inherited port
					for(j = 0; j < parent->ports.size(); j++)
					{
						port temp = ports[parent->ports[j].first];
						if(!prt->portNum.compare(temp.portNum) && !prt->type.compare(temp.type))
						{
							p->ports[i].first = temp.portName;
							p->ports[i].second = true;
						}
					}
					if(!p->ports[i].second)
					{
						loadingItems = false;
						mainwindow->prompter->DisplayPrompt(mainwindow->CANNOT_INHERIT_PORT, "The selected port cannot be inherited"
								" from any ancestors, would you like to keep it?", ui.actionNo_Action, ui.actionDeletePort, this);
						loadingItems = true;
					}
					//TODO display prompt that allows the user to delete the port or do nothing if port isn't found
				}
				//If the port isn't inherited and the profile has no parent
				else
				{
					syslog(SYSL_ERR, "File: %s Line: %d Cannot inherit without any ancestors.", __FILE__, __LINE__);
					mainwindow->prompter->DisplayPrompt(mainwindow->NO_ANCESTORS, "Cannot inherit without any ancestors.");
				}
				break;
			}
		}
		loadProfile();
		saveProfile();
		loadingItems = false;
	}
}

void NovaConfig::on_actionAddPort_triggered()
{
	if(!loadingItems)
	{
		if(profiles.find(currentProfile) != profiles.end())
		{
			loadingItems = true;
			profile p = profiles[currentProfile];

			port pr;
			pr.portNum = "0";
			pr.type = "TCP";
			pr.behavior = "open";
			pr.portName = "0_TCP_open";
			pr.scriptName = "";

			//These don't need to be deleted because the clear function
			// and destructor of the tree widget does that already.
			QTreeWidgetItem * item = new QTreeWidgetItem(0);
			item->setText(0,(QString)pr.portNum.c_str());
			item->setText(1,(QString)pr.type.c_str());
			if(!pr.behavior.compare("script"))
				item->setText(2, (QString)pr.scriptName.c_str());
			else
				item->setText(2,(QString)pr.behavior.c_str());
			ui.portTreeWidget->addTopLevelItem(item);

			TreeItemComboBox *typeBox = new TreeItemComboBox(this, item);
			typeBox->addItem("TCP");
			typeBox->addItem("UDP");
			typeBox->setItemText(0, "TCP");
			typeBox->setItemText(1, "UDP");
			connect(typeBox, SIGNAL(notifyParent(QTreeWidgetItem *, bool)), this, SLOT(portTreeWidget_comboBoxChanged(QTreeWidgetItem *, bool)));

			TreeItemComboBox *behaviorBox = new TreeItemComboBox(this, item);
			behaviorBox->addItem("reset");
			behaviorBox->addItem("open");
			behaviorBox->addItem("block");
			behaviorBox->insertSeparator(3);
			for(ScriptTable::iterator it = scripts.begin(); it != scripts.end(); it++)
			{
				behaviorBox->addItem((QString)it->first.c_str());
			}
			connect(behaviorBox, SIGNAL(notifyParent(QTreeWidgetItem *, bool)), this, SLOT(portTreeWidget_comboBoxChanged(QTreeWidgetItem *, bool)));

			item->setFlags(item->flags() | Qt::ItemIsEditable);

			typeBox->setCurrentIndex(typeBox->findText(pr.type.c_str()));

			behaviorBox->setCurrentIndex(behaviorBox->findText(pr.behavior.c_str()));

			ui.portTreeWidget->setItemWidget(item, 1, typeBox);
			ui.portTreeWidget->setItemWidget(item, 2, behaviorBox);
			pr.item = item;
			ui.portTreeWidget->setCurrentItem(pr.item);
			pair<string, bool> portPair;
			portPair.first = pr.portName;
			portPair.second = false;

			bool conflict = false;
			for(uint i = 0; i < p.ports.size(); i++)
			{
				port temp = ports[p.ports[i].first];
				if(!pr.portNum.compare(temp.portNum) && !pr.type.compare(temp.type))
					conflict = true;
			}
			if(!conflict)
			{
				p.ports.insert(p.ports.begin(),portPair);
				ports[pr.portName] = pr;
				profiles[p.name] = p;

				portPair.second = true;
				vector<profile> profList;
				for(ProfileTable::iterator it = profiles.begin(); it != profiles.end(); it++)
				{
					profile ptemp = it->second;
					while(ptemp.parentProfile.compare("") && ptemp.parentProfile.compare(p.name))
					{
						ptemp = profiles[ptemp.parentProfile];
					}
					if(!ptemp.parentProfile.compare(p.name))
					{
						ptemp = it->second;
						conflict = false;
						for(uint i = 0; i < ptemp.ports.size(); i++)
						{
							port temp = ports[ptemp.ports[i].first];
							if(!pr.portNum.compare(temp.portNum) && !pr.type.compare(temp.type))
								conflict = true;
						}
						if(!conflict)
							ptemp.ports.insert(ptemp.ports.begin(),portPair);
					}
					profiles[ptemp.name] = ptemp;
				}
			}
			loadProfile();
			saveProfile();
			ui.portTreeWidget->editItem(ports[pr.portName].item, 0);
			loadingItems = false;
		}
	}
}

void NovaConfig::on_actionDeletePort_triggered()
{
	if(!loadingItems && !ui.portTreeWidget->selectedItems().empty())
	{
		loadingItems = true;
		port * prt = NULL;
		for(PortTable::iterator it = ports.begin(); it != ports.end(); it++)
		{
			if(ui.portTreeWidget->currentItem() == it->second.item)
			{
				//iterators are copies not the actual items
				prt = &ports[it->second.portName];
				break;
			}
		}
		profile * p = &profiles[currentProfile];
		uint i;
		for(i = 0; i < p->ports.size(); i++)
		{
			if(!p->ports[i].first.compare(prt->portName))
			{
				//Check for inheritance on the deleted port.
				//If valid parent
				if(p->parentProfile.compare(""))
				{
					profile * parent = &profiles[p->parentProfile];
					bool matched = false;
					//check for the inherited port
					for(uint j = 0; j < parent->ports.size(); j++)
					{
						if((!prt->type.compare(ports[parent->ports[j].first].type))
								&& (!prt->portNum.compare(ports[parent->ports[j].first].portNum)))
						{
							p->ports[i].second = true;
							p->ports[i].first = parent->ports[j].first;
							matched = true;
						}
					}
					if(!matched)
						p->ports.erase(p->ports.begin()+i);
				}
				//If no parent.
				else
					p->ports.erase(p->ports.begin()+i);

				//Check for children with inherited port.
				for(ProfileTable::iterator it = profiles.begin(); it != profiles.end(); it++)
					if(!it->second.parentProfile.compare(p->name))
						for(uint j = 0; j < it->second.ports.size(); j++)
						{
							if(!it->second.ports[j].first.compare(prt->portName) && it->second.ports[j].second)
							{
								it->second.ports.erase(it->second.ports.begin()+j);
								break;
							}
						}
				break;
			}
			if(i == p->ports.size())
			{
				syslog(SYSL_ERR, "File: %s Line: %d Port %s could not be found in profile %s",__FILE__,
						__LINE__, prt->portName.c_str(),p->name.c_str());
				mainwindow->prompter->DisplayPrompt(mainwindow->CANNOT_DELETE_PORT, "Port " + prt->portName
						+ " cannot be found.");
			}
		}
		loadProfile();
		saveProfile();
		loadingItems = false;
	}
}

//Action to take when window is closing
void NovaConfig::closeEvent(QCloseEvent * e)
{
	e = e;
	if(editingNodes && (nodewindow != NULL))
	{
		nodewindow->remoteCall = true;
		nodewindow->close();
	}
	mainwindow->editingPreferences = false;

}

void NovaConfig::on_msgTypeListWidget_currentRowChanged()
{
	loadingDefaultActions = true;
	int item = ui.msgTypeListWidget->currentRow();

	ui.defaultActionListWidget->clear();
	ui.defaultActionListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	QListWidgetItem *listItem;

	switch (mainwindow->prompter->registeredMessageTypes[item].type)
	{
	case notifyPrompt:
	case warningPrompt:
	case errorPrompt:
		listItem = new QListWidgetItem("Always Show");
		ui.defaultActionListWidget->insertItem(0, listItem);
		if (mainwindow->prompter->registeredMessageTypes[item].action == CHOICE_SHOW)
			listItem->setSelected(true);

		listItem = new QListWidgetItem("Always Hide");
		ui.defaultActionListWidget->insertItem(1, listItem);
		if (mainwindow->prompter->registeredMessageTypes[item].action == CHOICE_HIDE)
			listItem->setSelected(true);
		break;

	case warningPreventablePrompt:
	case notifyActionPrompt:
	case warningActionPrompt:
		listItem = new QListWidgetItem("Always Show");
		ui.defaultActionListWidget->insertItem(0, listItem);
		if (mainwindow->prompter->registeredMessageTypes[item].action == CHOICE_SHOW)
			listItem->setSelected(true);

		listItem = new QListWidgetItem("Always Yes");
		ui.defaultActionListWidget->insertItem(1, listItem);
		if (mainwindow->prompter->registeredMessageTypes[item].action == CHOICE_DEFAULT)
			listItem->setSelected(true);

		listItem = new QListWidgetItem("Always No");
		ui.defaultActionListWidget->insertItem(2, listItem);
		if (mainwindow->prompter->registeredMessageTypes[item].action == CHOICE_ALT)
			listItem->setSelected(true);
		break;
	case notSet:
		break;
	}

	loadingDefaultActions = false;
}

void NovaConfig::on_defaultActionListWidget_currentRowChanged()
{
	// If we're still populating the list
	if (loadingDefaultActions)
		return;

	openlog("NovaGUI", OPEN_SYSL, LOG_AUTHPRIV);
	QString selected = 	ui.defaultActionListWidget->currentItem()->text();
	messageHandle msgType = (messageHandle)ui.msgTypeListWidget->currentRow();

	if (!selected.compare("Always Show"))
		mainwindow->prompter->SetDefaultAction(msgType, CHOICE_SHOW);
	else if (!selected.compare("Always Hide"))
		mainwindow->prompter->SetDefaultAction(msgType, CHOICE_HIDE);
	else if (!selected.compare("Always Yes"))
		mainwindow->prompter->SetDefaultAction(msgType, CHOICE_DEFAULT);
	else if (!selected.compare("Always No"))
		mainwindow->prompter->SetDefaultAction(msgType, CHOICE_ALT);
	else
		syslog(SYSL_ERR, "File: %s Line: %d Invalid user dialog default action selected, shouldn't get here",
				__FILE__, __LINE__);

	closelog();
}

// Feature enable/disable stuff
void NovaConfig::advanceFeatureSelection()
{
	int nextRow = ui.featureList->currentRow() + 1;
	if (nextRow >= ui.featureList->count())
	{
		nextRow = 0;
	}
	ui.featureList->setCurrentRow(nextRow);
}

void NovaConfig::on_featureEnableButton_clicked()
{
	updateFeatureListItem(ui.featureList->currentItem(), '1');
	advanceFeatureSelection();
}

void NovaConfig::on_featureDisableButton_clicked()
{
	updateFeatureListItem(ui.featureList->currentItem(), '0');
	advanceFeatureSelection();
}

//Inheritance Check boxes
void NovaConfig::on_ipModeCheckBox_stateChanged()
{
	if(!loadingItems)
	{
		loadingItems = true;
		saveProfile();
		loadProfile();
		loadingItems = false;
	}
}

void NovaConfig::on_ethernetCheckBox_stateChanged()
{
	if(!loadingItems)
	{
		loadingItems = true;
		saveProfile();
		loadProfile();
		loadingItems = false;
	}
}

void NovaConfig::on_uptimeCheckBox_stateChanged()
{
	if(!loadingItems)
	{
		loadingItems = true;
		saveProfile();
		loadProfile();
		loadingItems = false;
	}
}

void NovaConfig::on_personalityCheckBox_stateChanged()
{
	if(!loadingItems)
	{
		loadingItems = true;
		saveProfile();
		loadProfile();
		loadingItems = false;
	}
}

void NovaConfig::on_dropRateCheckBox_stateChanged()
{
	if(!loadingItems)
	{
		loadingItems = true;
		saveProfile();
		loadProfile();
		loadingItems = false;
	}
}

void NovaConfig::on_tcpCheckBox_stateChanged()
{
	if(!loadingItems)
	{
		loadingItems = true;
		saveProfile();
		loadProfile();
		loadingItems = false;
	}
}

void NovaConfig::on_udpCheckBox_stateChanged()
{
	if(!loadingItems)
	{
		loadingItems = true;
		saveProfile();
		loadProfile();
		loadingItems = false;
	}
}

void NovaConfig::on_icmpCheckBox_stateChanged()
{
	if(!loadingItems)
	{
		loadingItems = true;
		saveProfile();
		loadProfile();
		loadingItems = false;
	}
}

void NovaConfig::on_portTreeWidget_itemChanged(QTreeWidgetItem * item)
{
	portTreeWidget_comboBoxChanged(item, true);
}

void NovaConfig::portTreeWidget_comboBoxChanged(QTreeWidgetItem *item,  bool edited)
{
	//On user action with a valid port, and the user actually changed something
	if(!loadingItems && (item != NULL) && edited)
	{
		loadingItems = true;
		//Ensure the signaling item is selected
		ui.portTreeWidget->setCurrentItem(item);
		profile p = profiles[currentProfile];
		string oldPort;
		port oldPrt;

		//Find the port before the changes
		for(PortTable::iterator it = ports.begin(); it != ports.end(); it++)
		{
			if(it->second.item == item)
				oldPort = it->second.portName;
		}
		oldPrt = ports[oldPort];

		//Use the combo boxes to update the hidden text underneath them.
		TreeItemComboBox * qTypeBox = (TreeItemComboBox*)ui.portTreeWidget->itemWidget(item, 1);
		item->setText(1, qTypeBox->currentText());

		TreeItemComboBox * qBehavBox = (TreeItemComboBox*)ui.portTreeWidget->itemWidget(item, 2);
		item->setText(2, qBehavBox->currentText());

		//Generate a unique identifier for a particular protocol, number and behavior combination
		// this is pulled from the recently updated hidden text and reflects any changes
		string portName = item->text(0).toStdString() + "_" + item->text(1).toStdString()
				+ "_" + item->text(2).toStdString();

		port prt;
		//Locate the port in the table or create the port if it doesn't exist
		if(ports.find(portName) == ports.end())
		{
			prt.portName = portName;
			prt.portNum = item->text(0).toStdString();
			prt.type = item->text(1).toStdString();
			prt.behavior = item->text(2).toStdString();
			//If the behavior is actually a script name
			if(prt.behavior.compare("open") && prt.behavior.compare("reset") && prt.behavior.compare("block"))
			{
				prt.scriptName = prt.behavior;
				prt.behavior = "script";
			}
			ports[portName] = prt;
		}
		else
			prt = ports[portName];

		//Check for port conflicts
		for(uint i = 0; i < p.ports.size(); i++)
		{
			port temp = ports[p.ports[i].first];
			//If theres a conflict other than with the old port
			if((!(temp.portNum.compare(prt.portNum))) && (!(temp.type.compare(prt.type)))
					&& temp.portName.compare(oldPort))
			{
				syslog(SYSL_ERR, "File: %s Line: %d WARNING: Port number and protocol already used.",
						__FILE__, __LINE__);
				portName = "";
			}
		}

		//If there were no conflicts and the port will be included, insert in sorted location
		if(portName.compare(""))
		{
			uint index;
			pair<string, bool> portPair;

			//Erase the old port from the current profile
			for(index = 0; index < p.ports.size(); index++)
				if(!p.ports[index].first.compare(oldPort))
					p.ports.erase(p.ports.begin()+index);

			//If the port number or protocol is different, check for inherited ports
			if((prt.portNum.compare(oldPrt.portNum) || prt.type.compare(oldPrt.type))
					&& (profiles.find(p.parentProfile) != profiles.end()))
			{
				profile parent = profiles[p.parentProfile];
				for(uint i = 0; i < parent.ports.size(); i++)
				{
					port temp = ports[parent.ports[i].first];
					//If a parent's port matches the number and protocol of the old port being removed
					if((!(temp.portNum.compare(oldPrt.portNum))) && (!(temp.type.compare(oldPrt.type))))
					{
						portPair.first = temp.portName;
						portPair.second = true;
						if(index < p.ports.size())
							p.ports.insert(p.ports.begin() + index, portPair);
						else
							p.ports.push_back(portPair);
						break;
					}
				}
			}

			//Create the vector item for the profile
			portPair.first = portName;
			portPair.second = false;

			//Insert it at the numerically sorted position.
			uint i = 0;
			for(i = 0; i < p.ports.size(); i++)
			{
				port temp = ports[p.ports[i].first];
				if((atoi(temp.portNum.c_str())) < (atoi(prt.portNum.c_str())))
				{
					continue;
				}
				break;
			}
			if(i < p.ports.size())
				p.ports.insert(p.ports.begin()+i, portPair);
			else
				p.ports.push_back(portPair);

			profiles[p.name] = p;
			//Check for children who inherit the port
			vector<string> updateList;
			updateList.push_back(currentProfile);
			bool changed = true;
			bool valid;

			//While any changes have been made
			while(changed)
			{
				//In this while, no changes have been found
				changed = false;
				//Check profile table for children of currentProfile at it's children and so on
				for(ProfileTable::iterator it = profiles.begin(); it != profiles.end(); it++)
				{
					//Profile invalid to add to start
					valid = false;
					for(uint i = 0; i < updateList.size(); i++)
					{
						//If the parent is being updated this profile is tenatively valid
						if(!it->second.parentProfile.compare(updateList[i]))
							valid = true;
						//If this profile is already in the list, this profile shouldn't be included
						if(!it->first.compare(updateList[i]))
						{
							valid = false;
							break;
						}
					}
					//A valid profile is stored in the list
					if(valid)
					{
						profile ptemp = it->second;
						//Check if we inherited the old port, and delete it if so
						for(uint i = 0; i < ptemp.ports.size(); i++)
							if(!ptemp.ports[i].first.compare(oldPort) && ptemp.ports[i].second)
								ptemp.ports.erase(ptemp.ports.begin()+i);

						profile parentTemp = profiles[ptemp.parentProfile];

						//insert any ports the parent has that doesn't conflict
						for(uint i = 0; i < parentTemp.ports.size(); i++)
						{
							bool conflict = false;
							port pr = ports[parentTemp.ports[i].first];
							//Check the child for conflicts
							for(uint j = 0; j < ptemp.ports.size(); j++)
							{
								port temp = ports[ptemp.ports[j].first];
								if(!temp.portNum.compare(pr.portNum) && !temp.type.compare(pr.type))
									conflict = true;
							}
							if(!conflict)
							{
								portPair.first = pr.portName;
								portPair.second = true;
								//Insert it at the numerically sorted position.
								uint j = 0;
								for(j = 0; j < ptemp.ports.size(); j++)
								{
									port temp = ports[ptemp.ports[j].first];
									if((atoi(temp.portNum.c_str())) < (atoi(pr.portNum.c_str())))
										continue;

									break;
								}
								if(j < ptemp.ports.size())
									ptemp.ports.insert(ptemp.ports.begin()+j, portPair);
								else
									ptemp.ports.push_back(portPair);
							}
						}
						updateList.push_back(ptemp.name);
						profiles[ptemp.name] = ptemp;
						//Since we found at least one profile this iteration flag as changed
						// so we can check for it's children
						changed = true;
						break;
					}
				}
			}
		}
		loadProfile();
		saveProfile();
		ui.portTreeWidget->setFocus(Qt::OtherFocusReason);
		ui.portTreeWidget->setCurrentItem(ports[prt.portName].item);
		loadingItems = false;
		loadAllProfiles();
	}
	//On user interaction with a valid port item without any changes
	else if(!edited && !loadingItems && (item != NULL))
	{
		//Select the valid port item under the associated combo box
		loadingItems = true;
		ui.portTreeWidget->setFocus(Qt::OtherFocusReason);
		ui.portTreeWidget->setCurrentItem(item);
		loadingItems = false;
	}
}

QListWidgetItem* NovaConfig::getFeatureListItem(QString name, char enabled)
{
	QListWidgetItem *newFeatureEntry = new QListWidgetItem();
	name.prepend("+  ");
	newFeatureEntry->setText(name);
	updateFeatureListItem(newFeatureEntry, enabled);

	return newFeatureEntry;
}

void NovaConfig::onFeatureClick(const QPoint & pos)
{
	QPoint globalPos = ui.featureList->mapToGlobal(pos);
	QModelIndex t = ui.featureList->indexAt(pos);

	// Handle clicking on a row that isn't populated
	if (t.row() < 0 || t.row() >= DIM)
	{
		return;
	}

	// Make the menu
    QMenu myMenu;
    myMenu.addAction(new QAction("Enable", this));
    myMenu.addAction(new QAction("Disable", this));

    // Figure out what the user selected
    QAction* selectedItem = myMenu.exec(globalPos);
    if (selectedItem)
    {
		if (selectedItem->text() == "Enable")
		{
			updateFeatureListItem(ui.featureList->item(t.row()), '1');
		}
		else if (selectedItem->text() == "Disable")
		{
			updateFeatureListItem(ui.featureList->item(t.row()), '0');
		}
    }
}

void NovaConfig::updateFeatureListItem(QListWidgetItem* newFeatureEntry, char enabled)
{
	QBrush *enabledColor = new QBrush(QColor("black"));
	QBrush *disabledColor = new QBrush(QColor("grey"));
	QString name = newFeatureEntry->text().remove(0, 2);

	if (enabled == '1')
	{
		name.prepend(QString("+ "));
		newFeatureEntry->setForeground(*enabledColor);
	}
	else
	{
		name.prepend(QString("- "));
		newFeatureEntry->setForeground(*disabledColor);
	}
	newFeatureEntry->setText(name);
}


/************************************************
 * Loading preferences from configuration files
 ************************************************/

void NovaConfig::loadNmapPersonalities()
{
	string NMapFile = GetReadPath() + "/nmap-os-db";
	ifstream nmapPers(NMapFile.c_str());
	string line, fprint, prefix, printClass;
	if(nmapPers.is_open())
	{
		while(nmapPers.good())
		{
			getline(nmapPers, line);
			/* From 'man strtoul'  Since strtoul() can legitimately return 0 or  LONG_MAX  (LLONG_MAX  for
			   strtoull()) on both success and failure, the calling program should set
			   errno to 0 before the call, and then determine if an error occurred  by
			   checking whether errno has a nonzero value after the call. */

			//We've hit a fingerprint line
			prefix = "Fingerprint";
			if(!line.substr(0, prefix.size()).compare(prefix))
			{
				//Remove 'Fingerprint ' prefix.
				line = line.substr(prefix.size()+1,line.size());
				//If there are multiple fingerprints on this line, locate the end of the first.
				uint i = line.find(" or", 0);
				uint j = line.find(";", 0);

				//trim the line down to the first fingerprint
				if((i != string::npos) && (j != string::npos))
				{
					if(i < j)
						fprint = line.substr(0, i);
					else
						fprint = line.substr(0, j);
				}

				else if(i != string::npos)
					fprint = line.substr(0, i);

				else if(j != string::npos)
					fprint = line.substr(0, j);

				else
					fprint = line;

				//All fingerprint lines are followed by one or more class lines, get the first class line
				getline(nmapPers, line);
				prefix = "Class";
				if(!line.substr(0, prefix.size()).compare(prefix))
				{
					printClass = line.substr(prefix.size()+1, line.size());
					pair<string, string> printPair;
					printPair.first = fprint;
					printPair.second = printClass;
					nmapPersonalities.push_back(printPair);
				}
				else
				{
					syslog(SYSL_ERR, "File: %s Line: %d ERROR: Unable to load nmap fingerpint: %s", __FILE__, __LINE__,fprint.c_str());
				}
			}
		}
	}
	nmapPers.close();
}
void NovaConfig::loadMACPrefixs()
{
	string MACPrefixFile = GetReadPath() + "/nmap-mac-prefixes";
	ifstream MACPrefixes(MACPrefixFile.c_str());
	string line, vendor, prefixStr;
	char * notUsed;
	uint prefix;
	if(MACPrefixes.is_open())
	{
		while(MACPrefixes.good())
		{
			getline(MACPrefixes, prefixStr, ' ');
			/* From 'man strtoul'  Since strtoul() can legitimately return 0 or  LONG_MAX  (LLONG_MAX  for
		       strtoull()) on both success and failure, the calling program should set
		       errno to 0 before the call, and then determine if an error occurred  by
		       checking whether errno has a nonzero value after the call. */
			errno = 0;
			prefix = strtoul(prefixStr.c_str(), &notUsed, 16);
			if(errno)
				continue;
			getline(MACPrefixes, vendor);
			MACVendorTable[prefix] = vendor;
			if(VendorMACTable.find(vendor) != VendorMACTable.end())
			{
				VendorMACTable[vendor]->push_back(prefix);
			}
			else
			{
				vector<uint> * vect = new vector<uint>;
				vect->push_back(prefix);
				VendorMACTable[vendor] = vect;
			}
		}
	}
}

//Randomly selects one of the ranges associated with vendor and generates the remainder of the MAC address
// *note conflicts are only checked for locally, weird things may happen if the address is already being used.
string NovaConfig::generateUniqueMACAddr(string vendor)
{
	char addrBuffer[8];
	stringstream addrStrm;
	pair<uint32_t,uint32_t> addr;
	VendorToMACTable::iterator it;
	string testStr;

	//If we can resolve the vendor to a range
	if((it = VendorMACTable.find(vendor)) != VendorMACTable.end())
	{
		do
		{
			//Randomly select one of the ranges
			uint j;
			uint i = rand() % it->second->size();
			addr.first = it->second->at(i);
			i = 0;

			//Convert the first part to a string and format it for output
			sprintf(addrBuffer, "%x", addr.first);
			testStr = string(addrBuffer);
			for(j = 0; j < (6-testStr.size()); j++)
			{
				if(!(i%2)&& i )
				{
					addrStrm << ":";
				}
				addrStrm << "0";
				i++;
			}
			j = 0;
			if(testStr.size() > 6)
				j = testStr.size() - 6;
			for(j = j; j < testStr.size(); j++)
			{
				if(!(i%2)&& i )
				{
					addrStrm << ":";
				}
				addrStrm << addrBuffer[j];
				i++;
			}

			//Randomly generate the remaining portion
			addr.second = ((uint)rand() & (uint)(pow(2,24)-1));

			//Convert the second part to a string and format it for output
			bzero(addrBuffer, 8);
			sprintf(addrBuffer, "%x", addr.second);
			testStr = string(addrBuffer);
			i = 0;
			for(j = 0; j < (6-testStr.size()); j++)
			{
				if(!(i%2)&& i )
				{
					addrStrm << ":";
				}
				addrStrm << "0";
				i++;
			}
			j = 0;
			if(testStr.size() > 6)
				j = testStr.size() - 6;
			for(j = j; j < testStr.size(); j++)
			{
				if(!(i%2)&& (i!=6))
				{
					addrStrm << ":";
				}
				addrStrm << addrBuffer[j];
				i++;
			}
		}while(nodes.find(addrStrm.str()) != nodes.end());
	}
	return addrStrm.str();
}

//Resolve the first 3 bytes of a MAC Address to a MAC vendor that owns the range, returns the vendor string
string NovaConfig::resolveMACVendor(uint MACPrefix)
{
	if(MACVendorTable.find((uint)(MACPrefix)) != MACVendorTable.end())
		return MACVendorTable[(uint)(MACPrefix)];
	else
		return "";
}

//Load Personality choices from nmap fingerprints file
void NovaConfig::displayNmapPersonalityTree()
{
	retVal = "";
	NovaComplexDialog * NmapPersonalityWindow = new NovaComplexDialog(
			PersonalityDialog, &retVal, this);
	NmapPersonalityWindow->exec();
	if(retVal.compare(""))
	{
		ui.personalityEdit->setText((QString)retVal.c_str());
	}
}
bool NovaConfig::updateNodeTypes()
{
	bool hasDoppelganger = false;
	bool nameUnique = false;
	stringstream ss;
	uint i = 0, j = 0;
	j = ~j; // 2^32-1
	vector<string> delList;
	vector<node> addList;
	string prefix = "";

	for(NodeTable::iterator it = nodes.begin(); it != nodes.end(); it++)
	{
		node tempNode = it->second;
		switch(profiles[tempNode.pfile].type)
		{
			case static_IP:
				//If name/key is not the IP
				if(it->first.compare(tempNode.IP))
				{
					tempNode.name = tempNode.IP;
					delList.push_back(it->first);
					addList.push_back(tempNode);
				}
				break;

			case staticDHCP:

				//If there is no MAC
				if(!tempNode.MAC.size())
				{
					tempNode.MAC = generateUniqueMACAddr(profiles[tempNode.pfile].ethernet);
				}
				//If name/key is not the MAC
				if(it->first.compare(tempNode.MAC))
				{
					tempNode.name = tempNode.MAC;
					delList.push_back(it->first);
					addList.push_back(tempNode);
				}
				break;

			case randomDHCP:
				prefix = tempNode.pfile + " on " + tempNode.interface;
				//If the key is at least long enough to be correct
				if(it->first.size() >= prefix.size())
				{
					//If the key starts with the correct prefix do nothing
					if(!it->first.substr(0,prefix.size()).compare(prefix))
						break;
				}
				//If the key doesn't start with the correct prefix, generate the correct name
				tempNode.name = prefix;
				i = 0;
				ss.str("");
				//Finds a unique identifier
				while(i < j)
				{
					nameUnique = true;
					for(uint k = 0; k < addList.size(); k++)
					{
						if(!addList[k].name.compare(tempNode.name))
							nameUnique = false;
					}
					if(nodes.find(tempNode.name) != nodes.end())
						nameUnique = false;

					if(nameUnique)
						break;
					i++;
					ss.str("");
					ss << tempNode.pfile << " on " << tempNode.interface << "-" << i;
					tempNode.name = ss.str();
				}
				delList.push_back(it->first);
				addList.push_back(tempNode);
				break;

			case Doppelganger:
				//If name isn't doppelganger and we haven't found a doppelganger yet
				if(!hasDoppelganger)
				{
					tempNode.name = "Doppelganger";
					delList.push_back(it->first);
					addList.push_back(tempNode);
					hasDoppelganger = true;
				}
				else
				{
					syslog(SYSL_ERR, "File: %s Line: %d ERROR: A Doppelganger already exists.", __FILE__, __LINE__);
					if(mainwindow->prompter->DisplayPrompt(mainwindow->DOPP_EXISTS, "Only one node can be a Doppelganger for a host, would"
							" you like to delete the duplicate node using profile: " + tempNode.pfile + "?") == CHOICE_ALT)
						return false;
					else
						delList.push_back(it->first);
				}
				break;
		}
	}

	if(!hasDoppelganger && ui.dmCheckBox->isChecked())
	{
		syslog(SYSL_ERR, "File: %s Line: %d ERROR: No Doppelganger exists.", __FILE__, __LINE__);
		if(mainwindow->prompter->DisplayPrompt(mainwindow->NO_DOPP, "No Doppelganger node could be found. Do you wish to disable"
				" the Doppelganger and continue?") == CHOICE_ALT)
			return false;
		else
			ui.dmCheckBox->setChecked(false);
	}
	while(!delList.empty())
	{
		string delStr = delList.back();
		delList.pop_back();
		deleteNode(&nodes[delStr]);
	}
	while(!addList.empty())
	{
		node tempNode = addList.back();
		addList.pop_back();
		nodes[tempNode.name] = tempNode;
		subnets[tempNode.sub].nodes.push_back(tempNode.name);
	}
	return true;
}
//Load MAC vendor prefix choices from nmap mac prefix file
bool NovaConfig::displayMACPrefixWindow()
{
	retVal = "";
	NovaComplexDialog * MACPrefixWindow = new NovaComplexDialog(
			MACDialog, &retVal, this, ui.ethernetEdit->text().toStdString());
	MACPrefixWindow->exec();
	if(retVal.compare(""))
	{
		ui.ethernetEdit->setText((QString)retVal.c_str());

		//If there is no change in vendor, nothing left to be done.
		if(profiles[currentProfile].ethernet.compare(retVal))
			return true;
		for(NodeTable::iterator it = nodes.begin(); it != nodes.end(); it++)
		{
			if(!it->second.pfile.compare(currentProfile))
			{
				it->second.MAC = generateUniqueMACAddr(retVal);
			}
		}
		//If IP's arent staticDHCP, key wont change so do nothing
		if(profiles[currentProfile].type != staticDHCP)
			return true;

		if(updateNodeTypes())
			return true;
		else
			return false;
	}
	return false;
}

void NovaConfig::loadPreferences()
{
	//Read from CE Config
	string configurationFile = homePath + "/Config/NOVAConfig.txt";

	openlog("NovaGUI", OPEN_SYSL, LOG_AUTHPRIV);

	NOVAConfiguration * NConfig = new NOVAConfiguration();
	NConfig->LoadConfig((char*)configurationFile.c_str(), homePath, __FILE__);
	if (!NConfig->options["INTERFACE"].isValid || !NConfig->options["DATAFILE"].isValid
			|| !NConfig->options["SA_MAX_ATTEMPTS"].isValid || !NConfig->options["SA_SLEEP_DURATION"].isValid
			|| !NConfig->options["SILENT_ALARM_PORT"].isValid || !NConfig->options["K"].isValid
			|| !NConfig->options["EPS"].isValid || !NConfig->options["CLASSIFICATION_TIMEOUT"].isValid
			|| !NConfig->options["CLASSIFICATION_THRESHOLD"].isValid || !NConfig->options["IS_TRAINING"].isValid
			|| !NConfig->options["HS_HONEYD_CONFIG"].isValid || !NConfig->options["TCP_TIMEOUT"].isValid
			|| !NConfig->options["TCP_CHECK_FREQ"].isValid || !NConfig->options["DM_HONEYD_CONFIG"].isValid
			|| !NConfig->options["DOPPELGANGER_IP"].isValid || !NConfig->options["DM_ENABLED"].isValid
			|| !NConfig->options["READ_PCAP"].isValid || !NConfig->options["PCAP_FILE"].isValid
			|| !NConfig->options["GO_TO_LIVE"].isValid || !NConfig->options["USE_TERMINALS"].isValid
			|| !NConfig->options["ENABLED_FEATURES"].isValid)
	{
		syslog(SYSL_ERR, "File: %s Line: %d ERROR: Unable to load configuration file.", __FILE__, __LINE__);
		mainwindow->prompter->DisplayPrompt(mainwindow->CONFIG_READ_FAIL, "Error: Unable to read NOVA configuration file " + configurationFile);
		this->close();
	}

	ui.interfaceEdit->setText((QString)NConfig->options["INTERFACE"].data.c_str());
	ui.dataEdit->setText((QString)NConfig->options["DATAFILE"].data.c_str());
	ui.saAttemptsMaxEdit->setText((QString)NConfig->options["SA_MAX_ATTEMPTS"].data.c_str());
	ui.saAttemptsTimeEdit->setText((QString)NConfig->options["SA_SLEEP_DURATION"].data.c_str());
	ui.saPortEdit->setText((QString)NConfig->options["SILENT_ALARM_PORT"].data.c_str());
	ui.ceIntensityEdit->setText((QString)NConfig->options["K"].data.c_str());
	ui.ceErrorEdit->setText((QString)NConfig->options["EPS"].data.c_str());
	ui.ceFrequencyEdit->setText((QString)NConfig->options["CLASSIFICATION_TIMEOUT"].data.c_str());
	ui.ceThresholdEdit->setText((QString)NConfig->options["CLASSIFICATION_THRESHOLD"].data.c_str());
	ui.trainingCheckBox->setChecked(atoi(NConfig->options["IS_TRAINING"].data.c_str()));
	ui.hsConfigEdit->setText((QString)NConfig->options["HS_HONEYD_CONFIG"].data.c_str());
	ui.tcpTimeoutEdit->setText((QString)NConfig->options["TCP_TIMEOUT"].data.c_str());
	ui.tcpFrequencyEdit->setText((QString)NConfig->options["TCP_CHECK_FREQ"].data.c_str());
	ui.dmConfigEdit->setText((QString)NConfig->options["DM_HONEYD_CONFIG"].data.c_str());
	ui.dmIPEdit->setText((QString)NConfig->options["DOPPELGANGER_IP"].data.c_str());
	ui.dmCheckBox->setChecked(atoi(NConfig->options["DM_ENABLED"].data.c_str()));
	ui.pcapCheckBox->setChecked(atoi(NConfig->options["READ_PCAP"].data.c_str()));
	ui.pcapGroupBox->setEnabled(ui.pcapCheckBox->isChecked());
	ui.pcapEdit->setText((QString)NConfig->options["PCAP_FILE"].data.c_str());
	ui.liveCapCheckBox->setChecked(atoi(NConfig->options["GO_TO_LIVE"].data.c_str()));
	ui.terminalCheckBox->setChecked(atoi(NConfig->options["USE_TERMINALS"].data.c_str()));
	{
		string featuresEnabled = NConfig->options["ENABLED_FEATURES"].data;
		ui.featureList->clear();
		// Populate the list, row order is based on dimension macros
		ui.featureList->insertItem(IP_TRAFFIC_DISTRIBUTION,
				getFeatureListItem(QString("IP Traffic Distribution"),featuresEnabled.at(IP_TRAFFIC_DISTRIBUTION)));

		ui.featureList->insertItem(PORT_TRAFFIC_DISTRIBUTION,
				getFeatureListItem(QString("Port Traffic Distribution"),featuresEnabled.at(PORT_TRAFFIC_DISTRIBUTION)));

		ui.featureList->insertItem(HAYSTACK_EVENT_FREQUENCY,
				getFeatureListItem(QString("Haystack Event Frequency"),featuresEnabled.at(HAYSTACK_EVENT_FREQUENCY)));

		ui.featureList->insertItem(PACKET_SIZE_MEAN,
				getFeatureListItem(QString("Packet Size Mean"),featuresEnabled.at(PACKET_SIZE_MEAN)));

		ui.featureList->insertItem(PACKET_SIZE_DEVIATION,
				getFeatureListItem(QString("Packet Size Deviation"),featuresEnabled.at(PACKET_SIZE_DEVIATION)));

		ui.featureList->insertItem(DISTINCT_IPS,
				getFeatureListItem(QString("IPs Contacted"),featuresEnabled.at(DISTINCT_IPS)));

		ui.featureList->insertItem(DISTINCT_PORTS,
				getFeatureListItem(QString("Ports Contacted"),featuresEnabled.at(DISTINCT_PORTS)));

		ui.featureList->insertItem(PACKET_INTERVAL_MEAN,
				getFeatureListItem(QString("Packet Interval Mean"),featuresEnabled.at(PACKET_INTERVAL_MEAN)));

		ui.featureList->insertItem(PACKET_INTERVAL_DEVIATION,
				getFeatureListItem(QString("Packet Interval Deviation"),featuresEnabled.at(PACKET_INTERVAL_DEVIATION)));
	}
	closelog();
	delete NConfig;
}

//Draws the current honeyd configuration
void NovaConfig::loadHaystack()
{
	//Sets an initial selection
	updatePointers();
	//Draws all profile heriarchy
	loadAllProfiles();
	//Draws all node heirarchy
	loadAllNodes();
	ui.nodeTreeWidget->expandAll();
	ui.hsNodeTreeWidget->expandAll();
}

//Checks for ports that aren't used and removes them from the table if so
void NovaConfig::cleanPorts()
{
	vector<string> delList;
	bool found;
	for(PortTable::iterator it = ports.begin(); it != ports.end(); it++)
	{
		found = false;
		for(ProfileTable::iterator jt = profiles.begin(); (jt != profiles.end()) && !found; jt++)
		{
			for(uint i = 0; (i < jt->second.ports.size()) && !found; i++)
			{
				if(!jt->second.ports[i].first.compare(it->first))
					found = true;
			}
		}
		if(!found)
			delList.push_back(it->first);
	}
	while(!delList.empty())
	{
		ports.erase(delList.back());
		delList.pop_back();
	}
}

//Saves the changes to parent novagui window
void NovaConfig::pushData()
{
	//Clean up unused ports
	cleanPorts();
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

	//Saves the current configuration to XML files
	mainwindow->saveAll();
	mainwindow->writeHoneyd();
}

//Pulls the last stored configuration from novagui
//used on start up or to undo all changes (currently defaults button)
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

//Attempts to use the same key previously used, if that key is no longer available
//It selects a new one if possible
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
			currentNode = nodes.begin()->second.name;
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

//Stores all changes and closes the window
void NovaConfig::on_okButton_clicked()
{
	saveProfile();
	openlog("NovaGUI", OPEN_SYSL, LOG_AUTHPRIV);

	if (!saveConfigurationToFile())
	{
		syslog(SYSL_ERR, "File: %s Line: %d Error writing to Nova config file.", __FILE__, __LINE__);
		mainwindow->prompter->DisplayPrompt(mainwindow->CONFIG_WRITE_FAIL, "Error: Unable to write to NOVA configuration file");
		this->close();
	}
	closelog();
	//Save changes
	pushData();
	this->close();
}


//Stores all changes the repopulates the window
void NovaConfig::on_applyButton_clicked()
{
	saveProfile();
	openlog("NovaGUI", OPEN_SYSL, LOG_AUTHPRIV);
	if (!saveConfigurationToFile())
	{
		syslog(SYSL_ERR, "File: %s Line: %d Error writing to Nova config file.", __FILE__, __LINE__);
		mainwindow->prompter->DisplayPrompt(mainwindow->CONFIG_WRITE_FAIL, "Error: Unable to write to NOVA configuration file ");
		this->close();
	}

	//Reloads NOVAConfig preferences to assert concurrency
	loadPreferences();
	//Saves honeyd changes
	pushData();
	loadingItems = true;
	//Reloads honeyd configuration to assert concurrency
	loadHaystack();
	loadingItems = false;
	closelog();
}


bool NovaConfig::saveConfigurationToFile() {
	string line, prefix;

	//Rewrite the config file with the new settings
	string configurationFile = homePath + "/Config/NOVAConfig.txt";
	string configurationBackup = homePath + "/Config/.NOVAConfig.tmp";
	string copyCommand = "cp -f " + configurationFile + " " + configurationBackup;
	system(copyCommand.c_str());
	ifstream *in = new ifstream(configurationBackup.c_str());
	ofstream *out = new ofstream(configurationFile.c_str());

	if(out->is_open() && in->is_open())
	{
		while(in->good())
		{
			if (!getline(*in, line))
			{
				continue;
			}

			prefix = "DM_ENABLED";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				if(ui.dmCheckBox->isChecked())
				{
					*out << "DM_ENABLED 1"<<endl;
				}
				else
				{
					*out << "DM_ENABLED 0"<<endl;
				}
				continue;
			}

			prefix = "IS_TRAINING";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				if(ui.trainingCheckBox->isChecked())
				{
					*out << "IS_TRAINING 1"<<endl;
				}
				else
				{
					*out << "IS_TRAINING 0"<<endl;
				}
				continue;
			}

			prefix = "INTERFACE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << this->ui.interfaceEdit->displayText().toStdString() << endl;
				continue;
			}

			prefix = "DATAFILE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << this->ui.dataEdit->displayText().toStdString() << endl;
				continue;
			}

			prefix = "SA_SLEEP_DURATION";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << this->ui.saAttemptsTimeEdit->displayText().toStdString() << endl;
				continue;
			}

			prefix = "SA_MAX_ATTEMPTS";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << this->ui.saAttemptsMaxEdit->displayText().toStdString() << endl;
				continue;
			}

			prefix = "SILENT_ALARM_PORT";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << this->ui.saPortEdit->displayText().toStdString() << endl;
				continue;
			}

			prefix = "K";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << this->ui.ceIntensityEdit->displayText().toStdString()  << endl;
				continue;
			}

			prefix = "EPS";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << this->ui.ceErrorEdit->displayText().toStdString() << endl;
				continue;
			}

			prefix = "CLASSIFICATION_TIMEOUT";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << this->ui.ceFrequencyEdit->displayText().toStdString() << endl;
				continue;
			}

			prefix = "CLASSIFICATION_THRESHOLD";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << this->ui.ceThresholdEdit->displayText().toStdString() << endl;
				continue;
			}

			prefix = "DM_HONEYD_CONFIG";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << this->ui.dmConfigEdit->displayText().toStdString() << endl;
				continue;
			}

			prefix = "DOPPELGANGER_IP";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << this->ui.dmIPEdit->displayText().toStdString() << endl;
				continue;
			}

			prefix = "HS_HONEYD_CONFIG";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << this->ui.hsConfigEdit->displayText().toStdString() << endl;
				continue;
			}

			prefix = "TCP_TIMEOUT";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << this->ui.tcpTimeoutEdit->displayText().toStdString() << endl;
				continue;
			}

			prefix = "TCP_CHECK_FREQ";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << this->ui.tcpFrequencyEdit->displayText().toStdString()  << endl;
				continue;
			}

			prefix = "PCAP_FILE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " " << ui.pcapEdit->displayText().toStdString()  << endl;
				continue;
			}

			prefix = "ENABLED_FEATURES";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				*out << prefix << " ";
				for (uint i = 0; i < DIM; i++)
				{
					char state = ui.featureList->item(i)->text().at(0).toAscii();
					if (state == '+')
						*out << 1;
					else
						*out << 0;
				}

				*out << endl;
				continue;
			}

			prefix = "READ_PCAP";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				if(ui.pcapCheckBox->isChecked())
				{
					*out << "READ_PCAP 1"<< endl;
				}
				else
				{
					*out << "READ_PCAP 0"<< endl;
				}
				continue;
			}

			prefix = "GO_TO_LIVE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				if(ui.liveCapCheckBox->isChecked())
				{
					*out << "GO_TO_LIVE 1" << endl;
				}
				else
				{
					*out << "GO_TO_LIVE 0" << endl;
				}
				continue;
			}

			prefix = "USE_TERMINALS";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				if(ui.terminalCheckBox->isChecked())
				{
					*out << "USE_TERMINALS 1" << endl;
				}
				else
				{
					*out << "USE_TERMINALS 0" << endl;
				}
				continue;
			}

			*out << line << endl;
		}
	}
	else
	{
		openlog("NovaGUI", OPEN_SYSL, LOG_AUTHPRIV);
		syslog(SYSL_ERR, "File: %s Line: %d Error writing to Nova config file.", __FILE__, __LINE__);
		closelog();
		mainwindow->prompter->DisplayPrompt(mainwindow->CONFIG_WRITE_FAIL, "Error: Unable to write to NOVA configuration file");
		in->close();
		out->close();
		delete in;
		delete out;

		return false;
	}

	in->close();
	out->close();
	delete in;
	delete out;
	system("rm -f Config/.NOVAConfig.tmp");

	return true;
}

//Exit the window and ignore any changes since opening or apply was pressed
void NovaConfig::on_cancelButton_clicked()
{
	this->close();
}

void NovaConfig::on_defaultsButton_clicked() //TODO
{
	//Currently just restores to last save changes
	//We should really identify default values and write those while maintaining
	//Critical values that might cause nova to break if changed.

	//Reloads from NOVAConfig
	loadPreferences();
	//Has NovaGUI reload honeyd configuration from XML files
	mainwindow->loadAll();
	//Pulls honeyd configuration
	pullData();
	loadingItems = true;
	//Populates honeyd configuration pulled
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
		ui.stackedWidget->setCurrentIndex(i);
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
			openlog("NovaGUI", OPEN_SYSL, LOG_AUTHPRIV);
			syslog(SYSL_ERR, "File: %s Line: %d Unable to set stackedWidget page index from treeWidgetItem", __FILE__, __LINE__);
			closelog();
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

/* Enables or disables options specific for reading from pcap file */
void NovaConfig::on_dhcpComboBox_currentIndexChanged(int index)
{
	if(loadingItems)
		return;
	else
		loadingItems = true;

	vector<string> delList;
	vector<node> addList;
	bool nodeExists = false;

	//Find out if any nodes use this profile
	for(NodeTable::iterator it = nodes.begin(); it != nodes.end(); it++)
	{
		//if we find a node using this profile
		if(!it->second.pfile.compare(currentProfile))
			nodeExists = true;
	}

	//If were switching to Doppelganger mode and a node uses this profile
	if((index == Doppelganger) && nodeExists)
	{
		//Check that another node isn't already the Doppelganger
		for(NodeTable::iterator it = nodes.begin(); it != nodes.end(); it++)
		{
			if(profiles[it->second.pfile].type == Doppelganger)
			{
				syslog(SYSL_ERR, "File: %s Line: %d ERROR: A Doppelganger already exists.", __FILE__, __LINE__);
				//TODO Appropriate Doppelganger exists displayPrompt
				ui.dhcpComboBox->setCurrentIndex((int)profiles[currentProfile].type);
				loadingItems = false;
				return;
			}
		}
	}

	//If the current ethernet is an invalid selection
	//TODO this should display a dialog asking the user if they wish to pick a valid ethernet or cancel mode change
	if(((VendorMACTable.find(profiles[currentProfile].ethernet)) == VendorMACTable.end()) && (index == staticDHCP))
	{
		if(!displayMACPrefixWindow())
		{
			ui.dhcpComboBox->setCurrentIndex((int)profiles[currentProfile].type);
			loadingItems = false;
			return;
		}
	}

	profiles[currentProfile].type = (profileType)index;
	ui.dhcpComboBox->setCurrentIndex((int)profiles[currentProfile].type);
	loadingItems = true;
	saveProfile();
	loadProfile();
	updateNodeTypes();
	loadAllNodes();
	loadingItems = false;
	bool doppExists = false;
	//If there is no doppelganger node, disable the DM
	for(NodeTable::iterator it = nodes.begin(); it != nodes.end(); it++)
	{
		if(profiles[it->second.pfile].type == Doppelganger)
		{
			doppExists = true;
			break;
		}
	}
	//Disables the dm if no dopp node, otherwise it keeps the same state.
	ui.dmCheckBox->setChecked(doppExists & ui.dmCheckBox->isChecked());
}

//Combo box signal for changing the uptime behavior
void NovaConfig::on_uptimeBehaviorComboBox_currentIndexChanged(int index)
{
	//If uptime behavior = random in range
	if(index)
		ui.uptimeRangeEdit->setText(ui.uptimeEdit->displayText());

	ui.uptimeRangeLabel->setVisible(index);
	ui.uptimeRangeEdit->setVisible(index);
}

void NovaConfig::saveProfile()
{
	QTreeWidgetItem * item = NULL;
	struct port pr;

	//Saves any modifications to the last selected profile object.
	if(profiles.find(currentProfile) != profiles.end())
	{
		profile p = profiles[currentProfile];
		//currentProfile->name is set in updateProfile
		p.ethernet = ui.ethernetEdit->displayText().toStdString();
		p.tcpAction = ui.tcpActionComboBox->currentText().toStdString();
		p.udpAction = ui.udpActionComboBox->currentText().toStdString();
		p.icmpAction = ui.icmpActionComboBox->currentText().toStdString();
		p.uptime = ui.uptimeEdit->displayText().toStdString();
		//If random in range behavior
		if(ui.uptimeBehaviorComboBox->currentIndex())
			p.uptimeRange = ui.uptimeRangeEdit->displayText().toStdString();
		//If flat behavior
		else
			p.uptimeRange = "";
		p.personality = ui.personalityEdit->displayText().toStdString();
		p.type = (profileType)ui.dhcpComboBox->currentIndex();
		stringstream ss;
		ss << ui.dropRateSlider->value();
		p.dropRate = ss.str();

		//Save the port table
		for(int i = 0; i < ui.portTreeWidget->topLevelItemCount(); i++)
		{
			pr = ports[p.ports[i].first];
			item = ui.portTreeWidget->topLevelItem(i);
			pr.portNum = item->text(0).toStdString();
			TreeItemComboBox * qTypeBox = (TreeItemComboBox*)ui.portTreeWidget->itemWidget(item, 1);
			TreeItemComboBox * qBehavBox = (TreeItemComboBox*)ui.portTreeWidget->itemWidget(item, 2);
			pr.type = qTypeBox->currentText().toStdString();
			if(!pr.portNum.compare(""))
			{
				continue;
			}
			pr.behavior = qBehavBox->currentText().toStdString();
			//If the behavior names a script
			if(pr.behavior.compare("open") && pr.behavior.compare("reset") && pr.behavior.compare("block"))
			{
				pr.behavior = "script";
				pr.scriptName = qBehavBox->currentText().toStdString();
			}
			if(!pr.behavior.compare("script"))
				pr.portName = pr.portNum + "_" +pr.type + "_" + pr.scriptName;
			else
				pr.portName = pr.portNum + "_" +pr.type + "_" + pr.behavior;

			p.ports[i].first = pr.portName;
			ports[p.ports[i].first] = pr;
			p.ports[i].second = item->font(0).italic();
		}
		profiles[currentProfile] = p;
		saveInherited();
		createProfileTree(currentProfile);
	}
}

void NovaConfig::saveInherited()
{
	profile p = profiles[currentProfile];

	p.inherited[TYPE] = ui.ipModeCheckBox->isChecked();
	if(ui.ipModeCheckBox->isChecked())
		p.type = profiles[p.parentProfile].type;

	p.inherited[TCP_ACTION] = ui.tcpCheckBox->isChecked();
	if(ui.tcpCheckBox->isChecked())
		p.tcpAction = profiles[p.parentProfile].tcpAction;

	p.inherited[UDP_ACTION] = ui.udpCheckBox->isChecked();
	if(ui.udpCheckBox->isChecked())
		p.udpAction = profiles[p.parentProfile].udpAction;

	p.inherited[ICMP_ACTION] = ui.icmpCheckBox->isChecked();
	if(ui.icmpCheckBox->isChecked())
		p.icmpAction = profiles[p.parentProfile].icmpAction;

	p.inherited[ETHERNET] = ui.ethernetCheckBox->isChecked();
	if(ui.ethernetCheckBox->isChecked())
		p.ethernet = profiles[p.parentProfile].ethernet;

	p.inherited[UPTIME] = ui.uptimeCheckBox->isChecked();
	if(ui.uptimeCheckBox->isChecked())
	{
		p.uptime = profiles[p.parentProfile].uptime;
		p.uptimeRange = profiles[p.parentProfile].uptimeRange;
		if(profiles[p.parentProfile].uptimeRange.compare(""))
			ui.uptimeBehaviorComboBox->setCurrentIndex(1);
		else
			ui.uptimeBehaviorComboBox->setCurrentIndex(0);
	}

	p.inherited[PERSONALITY] = ui.personalityCheckBox->isChecked();
	if(ui.personalityCheckBox->isChecked())
		p.personality = profiles[p.parentProfile].personality;

	p.inherited[DROP_RATE] = ui.dropRateCheckBox->isChecked();
	if(ui.dropRateCheckBox->isChecked())
		p.dropRate = profiles[p.parentProfile].dropRate;

	profiles[currentProfile] = p;

}
//Removes a profile, all of it's children and any nodes that currently use it
void NovaConfig::deleteProfile(string name)
{
	//Recursive descent to find and call delete on any children of the profile
	for(ProfileTable::iterator it = profiles.begin(); it != profiles.end(); it++)
	{
		//If the profile at the iterator is a child of this profile
		if(!it->second.parentProfile.compare(name))
		{
			deleteProfile(it->second.name);
		}
	}

	//If it is not the original profile deleted skip this part
	if(!name.compare(currentProfile))
	{
		//Store a copy of the profile for cleanup after deletion
		profile  p = profiles[name];

		QTreeWidgetItem * item = NULL, *temp = NULL;

		//If there is at least one other profile after deleting all children
		if(profiles.size() > 1)
		{
			//Get the current profile item
			item = profiles[currentProfile].profileItem;
			//Try to find another profile below it
			temp = ui.profileTreeWidget->itemBelow(item);

			//If no profile below, find a profile above
			if(temp == NULL)
			{
				item = ui.profileTreeWidget->itemAbove(item);
			}
			else item = temp; //if profile below was found
		}

		//Remove the current profiles tree widget items
		ui.profileTreeWidget->removeItemWidget(p.profileItem, 0);
		ui.hsProfileTreeWidget->removeItemWidget(p.item, 0);

		//Clear the tree of the current profile (may not be needed)
		profiles[name].tree.clear();

		//Erase the profile from the table and any nodes that use it
		updateProfile(DELETE_PROFILE, &profiles[name]);

		//If this profile has a parent
		if(p.parentProfile.compare(""))
		{
			//save a copy of the parent
			profile parent = profiles[p.parentProfile];

			//point to the profiles subtree of parent-copy ptree and clear it
			ptree * pt = &parent.tree.get_child("profiles");
			pt->clear();

			//Find all profiles still in the table that are sibilings of deleted profile
			//* We should be using an iterator to find the original profile and erase it
			//* but boost's iterator implementation doesn't seem to be able to access data
			//* correctly and are frequently invalidated.

			for(ProfileTable::iterator it = profiles.begin(); it != profiles.end(); it++)
			{
				if(!it->second.parentProfile.compare(parent.name))
				{
					//Put sibiling profiles into the tree
					pt->add_child("profile", it->second.tree);
				}
			}	//parent-copy now has the ptree of all children except deleted profile

			//point to the original parent's profiles subtree and replace it with our new ptree
			ptree * treePtr = &profiles[p.parentProfile].tree.get_child("profiles");
			treePtr->clear();
			*treePtr = *pt;

			//Updates all ancestors with the deletion
			updateProfileTree(p.parentProfile, ALL);
		}
		//If an item was found for a new selection
		if(item != NULL)
		{	//Set the current selection
			currentProfile = item->text(0).toStdString();
		}
		//If no profiles remain
		else
		{	//No selection
			currentProfile = "";
		}
		//Redraw honeyd configuration to reflect changes
		loadHaystack();
	}

	//If a child profile just delete, no more action needed
	else
	{
		//Erase the profile from the table and any nodes that use it
		updateProfile(DELETE_PROFILE, &profiles[name]);
	}
}

//Populates the window with the selected profile's options
void NovaConfig::loadProfile()
{
	struct port * pr = NULL;
	QTreeWidgetItem * item = NULL;
	//If the selected profile can be found
	if(profiles.find(currentProfile) != profiles.end())
	{
		loadInherited();
		//Clear the tree widget and load new selections
		QTreeWidgetItem * portCurrentItem;
		portCurrentItem = ui.portTreeWidget->currentItem();
		string portCurrentString = "";
		if(portCurrentItem != NULL)
			portCurrentString = portCurrentItem->text(0).toStdString()+ "_"
				+ portCurrentItem->text(1).toStdString() + "_" + portCurrentItem->text(2).toStdString();

		ui.portTreeWidget->clear();

		profile * p = &profiles[currentProfile];
		//Set the variables of the profile
		ui.profileEdit->setText((QString)p->name.c_str());
		ui.ethernetEdit->setText((QString)p->ethernet.c_str());
		ui.tcpActionComboBox->setCurrentIndex( ui.tcpActionComboBox->findText(p->tcpAction.c_str() ) );
		ui.udpActionComboBox->setCurrentIndex( ui.udpActionComboBox->findText(p->udpAction.c_str() ) );
		ui.icmpActionComboBox->setCurrentIndex( ui.icmpActionComboBox->findText(p->icmpAction.c_str() ) );
		ui.uptimeEdit->setText((QString)p->uptime.c_str());
		if(p->uptimeRange.compare(""))
		{
			ui.uptimeBehaviorComboBox->setCurrentIndex(1);
			ui.uptimeRangeEdit->setText((QString)p->uptimeRange.c_str());
			ui.uptimeRangeLabel->setVisible(true);
			ui.uptimeRangeEdit->setVisible(true);
		}
		else
		{
			ui.uptimeBehaviorComboBox->setCurrentIndex(0);
			ui.uptimeRangeLabel->setVisible(false);
			ui.uptimeRangeEdit->setVisible(false);
		}
		ui.personalityEdit->setText((QString)p->personality.c_str());
		ui.dhcpComboBox->setCurrentIndex(p->type);
		if(p->dropRate.size())
		{
			ui.dropRateSlider->setValue(atoi(p->dropRate.c_str()));
			stringstream ss;
			ss << ui.dropRateSlider->value() << "%";
			ui.dropRateSetting->setText((QString)ss.str().c_str());
		}
		else
		{
			ui.dropRateSetting->setText("0%");
			ui.dropRateSlider->setValue(0);
		}

		//Populate the port table
		for(uint i = 0; i < p->ports.size(); i++)
		{
			pr = &ports[p->ports[i].first];


			//These don't need to be deleted because the clear function
			// and destructor of the tree widget does that already.
			item = new QTreeWidgetItem(0);
			item->setText(0,(QString)pr->portNum.c_str());
			item->setText(1,(QString)pr->type.c_str());
			if(!pr->behavior.compare("script"))
				item->setText(2, (QString)pr->scriptName.c_str());
			else
				item->setText(2,(QString)pr->behavior.c_str());
			ui.portTreeWidget->insertTopLevelItem(i, item);

			QFont tempFont;
			tempFont = QFont(item->font(0));
			tempFont.setItalic(p->ports[i].second);
			item->setFont(0,tempFont);
			item->setFont(1,tempFont);
			item->setFont(2,tempFont);

			TreeItemComboBox *typeBox = new TreeItemComboBox(this, item);
			typeBox->addItem("TCP");
			typeBox->addItem("UDP");
			typeBox->setItemText(0, "TCP");
			typeBox->setItemText(1, "UDP");
			typeBox->setFont(tempFont);
			connect(typeBox, SIGNAL(notifyParent(QTreeWidgetItem *, bool)), this, SLOT(portTreeWidget_comboBoxChanged(QTreeWidgetItem *, bool)));

			TreeItemComboBox *behaviorBox = new TreeItemComboBox(this, item);
			behaviorBox->addItem("reset");
			behaviorBox->addItem("open");
			behaviorBox->addItem("block");
			behaviorBox->insertSeparator(3);
			for(ScriptTable::iterator it = scripts.begin(); it != scripts.end(); it++)
			{
				behaviorBox->addItem((QString)it->first.c_str());
			}
			behaviorBox->setFont(tempFont);
			connect(behaviorBox, SIGNAL(notifyParent(QTreeWidgetItem *, bool)), this, SLOT(portTreeWidget_comboBoxChanged(QTreeWidgetItem *, bool)));


			if(p->ports[i].second)
				item->setFlags(item->flags() & ~Qt::ItemIsEditable);
			else
				item->setFlags(item->flags() | Qt::ItemIsEditable);

			typeBox->setCurrentIndex(typeBox->findText(pr->type.c_str()));

			if(!pr->behavior.compare("script"))
				behaviorBox->setCurrentIndex(behaviorBox->findText(pr->scriptName.c_str()));
			else
				behaviorBox->setCurrentIndex(behaviorBox->findText(pr->behavior.c_str()));

			ui.portTreeWidget->setItemWidget(item, 1, typeBox);
			ui.portTreeWidget->setItemWidget(item, 2, behaviorBox);
			pr->item = item;
			if(!portCurrentString.compare(pr->portName))
				ui.portTreeWidget->setCurrentItem(pr->item);
		}
	}
	else
	{
		ui.portTreeWidget->clear();

		//Set the variables of the profile
		ui.profileEdit->clear();
		ui.ethernetEdit->clear();
		ui.tcpActionComboBox->setCurrentIndex(0);
		ui.udpActionComboBox->setCurrentIndex(0);
		ui.icmpActionComboBox->setCurrentIndex(0);
		ui.uptimeEdit->clear();
		ui.personalityEdit->clear();
		ui.uptimeBehaviorComboBox->setCurrentIndex(0);
		ui.uptimeRangeLabel->setVisible(false);
		ui.uptimeRangeEdit->setVisible(false);
		ui.dhcpComboBox->setCurrentIndex(0);
		ui.dropRateSlider->setValue(0);
		ui.dropRateSetting->setText("0%");
		ui.profileEdit->setEnabled(false);
		ui.ethernetEdit->setEnabled(false);
		ui.tcpActionComboBox->setEnabled(false);
		ui.udpActionComboBox->setEnabled(false);
		ui.icmpActionComboBox->setEnabled(false);
		ui.uptimeEdit->setEnabled(false);
		ui.personalityEdit->setEnabled(false);
		ui.uptimeBehaviorComboBox->setEnabled(false);
		ui.dhcpComboBox->setEnabled(false);
		ui.dropRateSlider->setEnabled(false);
	}
}

void NovaConfig::loadInherited()
{
	QFont tempFont;
	profile * p = &profiles[currentProfile];

	ui.ipModeCheckBox->setChecked(p->inherited[TYPE]);
	ui.ipModeCheckBox->setEnabled(p->parentProfile.compare(""));
	//We set again incase the checkbox was disabled (previous selection was root profile)
	ui.ipModeCheckBox->setChecked(p->inherited[TYPE]);

	tempFont = QFont(ui.IPModeLabel->font());
	tempFont.setItalic(p->inherited[TYPE]);
	ui.IPModeLabel->setFont(tempFont);
	ui.dhcpComboBox->setEnabled(!p->inherited[TYPE]);

	ui.udpActionComboBox->setCurrentIndex( ui.udpActionComboBox->findText(p->udpAction.c_str() ) );
	ui.icmpActionComboBox->setCurrentIndex( ui.icmpActionComboBox->findText(p->icmpAction.c_str() ) );

	ui.tcpCheckBox->setChecked(p->inherited[ICMP_ACTION]);
	ui.tcpCheckBox->setEnabled(p->parentProfile.compare(""));
	//We set again incase the checkbox was disabled (previous selection was root profile)
	ui.tcpCheckBox->setChecked(p->inherited[TCP_ACTION]);

	tempFont = QFont(ui.tcpActionComboBox->font());
	tempFont.setItalic(p->inherited[TCP_ACTION]);
	ui.tcpActionComboBox->setFont(tempFont);
	ui.tcpActionLabel->setFont(tempFont);
	ui.tcpActionComboBox->setEnabled(!p->inherited[TCP_ACTION]);

	ui.udpCheckBox->setChecked(p->inherited[UDP_ACTION]);
	ui.udpCheckBox->setEnabled(p->parentProfile.compare(""));
	//We set again incase the checkbox was disabled (previous selection was root profile)
	ui.udpCheckBox->setChecked(p->inherited[UDP_ACTION]);

	tempFont = QFont(ui.udpActionComboBox->font());
	tempFont.setItalic(p->inherited[UDP_ACTION]);
	ui.udpActionComboBox->setFont(tempFont);
	ui.udpActionLabel->setFont(tempFont);
	ui.udpActionComboBox->setEnabled(!p->inherited[UDP_ACTION]);

	ui.icmpCheckBox->setChecked(p->inherited[ICMP_ACTION]);
	ui.icmpCheckBox->setEnabled(p->parentProfile.compare(""));
	//We set again incase the checkbox was disabled (previous selection was root profile)
	ui.icmpCheckBox->setChecked(p->inherited[ICMP_ACTION]);

	tempFont = QFont(ui.icmpActionComboBox->font());
	tempFont.setItalic(p->inherited[ICMP_ACTION]);
	ui.icmpActionComboBox->setFont(tempFont);
	ui.icmpActionLabel->setFont(tempFont);
	ui.icmpActionComboBox->setEnabled(!p->inherited[ICMP_ACTION]);

	ui.ethernetCheckBox->setChecked(p->inherited[ETHERNET]);
	ui.ethernetCheckBox->setEnabled(p->parentProfile.compare(""));
	//We set again incase the checkbox was disabled (previous selection was root profile)
	ui.ethernetCheckBox->setChecked(p->inherited[ETHERNET]);
	tempFont = QFont(ui.ethernetLabel->font());
	tempFont.setItalic(p->inherited[ETHERNET]);
	ui.ethernetLabel->setFont(tempFont);
	ui.ethernetEdit->setFont(tempFont);
	ui.ethernetEdit->setEnabled(!p->inherited[ETHERNET]);
	ui.setEthernetButton->setEnabled(!p->inherited[ETHERNET]);

	ui.uptimeCheckBox->setChecked(p->inherited[UPTIME]);
	ui.uptimeCheckBox->setEnabled(p->parentProfile.compare(""));
	//We set again incase the checkbox was disabled (previous selection was root profile)
	ui.uptimeCheckBox->setChecked(p->inherited[UPTIME]);
	tempFont = QFont(ui.uptimeLabel->font());
	tempFont.setItalic(p->inherited[UPTIME]);
	ui.uptimeLabel->setFont(tempFont);
	ui.uptimeEdit->setFont(tempFont);
	ui.uptimeRangeEdit->setFont(tempFont);
	ui.uptimeRangeLabel->setFont(tempFont);
	ui.uptimeEdit->setEnabled(!p->inherited[UPTIME]);
	ui.uptimeBehaviorComboBox->setFont(tempFont);
	ui.uptimeBehaviorComboBox->setEnabled(!p->inherited[UPTIME]);

	ui.personalityCheckBox->setChecked(p->inherited[PERSONALITY]);
	ui.personalityCheckBox->setEnabled(p->parentProfile.compare(""));
	//We set again incase the checkbox was disabled (previous selection was root profile)
	ui.personalityCheckBox->setChecked(p->inherited[PERSONALITY]);
	tempFont = QFont(ui.personalityLabel->font());
	tempFont.setItalic(p->inherited[PERSONALITY]);
	ui.personalityLabel->setFont(tempFont);
	ui.personalityEdit->setFont(tempFont);
	ui.personalityEdit->setEnabled(!p->inherited[PERSONALITY]);
	ui.setPersonalityButton->setEnabled(!p->inherited[PERSONALITY]);

	ui.dropRateCheckBox->setChecked(p->inherited[DROP_RATE]);
	ui.dropRateCheckBox->setEnabled(p->parentProfile.compare(""));
	//We set again incase the checkbox was disabled (previous selection was root profile)
	ui.dropRateCheckBox->setChecked(p->inherited[DROP_RATE]);
	tempFont = QFont(ui.dropRateLabel->font());
	tempFont.setItalic(p->inherited[DROP_RATE]);
	ui.dropRateLabel->setFont(tempFont);
	ui.dropRateSetting->setFont(tempFont);
	ui.dropRateSlider->setEnabled(!p->inherited[DROP_RATE]);

	if(ui.ipModeCheckBox->isChecked())
		p->type = profiles[p->parentProfile].type;

	if(ui.ethernetCheckBox->isChecked())
	{
		p->ethernet = profiles[p->parentProfile].ethernet;
	}

	if(ui.uptimeCheckBox->isChecked())
	{
		p->uptime = profiles[p->parentProfile].uptime;
		p->uptimeRange = profiles[p->parentProfile].uptimeRange;
		if(profiles[p->parentProfile].uptimeRange.compare(""))
			ui.uptimeBehaviorComboBox->setCurrentIndex(1);
		else
			ui.uptimeBehaviorComboBox->setCurrentIndex(0);
	}

	if(ui.personalityCheckBox->isChecked())
		p->personality = profiles[p->parentProfile].personality;

	if(ui.dropRateCheckBox->isChecked())
		p->dropRate = profiles[p->parentProfile].dropRate;

	if(ui.tcpCheckBox->isChecked())
		p->tcpAction = profiles[p->parentProfile].tcpAction;

	if(ui.udpCheckBox->isChecked())
		p->udpAction = profiles[p->parentProfile].udpAction;

	if(ui.icmpCheckBox->isChecked())
		p->icmpAction = profiles[p->parentProfile].icmpAction;
}

//This is called to update all ancestor ptrees, does not update current ptree to do that call
// createProfileTree(profile.name) which will call this function afterwards currently this will
// only be called when a profile is deleted and has no current ptree. to update all other
// changes use createProfileTree
void NovaConfig::updateProfileTree(string name, updateDir direction)
{
	//Copy the profile
	profile p = profiles[name];
	bool up = false, down = false;
	switch(direction)
	{
		case ALL:
			up = true;
			down = true;
			break;
		case UP:
			up = true;
			break;
		case DOWN:
			down = true;
			break;
		default:
			break;
	}
	if(down)
	{
		//Find all children
		for(ProfileTable::iterator it = profiles.begin(); it != profiles.end(); it++)
		{
			//If child is found
			if(!it->second.parentProfile.compare(p.name))
			{
				//Update the child
				updateProfileTree(it->first, DOWN);
				//Put the child in the parent's ptree
				p.tree.add_child("profiles.profile", it->second.tree);
			}
		}
		profiles[name] = p;
	}
	//If the original calling profile has a parent to update
	if(p.parentProfile.compare("") && up)
	{
		//Get the parents name and create an empty ptree
		profile parent = profiles[p.parentProfile];
		ptree pt;
		pt.clear();
		pt.add_child("profile", p.tree);

		//Find all children of the parent and put them in the empty ptree
		// Ideally we could just replace the individual child but the data structure doesn't seem
		// to support this very well when all keys in the ptree (ie. profiles.profile) are the same
		// because the ptree iterators just don't seem to work correctly and documentation is very poor
		for(ProfileTable::iterator it = profiles.begin(); it != profiles.end(); it++)
		{
			if(!it->second.parentProfile.compare(parent.name))
			{
				pt.add_child("profile", it->second.tree);
			}
		}
		//Replace the parent's profiles subtree (stores all children) with the new one
		parent.tree.put_child("profiles", pt);
		profiles[parent.name] = parent;
		//Recursively ascend to update all ancestors
		updateProfileTree(parent.name, UP);
	}
}

//This is used when a profile is cloned, it allows us to copy a ptree and extract all children from it
// it is exactly the same as novagui's xml extraction functions except that it gets the ptree from the
// cloned profile and it asserts a profile's name is unique and changes the name if it isn't
void NovaConfig::loadProfilesFromTree(string parent)
{
	using boost::property_tree::ptree;
	ptree * ptr, pt = profiles[parent].tree;
	openlog("NovaGUI", OPEN_SYSL, LOG_AUTHPRIV);
	try
	{
		BOOST_FOREACH(ptree::value_type &v, pt.get_child("profiles"))
		{
			//Generic profile, essentially a honeyd template
			if(!string(v.first.data()).compare("profile"))
			{
				profile p = profiles[parent];
				//Root profile has no parent
				p.parentProfile = parent;
				p.tree = v.second;

				for(uint i = 0; i < INHERITED_MAX; i++)
				{
					p.inherited[i] = true;
				}

				//Name required, DCHP boolean intialized (set in loadProfileSet)
				p.name = v.second.get<std::string>("name");
				try
				{
					p.type = (profileType)v.second.get<int>("type");
					p.inherited[TYPE] = false;
				}
				catch(...){}


				//Asserts the name is unique, if it is not it finds a unique name
				// up to the range of 2^32
				string profileStr = p.name;
				stringstream ss;
				uint i = 0, j = 0;
				j = ~j; //2^32-1

				while((profiles.find(p.name) != profiles.end()) && (i < j))
				{
					ss.str("");
					i++;
					ss << profileStr << "-" << i;
					p.name = ss.str();
				}
				p.tree.put<std::string>("name", p.name);

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
				updateProfileTree(p.name, ALL);

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
				syslog(SYSL_ERR, "File: %s Line: %d Invalid XML Path %s", __FILE__, __LINE__, string(v.first.data()).c_str());
			}
		}
	}
	catch(std::exception &e)
	{
		syslog(SYSL_ERR, "File: %s Line: %d Problem loading Profiles: %s", __FILE__, __LINE__, string(e.what()).c_str());
		mainwindow->prompter->DisplayPrompt(mainwindow->HONEYD_LOAD_FAIL, "Problem loading Profiles: " + string(e.what()));
	}
	closelog();
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
				p->inherited[TCP_ACTION] = false;
				continue;
			}
			prefix = "UDP";
			if(!string(v.first.data()).compare(prefix))
			{
				p->udpAction = v.second.data();
				p->inherited[UDP_ACTION] = false;
				continue;
			}
			prefix = "ICMP";
			if(!string(v.first.data()).compare(prefix))
			{
				p->icmpAction = v.second.data();
				p->inherited[ICMP_ACTION] = false;
				continue;
			}
			prefix = "personality";
			if(!string(v.first.data()).compare(prefix))
			{
				p->personality = v.second.data();
				p->inherited[PERSONALITY] = false;
				continue;
			}
			prefix = "ethernet";
			if(!string(v.first.data()).compare(prefix))
			{
				p->ethernet = v.second.data();
				p->inherited[ETHERNET] = false;
				continue;
			}
			prefix = "uptime";
			if(!string(v.first.data()).compare(prefix))
			{
				p->uptime = v.second.data();
				p->inherited[UPTIME] = false;
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
				p->inherited[DROP_RATE] = false;
				continue;
			}
		}
	}
	catch(std::exception &e)
	{
		openlog("NovaGUI", OPEN_SYSL, LOG_AUTHPRIV);
		syslog(SYSL_ERR, "File: %s Line: %d Problem loading profile set parameters: %s", __FILE__, __LINE__, string(e.what()).c_str());
		closelog();
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
		for(uint i = 0; i < p->ports.size(); i++)
		{
			p->ports[i].second = true;
		}
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
						if(!prt->portNum.compare(ports[p->ports[i].first].portNum) && !prt->type.compare(ports[p->ports[i].first].type))
						{
							p->ports.erase(p->ports.begin()+i);
						}
					}
					//Add specified port
					pair<string, bool> portPair;
					portPair.first = prt->portName;
					portPair.second = false;
					if(!p->ports.size())
						p->ports.push_back(portPair);
					else
					{
						uint i = 0;
						for(i = 0; i < p->ports.size(); i++)
						{
							port * temp = &ports[p->ports[i].first];
							if((atoi(temp->portNum.c_str())) < (atoi(prt->portNum.c_str())))
							{
								continue;
							}
							break;
						}
						if(i < p->ports.size())
							p->ports.insert(p->ports.begin()+i, portPair);
						else
							p->ports.push_back(portPair);
					}
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
		openlog("NovaGUI", OPEN_SYSL, LOG_AUTHPRIV);
		syslog(SYSL_ERR, "File: %s Line: %d Problem loading profile add parameters: %s", __FILE__, __LINE__, string(e.what()).c_str());
		closelog();
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

			for(uint i = 0; i < INHERITED_MAX; i++)
			{
				prof.inherited[i] = true;
			}

			string profileStr = prof.name;
			stringstream ss;
			uint i = 0, j = 0;
			j = ~j; //2^32-1

			//Asserts the name is unique, if it is not it finds a unique name
			// up to the range of 2^32
			while((profiles.find(prof.name) != profiles.end()) && (i < j))
			{
				ss.str("");
				i++;
				ss << profileStr << "-" << i;
				prof.name = ss.str();
			}
			prof.tree.put<std::string>("name", prof.name);

			try //Conditional: If profile overrides type
			{
				prof.type = (profileType)v.second.get<int>("type");
				prof.inherited[TYPE] = false;
			}
			catch(...){}

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
			updateProfileTree(prof.name, ALL);

			try //Conditional: if profile has children (not leaf)
			{
				loadSubProfiles(prof.name);
			}
			catch(...){}
		}
	}
	catch(std::exception &e)
	{
		openlog("NovaGUI", OPEN_SYSL, LOG_AUTHPRIV);
		syslog(SYSL_ERR, "File: %s Line: %d Problem loading sub profiles: %s", __FILE__, __LINE__, string(e.what()).c_str());
		closelog();
	}
}

//Draws all profile heirarchy in the tree widget
void NovaConfig::loadAllProfiles()
{
	loadingItems = true;
	ui.hsProfileTreeWidget->clear();
	ui.profileTreeWidget->clear();
	ui.hsProfileTreeWidget->sortByColumn(0,Qt::AscendingOrder);
	ui.profileTreeWidget->sortByColumn(0,Qt::AscendingOrder);

	if(profiles.size())
	{
		//First sets all pointers to NULL, clear has already deleted so these pointers are invalid
		// createProfileItem then uses these NULL pointers as a flag to avoid creating duplicate items
		for(ProfileTable::iterator it = profiles.begin(); it != profiles.end(); it++)
		{
			it->second.item = NULL;
			it->second.profileItem = NULL;
		}
		//calls createProfileItem on every profile, this will first assert that all ancestors have items
		// and create them if not to draw the table correctly, thus the need for the NULL pointer as a flag
		for(ProfileTable::iterator it = profiles.begin(); it != profiles.end(); it++)
		{
			createProfileItem(it->first);
		}
		//Sets the current selection to the original selection
		ui.profileTreeWidget->setCurrentItem(profiles[currentProfile].profileItem);
		//populates the window and expand the profile heirarchy
		ui.hsProfileTreeWidget->expandAll();
		ui.profileTreeWidget->expandAll();
		loadProfile();
	}
	//If no profiles exist, do nothing and ensure no profile is selected
	else
	{
		currentProfile = "";
	}
	loadingItems = false;
}

//Creates tree widget items for a profile and all ancestors if they need one.
void NovaConfig::createProfileItem(string pstr)
{
	profile p = profiles[pstr];
	//If the profile hasn't had an item created yet
	if(p.item == NULL)
	{
		QTreeWidgetItem * item = NULL;
		//get the name
		string profileStr = p.name;

		//if the profile has no parents create the item at the top level
		if(!p.parentProfile.compare(""))
		{
			/*NOTE*/
			//These items don't need to be deleted because the clear function
			// and destructor of the tree widget does that already.
			item = new QTreeWidgetItem(ui.hsProfileTreeWidget,0);
			item->setText(0, (QString)profileStr.c_str());
			p.item = item;

			//Create the profile item for the profile tree
			item = new QTreeWidgetItem(ui.profileTreeWidget,0);
			item->setText(0, (QString)profileStr.c_str());
			p.profileItem = item;
		}
		//if the profile has ancestors
		else
		{
			//find the parent and assert that they have an item
			if(profiles.find(p.parentProfile) != profiles.end())
			{
				profile parent = profiles[p.parentProfile];

				if(parent.item == NULL)
				{
					//if parent has no item recursively ascend until all parents do
					createProfileItem(p.parentProfile);
					parent = profiles[p.parentProfile];
				}
				//Now that all ancestors have items, create the profile's item

				//*NOTE*
				//These items don't need to be deleted because the clear function
				// and destructor of the tree widget does that already.
				item = new QTreeWidgetItem(parent.item,0);
				item->setText(0, (QString)profileStr.c_str());
				p.item = item;

				//Create the profile item for the profile tree
				item = new QTreeWidgetItem(parent.profileItem,0);
				item->setText(0, (QString)profileStr.c_str());
				p.profileItem = item;
			}
		}
		profiles[p.name] = p;
	}
}

//Populates an emptry ptree with all used values
void NovaConfig::createProfileTree(string name)
{
	ptree temp;
	profile p = profiles[name];
	if(p.name.compare(""))
		temp.put<std::string>("name", p.name);
	if(p.tcpAction.compare("") && !p.inherited[TCP_ACTION])
		temp.put<std::string>("set.TCP", p.tcpAction);
	if(p.udpAction.compare("") && !p.inherited[UDP_ACTION])
		temp.put<std::string>("set.UDP", p.udpAction);
	if(p.icmpAction.compare("") && !p.inherited[ICMP_ACTION])
		temp.put<std::string>("set.ICMP", p.icmpAction);
	if(p.personality.compare("") && !p.inherited[PERSONALITY])
		temp.put<std::string>("set.personality", p.personality);
	if(p.ethernet.compare("") && !p.inherited[ETHERNET])
		temp.put<std::string>("set.ethernet", p.ethernet);
	if(p.uptime.compare("") && !p.inherited[UPTIME])
		temp.put<std::string>("set.uptime", p.uptime);
	if(p.uptimeRange.compare("") && !p.inherited[UPTIME])
		temp.put<std::string>("set.uptimeRange", p.uptimeRange);
	if(p.dropRate.compare("") && !p.inherited[DROP_RATE])
		temp.put<std::string>("set.dropRate", p.dropRate);
	if(!p.inherited[TYPE])
		temp.put<int>("type", p.type);

	//Populates the ports, if none are found create an empty field because it is expected.
	ptree pt;
	if(p.ports.size())
	{
		temp.put_child("add.ports",pt);
		for(uint i = 0; i < p.ports.size(); i++)
		{
			//If the port isn't inherited
			if(!p.ports[i].second)
				temp.add<std::string>("add.ports.port", p.ports[i].first);
		}
	}
	else
	{
		temp.put_child("add.ports",pt);
	}
	//put empty ptree in profiles as well because it is expected, does not matter that it is the same
	// as the one in add.ports if profile has no ports, since both are empty.
	temp.put_child("profiles", pt);

	//copy the tree over and update ancestors
	p.tree = temp;
	profiles[name] = p;
	updateProfileTree(name, ALL);
}

//Either deletes a profile or updates the window to reflect a profile name change
void NovaConfig::updateProfile(bool deleteProfile, profile * p)
{
	//If the profile is being deleted
	if(deleteProfile)
	{
		vector<string> delList;
		for(NodeTable::iterator it = nodes.begin(); it != nodes.end(); it++)
		{
			if(!it->second.pfile.compare(p->name))
			{
				delList.push_back(it->first);
			}
		}
		while(!delList.empty())
		{
			deleteNode(&nodes[delList.back()]);
			delList.pop_back();
		}
		profiles.erase(p->name);
	}
	//If the profile needs to be updated
	else
	{
		string pfile = p->profileItem->text(0).toStdString();
		profile tempPfile = * p;

		//If item text and profile name don't match, we need to update
		if(tempPfile.name.compare(pfile))
		{
			//Set the profile to the correct name and put the profile in the table
			profiles[pfile] = tempPfile;
			profiles[pfile].name = pfile;

			//Find all nodes who use this profile and update to the new one
			for(NodeTable::iterator it = nodes.begin(); it != nodes.end(); it++)
			{
				if(!it->second.pfile.compare(tempPfile.name))
				{
					it->second.pfile = pfile;
				}
			}
			if(!tempPfile.name.compare(currentProfile))
				currentProfile = pfile;
			//Remove the old profile and update the currentProfile pointer
			profiles.erase(tempPfile.name);
		}
	}
}


void NovaConfig::setInputValidators()
{
	// Allows positive integers
	QIntValidator *uintValidator = new QIntValidator();
	uintValidator->setBottom(0);

	// Allows positive doubles
	QDoubleValidator *udoubleValidator = new QDoubleValidator();
	udoubleValidator->setBottom(0);

	// Disallows whitespace
	QRegExp rx("\\S+");
	QRegExpValidator *noSpaceValidator = new QRegExpValidator(rx, this);

	// Set up input validators so user can't enter obviously bad data in the QLineEdits
	// General settings
	ui.interfaceEdit->setValidator(noSpaceValidator);
	ui.saAttemptsMaxEdit->setValidator(uintValidator);
	ui.saAttemptsTimeEdit->setValidator(udoubleValidator);
	ui.saPortEdit->setValidator(uintValidator);
	ui.tcpTimeoutEdit->setValidator(uintValidator);
	ui.tcpFrequencyEdit->setValidator(uintValidator);

	// Profile page
	ui.uptimeEdit->setValidator(uintValidator);
	ui.uptimeRangeEdit->setValidator(uintValidator);

	// Classification engine settings
	ui.ceIntensityEdit->setValidator(uintValidator);
	ui.ceFrequencyEdit->setValidator(uintValidator);
	ui.ceThresholdEdit->setValidator(udoubleValidator);
	ui.ceErrorEdit->setValidator(udoubleValidator);

	// Doppelganger
	// TODO: Make a custom validator for ipv4 and ipv6 IP addresses
	// For now we just make sure someone doesn't enter whitespace
	ui.dmIPEdit->setValidator(noSpaceValidator);
}

/******************************************
 * Profile Menu GUI Signals ***************/


/* This loads the profile config menu for the profile selected */
void NovaConfig::on_profileTreeWidget_itemSelectionChanged()
{
	if(!loadingItems && profiles.size())
	{
		//Save old profile
		loadingItems = true;
		saveProfile();
		if(!ui.profileTreeWidget->selectedItems().isEmpty())
		{
			QTreeWidgetItem * item = ui.profileTreeWidget->selectedItems().first();
			currentProfile = item->text(0).toStdString();
		}
		loadProfile();
		loadingItems = false;
	}
}

//Self explanatory, see deleteProfile for details
void NovaConfig::on_deleteButton_clicked()
{
	Q_EMIT on_actionProfileDelete_triggered();
}

void NovaConfig::on_actionProfileDelete_triggered()
{
	if((!ui.profileTreeWidget->selectedItems().isEmpty()) && profiles.size())
	{
		bool nodeExists = false;
		//Find out if any nodes use this profile
		for(NodeTable::iterator it = nodes.begin(); it != nodes.end(); it++)
		{
			//if we find a node using this profile
			if(!it->second.pfile.compare(currentProfile))
				nodeExists = true;
		}
		if(nodeExists)
			syslog(SYSL_ERR, "File: %s Line: %d ERROR: A Node is currently using this profile.", __FILE__, __LINE__);
		//TODO appropriate display prompt here
		else
			deleteProfile(currentProfile);
	}
	loadAllNodes();
}

//Creates a base profile with default values seen below
void NovaConfig::on_addButton_clicked()
{
	Q_EMIT on_actionProfileAdd_triggered();
}

void NovaConfig::on_actionProfileAdd_triggered()
{
	struct profile temp;
	temp.name = "New Profile";

	stringstream ss;
	uint i = 0, j = 0;
	j = ~j; // 2^32-1

	//Finds a unique identifier
	while((profiles.find(temp.name) != profiles.end()) && (i < j))
	{
		i++;
		ss.str("");
		ss << "New Profile-" << i;
		temp.name = ss.str();
	}
	//If there is currently a selected profile, that profile will be the parent of the new profile
	if(profiles.find(currentProfile) != profiles.end())
	{
		string tempName = temp.name;
		temp = profiles[currentProfile];
		temp.name = tempName;
		temp.parentProfile = currentProfile;
		for(uint i = 0; i < INHERITED_MAX; i++)
			temp.inherited[i] = true;
		for(uint i = 0; i < temp.ports.size(); i++)
			temp.ports[i].second = true;
		currentProfile = temp.name;
	}
	//If no profile is selected the profile is a root node
	else
	{
		temp.parentProfile = "";
		temp.ethernet = "";
		temp.personality = "";
		temp.tcpAction = "reset";
		temp.udpAction = "reset";
		temp.icmpAction = "reset";
		temp.type = static_IP;
		temp.uptime = "0";
		temp.dropRate = "0";
		temp.ports.clear();
		currentProfile = temp.name;
		for(uint i = 0; i < INHERITED_MAX; i++)
			temp.inherited[i] = false;
	}
	//Puts the profile in the table, creates a ptree and loads the new configuration
	profiles[temp.name] = temp;
	createProfileTree(temp.name);
	loadAllProfiles();
	loadAllNodes();
}


//Copies a profile and all of it's descendants
void NovaConfig::on_cloneButton_clicked()
{
	Q_EMIT on_actionProfileClone_triggered();
}

void NovaConfig::on_actionProfileClone_triggered()
{

	//Do nothing if no profiles
	if(profiles.size())
	{
		loadingItems = true;
		QTreeWidgetItem * item = ui.profileTreeWidget->selectedItems().first();
		string profileStr = item->text(0).toStdString();
		profile p = profiles[currentProfile];

		stringstream ss;
		uint i = 1, j = 0;
		j = ~j; //2^32-1

		//Since we are cloning, it will already be a duplicate
		ss.str("");
		ss << profileStr << "-" << i;
		p.name = ss.str();

		//Check for name in use, if so increase number until unique name is found
		while((profiles.find(p.name) != profiles.end()) && (i < j))
		{
			ss.str("");
			i++;
			ss << profileStr << "-" << i;
			p.name = ss.str();
		}
		p.tree.put<std::string>("name",p.name);
		//Change the profile name and put in the table, update the current profile
		//Extract all descendants, create a ptree, update with new configuration
		profiles[p.name] = p;
		loadProfilesFromTree(p.name);
		updateProfileTree(p.name, ALL);
		loadAllProfiles();
		loadAllNodes();
		loadingItems = false;
	}
}

void NovaConfig::on_profileEdit_editingFinished()
{
	if(!loadingItems && !profiles.empty())
	{
		loadingItems = true;
		profiles[currentProfile].item->setText(0,ui.profileEdit->displayText());
		profiles[currentProfile].profileItem->setText(0,ui.profileEdit->displayText());
		//If the name has changed we need to move it in the profile hash table and point all
		//nodes that use the profile to the new location.
		updateProfile(UPDATE_PROFILE, &profiles[currentProfile]);
		saveProfile();
		loadProfile();
		loadingItems = false;
		loadAllNodes();
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
			item->setText(0, (QString)n->name.c_str());
			item->setText(1, (QString)n->pfile.c_str());
			n->item = item;

			//Create the node item for the node edit tree
			item = new QTreeWidgetItem(it->second.nodeItem, 0);
			item->setText(0, (QString)n->name.c_str());
			item->setText(1, (QString)n->pfile.c_str());

			TreeItemComboBox *pfileBox = new TreeItemComboBox(this, item);
			uint i = 0;
			for(ProfileTable::iterator it = profiles.begin(); it != profiles.end(); it++)
			{
				pfileBox->addItem(it->first.c_str());
				pfileBox->setItemText(i, it->first.c_str());
				i++;
			}

			connect(pfileBox, SIGNAL(notifyParent(QTreeWidgetItem *, bool)), this, SLOT(nodeTreeWidget_comboBoxChanged(QTreeWidgetItem *, bool)));

			pfileBox->setCurrentIndex(pfileBox->findText(n->pfile.c_str()));

			ui.nodeTreeWidget->setItemWidget(item, 1, pfileBox);
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
		loadingItems = false;
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
	loadingItems = false;
	loadAllNodes();
}

// Removes the node from item widgets and data structures.
void NovaConfig::deleteNode(node *n)
{
	ui.nodeTreeWidget->removeItemWidget(n->nodeItem, 0);
	ui.hsNodeTreeWidget->removeItemWidget(n->item, 0);
	subnet * s = &subnets[n->sub];

	for(uint i = 0; i < s->nodes.size(); i++)
	{
		if(!s->nodes[i].compare(n->name))
		{
			s->nodes.erase(s->nodes.begin()+i);
		}
	}
	nodes.erase(n->name);
}

/******************************************
 * Node Menu GUI Signals ******************/

//The current selection in the node list
void NovaConfig::on_nodeTreeWidget_itemSelectionChanged()
{
	//If the user is changing the selection AND something is selected
	if(!loadingItems)
	{
		loadingItems = true;
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
		loadingItems = false;
	}
}

void NovaConfig::nodeTreeWidget_comboBoxChanged(QTreeWidgetItem * item, bool edited)
{
	if(!loadingItems && edited)
	{
		loadingItems = true;
		ui.nodeTreeWidget->setCurrentItem(item);
		string oldPfile;
		if(!ui.nodeTreeWidget->selectedItems().isEmpty())
		{
			node * n = &nodes[item->text(0).toStdString()];
			oldPfile = n->pfile;
			TreeItemComboBox * pfileBox = (TreeItemComboBox* )ui.nodeTreeWidget->itemWidget(item, 1);
			n->pfile = pfileBox->currentText().toStdString();
		}
		if(updateNodeTypes())
		{
			item = nodes[currentNode].nodeItem;
			ui.nodeTreeWidget->setFocus(Qt::OtherFocusReason);
			ui.nodeTreeWidget->setCurrentItem(item);
			loadingItems = false;
		}
		else
		{
			node * n = &nodes[item->text(0).toStdString()];
			TreeItemComboBox * pfileBox = (TreeItemComboBox* )ui.nodeTreeWidget->itemWidget(item, 1);
			n->pfile = oldPfile;
			pfileBox->setCurrentIndex(pfileBox->findText((QString)oldPfile.c_str()));

			updateNodeTypes();
			item = nodes[currentNode].nodeItem;
			ui.nodeTreeWidget->setFocus(Qt::OtherFocusReason);
			ui.nodeTreeWidget->setCurrentItem(item);
			loadingItems = false;
		}
	}
	else if(!loadingItems && !edited)
	{
		loadingItems = true;
		ui.nodeTreeWidget->setFocus(Qt::OtherFocusReason);
		ui.nodeTreeWidget->setCurrentItem(item);
		loadingItems = false;
	}
}

// Right click menus for the Node tree
void NovaConfig::on_actionNodeAdd_triggered()
{

}

void NovaConfig::on_actionNodeDelete_triggered()
{
	if(subnets.size() || nodes.size())
		deleteNodes();
}

void NovaConfig::on_actionNodeClone_triggered()
{

}

void NovaConfig::on_actionNodeEdit_triggered()
{

}

void NovaConfig::on_actionNodeEnable_triggered()
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
	loadAllNodes();
	loadingItems = true;
	if(selectedSubnet)
		ui.nodeTreeWidget->setCurrentItem(subnets[currentSubnet].nodeItem);
	else
		ui.nodeTreeWidget->setCurrentItem(nodes[currentNode].nodeItem);
	loadingItems = false;
}

void NovaConfig::on_actionNodeDisable_triggered()
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
	loadAllNodes();
	loadingItems = true;
	if(selectedSubnet)
		ui.nodeTreeWidget->setCurrentItem(subnets[currentSubnet].nodeItem);
	else
		ui.nodeTreeWidget->setCurrentItem(nodes[currentNode].nodeItem);
	loadingItems = false;
}


//Not currently used, will be implemented in the new GUI design TODO
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
//Not currently used, will be implemented in the new GUI design TODO
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
//Not currently used, will be implemented in the new GUI design TODO
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

//Calls the function(s) to remove the node(s)
void NovaConfig::on_nodeDeleteButton_clicked()
{
	Q_EMIT on_actionNodeDelete_triggered();
}

//Enables a node or a subnet in honeyd
void NovaConfig::on_nodeEnableButton_clicked()
{
	Q_EMIT on_actionNodeEnable_triggered();
}

//Disables a node or a subnet in honeyd
void NovaConfig::on_nodeDisableButton_clicked()
{
	Q_EMIT on_actionNodeDisable_triggered();
}

void NovaConfig::on_setEthernetButton_clicked()
{
	displayMACPrefixWindow();
}

void NovaConfig::on_setPersonalityButton_clicked()
{
	displayNmapPersonalityTree();
}

void NovaConfig::on_dropRateSlider_valueChanged()
{
	stringstream ss;
	ss << ui.dropRateSlider->value() << "%";
	ui.dropRateSetting->setText((QString)ss.str().c_str());
}
