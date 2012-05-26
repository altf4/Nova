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

#include <boost/foreach.hpp>
#include <QRadioButton>
#include <netinet/in.h>
#include <QFileDialog>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <errno.h>
#include <fstream>
#include <QDir>

#include "Logger.h"
#include "Config.h"
#include "NovaUtil.h"
#include "novaconfig.h"
#include "subnetPopup.h"
#include "NovaComplexDialog.h"

using boost::property_tree::ptree;
using namespace Nova;
using namespace std;


/************************************************
 * Construct and Initialize GUI
 ************************************************/

NovaConfig::NovaConfig(QWidget *parent, string home)
    : QMainWindow(parent)
{
	m_portMenu = new QMenu(this);
	m_profileTreeMenu = new QMenu(this);
	m_nodeTreeMenu = new QMenu(this);
	m_loading = new QMutex(QMutex::NonRecursive);

	m_honeydConfig = new HoneydConfiguration();

    //Keys used to maintain and lookup current selections
    m_currentProfile = "";
    m_currentNode = "";
    m_currentSubnet = "";

    //flag to avoid GUI signal conflicts
    m_editingItems = false;
    m_selectedSubnet = false;
    m_loadingDefaultActions = false;

	//store current directory / base path for Nova
	m_homePath = home;

	//Store parent and load UI
	m_mainwindow = (NovaGUI*)parent;
	//Set up a Reference to the dialog prompter
	m_prompter = m_mainwindow->m_prompter;
	// Set up the GUI
	ui.setupUi(this);

	// for internal use only
	ui.portTreeWidget->setColumnHidden(3, true);

	SetInputValidators();
	m_loading->lock();
	m_radioButtons = new QButtonGroup(ui.loopbackGroupBox);
	m_interfaceCheckBoxes = new QButtonGroup(ui.interfaceGroupBox);
	//Read NOVAConfig, pull honeyd info from parent, populate GUI
	LoadNovadPreferences();
	m_honeydConfig->LoadAllTemplates();
	LoadHaystackConfiguration();

	m_loading->unlock();
	// Populate the dialog menu
	for (uint i = 0; i < m_mainwindow->m_prompter->m_registeredMessageTypes.size(); i++)
	{
		ui.msgTypeListWidget->insertItem(i, new QListWidgetItem(QString::fromStdString(
				m_mainwindow->m_prompter->m_registeredMessageTypes[i].descriptionUID)));
	}

	ui.menuTreeWidget->expandAll();

	ui.featureList->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui.featureList, SIGNAL(customContextMenuRequested(const QPoint &)), this,
			SLOT(onFeatureClick(const QPoint &)));
	connect(m_interfaceCheckBoxes, SIGNAL(buttonReleased(QAbstractButton *)), this,
			SLOT(interfaceCheckBoxes_buttonClicked(QAbstractButton *)));
}

NovaConfig::~NovaConfig()
{

}

void NovaConfig::interfaceCheckBoxes_buttonClicked(QAbstractButton * button)
{
	if(m_interfaceCheckBoxes->checkedButton() == NULL)
	{
		((QCheckBox *)button)->setChecked(true);
	}
}

void NovaConfig::contextMenuEvent(QContextMenuEvent *event)
{
	if(ui.portTreeWidget->hasFocus() || ui.portTreeWidget->underMouse())
	{
		m_portMenu->clear();
		m_portMenu->addAction(ui.actionAddPort);
		if(ui.portTreeWidget->topLevelItemCount())
		{
			m_portMenu->addSeparator();
			m_portMenu->addAction(ui.actionToggle_Inherited);
			m_portMenu->addAction(ui.actionDeletePort);
		}
		QPoint globalPos = event->globalPos();
		m_portMenu->popup(globalPos);
	}
	else if(ui.profileTreeWidget->hasFocus() || ui.profileTreeWidget->underMouse())
	{
		m_profileTreeMenu->clear();
		m_profileTreeMenu->addAction(ui.actionProfileAdd);
		m_profileTreeMenu->addSeparator();
		m_profileTreeMenu->addAction(ui.actionProfileClone);
		m_profileTreeMenu->addSeparator();
		m_profileTreeMenu->addAction(ui.actionProfileDelete);

		QPoint globalPos = event->globalPos();
		m_profileTreeMenu->popup(globalPos);
	}
	else if(ui.nodeTreeWidget->hasFocus() || ui.nodeTreeWidget->underMouse())
	{
		m_nodeTreeMenu->clear();
		m_nodeTreeMenu->addAction(ui.actionSubnetAdd);
		m_nodeTreeMenu->addSeparator();
		m_nodeTreeMenu->addAction(ui.actionNodeEdit);
		if(m_selectedSubnet)
		{
			ui.actionNodeEdit->setText("Edit Subnet");
			ui.actionNodeEnable->setText("Enable Subnet");
			ui.actionNodeDisable->setText("Disable Subnet");
			m_nodeTreeMenu->addAction(ui.actionNodeAdd);
			m_nodeTreeMenu->addSeparator();
		}
		else
		{
			ui.actionNodeEdit->setText("Edit Node");
			ui.actionNodeEnable->setText("Enable Node");
			ui.actionNodeDisable->setText("Disable Node");
			m_nodeTreeMenu->addAction(ui.actionNodeCustomizeProfile);
			m_nodeTreeMenu->addSeparator();
			m_nodeTreeMenu->addAction(ui.actionNodeAdd);
			m_nodeTreeMenu->addAction(ui.actionNodeClone);
			m_nodeTreeMenu->addSeparator();

		}
		m_nodeTreeMenu->addAction(ui.actionNodeDelete);
		m_nodeTreeMenu->addSeparator();
		m_nodeTreeMenu->addAction(ui.actionNodeEnable);
		m_nodeTreeMenu->addAction(ui.actionNodeDisable);

		QPoint globalPos = event->globalPos();
		m_nodeTreeMenu->popup(globalPos);
	}
}

void NovaConfig::on_actionNo_Action_triggered()
{
	return;
}

void NovaConfig::on_actionToggle_Inherited_triggered()
{
	if(!m_loading->tryLock())
	{
		return;
	}
	if(!ui.portTreeWidget->selectedItems().empty())
	{
		port *prt = NULL;
		for(PortTable::iterator it = m_honeydConfig->m_ports.begin(); it != m_honeydConfig->m_ports.end(); it++)
		{
			if(IsPortTreeWidgetItem(it->second.portName, ui.portTreeWidget->currentItem()))
			{
				//iterators are copies not the actual items
				prt = &m_honeydConfig->m_ports[it->second.portName];
				break;
			}
		}
		profile *p = &m_honeydConfig->m_profiles[m_currentProfile];
		for(uint i = 0; i < p->ports.size(); i++)
		{
			if(!p->ports[i].first.compare(prt->portName))
			{
				//If the port is inherited we can just make it explicit
				if(p->ports[i].second)
				{
					p->ports[i].second = false;
				}

				//If the port isn't inherited and the profile has parents
				else if(p->parentProfile.compare(""))
				{
					profile *parent = &m_honeydConfig->m_profiles[p->parentProfile];
					uint j = 0;
					//check for the inherited port
					for(j = 0; j < parent->ports.size(); j++)
					{
						port temp = m_honeydConfig->m_ports[parent->ports[j].first];
						if(!prt->portNum.compare(temp.portNum) && !prt->type.compare(temp.type))
						{
							p->ports[i].first = temp.portName;
							p->ports[i].second = true;
						}
					}
					if(!p->ports[i].second)
					{
						m_loading->unlock();
						m_mainwindow->m_prompter->DisplayPrompt(m_mainwindow->CANNOT_INHERIT_PORT, "The selected port cannot be inherited"
								" from any ancestors, would you like to keep it?", ui.actionNo_Action, ui.actionDeletePort, this);
						m_loading->lock();
					}
				}
				//If the port isn't inherited and the profile has no parent
				else
				{
					LOG(ERROR, "Cannot inherit without any ancestors.", "");
					m_mainwindow->m_prompter->DisplayPrompt(m_mainwindow->NO_ANCESTORS,
						"Cannot inherit without any ancestors.");
				}
				break;
			}
		}
		LoadProfileSettings();
		SaveProfileSettings();
	}
	m_loading->unlock();
}

void NovaConfig::on_actionAddPort_triggered()
{
	if(m_loading->tryLock())
	{
		if(m_honeydConfig->m_profiles.keyExists(m_currentProfile))
		{
			profile p = m_honeydConfig->m_profiles[m_currentProfile];

			port pr;
			pr.portNum = "1";
			pr.type = "TCP";
			pr.behavior = "open";
			pr.portName = "1_TCP_open";
			pr.scriptName = "";

			//These don't need to be deleted because the clear function
			// and destructor of the tree widget does that already.
			QTreeWidgetItem *item = new QTreeWidgetItem(0);
			item->setText(0,(QString)pr.portNum.c_str());
			item->setText(1,(QString)pr.type.c_str());
			if(!pr.behavior.compare("script"))
			{
				item->setText(2, (QString)pr.scriptName.c_str());
			}
			else
			{
				item->setText(2,(QString)pr.behavior.c_str());
			}
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

			vector<string> scriptNames = m_honeydConfig->GetScriptNames();
			for(vector<string>::iterator it = scriptNames.begin(); it != scriptNames.end(); it++)
			{
				behaviorBox->addItem((QString)(*it).c_str());
			}

			connect(behaviorBox, SIGNAL(notifyParent(QTreeWidgetItem *, bool)), this, SLOT(portTreeWidget_comboBoxChanged(QTreeWidgetItem *, bool)));

			item->setFlags(item->flags() | Qt::ItemIsEditable);

			typeBox->setCurrentIndex(typeBox->findText(pr.type.c_str()));

			behaviorBox->setCurrentIndex(behaviorBox->findText(pr.behavior.c_str()));

			ui.portTreeWidget->setItemWidget(item, 1, typeBox);
			ui.portTreeWidget->setItemWidget(item, 2, behaviorBox);
			ui.portTreeWidget->setCurrentItem(item);
			pair<string, bool> portPair;
			portPair.first = pr.portName;
			portPair.second = false;

			bool conflict = false;
			for(uint i = 0; i < p.ports.size(); i++)
			{
				port temp = m_honeydConfig->m_ports[p.ports[i].first];
				if(!pr.portNum.compare(temp.portNum) && !pr.type.compare(temp.type))
				{
					conflict = true;
				}
			}
			if(!conflict)
			{
				p.ports.insert(p.ports.begin(),portPair);
				m_honeydConfig->m_ports[pr.portName] = pr;
				m_honeydConfig->m_profiles[p.name] = p;

				portPair.second = true;
				vector<profile> profList;
				for(ProfileTable::iterator it = m_honeydConfig->m_profiles.begin(); it != m_honeydConfig->m_profiles.end(); it++)
				{
					profile ptemp = it->second;
					while(ptemp.parentProfile.compare("") && ptemp.parentProfile.compare(p.name))
					{
						ptemp = m_honeydConfig->m_profiles[ptemp.parentProfile];
					}
					if(!ptemp.parentProfile.compare(p.name))
					{
						ptemp = it->second;
						conflict = false;
						for(uint i = 0; i < ptemp.ports.size(); i++)
						{
							port temp = m_honeydConfig->m_ports[ptemp.ports[i].first];
							if(!pr.portNum.compare(temp.portNum) && !pr.type.compare(temp.type))
							{
								conflict = true;
							}
						}
						if(!conflict)
						{
							ptemp.ports.insert(ptemp.ports.begin(),portPair);
						}
					}
					m_honeydConfig->m_profiles[ptemp.name] = ptemp;
				}
			}
			LoadProfileSettings();
			SaveProfileSettings();

			// TODO QT Port fix
			//ui.portTreeWidget->editItem(item, 0);
		}
		m_loading->unlock();
	}
}

void NovaConfig::on_actionDeletePort_triggered()
{
	if(!m_loading->tryLock())
	{
		return;
	}
	if(!ui.portTreeWidget->selectedItems().empty())
	{
		port *prt = NULL;
		for(PortTable::iterator it = m_honeydConfig->m_ports.begin(); it != m_honeydConfig->m_ports.end(); it++)
		{
			if(IsPortTreeWidgetItem(it->second.portName, ui.portTreeWidget->currentItem()))
			{
				//iterators are copies not the actual items
				prt = &m_honeydConfig->m_ports[it->second.portName];
				break;
			}
		}
		profile *p = &m_honeydConfig->m_profiles[m_currentProfile];
		uint i;
		for(i = 0; i < p->ports.size(); i++)
		{
			if(!p->ports[i].first.compare(prt->portName))
			{
				//Check for inheritance on the deleted port.
				//If valid parent
				if(p->parentProfile.compare(""))
				{
					profile *parent = &m_honeydConfig->m_profiles[p->parentProfile];
					bool matched = false;
					//check for the inherited port
					for(uint j = 0; j < parent->ports.size(); j++)
					{
						if((!prt->type.compare(m_honeydConfig->m_ports[parent->ports[j].first].type))
								&& (!prt->portNum.compare(m_honeydConfig->m_ports[parent->ports[j].first].portNum)))
						{
							p->ports[i].second = true;
							p->ports[i].first = parent->ports[j].first;
							matched = true;
						}
					}
					if(!matched)
					{
						p->ports.erase(p->ports.begin()+i);
					}
				}
				//If no parent.
				else
				{
					p->ports.erase(p->ports.begin()+i);
				}

				//Check for children with inherited port.
				for(ProfileTable::iterator it = m_honeydConfig->m_profiles.begin(); it != m_honeydConfig->m_profiles.end(); it++)
				{
					if(!it->second.parentProfile.compare(p->name))
					{
						for(uint j = 0; j < it->second.ports.size(); j++)
						{
							if(!it->second.ports[j].first.compare(prt->portName) && it->second.ports[j].second)
							{
								it->second.ports.erase(it->second.ports.begin()+j);
								break;
							}
						}
					}
				}
				break;
			}
			if(i == p->ports.size())
			{
				LOG(ERROR, "Port "+prt->portName+" could not be found in profile " + p->name+".", "");
				m_mainwindow->m_prompter->DisplayPrompt(m_mainwindow->CANNOT_DELETE_PORT, "Port " + prt->portName
					+ " cannot be found.");
			}
		}
		LoadProfileSettings();
		SaveProfileSettings();
	}
	m_loading->unlock();
}

void NovaConfig::on_msgTypeListWidget_currentRowChanged()
{
	m_loadingDefaultActions = true;
	int item = ui.msgTypeListWidget->currentRow();

	ui.defaultActionListWidget->clear();
	ui.defaultActionListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	QListWidgetItem *listItem;

	switch (m_mainwindow->m_prompter->m_registeredMessageTypes[item].type)
	{
		case notifyPrompt:
		case warningPrompt:
		case errorPrompt:
		{
			listItem = new QListWidgetItem("Always Show");
			ui.defaultActionListWidget->insertItem(0, listItem);

			if(m_mainwindow->m_prompter->m_registeredMessageTypes[item].action == CHOICE_SHOW)
			{
				listItem->setSelected(true);
			}

			listItem = new QListWidgetItem("Always Hide");
			ui.defaultActionListWidget->insertItem(1, listItem);

			if(m_mainwindow->m_prompter->m_registeredMessageTypes[item].action == CHOICE_HIDE)
			{
				listItem->setSelected(true);
			}
			break;
		}
		case warningPreventablePrompt:
		case notifyActionPrompt:
		case warningActionPrompt:
		{
			listItem = new QListWidgetItem("Always Show");
			ui.defaultActionListWidget->insertItem(0, listItem);

			if(m_mainwindow->m_prompter->m_registeredMessageTypes[item].action == CHOICE_SHOW)
			{
				listItem->setSelected(true);
			}

			listItem = new QListWidgetItem("Always Yes");
			ui.defaultActionListWidget->insertItem(1, listItem);

			if(m_mainwindow->m_prompter->m_registeredMessageTypes[item].action == CHOICE_DEFAULT)
			{
				listItem->setSelected(true);
			}

			listItem = new QListWidgetItem("Always No");
			ui.defaultActionListWidget->insertItem(2, listItem);

			if(m_mainwindow->m_prompter->m_registeredMessageTypes[item].action == CHOICE_ALT)
			{
				listItem->setSelected(true);
			}
			break;
		}
		case notSet:
		{
			break;
		}
		default:
		{
			break;
		}
	}

	m_loadingDefaultActions = false;
}

void NovaConfig::on_defaultActionListWidget_currentRowChanged()
{
	// If we're still populating the list
	if(m_loadingDefaultActions)
	{
		return;
	}

	QString selected = 	ui.defaultActionListWidget->currentItem()->text();
	messageHandle msgType = (messageHandle)ui.msgTypeListWidget->currentRow();

	if(!selected.compare("Always Show"))
	{
		m_mainwindow->m_prompter->SetDefaultAction(msgType, CHOICE_SHOW);
	}
	else if(!selected.compare("Always Hide"))
	{
		m_mainwindow->m_prompter->SetDefaultAction(msgType, CHOICE_HIDE);
	}
	else if(!selected.compare("Always Yes"))
	{
		m_mainwindow->m_prompter->SetDefaultAction(msgType, CHOICE_DEFAULT);
	}
	else if(!selected.compare("Always No"))
	{
		m_mainwindow->m_prompter->SetDefaultAction(msgType, CHOICE_ALT);
	}
	else
	{
		LOG(ERROR, "Invalid user dialog default action selected, shouldn't get here.", "");
	}
}

// Feature enable/disable stuff
void NovaConfig::AdvanceFeatureSelection()
{
	int nextRow = ui.featureList->currentRow() + 1;
	if(nextRow >= ui.featureList->count())
	{
		nextRow = 0;
	}
	ui.featureList->setCurrentRow(nextRow);
}

void NovaConfig::on_featureEnableButton_clicked()
{
	UpdateFeatureListItem(ui.featureList->currentItem(), '1');
	AdvanceFeatureSelection();
}

void NovaConfig::on_featureDisableButton_clicked()
{
	UpdateFeatureListItem(ui.featureList->currentItem(), '0');
	AdvanceFeatureSelection();
}

void NovaConfig::on_ethernetCheckBox_stateChanged()
{
	if(m_loading->tryLock())
	{
		SaveProfileSettings();
		LoadProfileSettings();
		m_loading->unlock();
	}
}

void NovaConfig::on_uptimeCheckBox_stateChanged()
{
	if(m_loading->tryLock())
	{
		SaveProfileSettings();
		LoadProfileSettings();
		m_loading->unlock();
	}
}

void NovaConfig::on_personalityCheckBox_stateChanged()
{
	if(m_loading->tryLock())
	{
		SaveProfileSettings();
		LoadProfileSettings();
		m_loading->unlock();
	}
}

void NovaConfig::on_dropRateCheckBox_stateChanged()
{
	if(m_loading->tryLock())
	{
		SaveProfileSettings();
		LoadProfileSettings();
		m_loading->unlock();
	}
}

void NovaConfig::on_tcpCheckBox_stateChanged()
{
	if(m_loading->tryLock())
	{
		SaveProfileSettings();
		LoadProfileSettings();
		m_loading->unlock();
	}
}

void NovaConfig::on_udpCheckBox_stateChanged()
{
	if(m_loading->tryLock())
	{
		SaveProfileSettings();
		LoadProfileSettings();
		m_loading->unlock();
	}
}

void NovaConfig::on_icmpCheckBox_stateChanged()
{
	if(m_loading->tryLock())
	{
		SaveProfileSettings();
		LoadProfileSettings();
		m_loading->unlock();
	}
}

void NovaConfig::on_portTreeWidget_itemChanged(QTreeWidgetItem *item)
{
	portTreeWidget_comboBoxChanged(item, true);
}

void NovaConfig::portTreeWidget_comboBoxChanged(QTreeWidgetItem *item,  bool edited)
{
	if(!m_loading->tryLock())
	{
		return;
	}
	//On user action with a valid port, and the user actually changed something
	if((item != NULL) && edited)
	{
		//Ensure the signaling item is selected
		ui.portTreeWidget->setCurrentItem(item);
		profile p = m_honeydConfig->m_profiles[m_currentProfile];
		string oldPort;
		port oldPrt;

		//Find the port before the changes
		//for(PortTable::iterator it = m_honeydConfig->m_ports.begin(); it != m_honeydConfig->m_ports.end(); it++)
		//{
		//*****************************************************
		// Can'to do this anymore.
		//	if(it->second.item == item)
		//*****************************************************
		//	{
		//		oldPort = it->second.portName;
		//	}
		//}
		//oldPort = item->text(0).toStdString() + "_" + item->text(1).toStdString() + "_" + item->text(2).toStdString();

		oldPort = item->text(3).toStdString();
		oldPrt = m_honeydConfig->m_ports[oldPort];


		//Use the combo boxes to update the hidden text underneath them.
		TreeItemComboBox *qTypeBox = (TreeItemComboBox*)ui.portTreeWidget->itemWidget(item, 1);
		item->setText(1, qTypeBox->currentText());

		TreeItemComboBox *qBehavBox = (TreeItemComboBox*)ui.portTreeWidget->itemWidget(item, 2);
		item->setText(2, qBehavBox->currentText());

		//Generate a unique identifier for a particular protocol, number and behavior combination
		// this is pulled from the recently updated hidden text and reflects any changes
		string portName = item->text(0).toStdString() + "_" + item->text(1).toStdString()
				+ "_" + item->text(2).toStdString();
		item->setText(3, QString::fromStdString(portName));


		port prt;
		//Locate the port in the table or create the port if it doesn't exist
		if(!m_honeydConfig->m_ports.keyExists(portName))
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
			m_honeydConfig->m_ports[portName] = prt;
		}
		else
		{
			prt = m_honeydConfig->m_ports[portName];
		}

		//Check for port conflicts
		for(uint i = 0; i < p.ports.size(); i++)
		{
			port temp = m_honeydConfig->m_ports[p.ports[i].first];
			//If theres a conflict other than with the old port
			if((!(temp.portNum.compare(prt.portNum))) && (!(temp.type.compare(prt.type)))
					&& temp.portName.compare(oldPort))
			{
				LOG(ERROR, "WARNING: Port number and protocol already used.", "");

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
			{
				if(!p.ports[index].first.compare(oldPort))
				{
					p.ports.erase(p.ports.begin()+index);
				}
			}

			//If the port number or protocol is different, check for inherited ports
			if((prt.portNum.compare(oldPrt.portNum) || prt.type.compare(oldPrt.type))
				&& (m_honeydConfig->m_profiles.keyExists(p.parentProfile)))
			{
				profile parent = m_honeydConfig->m_profiles[p.parentProfile];
				for(uint i = 0; i < parent.ports.size(); i++)
				{
					port temp = m_honeydConfig->m_ports[parent.ports[i].first];
					//If a parent's port matches the number and protocol of the old port being removed
					if((!(temp.portNum.compare(oldPrt.portNum))) && (!(temp.type.compare(oldPrt.type))))
					{
						portPair.first = temp.portName;
						portPair.second = true;
						if(index < p.ports.size())
						{
							p.ports.insert(p.ports.begin() + index, portPair);
						}
						else
						{
							p.ports.push_back(portPair);
						}
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
				port temp = m_honeydConfig->m_ports[p.ports[i].first];
				if((atoi(temp.portNum.c_str())) < (atoi(prt.portNum.c_str())))
				{
					continue;
				}
				break;
			}
			if(i < p.ports.size())
			{
				p.ports.insert(p.ports.begin()+i, portPair);
			}
			else
			{
				p.ports.push_back(portPair);
			}

			m_honeydConfig->m_profiles[p.name] = p;
			//Check for children who inherit the port
			vector<string> updateList;
			updateList.push_back(m_currentProfile);
			bool changed = true;
			bool valid;

			//While any changes have been made
			while(changed)
			{
				//In this while, no changes have been found
				changed = false;
				//Check profile table for children of currentProfile at it's children and so on
				for(ProfileTable::iterator it = m_honeydConfig->m_profiles.begin(); it != m_honeydConfig->m_profiles.end(); it++)
				{
					//Profile invalid to add to start
					valid = false;
					for(uint i = 0; i < updateList.size(); i++)
					{
						//If the parent is being updated this profile is tenatively valid
						if(!it->second.parentProfile.compare(updateList[i]))
						{
							valid = true;
						}
						//If this profile is already in the list, this profile shouldn't be included
						if(!it->second.name.compare(updateList[i]))
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
						{
							if(!ptemp.ports[i].first.compare(oldPort) && ptemp.ports[i].second)
							{
								ptemp.ports.erase(ptemp.ports.begin()+i);
							}
						}

						profile parentTemp = m_honeydConfig->m_profiles[ptemp.parentProfile];

						//insert any ports the parent has that doesn't conflict
						for(uint i = 0; i < parentTemp.ports.size(); i++)
						{
							bool conflict = false;
							port pr = m_honeydConfig->m_ports[parentTemp.ports[i].first];
							//Check the child for conflicts
							for(uint j = 0; j < ptemp.ports.size(); j++)
							{
								port temp = m_honeydConfig->m_ports[ptemp.ports[j].first];
								if(!temp.portNum.compare(pr.portNum) && !temp.type.compare(pr.type))
								{
									conflict = true;
								}
							}
							if(!conflict)
							{
								portPair.first = pr.portName;
								portPair.second = true;
								//Insert it at the numerically sorted position.
								uint j = 0;
								for(j = 0; j < ptemp.ports.size(); j++)
								{
									port temp = m_honeydConfig->m_ports[ptemp.ports[j].first];
									if((atoi(temp.portNum.c_str())) < (atoi(pr.portNum.c_str())))
										continue;

									break;
								}
								if(j < ptemp.ports.size())
								{
									ptemp.ports.insert(ptemp.ports.begin()+j, portPair);
								}
								else
								{
									ptemp.ports.push_back(portPair);
								}
							}
						}
						updateList.push_back(ptemp.name);
						m_honeydConfig->m_profiles[ptemp.name] = ptemp;
						//Since we found at least one profile this iteration flag as changed
						// so we can check for it's children
						changed = true;
						break;
					}
				}
			}
		}
		LoadProfileSettings();
		SaveProfileSettings();
		ui.portTreeWidget->setFocus(Qt::OtherFocusReason);

		// TODO
		//ui.portTreeWidget->setCurrentItem(m_honeydConfig->m_ports[prt.portName].item);
		m_loading->unlock();
		LoadAllProfiles();
	}
	//On user interaction with a valid port item without any changes
	else if(!edited && (item != NULL))
	{
		//Select the valid port item under the associated combo box
		ui.portTreeWidget->setFocus(Qt::OtherFocusReason);
		ui.portTreeWidget->setCurrentItem(item);
		m_loading->unlock();
	}
	else
	{
		m_loading->unlock();
	}
}

QListWidgetItem* NovaConfig::GetFeatureListItem(QString name, char enabled)
{
	QListWidgetItem *newFeatureEntry = new QListWidgetItem();
	name.prepend("+  ");
	newFeatureEntry->setText(name);
	UpdateFeatureListItem(newFeatureEntry, enabled);

	return newFeatureEntry;
}

void NovaConfig::onFeatureClick(const QPoint & pos)
{
	QPoint globalPos = ui.featureList->mapToGlobal(pos);
	QModelIndex t = ui.featureList->indexAt(pos);

	// Handle clicking on a row that isn't populated
	if(t.row() < 0 || t.row() >= DIM)
	{
		return;
	}

	// Make the menu
    QMenu myMenu(this);
    myMenu.addAction(new QAction("Enable", this));
    myMenu.addAction(new QAction("Disable", this));

    // Figure out what the user selected
    QAction* selectedItem = myMenu.exec(globalPos);
    if(selectedItem)
    {
		if(selectedItem->text() == "Enable")
		{
			UpdateFeatureListItem(ui.featureList->item(t.row()), '1');
		}
		else if(selectedItem->text() == "Disable")
		{
			UpdateFeatureListItem(ui.featureList->item(t.row()), '0');
		}
    }
}

void NovaConfig::UpdateFeatureListItem(QListWidgetItem* newFeatureEntry, char enabled)
{
	QBrush *enabledColor = new QBrush(QColor("black"));
	QBrush *disabledColor = new QBrush(QColor("grey"));
	QString name = newFeatureEntry->text().remove(0, 2);

	if(enabled == '1')
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


//Load Personality choices from nmap fingerprints file
void NovaConfig::DisplayNmapPersonalityWindow()
{
	m_retVal = "";
	NovaComplexDialog *NmapPersonalityWindow = new NovaComplexDialog(PersonalityDialog, &m_retVal, this);
	NmapPersonalityWindow->exec();
	if(m_retVal.compare(""))
	{
		ui.personalityEdit->setText((QString)m_retVal.c_str());
	}
}


//Load MAC vendor prefix choices from nmap mac prefix file
bool NovaConfig::DisplayMACPrefixWindow()
{
	m_retVal = "";
	NovaComplexDialog *MACPrefixWindow = new NovaComplexDialog(
			MACDialog, &m_retVal, this, ui.ethernetEdit->text().toStdString());
	MACPrefixWindow->exec();
	if(m_retVal.compare(""))
	{
		ui.ethernetEdit->setText((QString)m_retVal.c_str());

		//If there is no change in vendor, nothing left to be done.
		if(m_honeydConfig->m_profiles[m_currentProfile].ethernet.compare(m_retVal))
		{
			return true;
		}

		m_honeydConfig->GenerateMACAddresses(m_currentProfile);
	}
	return false;
}

void NovaConfig::LoadNovadPreferences()
{
	struct ifaddrs * devices = NULL;
	struct ifaddrs *curIf = NULL;
	stringstream ss;

	//Get a list of interfaces
	if(getifaddrs(&devices))
	{
		LOG(ERROR, string("Ethernet Interface Auto-Detection failed: ") + string(strerror(errno)), "");
	}

	QList<QAbstractButton *> radioButtons = m_radioButtons->buttons();
	while(!radioButtons.isEmpty())
	{
		delete radioButtons.takeFirst();
	}
	QList<QAbstractButton *> checkBoxes = m_interfaceCheckBoxes->buttons();
	while(!checkBoxes.isEmpty())
	{
		delete checkBoxes.takeFirst();
	}
	delete m_radioButtons;
	delete m_interfaceCheckBoxes;
	m_radioButtons = new QButtonGroup(ui.loopbackGroupBox);
	m_radioButtons->setExclusive(true);
	m_interfaceCheckBoxes = new QButtonGroup(ui.interfaceGroupBox);

	for(curIf = devices; curIf != NULL; curIf = curIf->ifa_next)
	{
		if((int)curIf->ifa_addr->sa_family == AF_INET)
		{
			//Create radio button for each loop back
			if(curIf->ifa_flags & IFF_LOOPBACK)
			{
				QRadioButton * radioButton = new QRadioButton(QString(curIf->ifa_name), ui.loopbackGroupBox);
				radioButton->setObjectName(QString(curIf->ifa_name));
				m_radioButtons->addButton(radioButton);
				ui.loopbackGroupBoxVLayout->addWidget(radioButton);
				radioButtons.push_back(radioButton);
			}
			//Create check box for each interface
			else
			{
				QCheckBox * checkBox = new QCheckBox(QString(curIf->ifa_name), ui.interfaceGroupBox);
				checkBox->setObjectName(QString(curIf->ifa_name));
				m_interfaceCheckBoxes->addButton(checkBox);
				ui.interfaceGroupBoxVLayout->addWidget(checkBox);
				checkBoxes.push_back(checkBox);
			}
		}
	}
	freeifaddrs(devices);
	if(checkBoxes.size() >= 1)
	{
		m_interfaceCheckBoxes->setExclusive(false);
	}
	else
	{
		m_interfaceCheckBoxes->setExclusive(true);
	}

	//Select the loopback
	{
		QString doppIf = QString::fromStdString(Config::Inst()->GetDoppelInterface());
		QRadioButton * checkObj = ui.loopbackGroupBox->findChild<QRadioButton *>(doppIf);
		if(checkObj != NULL)
		{
			checkObj->setChecked(true);
		}
	}
	vector<string> ifList = Config::Inst()->GetInterfaces();
	while(!ifList.empty())
	{
		QCheckBox* checkObj = ui.interfaceGroupBox->findChild<QCheckBox *>(QString::fromStdString(ifList.back()));
		if(checkObj != NULL)
		{
			checkObj->setChecked(true);
		}
		ifList.pop_back();
	}

	ui.useAllIfCheckBox->setChecked(Config::Inst()->GetUseAllInterfaces());
	ui.useAnyLoopbackCheckBox->setChecked(Config::Inst()->GetUseAnyLoopback());
	ui.dataEdit->setText(QString::fromStdString(Config::Inst()->GetPathTrainingFile()));
	ui.saAttemptsMaxEdit->setText(QString::number(Config::Inst()->GetSaMaxAttempts()));
	ui.saAttemptsTimeEdit->setText(QString::number(Config::Inst()->GetSaSleepDuration()));
	ui.saPortEdit->setText(QString::number(Config::Inst()->GetSaPort()));
	ui.ceIntensityEdit->setText(QString::number(Config::Inst()->GetK()));
	ui.ceErrorEdit->setText(QString::number(Config::Inst()->GetEps()));
	ui.ceFrequencyEdit->setText(QString::number(Config::Inst()->GetClassificationTimeout()));
	ui.ceThresholdEdit->setText(QString::number(Config::Inst()->GetClassificationThreshold()));
	ui.tcpTimeoutEdit->setText(QString::number(Config::Inst()->GetTcpTimout()));
	ui.tcpFrequencyEdit->setText(QString::number(Config::Inst()->GetTcpCheckFreq()));

	ui.trainingCheckBox->setChecked(Config::Inst()->GetIsTraining());
	switch(Config::Inst()->GetHaystackStorage())
	{
		case 'M':
		{
			ui.hsSaveTypeComboBox->setCurrentIndex(1);
			ui.hsSummaryGroupBox->setEnabled(false);
			ui.nodesGroupBox->setEnabled(false);
			ui.profileGroupBox->setEnabled(false);
			ui.hsConfigEdit->setText((QString)Config::Inst()->GetPathConfigHoneydUser().c_str());
			break;
		}
		/*case 'E': TODO Implement once we have multiple configurations
		{
			ui.hsSaveTypeComboBox->setCurrentIndex(1);
			ui.hsConfigEdit->setText((QString)Config::Inst()->GetPathConfigHoneydUser().c_str());
			break;
		}*/
		default:
		{
			ui.hsSaveTypeComboBox->setCurrentIndex(0);
			ui.hsConfigEdit->setText((QString)Config::Inst()->GetPathConfigHoneydHS().c_str());
			break;
		}
	}

	//Set first ip byte
	string ip = Config::Inst()->GetDoppelIp();
	int index = ip.find_first_of('.');
	ui.dmIPSpinBox_0->setValue(atoi(ip.substr(0,ip.find_first_of('.')).c_str()));

	//Set second ip byte
	ip = ip.substr(index+1, ip.size());
	index = ip.find_first_of('.');
	ui.dmIPSpinBox_1->setValue(atoi(ip.substr(0,ip.find_first_of('.')).c_str()));

	//Set third ip byte
	ip = ip.substr(index+1, ip.size());
	index = ip.find_first_of('.');
	ui.dmIPSpinBox_2->setValue(atoi(ip.substr(0,ip.find_first_of('.')).c_str()));

	//Set fourth ip byte
	ip = ip.substr(index+1, ip.size());
	index = ip.find_first_of('.');
	ui.dmIPSpinBox_3->setValue(atoi(ip.c_str()));

	ui.pcapCheckBox->setChecked(Config::Inst()->GetReadPcap());
	ui.pcapGroupBox->setEnabled(ui.pcapCheckBox->isChecked());
	ui.pcapEdit->setText((QString)Config::Inst()->GetPathPcapFile().c_str());
	ui.liveCapCheckBox->setChecked(Config::Inst()->GetGotoLive());
	{
		string featuresEnabled = Config::Inst()->GetEnabledFeatures();
		ui.featureList->clear();
		// Populate the list, row order is based on dimension macros
		ui.featureList->insertItem(IP_TRAFFIC_DISTRIBUTION,
			GetFeatureListItem(QString("IP Traffic Distribution"),featuresEnabled.at(IP_TRAFFIC_DISTRIBUTION)));

		ui.featureList->insertItem(PORT_TRAFFIC_DISTRIBUTION,
			GetFeatureListItem(QString("Port Traffic Distribution"),featuresEnabled.at(PORT_TRAFFIC_DISTRIBUTION)));

		ui.featureList->insertItem(HAYSTACK_EVENT_FREQUENCY,
			GetFeatureListItem(QString("Haystack Event Frequency"),featuresEnabled.at(HAYSTACK_EVENT_FREQUENCY)));

		ui.featureList->insertItem(PACKET_SIZE_MEAN,
			GetFeatureListItem(QString("Packet Size Mean"),featuresEnabled.at(PACKET_SIZE_MEAN)));

		ui.featureList->insertItem(PACKET_SIZE_DEVIATION,
			GetFeatureListItem(QString("Packet Size Deviation"),featuresEnabled.at(PACKET_SIZE_DEVIATION)));

		ui.featureList->insertItem(DISTINCT_IPS,
			GetFeatureListItem(QString("IPs Contacted"),featuresEnabled.at(DISTINCT_IPS)));

		ui.featureList->insertItem(DISTINCT_PORTS,
			GetFeatureListItem(QString("Ports Contacted"),featuresEnabled.at(DISTINCT_PORTS)));

		ui.featureList->insertItem(PACKET_INTERVAL_MEAN,
			GetFeatureListItem(QString("Packet Interval Mean"),featuresEnabled.at(PACKET_INTERVAL_MEAN)));

		ui.featureList->insertItem(PACKET_INTERVAL_DEVIATION,
			GetFeatureListItem(QString("Packet Interval Deviation"),featuresEnabled.at(PACKET_INTERVAL_DEVIATION)));
	}
}

//Draws the current honeyd configuration
void NovaConfig::LoadHaystackConfiguration()
{
	//Draws all profile heriarchy
	m_loading->unlock();
	LoadAllProfiles();
	//Draws all node heirarchy
	LoadAllNodes();
	ui.nodeTreeWidget->expandAll();
	ui.hsNodeTreeWidget->expandAll();
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
	QString fileName = QFileDialog::getSaveFileName(this, tr("Choose a File Location"),
			path.path(), tr("Text Files (*.config)"));
	QFile file(fileName);
	//Gets the relative path using the absolute path in fileName and the current path
	if(!file.exists())
	{
		file.open(file.ReadWrite);
		file.close();
	}
	fileName = file.fileName();
	fileName = path.relativeFilePath(fileName);
	ui.hsConfigEdit->setText(fileName);
	//LoadHaystackConfiguration();
}

/************************************************
 * General Preferences GUI Signals
 ************************************************/

//Stores all changes and closes the window
void NovaConfig::on_okButton_clicked()
{
	m_loading->lock();
	SaveProfileSettings();
	if(!SaveConfigurationToFile())
	{
		LOG(ERROR, "Error writing to Nova config file.", "");
		m_mainwindow->m_prompter->DisplayPrompt(m_mainwindow->CONFIG_WRITE_FAIL,
			"Error: Unable to write to NOVA configuration file");
		this->close();
	}
	LoadNovadPreferences();

	//Clean up unused ports
	m_honeydConfig->CleanPorts();
	//Saves the current configuration to XML files
	m_honeydConfig->SaveAllTemplates();
	m_honeydConfig->WriteHoneydConfiguration(Config::Inst()->GetUserPath());

	LoadHaystackConfiguration();
	m_mainwindow->InitConfiguration();

	m_loading->unlock();
	this->close();
}

//Stores all changes the repopulates the window
void NovaConfig::on_applyButton_clicked()
{
	m_loading->lock();
	SaveProfileSettings();
	if(!SaveConfigurationToFile())
	{
		LOG(ERROR, "Error writing to Nova config file.", "");

		m_mainwindow->m_prompter->DisplayPrompt(m_mainwindow->CONFIG_WRITE_FAIL,
			"Error: Unable to write to NOVA configuration file ");
		this->close();
	}
	//Reloads NOVAConfig preferences to assert concurrency
	LoadNovadPreferences();

	//Clean up unused ports
	m_honeydConfig->CleanPorts();
	//Saves the current configuration to XML files
	m_honeydConfig->SaveAllTemplates();
	m_honeydConfig->WriteHoneydConfiguration(Config::Inst()->GetUserPath());

	//Reloads honeyd configuration to assert concurrency
	LoadHaystackConfiguration();
	m_mainwindow->InitConfiguration();
	m_loading->unlock();
}

void NovaConfig::SetSelectedNode(string node)
{
	m_currentNode = node;
}

bool NovaConfig::SaveConfigurationToFile()
{
	string line, prefix;
	stringstream ss;
	for (uint i = 0; i < DIM; i++)
	{
		char state = ui.featureList->item(i)->text().at(0).toAscii();
		if(state == '+')
		{
			ss << 1;
		}
		else
		{
			ss << 0;
		}
	}
	Config::Inst()->SetIsDmEnabled(ui.dmCheckBox->isChecked());
	Config::Inst()->SetIsTraining(ui.trainingCheckBox->isChecked());
	Config::Inst()->SetPathTrainingFile(this->ui.dataEdit->displayText().toStdString());
	Config::Inst()->SetSaSleepDuration(this->ui.saAttemptsTimeEdit->displayText().toDouble());
	Config::Inst()->SetSaMaxAttempts(this->ui.saAttemptsMaxEdit->displayText().toInt());
	Config::Inst()->SetSaPort(this->ui.saPortEdit->displayText().toInt());
	Config::Inst()->SetK(this->ui.ceIntensityEdit->displayText().toInt());
	Config::Inst()->SetEps(this->ui.ceErrorEdit->displayText().toDouble());
	Config::Inst()->SetClassificationTimeout(this->ui.ceFrequencyEdit->displayText().toInt());
	Config::Inst()->SetClassificationThreshold(this->ui.ceThresholdEdit->displayText().toDouble());
	Config::Inst()->SetEnabledFeatures(ss.str());

	QList<QAbstractButton *> qButtonList = m_interfaceCheckBoxes->buttons();
	Config::Inst()->ClearInterfaces();
	Config::Inst()->SetUseAllInterfaces(ui.useAllIfCheckBox->isChecked());

	for(int i = 0; i < qButtonList.size(); i++)
	{
		QCheckBox * checkBoxPtr = (QCheckBox *)qButtonList.at(i);
		if(checkBoxPtr->isChecked())
		{
			Config::Inst()->AddInterface(checkBoxPtr->text().toStdString());
		}
	}

	qButtonList = m_radioButtons->buttons();

	Config::Inst()->SetUseAnyLoopback(ui.useAnyLoopbackCheckBox->isChecked());
	for(int i = 0; i < qButtonList.size(); i++)
	{
		//XXX configuration for selection 'any' interface aka 'default'
		QRadioButton * radioBtnPtr = (QRadioButton *)qButtonList.at(i);
		if(radioBtnPtr->isChecked())
		{
			Config::Inst()->SetDoppelInterface(radioBtnPtr->text().toStdString());
		}
	}

	ss.str("");
	ss << ui.dmIPSpinBox_0->value() << "." << ui.dmIPSpinBox_1->value() << "." << ui.dmIPSpinBox_2->value() << "."
		<< ui.dmIPSpinBox_3->value();
	Config::Inst()->SetDoppelIp(ss.str());

	switch(ui.hsSaveTypeComboBox->currentIndex())
	{
		case 1:
		{
			Config::Inst()->SetHaystackStorage('M');
			Config::Inst()->SetPathConfigHoneydUser(this->ui.hsConfigEdit->displayText().toStdString());
			break;
		}
		case 0:
		default:
		{
			Config::Inst()->SetHaystackStorage('I');
			Config::Inst()->SetPathConfigHoneydHs(this->ui.hsConfigEdit->displayText().toStdString());
			break;
		}
	}

	Config::Inst()->SetTcpTimout(this->ui.tcpTimeoutEdit->displayText().toInt());
	Config::Inst()->SetTcpCheckFreq(this->ui.tcpFrequencyEdit->displayText().toInt());
	Config::Inst()->SetPathPcapFile(ui.pcapEdit->displayText().toStdString());
	Config::Inst()->SetReadPcap(ui.pcapCheckBox->isChecked());
	Config::Inst()->SetGotoLive(ui.liveCapCheckBox->isChecked());

	return Config::Inst()->SaveConfig();
}

//Exit the window and ignore any changes since opening or apply was pressed
void NovaConfig::on_cancelButton_clicked()
{
	//Reloads from NOVAConfig
	LoadNovadPreferences();
	this->close();
}

void NovaConfig::on_defaultsButton_clicked() //TODO
{
	//Currently just restores to last save changes
	//We should really identify default values and write those while maintaining
	//Critical values that might cause nova to break if changed.

	//Reloads from NOVAConfig
	LoadNovadPreferences();
	//Has NovaGUI reload honeyd configuration from XML files
	m_honeydConfig->LoadAllTemplates();
	m_loading->lock();
	//Populates honeyd configuration pulled
	LoadHaystackConfiguration();
	m_loading->unlock();
}

void NovaConfig::on_menuTreeWidget_itemSelectionChanged()
{
	QTreeWidgetItem *item = ui.menuTreeWidget->selectedItems().first();

	//If last window was the profile window, save any changes
	if(m_editingItems && m_honeydConfig->m_profiles.size())
	{
		SaveProfileSettings();
	}
	m_editingItems = false;

	//If it's a top level item the page corresponds to their index in the tree
	//Any new top level item should be inserted at the corresponding index and the defines
	// for the lower level items will need to be adjusted appropriately.
	int i = ui.menuTreeWidget->indexOfTopLevelItem(item);

	if(i != -1)
	{
		ui.stackedWidget->setCurrentIndex(i);
	}
	//If the item is a child of a top level item, find out what type of item it is
	else
	{
		//Find the parent and keep getting parents until we have a top level item
		QTreeWidgetItem *parent = item->parent();
		while(ui.menuTreeWidget->indexOfTopLevelItem(parent) == -1)
		{
			parent = parent->parent();
		}
		if(ui.menuTreeWidget->indexOfTopLevelItem(parent) == HAYSTACK_MENU_INDEX)
		{
			//If the 'Nodes' Item
			if(parent->child(NODE_INDEX) == item)
			{
				ui.stackedWidget->setCurrentIndex(ui.menuTreeWidget->topLevelItemCount());
			}
			//If the 'Profiles' item
			else if(parent->child(PROFILE_INDEX) == item)
			{
				m_editingItems = true;
				ui.stackedWidget->setCurrentIndex(ui.menuTreeWidget->topLevelItemCount()+1);
			}
		}
		else
		{
			LOG(ERROR, "Unable to set stackedWidget page index from menuTreeWidgetItem", "");
		}
	}
}

/************************************************
 * Preference Menu Specific GUI Signals
 ************************************************/

//Enable DM checkbox, action syncs Node displayed in haystack as disabled/enabled
void NovaConfig::on_dmCheckBox_stateChanged(int state)
{
	if(!m_loading->tryLock())
	{
		return;
	}

	if (state)
	{
		m_honeydConfig->EnableNode("Doppelganger");
	}
	else
	{
		m_honeydConfig->DisableNode("Doppelganger");
	}

	m_loading->unlock();
	LoadAllNodes();
}
/*************************
 * Pcap Menu GUI Signals */

/* Enables or disables options specific for reading from pcap file */
void NovaConfig::on_pcapCheckBox_stateChanged(int state)
{
	ui.pcapGroupBox->setEnabled(state);
}

/******************************************
 * Profile Menu GUI Functions *************/

//Combo box signal for changing the uptime behavior
void NovaConfig::on_uptimeBehaviorComboBox_currentIndexChanged(int index)
{
	//If uptime behavior = random in range
	if(index)
	{
		ui.uptimeRangeEdit->setText(ui.uptimeEdit->displayText());
	}

	ui.uptimeRangeLabel->setVisible(index);
	ui.uptimeRangeEdit->setVisible(index);
}

void NovaConfig::SaveProfileSettings()
{
	QTreeWidgetItem *item = NULL;
	struct port pr;

	//Saves any modifications to the last selected profile object.
	if(m_honeydConfig->m_profiles.keyExists(m_currentProfile))
	{
		profile p = m_honeydConfig->m_profiles[m_currentProfile];
		//currentProfile->name is set in updateProfile
		p.ethernet = ui.ethernetEdit->displayText().toStdString();
		p.tcpAction = ui.tcpActionComboBox->currentText().toStdString();
		p.udpAction = ui.udpActionComboBox->currentText().toStdString();
		p.icmpAction = ui.icmpActionComboBox->currentText().toStdString();


		p.uptimeMin = ui.uptimeEdit->displayText().toStdString();
		//If random in range behavior
		if(ui.uptimeBehaviorComboBox->currentIndex())
		{
			p.uptimeMax = ui.uptimeRangeEdit->displayText().toStdString();
		}
		//If flat behavior
		else
		{
			p.uptimeMax = p.uptimeMin;
		}
		p.personality = ui.personalityEdit->displayText().toStdString();
		stringstream ss;
		ss << ui.dropRateSlider->value();
		p.dropRate = ss.str();

		//Save the port table
		for(int i = 0; i < ui.portTreeWidget->topLevelItemCount(); i++)
		{
			pr = m_honeydConfig->m_ports[p.ports[i].first];
			item = ui.portTreeWidget->topLevelItem(i);
			pr.portNum = item->text(0).toStdString();
			TreeItemComboBox *qTypeBox = (TreeItemComboBox*)ui.portTreeWidget->itemWidget(item, 1);
			TreeItemComboBox *qBehavBox = (TreeItemComboBox*)ui.portTreeWidget->itemWidget(item, 2);
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
			{
				pr.portName = pr.portNum + "_" +pr.type + "_" + pr.scriptName;
			}
			else
			{
				pr.portName = pr.portNum + "_" +pr.type + "_" + pr.behavior;
			}

			p.ports[i].first = pr.portName;
			m_honeydConfig->m_ports[p.ports[i].first] = pr;
			p.ports[i].second = item->font(0).italic();
		}
		m_honeydConfig->m_profiles[m_currentProfile] = p;
		SaveInheritedProfileSettings();
		m_honeydConfig->UpdateProfile(m_currentProfile);
	}
}

void NovaConfig::SaveInheritedProfileSettings()
{
	profile p = m_honeydConfig->m_profiles[m_currentProfile];

	p.inherited[TCP_ACTION] = ui.tcpCheckBox->isChecked();
	if(ui.tcpCheckBox->isChecked())
	{
		p.tcpAction = m_honeydConfig->m_profiles[p.parentProfile].tcpAction;
	}
	p.inherited[UDP_ACTION] = ui.udpCheckBox->isChecked();
	if(ui.udpCheckBox->isChecked())
	{
		p.udpAction = m_honeydConfig->m_profiles[p.parentProfile].udpAction;
	}
	p.inherited[ICMP_ACTION] = ui.icmpCheckBox->isChecked();
	if(ui.icmpCheckBox->isChecked())
	{
		p.icmpAction = m_honeydConfig->m_profiles[p.parentProfile].icmpAction;
	}
	p.inherited[ETHERNET] = ui.ethernetCheckBox->isChecked();
	if(ui.ethernetCheckBox->isChecked())
	{
		p.ethernet = m_honeydConfig->m_profiles[p.parentProfile].ethernet;
	}
	p.inherited[UPTIME] = ui.uptimeCheckBox->isChecked();
	if(ui.uptimeCheckBox->isChecked())
	{
		p.uptimeMin = m_honeydConfig->m_profiles[p.parentProfile].uptimeMin;
		p.uptimeMax = m_honeydConfig->m_profiles[p.parentProfile].uptimeMax;
		if(m_honeydConfig->m_profiles[p.parentProfile].uptimeMin == m_honeydConfig->m_profiles[p.parentProfile].uptimeMax)
		{
			ui.uptimeBehaviorComboBox->setCurrentIndex(1);
		}
		else
		{
			ui.uptimeBehaviorComboBox->setCurrentIndex(0);
		}
	}

	p.inherited[PERSONALITY] = ui.personalityCheckBox->isChecked();
	if(ui.personalityCheckBox->isChecked())
	{
		p.personality = m_honeydConfig->m_profiles[p.parentProfile].personality;
	}
	p.inherited[DROP_RATE] = ui.dropRateCheckBox->isChecked();
	if(ui.dropRateCheckBox->isChecked())
	{
		p.dropRate = m_honeydConfig->m_profiles[p.parentProfile].dropRate;
	}
	m_honeydConfig->m_profiles[m_currentProfile] = p;

}
//Removes a profile, all of it's children and any nodes that currently use it
void NovaConfig::DeleteProfile(string name)
{
	QTreeWidgetItem *item = NULL, *temp = NULL;
	//If there is at least one other profile after deleting all children
	if(m_honeydConfig->m_profiles.size() > 1)
	{
		//Get the current profile item
		item = GetProfileTreeWidgetItem(name);
		//Try to find another profile below it
		temp = ui.profileTreeWidget->itemBelow(item);

		//If no profile below, find a profile above
		if(temp == NULL)
		{
			item = ui.profileTreeWidget->itemAbove(item);
		}
		else
		{
			item = temp; //if profile below was found
		}
	}
	//Remove the current profiles tree widget items
	ui.profileTreeWidget->removeItemWidget(GetProfileTreeWidgetItem(name), 0);
	ui.hsProfileTreeWidget->removeItemWidget(GetProfileHsTreeWidgetItem(name), 0);
	//If an item was found for a new selection
	if(item != NULL)
	{	//Set the current selection
		m_currentProfile = item->text(0).toStdString();
	}
	//If no profiles remain
	else
	{	//No selection
		m_currentProfile = "";
	}
	m_honeydConfig->DeleteProfile(name);

	//Redraw honeyd configuration to reflect changes
	LoadHaystackConfiguration();
}

//Populates the window with the selected profile's options
void NovaConfig::LoadProfileSettings()
{
	port pr;
	QTreeWidgetItem *item = NULL;
	//If the selected profile can be found
	if(m_honeydConfig->m_profiles.keyExists(m_currentProfile))
	{
		LoadInheritedProfileSettings();
		//Clear the tree widget and load new selections
		QTreeWidgetItem *portCurrentItem;
		portCurrentItem = ui.portTreeWidget->currentItem();
		string portCurrentString = "";
		if(portCurrentItem != NULL)
		{
			portCurrentString = portCurrentItem->text(0).toStdString()+ "_"
				+ portCurrentItem->text(1).toStdString() + "_" + portCurrentItem->text(2).toStdString();
		}

		ui.portTreeWidget->clear();

		profile *p = &m_honeydConfig->m_profiles[m_currentProfile];
		//Set the variables of the profile
		ui.profileEdit->setText((QString)p->name.c_str());
		ui.profileEdit->setEnabled(true);
		ui.ethernetEdit->setText((QString)p->ethernet.c_str());
		ui.tcpActionComboBox->setCurrentIndex( ui.tcpActionComboBox->findText(p->tcpAction.c_str() ) );
		ui.udpActionComboBox->setCurrentIndex( ui.udpActionComboBox->findText(p->udpAction.c_str() ) );
		ui.icmpActionComboBox->setCurrentIndex( ui.icmpActionComboBox->findText(p->icmpAction.c_str() ) );
		ui.uptimeEdit->setText((QString)p->uptimeMin.c_str());
		if(p->uptimeMax != p->uptimeMin)
		{
			ui.uptimeBehaviorComboBox->setCurrentIndex(1);
			ui.uptimeRangeEdit->setText((QString)p->uptimeMax.c_str());
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
			pr =m_honeydConfig->m_ports[p->ports[i].first];

			//These don't need to be deleted because the clear function
			// and destructor of the tree widget does that already.
			item = new QTreeWidgetItem(0);
			item->setText(0,(QString)pr.portNum.c_str());
			item->setText(1,(QString)pr.type.c_str());
			if(!pr.behavior.compare("script"))
			{
				item->setText(2, (QString)pr.scriptName.c_str());
			}
			else
			{
				item->setText(2,(QString)pr.behavior.c_str());
			}
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

			item->setText(3, QString::fromStdString(p->ports[i].first));

			vector<string> scriptNames = m_honeydConfig->GetScriptNames();
			for(vector<string>::iterator it = scriptNames.begin(); it != scriptNames.end(); it++)
			{
				behaviorBox->addItem((QString)(*it).c_str());
			}

			behaviorBox->setFont(tempFont);
			connect(behaviorBox, SIGNAL(notifyParent(QTreeWidgetItem *, bool)), this, SLOT(portTreeWidget_comboBoxChanged(QTreeWidgetItem *, bool)));

			if(p->ports[i].second)
			{
				item->setFlags(item->flags() & ~Qt::ItemIsEditable);
			}
			else
			{
				item->setFlags(item->flags() | Qt::ItemIsEditable);
			}

			typeBox->setCurrentIndex(typeBox->findText(pr.type.c_str()));

			if(!pr.behavior.compare("script"))
			{
				behaviorBox->setCurrentIndex(behaviorBox->findText(pr.scriptName.c_str()));
			}
			else
			{
				behaviorBox->setCurrentIndex(behaviorBox->findText(pr.behavior.c_str()));
			}

			ui.portTreeWidget->setItemWidget(item, 1, typeBox);
			ui.portTreeWidget->setItemWidget(item, 2, behaviorBox);
			m_honeydConfig->m_ports[pr.portName] = pr;
			if(!portCurrentString.compare(pr.portName))
			{
				ui.portTreeWidget->setCurrentItem(item);
			}
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
		ui.dropRateSlider->setEnabled(false);
	}
}

void NovaConfig::LoadInheritedProfileSettings()
{
	QFont tempFont;
	profile *p = &m_honeydConfig->m_profiles[m_currentProfile];

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

	if(ui.ethernetCheckBox->isChecked())
	{
		p->ethernet = m_honeydConfig->m_profiles[p->parentProfile].ethernet;
	}

	if(ui.uptimeCheckBox->isChecked())
	{
		p->uptimeMin = m_honeydConfig->m_profiles[p->parentProfile].uptimeMin;
		p->uptimeMax = m_honeydConfig->m_profiles[p->parentProfile].uptimeMax;
		if(m_honeydConfig->m_profiles[p->parentProfile].uptimeMin == m_honeydConfig->m_profiles[p->parentProfile].uptimeMax)
		{
			ui.uptimeBehaviorComboBox->setCurrentIndex(1);
		}
		else
		{
			ui.uptimeBehaviorComboBox->setCurrentIndex(0);
		}
	}

	if(ui.personalityCheckBox->isChecked())
	{
		p->personality = m_honeydConfig->m_profiles[p->parentProfile].personality;
	}
	if(ui.dropRateCheckBox->isChecked())
	{
		p->dropRate = m_honeydConfig->m_profiles[p->parentProfile].dropRate;
	}
	if(ui.tcpCheckBox->isChecked())
	{
		p->tcpAction = m_honeydConfig->m_profiles[p->parentProfile].tcpAction;
	}
	if(ui.udpCheckBox->isChecked())
	{
		p->udpAction = m_honeydConfig->m_profiles[p->parentProfile].udpAction;
	}
	if(ui.icmpCheckBox->isChecked())
	{
		p->icmpAction = m_honeydConfig->m_profiles[p->parentProfile].icmpAction;
	}
}


//This is used when a profile is cloned, it allows us to copy a ptree and extract all children from it
// it is exactly the same as novagui's xml extraction functions except that it gets the ptree from the
// cloned profile and it asserts a profile's name is unique and changes the name if it isn't
void NovaConfig::LoadProfilesFromTree(string parent)
{
	using boost::property_tree::ptree;
	ptree *ptr, pt = m_honeydConfig->m_profiles[parent].tree;
	try
	{
		BOOST_FOREACH(ptree::value_type &v, pt.get_child("profiles"))
		{
			//Generic profile, essentially a honeyd template
			if(!string(v.first.data()).compare("profile"))
			{
				profile p = m_honeydConfig->m_profiles[parent];
				//Root profile has no parent
				p.parentProfile = parent;
				p.tree = v.second;

				for(uint i = 0; i < INHERITED_MAX; i++)
				{
					p.inherited[i] = true;
				}

				//Asserts the name is unique, if it is not it finds a unique name
				// up to the range of 2^32
				string profileStr = p.name;
				stringstream ss;
				uint i = 0, j = 0;
				j = ~j; //2^32-1

				while((m_honeydConfig->m_profiles.keyExists(p.name)) && (i < j))
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
					LoadProfileSettings(ptr, &p);
				}
				catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e) {};

				try //Conditional: has "add" values
				{
					ptr = &v.second.get_child("add");
					//pass 'add' subset and pointer to this profile
					LoadProfileServices(ptr, &p);
				}
				catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e) {};

				//Save the profile
				m_honeydConfig->m_profiles[p.name] = p;
				m_honeydConfig->UpdateProfile(p.name);

				try //Conditional: has children profiles
				{
					ptr = &v.second.get_child("profiles");

					//start recurisive descent down profile tree with this profile as the root
					//pass subtree and pointer to parent
					LoadProfileChildren(p.name);
				}
				catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e) {};
			}

			//Honeyd's implementation of switching templates based on conditions
			else if(!string(v.first.data()).compare("dynamic"))
			{
				//TODO
			}
			else
			{
				LOG(ERROR, "Invalid XML Path "+string(v.first.data()), "");
			}
		}
	}
	catch(std::exception &e)
	{
		LOG(ERROR, "Problem loading Profiles: "+string(e.what()), "");
		m_mainwindow->m_prompter->DisplayPrompt(m_mainwindow->HONEYD_LOAD_FAIL, "Problem loading Profiles: " + string(e.what()));
	}
}

//Sets the configuration of 'set' values for profile that called it
void NovaConfig::LoadProfileSettings(ptree *ptr, profile *p)
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
			prefix = "uptimeMax";
			if(!string(v.first.data()).compare(prefix))
			{
				p->uptimeMax = v.second.data();
				continue;
			}
			prefix = "uptimeMin";
			if(!string(v.first.data()).compare(prefix))
			{
				p->uptimeMin = v.second.data();
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
		LOG(ERROR, "Problem loading profile set parameters: "+string(e.what()), "");
	}
}

//Adds specified ports and subsystems
// removes any previous port with same number and type to avoid conflicts
void NovaConfig::LoadProfileServices(ptree *ptr, profile *p)
{
	string prefix;
	port *prt;

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
					prt = &m_honeydConfig->m_ports[v2.second.data()];

					//Checks inherited ports for conflicts
					for(uint i = 0; i < p->ports.size(); i++)
					{
						//Erase inherited port if a conflict is found
						if(!prt->portNum.compare(m_honeydConfig->m_ports[p->ports[i].first].portNum) && !prt->type.compare(m_honeydConfig->m_ports[p->ports[i].first].type))
						{
							p->ports.erase(p->ports.begin()+i);
						}
					}
					//Add specified port
					pair<string, bool> portPair;
					portPair.first = prt->portName;
					portPair.second = false;
					if(!p->ports.size())
					{
						p->ports.push_back(portPair);
					}
					else
					{
						uint i = 0;
						for(i = 0; i < p->ports.size(); i++)
						{
							port *temp = &m_honeydConfig->m_ports[p->ports[i].first];
							if((atoi(temp->portNum.c_str())) < (atoi(prt->portNum.c_str())))
							{
								continue;
							}
							break;
						}
						if(i < p->ports.size())
						{
							p->ports.insert(p->ports.begin()+i, portPair);
						}
						else
						{
							p->ports.push_back(portPair);
						}
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
		LOG(ERROR, "Problem loading profile add parameters: "+string(e.what()), "");
	}
}

//Recurisve descent down a profile tree, inherits parent, sets values and continues if not leaf.
void NovaConfig::LoadProfileChildren(string parent)
{
	ptree ptr = m_honeydConfig->m_profiles[parent].tree;
	try
	{
		BOOST_FOREACH(ptree::value_type &v, ptr.get_child("profiles"))
		{
			ptree *ptr2;

			//Inherits parent,
			profile prof = m_honeydConfig->m_profiles[parent];
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
			while((m_honeydConfig->m_profiles.keyExists(prof.name)) && (i < j))
			{
				ss.str("");
				i++;
				ss << profileStr << "-" << i;
				prof.name = ss.str();
			}
			prof.tree.put<std::string>("name", prof.name);

			try //Conditional: If profile has set configurations different from parent
			{
				ptr2 = &v.second.get_child("set");
				LoadProfileSettings(ptr2, &prof);
			}
			catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e) {};

			try //Conditional: If profile has port or subsystems different from parent
			{
				ptr2 = &v.second.get_child("add");
				LoadProfileServices(ptr2, &prof);
			}
			catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e) {};

			//Saves the profile
			m_honeydConfig->m_profiles[prof.name] = prof;
			m_honeydConfig->UpdateProfile(prof.name);

			try //Conditional: if profile has children (not leaf)
			{
				LoadProfileChildren(prof.name);
			}
			catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::property_tree::ptree_bad_path> > &e) {};
		}
	}
	catch(std::exception &e)
	{
		LOG(ERROR, "Problem loading sub profiles: "+string(e.what()), "");
	}
}

//Draws all profile heirarchy in the tree widget
void NovaConfig::LoadAllProfiles()
{
	m_loading->lock();
	ui.hsProfileTreeWidget->clear();
	ui.profileTreeWidget->clear();
	ui.hsProfileTreeWidget->sortByColumn(0,Qt::AscendingOrder);
	ui.profileTreeWidget->sortByColumn(0,Qt::AscendingOrder);

	if(m_honeydConfig->m_profiles.size())
	{
		//calls createProfileItem on every profile, this will first assert that all ancestors have items
		// and create them if not to draw the table correctly, thus the need for the NULL pointer as a flag
		for(ProfileTable::iterator it = m_honeydConfig->m_profiles.begin(); it != m_honeydConfig->m_profiles.end(); it++)
		{
			CreateProfileItem(it->first);
		}
		//Sets the current selection to the original selection
		//ui.profileTreeWidget->setCurrentItem(m_honeydConfig->m_profiles[m_currentProfile].profileItem);
		//populates the window and expand the profile heirarchy
		ui.hsProfileTreeWidget->expandAll();
		ui.profileTreeWidget->expandAll();
		LoadProfileSettings();
	}
	//If no profiles exist, do nothing and ensure no profile is selected
	else
	{
		m_currentProfile = "";
	}
	m_loading->unlock();
}



QTreeWidgetItem *NovaConfig::GetProfileTreeWidgetItem(string profileName)
{
	QList<QTreeWidgetItem*> items = ui.profileTreeWidget->findItems(QString(profileName.c_str()),
		Qt::MatchExactly | Qt::MatchFixedString | Qt::MatchRecursive, 0);
	if(items.empty())
	{
		return NULL;
	}
	return items.first();
}

QTreeWidgetItem *NovaConfig::GetProfileHsTreeWidgetItem(string profileName)
{
	QList<QTreeWidgetItem*> items = ui.hsProfileTreeWidget->findItems(QString(profileName.c_str()),
			Qt::MatchExactly | Qt::MatchFixedString | Qt::MatchRecursive, 1);
	if(items.empty())
	{
		return NULL;
	}
	return items.first();
}


QTreeWidgetItem *NovaConfig::GetSubnetTreeWidgetItem(string subnetName)
{
	QList<QTreeWidgetItem*> items = ui.nodeTreeWidget->findItems(QString(subnetName.c_str()),
		Qt::MatchEndsWith, 1);
	if(items.empty())
	{
		return NULL;
	}
	return items.first();
}



QTreeWidgetItem *NovaConfig::GetSubnetHsTreeWidgetItem(string subnetName)
{
	QList<QTreeWidgetItem*> items = ui.hsNodeTreeWidget->findItems(QString(subnetName.c_str()),
		Qt::MatchEndsWith, 0);
	if(items.empty())
	{
		return NULL;
	}
	return items.first();
}

QTreeWidgetItem *NovaConfig::GetNodeTreeWidgetItem(string nodeName)
{
	QList<QTreeWidgetItem*> items = ui.nodeTreeWidget->findItems(QString(nodeName.c_str()),
		Qt::MatchExactly | Qt::MatchFixedString | Qt::MatchRecursive, 0);
	if(items.empty())
	{
		return NULL;
	}
	return items.first();
}

QTreeWidgetItem *NovaConfig::GetNodeHsTreeWidgetItem(string nodeName)
{
	QList<QTreeWidgetItem*> items = ui.hsNodeTreeWidget->findItems(QString(nodeName.c_str()),
		Qt::MatchExactly | Qt::MatchFixedString | Qt::MatchRecursive, 0);
	if(items.empty())
	{
		return NULL;
	}
	return items.first();
}


bool NovaConfig::IsPortTreeWidgetItem(std::string port, QTreeWidgetItem* item)
{
	stringstream ss;
	ss << item->text(0).toStdString() << "_";
	ss << item->text(1).toStdString() << "_";
	ss << item->text(2).toStdString();
	return (ss.str() == port);
}

//Creates tree widget items for a profile and all ancestors if they need one.
void NovaConfig::CreateProfileItem(string pstr)
{
	profile p = m_honeydConfig->m_profiles[pstr];
	//If the profile hasn't had an item created yet
	if(GetProfileTreeWidgetItem(p.name) == NULL)
	{
		QTreeWidgetItem *item = NULL;
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

			//Create the profile item for the profile tree
			item = new QTreeWidgetItem(ui.profileTreeWidget,0);
			item->setText(0, (QString)profileStr.c_str());
		}
		//if the profile has ancestors
		else
		{
			//find the parent and assert that they have an item
			if(m_honeydConfig->m_profiles.keyExists(p.parentProfile))
			{
				profile parent = m_honeydConfig->m_profiles[p.parentProfile];

				if(GetProfileTreeWidgetItem(parent.name) == NULL)
				{
					//if parent has no item recursively ascend until all parents do
					CreateProfileItem(p.parentProfile);
					parent = m_honeydConfig->m_profiles[p.parentProfile];
				}
				//Now that all ancestors have items, create the profile's item

				//*NOTE*
				//These items don't need to be deleted because the clear function
				// and destructor of the tree widget does that already.
				item = new QTreeWidgetItem(item = GetProfileHsTreeWidgetItem(parent.name), 0);
				item->setText(0, (QString)profileStr.c_str());

				//Create the profile item for the profile tree
				item = new QTreeWidgetItem(item = GetProfileTreeWidgetItem(parent.name), 0);
				item->setText(0, (QString)profileStr.c_str());
			}
		}
		m_honeydConfig->m_profiles[p.name] = p;
	}
}

void NovaConfig::SetInputValidators()
{
	// Allows positive integers
	QIntValidator *uintValidator = new QIntValidator();
	uintValidator->setBottom(0);

	// Allows positive doubles
	QDoubleValidator *udoubleValidator = new QDoubleValidator();
	udoubleValidator->setBottom(0);

	// Set up input validators so user can't enter obviously bad data in the QLineEdits
	// General settings
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
	// XXX ui.dmIPEdit->setValidator(noSpaceValidator);
}

/******************************************
 * Profile Menu GUI Signals ***************/

/* This loads the profile config menu for the profile selected */
void NovaConfig::on_profileTreeWidget_itemSelectionChanged()
{
	if(!m_loading->tryLock())
	{
		return;
	}
	if(m_honeydConfig->m_profiles.size())
	{
		//Save old profile
		SaveProfileSettings();
		if(!ui.profileTreeWidget->selectedItems().isEmpty())
		{
			QTreeWidgetItem *item = ui.profileTreeWidget->selectedItems().first();
			m_currentProfile = item->text(0).toStdString();
		}
		LoadProfileSettings();
		m_loading->unlock();
	}
	m_loading->unlock();
}

//Self explanatory, see deleteProfile for details
void NovaConfig::on_deleteButton_clicked()
{
	Q_EMIT on_actionProfileDelete_triggered();
}

void NovaConfig::on_actionProfileDelete_triggered()
{
	if((!ui.profileTreeWidget->selectedItems().isEmpty()) && m_currentProfile.compare("default")
		&& (m_honeydConfig->m_profiles.keyExists(m_currentProfile)))
	{
		if( m_honeydConfig->IsProfileUsed(m_currentProfile))
		{
			LOG(ERROR, "ERROR: A Node is currently using this profile.","");
			if(m_mainwindow->m_prompter->DisplayPrompt(m_mainwindow->CANNOT_DELETE_ITEM, "Profile "
				+m_currentProfile+" cannot be deleted because some nodes are currently using it, would you like to "
				"disable all nodes currently using it?",ui.actionNo_Action, ui.actionNo_Action, this) == CHOICE_DEFAULT)
			{
				m_honeydConfig->DisableProfileNodes(m_currentProfile);
			}
		}
		//TODO appropriate display prompt here
		else
		{
			DeleteProfile(m_currentProfile);
		}
	}
	else if(!m_currentProfile.compare("default"))
	{
		LOG(INFO, "Notify: Cannot delete the default profile.","");

		if(m_mainwindow->m_prompter->DisplayPrompt(m_mainwindow->CANNOT_DELETE_ITEM, "Cannot delete the default profile, "
			"would you like to disable the Haystack instead?",ui.actionNo_Action, ui.actionNo_Action, this) == CHOICE_DEFAULT)
		{
			m_loading->lock();
			bool tempSelBool = m_selectedSubnet;
			m_selectedSubnet = true;
			string tempNode = m_currentNode;
			m_currentNode = "";
			string tempNet = m_currentSubnet;
			for(SubnetTable::iterator it = m_honeydConfig->m_subnets.begin(); it != m_honeydConfig->m_subnets.end(); it++)
			{
				m_currentSubnet = it->second.name;
				on_actionNodeDisable_triggered();
				m_loading->lock();
			}
			m_selectedSubnet = tempSelBool;
			m_currentNode = tempNode;
			m_currentSubnet = tempNet;
			m_loading->unlock();
		}
	}
	m_loading->unlock();
	LoadAllNodes();
}

//Creates a base profile with default values seen below
void NovaConfig::on_addButton_clicked()
{
	Q_EMIT on_actionProfileAdd_triggered();
}

void NovaConfig::on_actionProfileAdd_triggered()
{
	m_loading->lock();
	struct profile temp;
	temp.name = "New Profile";

	stringstream ss;
	uint i = 0, j = 0;
	j = ~j; // 2^32-1

	//Finds a unique identifier
	while((m_honeydConfig->m_profiles.keyExists(temp.name)) && (i < j))
	{
		i++;
		ss.str("");
		ss << "New Profile-" << i;
		temp.name = ss.str();
	}
	//If there is currently a selected profile, that profile will be the parent of the new profile
	if(m_honeydConfig->m_profiles.keyExists(m_currentProfile))
	{
		string tempName = temp.name;
		temp = m_honeydConfig->m_profiles[m_currentProfile];
		temp.name = tempName;
		temp.parentProfile = m_currentProfile;
		for(uint i = 0; i < INHERITED_MAX; i++)
		{
			temp.inherited[i] = true;
		}
		for(uint i = 0; i < temp.ports.size(); i++)
		{
			temp.ports[i].second = true;
		}
		m_currentProfile = temp.name;
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
		temp.uptimeMin = "0";
		temp.uptimeMax = "0";
		temp.dropRate = "0";
		temp.ports.clear();
		m_currentProfile = temp.name;
		for(uint i = 0; i < INHERITED_MAX; i++)
		{
			temp.inherited[i] = false;
		}
	}
	//Puts the profile in the table, creates a ptree and loads the new configuration
	m_honeydConfig->AddProfile(&temp);
	m_loading->unlock();
	LoadAllProfiles();
	LoadAllNodes();
}

//Copies a profile and all of it's descendants
void NovaConfig::on_cloneButton_clicked()
{
	Q_EMIT on_actionProfileClone_triggered();
}

void NovaConfig::on_actionProfileClone_triggered()
{

	//Do nothing if no profiles
	if(m_honeydConfig->m_profiles.size())
	{
		m_loading->lock();
		QTreeWidgetItem *item = ui.profileTreeWidget->selectedItems().first();
		string profileStr = item->text(0).toStdString();
		profile p = m_honeydConfig->m_profiles[m_currentProfile];

		stringstream ss;
		uint i = 1, j = 0;
		j = ~j; //2^32-1

		//Since we are cloning, it will already be a duplicate
		ss.str("");
		ss << profileStr << "-" << i;
		p.name = ss.str();

		//Check for name in use, if so increase number until unique name is found
		while((m_honeydConfig->m_profiles.keyExists(p.name)) && (i < j))
		{
			ss.str("");
			i++;
			ss << profileStr << "-" << i;
			p.name = ss.str();
		}
		p.tree.put<std::string>("name",p.name);
		//Change the profile name and put in the table, update the current profile
		//Extract all descendants, create a ptree, update with new configuration
		m_honeydConfig->m_profiles[p.name] = p;
		LoadProfilesFromTree(p.name);
		m_honeydConfig->UpdateProfile(p.name);
		m_loading->unlock();
		LoadAllProfiles();
		LoadAllNodes();
	}
}

void NovaConfig::on_profileEdit_editingFinished()
{
	if(!m_loading->tryLock())
	{
		return;
	}
	if(!m_honeydConfig->m_profiles.empty())
	{
		GetProfileTreeWidgetItem(m_currentProfile)->setText(0,ui.profileEdit->displayText());
		GetProfileHsTreeWidgetItem(m_currentProfile)->setText(0,ui.profileEdit->displayText());
		//If the name has changed we need to move it in the profile hash table and point all
		//nodes that use the profile to the new location.
		m_honeydConfig->RenameProfile(m_currentProfile, ui.profileEdit->displayText().toStdString());
		SaveProfileSettings();
		LoadProfileSettings();
		m_loading->unlock();
		LoadAllNodes();
	}
	m_loading->unlock();
}

/******************************************
 * Node Menu GUI Functions ****************/

void NovaConfig::LoadAllNodes()
{
	m_loading->lock();
	QBrush whitebrush(QColor(255, 255, 255, 255));
	QBrush greybrush(QColor(100, 100, 100, 255));
	greybrush.setStyle(Qt::SolidPattern);
	QBrush blackbrush(QColor(0, 0, 0, 255));
	blackbrush.setStyle(Qt::NoBrush);
	struct Node *n = NULL;

	QTreeWidgetItem *item = NULL;
	QTreeWidgetItem *hsItem = NULL;
	ui.nodeTreeWidget->clear();
	ui.hsNodeTreeWidget->clear();

	QTreeWidgetItem *subnetItem = NULL;
	QTreeWidgetItem *subnetHsItem = NULL;

	for(SubnetTable::iterator it = m_honeydConfig->m_subnets.begin(); it != m_honeydConfig->m_subnets.end(); it++)
	{
		//create the subnet item for the Haystack menu tree
		subnetHsItem  = new QTreeWidgetItem(ui.hsNodeTreeWidget, 0);
		subnetHsItem ->setText(0, (QString)it->second.address.c_str());
		if(it->second.isRealDevice)
		{
			subnetHsItem ->setText(1, (QString)"Physical Device - "+it->second.name.c_str());
		}
		else
		{
			subnetHsItem ->setText(1, (QString)"Virtual Interface - "+it->second.name.c_str());
		}

		//create the subnet item for the node edit tree
		subnetItem = new QTreeWidgetItem(ui.nodeTreeWidget, 0);
		subnetItem->setText(0, (QString)it->second.address.c_str());
		if(it->second.isRealDevice)
		{
			subnetItem->setText(1, (QString)"Physical Device - "+it->second.name.c_str());
		}
		else
		{
			subnetItem->setText(1, (QString)"Virtual Interface - "+it->second.name.c_str());
		}

		if(!it->second.enabled)
		{
			whitebrush.setStyle(Qt::NoBrush);
			subnetItem->setBackground(0,greybrush);
			subnetItem->setForeground(0,whitebrush);
			subnetHsItem->setBackground(0,greybrush);
			subnetHsItem->setForeground(0,whitebrush);
		}

		//Pre-create a list of profiles for the node profile selection
		QStringList *profileStrings = new QStringList();
		{
			for(ProfileTable::iterator it = m_honeydConfig->m_profiles.begin(); it != m_honeydConfig->m_profiles.end(); it++)
			{
				profileStrings->append(QString(it->first.c_str()));
			}
		}
		profileStrings->sort();

		for(uint i = 0; i < it->second.nodes.size(); i++)
		{

			n = &m_honeydConfig->m_nodes[it->second.nodes[i]];

			//Create the node item for the Haystack tree
			item = new QTreeWidgetItem(subnetHsItem, 0);
			item->setText(0, (QString)n->name.c_str());
			item->setText(1, (QString)n->pfile.c_str());

			//Create the node item for the node edit tree
			item = new QTreeWidgetItem(subnetItem, 0);
			item->setText(0, (QString)n->name.c_str());
			item->setText(1, (QString)n->pfile.c_str());

			TreeItemComboBox *pfileBox = new TreeItemComboBox(this, item);
			pfileBox->addItems(*profileStrings);
			QObject::connect(pfileBox, SIGNAL(notifyParent(QTreeWidgetItem *, bool)), this, SLOT(nodeTreeWidget_comboBoxChanged(QTreeWidgetItem *, bool)));
			pfileBox->setCurrentIndex(pfileBox->findText(n->pfile.c_str()));

			ui.nodeTreeWidget->setItemWidget(item, 1, pfileBox);
			if(!n->name.compare("Doppelganger"))
			{
				ui.dmCheckBox->setChecked(n->enabled);
				//Enable the loopback subnet as well if DM is enabled
				m_honeydConfig->m_subnets[n->sub].enabled |= n->enabled;
			}
			if(!n->enabled)
			{
				hsItem = GetNodeHsTreeWidgetItem(n->name);
				item = GetNodeTreeWidgetItem(n->name);
				whitebrush.setStyle(Qt::NoBrush);
				hsItem->setBackground(0,greybrush);
				hsItem->setForeground(0,whitebrush);
				item->setBackground(0,greybrush);
				item->setForeground(0,whitebrush);
			}
		}
		delete profileStrings;
	}
	ui.nodeTreeWidget->expandAll();

	// Reselect the last selected node if need be
	QTreeWidgetItem* nodeItem = GetNodeTreeWidgetItem(m_currentNode);
	if (nodeItem != NULL)
	{
		nodeItem->setSelected(true);
	}
	else
	{
		if(m_honeydConfig->m_subnets.size())
		{
			if(m_honeydConfig->m_subnets.keyExists(m_currentSubnet))
			{
				if (GetSubnetTreeWidgetItem(m_currentSubnet) != NULL)
				{
					ui.nodeTreeWidget->setCurrentItem(GetSubnetTreeWidgetItem(m_currentSubnet));
				}
			}
		}
	}
	m_loading->unlock();
}

//Function called when delete button
void NovaConfig::DeleteNodes()
{
	m_loading->lock();
	string name = "";
	if((m_selectedSubnet && (!m_currentSubnet.compare(""))) || (!m_selectedSubnet && (!m_currentNode.compare(""))))
	{
		LOG(CRITICAL,"No items in the UI to delete!",
			"Should never get here. Attempting to delete a GUI item, but there are no GUI items left.");
		m_loading->unlock();
		return;
	}
	//If we have a subnet selected
	if(m_selectedSubnet)
	{
		//Get the subnet
		subnet *s = &m_honeydConfig->m_subnets[m_currentSubnet];

		//Remove all nodes in the subnet
		while(!s->nodes.empty())
		{
			//If we are unable to delete the node
			if(!m_honeydConfig->DeleteNode(s->nodes.back()))
			{
				LOG(WARNING, string("Unabled to delete node:") + s->nodes.back(), "");
			}
		}

		//If this subnet is going to be removed
		if(!m_honeydConfig->m_subnets[m_currentSubnet].isRealDevice)
		{
			//remove the subnet from the config
			m_honeydConfig->m_subnets.erase(m_currentSubnet);

			//Get the current item in the list
			QTreeWidgetItem *cur = ui.nodeTreeWidget->selectedItems().first();
			//Get the subnet below it
			int index = ui.nodeTreeWidget->indexOfTopLevelItem(cur) + 1;
			//If there is no subnet below it, get the subnet above it
			if(index == ui.nodeTreeWidget->topLevelItemCount())
			{
				index -= 2;
			}

			//If we have the index of a valid subnet set it as the next selection
			if((index >= 0) && (index < ui.nodeTreeWidget->topLevelItemCount()))
			{
				QTreeWidgetItem *next = ui.nodeTreeWidget->topLevelItem(index);
				m_currentSubnet = next->text(1).toStdString();
				m_currentSubnet = m_currentSubnet.substr(m_currentSubnet.find("-"), m_currentSubnet.size());
			}
			//No valid selection
			else
			{
				m_currentSubnet = "";
			}
		}
		//If this subnet will remain
		else
		{
			m_loading->unlock();
			on_actionNodeDisable_triggered();
			m_loading->lock();
		}
	}
	//If we have a node selected
	else
	{
		//Get the current item
		QTreeWidgetItem *cur = ui.nodeTreeWidget->selectedItems().first();
		//Get the next item in the list
		QTreeWidgetItem *next = ui.nodeTreeWidget->itemBelow(cur);

		//If the next item is a subnet or doesn't exist
		if((next == NULL) || (ui.nodeTreeWidget->indexOfTopLevelItem(next) != -1))
		{
			//Get the previous item
			next = ui.nodeTreeWidget->itemAbove(cur);
		}
		//If there is no valid item above the node then theres a real problem
		if(next == NULL)
		{
			LOG(CRITICAL, "Qt UI Node list may be corrupted.",
				"Node is alone in the tree widget, this shouldn't be possible!");
			m_loading->unlock();
			return;
		}
		//If we are unable to delete the node
		if(!m_honeydConfig->DeleteNode(m_currentNode))
		{
			LOG(ERROR, string("Unabled to delete node:") + m_currentNode, "");
			m_loading->unlock();
			return;
		}

		//If we have a node selected
		if(ui.nodeTreeWidget->indexOfTopLevelItem(next) == -1)
		{
			m_currentNode = next->text(0).toStdString();
			m_currentSubnet = next->parent()->text(1).toStdString();
			m_currentSubnet = m_currentSubnet.substr(m_currentSubnet.find("-")+2, m_currentSubnet.size());
		}
		//If we have a subnet selected
		else
		{
			//flag the selection as that of a subnet
			m_selectedSubnet = true;
			m_currentNode = "";
			m_currentSubnet = next->text(1).toStdString();
			m_currentSubnet = m_currentSubnet.substr(m_currentSubnet.find("-")+2, m_currentSubnet.size());
		}
	}
	//Unlock and redraw the list
	m_loading->unlock();
	LoadAllNodes();
}


/******************************************
 * Node Menu GUI Signals ******************/

//The current selection in the node list
void NovaConfig::on_nodeTreeWidget_itemSelectionChanged()
{
	//If the user is changing the selection AND something is selected
	if(m_loading->tryLock())
	{
		if(!ui.nodeTreeWidget->selectedItems().isEmpty())
		{
			QTreeWidgetItem *item = ui.nodeTreeWidget->selectedItems().first();
			//If it's not a top level item (which means it's a node)
			if(ui.nodeTreeWidget->indexOfTopLevelItem(item) == -1)
			{
				m_currentNode = item->text(0).toStdString();
				m_currentSubnet = m_honeydConfig->GetNodeSubnet(m_currentNode);
				m_selectedSubnet = false;
			}
			else //If it's a subnet
			{
				m_currentSubnet = item->text(1).toStdString();
				m_currentSubnet = m_currentSubnet.substr(m_currentSubnet.find("-")+2, m_currentSubnet.size());
				m_selectedSubnet = true;
			}
		}
		m_loading->unlock();
	}
}

void NovaConfig::nodeTreeWidget_comboBoxChanged(QTreeWidgetItem *item, bool edited)
{
	if(m_loading->tryLock())
	{
		if(edited)
		{
			ui.nodeTreeWidget->setCurrentItem(item);
			string oldPfile;
			if(!ui.nodeTreeWidget->selectedItems().isEmpty())
			{
				Node *n = &m_honeydConfig->m_nodes[item->text(0).toStdString()];
				oldPfile = n->pfile;
				TreeItemComboBox *pfileBox = (TreeItemComboBox* )ui.nodeTreeWidget->itemWidget(item, 1);
				n->pfile = pfileBox->currentText().toStdString();
			}

			m_loading->unlock();
			LoadAllNodes();
		}
		else
		{
			ui.nodeTreeWidget->setFocus(Qt::OtherFocusReason);
			ui.nodeTreeWidget->setCurrentItem(item);
			m_loading->unlock();
		}
	}
}
void NovaConfig::on_actionSubnetAdd_triggered()
{
	if(m_currentSubnet.compare(""))
	{
		m_loading->lock();
		subnet s = m_honeydConfig->m_subnets[m_currentSubnet];
		s.name = m_currentSubnet + "-1";
		s.address = "0.0.0.0/24";
		s.nodes.clear();
		s.enabled = false;
		s.base = 0;
		s.maskBits = 24;
		s.max = 255;
		s.isRealDevice = false;
		m_honeydConfig->m_subnets[s.name] = s;
		m_currentSubnet = s.name;
		m_loading->unlock();
		subnetPopup *editSubnet = new subnetPopup(this, &m_honeydConfig->m_subnets[m_currentSubnet]);
		editSubnet->show();
	}
}
// Right click menus for the Node tree
void NovaConfig::on_actionNodeAdd_triggered()
{
	if(m_currentSubnet.compare(""))
	{
		Node n;
		n.sub = m_currentSubnet;
		n.interface = m_honeydConfig->m_subnets[m_currentSubnet].name;
		n.realIP = m_honeydConfig->m_subnets[m_currentSubnet].base;
		n.pfile = "default";
		nodePopup *editNode =  new nodePopup(this, &n);
		editNode->show();
	}
}

void NovaConfig::on_actionNodeDelete_triggered()
{
	DeleteNodes();
}

void NovaConfig::on_actionNodeClone_triggered()
{
	if (m_currentNode.compare(""))
	{
		m_loading->lock();
		Node n = m_honeydConfig->m_nodes[m_currentNode];
		m_loading->unlock();

		// Can't clone the doppelganger, only allowed one right now
		if (n.name == "Doppelganger")
		{
			return;
		}

		nodePopup *editNode =  new nodePopup(this, &n);
		editNode->show();
	}
}

void  NovaConfig::on_actionNodeEdit_triggered()
{
	if(!m_selectedSubnet)
	{
		// Can't change the doppel IP here, you change it in the doppel settings
		if (m_currentNode == "Doppelganger")
		{
			return;
		}

		nodePopup *editNode =  new nodePopup(this, &m_honeydConfig->m_nodes[m_currentNode], true);
		editNode->show();
	}
	else
	{
		subnetPopup *editSubnet = new subnetPopup(this, &m_honeydConfig->m_subnets[m_currentSubnet]);
		editSubnet->show();
	}
}

void NovaConfig::on_actionNodeCustomizeProfile_triggered()
{
	m_loading->lock();
	m_currentProfile = m_honeydConfig->m_nodes[m_currentNode].pfile;
	ui.stackedWidget->setCurrentIndex(ui.menuTreeWidget->topLevelItemCount()+1);
	QTreeWidgetItem *item = ui.menuTreeWidget->topLevelItem(HAYSTACK_MENU_INDEX);
	item = ui.menuTreeWidget->itemBelow(item);
	item = ui.menuTreeWidget->itemBelow(item);
	ui.menuTreeWidget->setCurrentItem(item);
	ui.profileTreeWidget->setCurrentItem(GetProfileTreeWidgetItem(m_currentProfile));
	m_loading->unlock();
	Q_EMIT on_actionProfileAdd_triggered();
	m_honeydConfig->m_nodes[m_currentNode].pfile = m_currentProfile;
	LoadHaystackConfiguration();
}

void NovaConfig::on_actionNodeEnable_triggered()
{
	if(m_selectedSubnet)
	{
		subnet s = m_honeydConfig->m_subnets[m_currentSubnet];
		for(uint i = 0; i < s.nodes.size(); i++)
		{
			m_honeydConfig->EnableNode(s.nodes[i]);

		}
		s.enabled = true;
		m_honeydConfig->m_subnets[m_currentSubnet] = s;
	}
	else
	{
		m_honeydConfig->EnableNode(m_currentNode);
	}

	//Draw the nodes and restore selection
	m_loading->unlock();
	LoadAllNodes();
	m_loading->lock();
	if(m_selectedSubnet)
	{
		if (GetSubnetTreeWidgetItem(m_currentSubnet) != NULL)
		{
			ui.nodeTreeWidget->setCurrentItem(GetSubnetTreeWidgetItem(m_currentSubnet));
		}
	}
	else
	{
		//ui.nodeTreeWidget->setCurrentItem(m_honeydConfig->m_nodes[m_currentNode].nodeItem);
	}
	m_loading->unlock();
}

void NovaConfig::on_actionNodeDisable_triggered()
{
	if(m_selectedSubnet)
	{
		subnet s = m_honeydConfig->m_subnets[m_currentSubnet];
		for(uint i = 0; i < s.nodes.size(); i++)
		{

			m_honeydConfig->DisableNode(s.nodes[i]);
		}
		s.enabled = false;
		m_honeydConfig->m_subnets[m_currentSubnet] = s;
	}
	else
	{
		m_honeydConfig->DisableNode(m_currentNode);
	}

	//Draw the nodes and restore selection
	m_loading->unlock();
	LoadAllNodes();
	m_loading->lock();
	if(m_selectedSubnet)
	{
		if (GetSubnetTreeWidgetItem(m_currentSubnet) != NULL)
		{
			ui.nodeTreeWidget->setCurrentItem(GetSubnetTreeWidgetItem(m_currentSubnet));
		}
	}
	else
	{
		//ui.nodeTreeWidget->setCurrentItem(m_honeydConfig->m_nodes[m_currentNode].nodeItem);
	}
	m_loading->unlock();
}

//Not currently used, will be implemented in the new GUI design TODO
//Pops up the node edit window selecting the current node
void NovaConfig::on_nodeEditButton_clicked()
{
	Q_EMIT on_actionNodeEdit_triggered();
}
//Not currently used, will be implemented in the new GUI design TODO
//Creates a copy of the current node and pops up the edit window
void NovaConfig::on_nodeCloneButton_clicked()
{
	on_actionNodeClone_triggered();
}
//Not currently used, will be implemented in the new GUI design TODO
//Creates a new node and pops up the edit window
void NovaConfig::on_nodeAddButton_clicked()
{
	on_actionNodeAdd_triggered();
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
	DisplayMACPrefixWindow();
}

void NovaConfig::on_setPersonalityButton_clicked()
{
	DisplayNmapPersonalityWindow();
}

void NovaConfig::on_dropRateSlider_valueChanged()
{
	stringstream ss;
	ss << ui.dropRateSlider->value() << "%";
	ui.dropRateSetting->setText((QString)ss.str().c_str());
}


bool NovaConfig::IsDoppelIPValid()
{
	stringstream ss;
	ss 		<< ui.dmIPSpinBox_0->value() << "."
			<< ui.dmIPSpinBox_1->value() << "."
			<< ui.dmIPSpinBox_2->value() << "."
			<< ui.dmIPSpinBox_3->value();

	if(m_honeydConfig->IsIPUsed(ss.str()) && (ss.str().compare(Config::Inst()->GetDoppelIp())))
	{
		return false;
	}
	return true;
}
//Doppelganger IP Address Spin boxes
void NovaConfig::on_dmIPSpinBox_0_valueChanged()
{
	if (!IsDoppelIPValid())
	{
		LOG(WARNING, "Current IP address conflicts with a statically address Haystack node.", "");
	}
}

void NovaConfig::on_dmIPSpinBox_1_valueChanged()
{
	if (!IsDoppelIPValid())
	{
		LOG(WARNING, "Current IP address conflicts with a statically address Haystack node.", "");
	}
}

void NovaConfig::on_dmIPSpinBox_2_valueChanged()
{
	if (!IsDoppelIPValid())
	{
		LOG(WARNING, "Current IP address conflicts with a statically address Haystack node.", "");
	}
}

void NovaConfig::on_dmIPSpinBox_3_valueChanged()
{
	if (!IsDoppelIPValid())
	{
		LOG(WARNING, "Current IP address conflicts with a statically address Haystack node.", "");
	}
}

//Haystack storage type combo box
void NovaConfig::on_hsSaveTypeComboBox_currentIndexChanged(int index)
{
	switch(index)
	{
		case 1:
		{
			ui.hsConfigEdit->setText((QString)Config::Inst()->GetPathConfigHoneydUser().c_str());
			ui.hsSummaryGroupBox->setEnabled(false);
			ui.nodesGroupBox->setEnabled(false);
			ui.profileGroupBox->setEnabled(false);
			break;
		}
		case 0:
		{
			ui.hsConfigEdit->setText((QString)Config::Inst()->GetPathConfigHoneydHS().c_str());
			ui.hsSummaryGroupBox->setEnabled(true);
			ui.nodesGroupBox->setEnabled(true);
			ui.profileGroupBox->setEnabled(true);
			break;
		}
		default:
		{
			LOG(ERROR, "Haystack save type set to undefined index!", "");
			break;
		}
	}
}

void NovaConfig::on_useAllIfCheckBox_stateChanged()
{
	ui.interfaceGroupBox->setEnabled(!ui.useAllIfCheckBox->isChecked());
}

void NovaConfig::on_useAnyLoopbackCheckBox_stateChanged()
{
	ui.loopbackGroupBox->setEnabled(!ui.useAnyLoopbackCheckBox->isChecked());
}
