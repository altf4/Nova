//============================================================================
// Name        : novagui.cpp
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
// Description : The main NovaGUI component, utilizes the auto-generated ui_novagui.h
//============================================================================
#include "novagui.h"
#include "NovaUtil.h"
#include "run_popup.h"
#include "novaconfig.h"
#include "nova_manual.h"
#include "classifierPrompt.h"
#include "nova_ui_core.h"
#include "CallbackHandler.h"
#include "Logger.h"

#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <QFileDialog>
#include <signal.h>
#include <errno.h>
#include <QDir>
#include <sys/un.h>

using namespace std;
using namespace Nova;

pthread_rwlock_t lock;
string homePath, readPath, writePath;

//General variables like tables, flags, locks, etc.
SuspectGUIHashTable SuspectTable;

// Defines the order of components in the process list and novaComponents array
#define COMPONENT_NOVAD 0
#define COMPONENT_HSH 1

#define NOVA_COMPONENTS 2

/************************************************
 * Constructors, Destructors and Closing Actions
 ************************************************/

//Called when process receives a SIGINT, like if you press ctrl+c
void sighandler(int __attribute__((unused)) param)
{
	if(!CloseNovadConnection())
	{
		LOG(ERROR, "Did not close down connection to Novad cleanly", "CloseNovadConnection() failed");
	}
	QCoreApplication::exit(EXIT_SUCCESS);
}

NovaGUI::NovaGUI(QWidget *parent)
    : QMainWindow(parent)
{
	signal(SIGINT, sighandler);
	pthread_rwlock_init(&lock, NULL);

	uint64_t initKey = 0;
	initKey--;
	SuspectTable.set_empty_key(initKey);
	initKey--;
	SuspectTable.set_deleted_key(initKey);


	m_editingSuspectList = false;
	m_pathsFile = (char*)"/etc/nova/paths";

	ui.setupUi(this);

	//Pre-forms the suspect menu
	m_suspectMenu = new QMenu(this);
	m_systemStatMenu = new QMenu(this);

	m_isHelpUp = false;

	InitConfiguration();
	InitPaths();
	InitiateSystemStatus();
	InitializeUI();

	// Create the dialog generator
	m_prompter= new DialogPrompter();

	// Register our desired error message types
	messageType t;
	t.action = CHOICE_SHOW;

	// Error prompts
	t.type = errorPrompt;

	t.descriptionUID = "Failure reading config files";
	CONFIG_READ_FAIL = m_prompter->RegisterDialog(t);

	t.descriptionUID = "Failure writing config files";
	CONFIG_WRITE_FAIL = m_prompter->RegisterDialog(t);

	t.descriptionUID = "Failure reading honeyd files";
	HONEYD_READ_FAIL = m_prompter->RegisterDialog(t);

	t.descriptionUID = "Failure loading honeyd config files";
	HONEYD_LOAD_FAIL = m_prompter->RegisterDialog(t);

	t.descriptionUID = "Unexpected file entries";
	UNEXPECTED_ENTRY = m_prompter->RegisterDialog(t);

	t.descriptionUID = "Honeyd subnets out of range";
	HONEYD_INVALID_SUBNET = m_prompter->RegisterDialog(t);

	t.descriptionUID = "Cannot delete the selected port";
	CANNOT_DELETE_PORT = m_prompter->RegisterDialog(t);

	// Action required notification prompts
	t.type = notifyActionPrompt;

	t.descriptionUID = "Request to merge CE capture into training Db";
	LAUNCH_TRAINING_MERGE = m_prompter->RegisterDialog(t);

	t.descriptionUID = "Cannot inherit the selected port";
	CANNOT_INHERIT_PORT = m_prompter->RegisterDialog(t);

	t.descriptionUID = "Cannot delete the selected item";
	CANNOT_DELETE_ITEM = m_prompter->RegisterDialog(t);

	// Preventable warnings
	t.type = warningPreventablePrompt;

	t.descriptionUID = "No Doppelganger could be found";
	NO_DOPP = m_prompter->RegisterDialog(t);

	// Misc other prompts
	t.descriptionUID = "Problem inheriting port";
	t.type = notifyPrompt;
	NO_ANCESTORS = m_prompter->RegisterDialog(t);

	t.descriptionUID = "Loading a Haystack Node Failed";
	t.type = warningPrompt;
	NODE_LOAD_FAIL = m_prompter->RegisterDialog(t);


	//This register meta type function needs to be called for any object types passed through a signal
	qRegisterMetaType<in_addr_t>("in_addr_t");
	qRegisterMetaType<QItemSelection>("QItemSelection");

	//Sets initial view
	this->ui.stackedWidget->setCurrentIndex(0);
	this->ui.mainButton->setFlat(true);
	this->ui.suspectButton->setFlat(false);
	this->ui.doppelButton->setFlat(false);
	this->ui.haystackButton->setFlat(false);
	connect(this, SIGNAL(newSuspect(in_addr_t)), this, SLOT(DrawSuspect(in_addr_t)), Qt::AutoConnection);
	connect(this, SIGNAL(refreshSystemStatus()), this, SLOT(UpdateSystemStatus()), Qt::AutoConnection);

	pthread_t StatusUpdateThread;

	pthread_create(&StatusUpdateThread, NULL, StatusUpdate, this);
}

NovaGUI::~NovaGUI()
{
	if(!CloseNovadConnection())
	{
		LOG(ERROR, "Did not close down connection to Novad cleanly", "CloseNovadConnection() failed");
	}
}

//Draws the suspect context menu
void NovaGUI::contextMenuEvent(QContextMenuEvent * event)
{
	if(ui.suspectList->hasFocus() || ui.suspectList->underMouse())
	{
		m_suspectMenu->clear();
		if(ui.suspectList->isItemSelected(ui.suspectList->currentItem()))
		{
			m_suspectMenu->addAction(ui.actionClear_Suspect);
			m_suspectMenu->addAction(ui.actionHide_Suspect);
		}

		m_suspectMenu->addSeparator();
		m_suspectMenu->addAction(ui.actionClear_All_Suspects);
		m_suspectMenu->addAction(ui.actionSave_Suspects);
		m_suspectMenu->addSeparator();

		m_suspectMenu->addAction(ui.actionShow_All_Suspects);
		m_suspectMenu->addAction(ui.actionHide_Old_Suspects);

		QPoint globalPos = event->globalPos();
		m_suspectMenu->popup(globalPos);
	}
	else if(ui.hostileList->hasFocus() || ui.hostileList->underMouse())
	{
		m_suspectMenu->clear();
		if(ui.hostileList->isItemSelected(ui.hostileList->currentItem()))
		{
			m_suspectMenu->addAction(ui.actionClear_Suspect);
			m_suspectMenu->addAction(ui.actionHide_Suspect);
		}

		m_suspectMenu->addSeparator();
		m_suspectMenu->addAction(ui.actionClear_All_Suspects);
		m_suspectMenu->addAction(ui.actionSave_Suspects);
		m_suspectMenu->addSeparator();

		m_suspectMenu->addAction(ui.actionShow_All_Suspects);
		m_suspectMenu->addAction(ui.actionHide_Old_Suspects);

		QPoint globalPos = event->globalPos();
		m_suspectMenu->popup(globalPos);
	}
	else if(ui.systemStatusTable->hasFocus() || ui.systemStatusTable->underMouse())
	{
		m_systemStatMenu->clear();

		int row = ui.systemStatusTable->currentRow();

		switch (row)
		{
			case COMPONENT_NOVAD:
			{
				if(IsNovadUp(false))
				{
					m_systemStatMenu->addAction(ui.actionSystemStatReload);
					m_systemStatMenu->addAction(ui.actionSystemStatStop);
				}
				else
				{
					m_systemStatMenu->addAction(ui.actionSystemStatStart);
				}
				break;
			}
			case COMPONENT_HSH:
			{
				if(IsHaystackUp())
				{
					m_systemStatMenu->addAction(ui.actionSystemStatStop);
				}
				else
				{
					m_systemStatMenu->addAction(ui.actionSystemStatStart);
				}
				break;
			}
			default:
			{
				LOG(ERROR, "Invalid System Status Selection Row, ignoring.","");
				return;
			}
		}

		QPoint globalPos = event->globalPos();
		m_systemStatMenu->popup(globalPos);
	}
	else
	{
		return;
	}
}

void NovaGUI::closeEvent()
{
	if(!CloseNovadConnection())
	{
		LOG(ERROR, "Did not close down connection to Novad cleanly", "CloseNovadConnection() failed");
	}
}

/************************************************
 * Gets preliminary information
 ************************************************/

void NovaGUI::InitConfiguration()
{
	for(uint i = 0; i < DIM; i++)
	{
		if('1' == Config::Inst()->GetEnabledFeatures().at(i))
		{
			m_featureEnabled[i] = true;
		}
		else
		{
			m_featureEnabled[i] = false;
		}
	}
}

void NovaGUI::InitPaths()
{
	homePath = Config::Inst()->GetPathHome();
	readPath = Config::Inst()->GetPathReadFolder();
	writePath = Config::Inst()->GetPathWriteFolder();

	if((homePath == "") || (readPath == "") || (writePath == ""))
	{
		::exit(EXIT_FAILURE);
	}

	QDir::setCurrent((QString)homePath.c_str());
}

void NovaGUI::SystemStatusRefresh()
{
	Q_EMIT refreshSystemStatus();
}

void NovaGUI::InitiateSystemStatus()
{
	// Pull in the icons now that homePath is set
	string greenPath = "/usr/share/nova/icons/greendot.png";
	string redPath = "/usr/share/nova/icons/reddot.png";

	m_greenIcon = new QIcon(QPixmap(QString::fromStdString(greenPath)));
	m_redIcon = new QIcon(QPixmap(QString::fromStdString(redPath)));

	// Populate the System Status table with empty widgets
	for(int i = 0; i < ui.systemStatusTable->rowCount(); i++)
	{
		for(int j = 0; j < ui.systemStatusTable->columnCount(); j++)
		{
			ui.systemStatusTable->setItem(i, j,  new QTableWidgetItem());
		}
	}
	// Add labels for our components
	ui.systemStatusTable->item(COMPONENT_NOVAD, 0)->setText("Novad");
	ui.systemStatusTable->item(COMPONENT_HSH, 0)->setText("Haystack");
}

bool NovaGUI::ConnectGuiToNovad()
{
	if(TryWaitConnectToNovad(2000))	//TODO: Call this asynchronously
	{
		if(!StartCallbackLoop(this))
		{
			LOG(ERROR, "Couldn't listen for Novad. Is NovaGUI already running?",
					"InitCallbackSocket() failed: "+string(strerror(errno))+".");
			return false;
		}

		// Get the list of current suspects
		vector<in_addr_t> *suspectIpList = GetSuspectList(SUSPECTLIST_ALL);

		// Failed to get an IP list for same reason
		if (suspectIpList == NULL)
		{
			return false;
		}

		for (uint i = 0; i < suspectIpList->size(); i++)
		{
			struct suspectItem suspectItem;
			suspectItem.suspect = GetSuspect(suspectIpList->at(i));

			// Case of the empty suspect
			if(suspectItem.suspect == NULL)
			{
				continue;
			}
			else if(suspectItem.suspect->GetIpAddress() == 0)
			{
				delete suspectItem.suspect;
				continue;
			}
			suspectItem.item = NULL;
			suspectItem.mainItem = NULL;
			this->ProcessReceivedSuspect(suspectItem, false);
		}

		return true;
	}
	else
	{
		return false;
	}
}

void NovaGUI::UpdateSystemStatus()
{
	QTableWidgetItem *item;

	//Novad
	item = ui.systemStatusTable->item(COMPONENT_NOVAD, 0);
	if(IsNovadUp(false))
	{
		item->setIcon(*m_greenIcon);

		int uptime = GetUptime();
		int uptimeSeconds =  uptime % 60;
		int uptimeMinutes =  uptime / 60 % 60;
		int uptimeHours =    uptime / 60 / 60;

		stringstream ss;
		ss << "Novad Uptime: " << uptimeHours << " hours : " << uptimeMinutes << " minutes : " << uptimeSeconds << " seconds";

		ui.uptimeLabel->setText(QString::fromStdString(ss.str()));
	}
	else
	{
		item->setIcon(*m_redIcon);
		ui.uptimeLabel->setText(QString());
	}

	//Haystack
	item = ui.systemStatusTable->item(COMPONENT_HSH, 0);
	if(IsHaystackUp())
	{
		item->setIcon(*m_greenIcon);
	}
	else
	{
		item->setIcon(*m_redIcon);
	}

	// Update the buttons if need be
	int row = ui.systemStatusTable->currentRow();
	if(row < 0 || row > NOVA_COMPONENTS)
	{
		return;
	}
	else
	{
		on_systemStatusTable_itemSelectionChanged();
	}
}

/************************************************
 * Suspect Functions
 ************************************************/

void NovaGUI::ProcessReceivedSuspect(suspectItem suspectItem, bool initialization)
{

	pthread_rwlock_wrlock(&lock);
	//If the suspect already exists in our table
	if(SuspectTable.keyExists(suspectItem.suspect->GetIpAddress()))
	{
		if (!initialization)
		{
			return;
		}

		//We point to the same item so it doesn't need to be deleted.
		suspectItem.item = SuspectTable[suspectItem.suspect->GetIpAddress()].item;
		suspectItem.mainItem = SuspectTable[suspectItem.suspect->GetIpAddress()].mainItem;

		//Delete the old Suspect since we created and pointed to a new one
		delete SuspectTable[suspectItem.suspect->GetIpAddress()].suspect;
	}
	//We borrow the flag to show there is new information.
	suspectItem.suspect->SetNeedsClassificationUpdate(true);
	//Update the entry in the table

	SuspectTable[suspectItem.suspect->GetIpAddress()] = suspectItem;
	in_addr_t address = suspectItem.suspect->GetIpAddress();

	pthread_rwlock_unlock(&lock);


	Q_EMIT newSuspect(address);
}

/************************************************
 * Display Functions
 ************************************************/

void NovaGUI::DrawAllSuspects()
{
	m_editingSuspectList = true;
	ClearSuspectList();

	QListWidgetItem * item = NULL;
	QListWidgetItem * mainItem = NULL;
	Suspect * suspect = NULL;
	QString str;
	QBrush brush;
	QColor color;

	pthread_rwlock_wrlock(&lock);
	for(SuspectGUIHashTable::iterator it = SuspectTable.begin() ; it != SuspectTable.end(); it++)
	{
		str = (QString) string(inet_ntoa(it->second.suspect->GetInAddr())).c_str();
		suspect = it->second.suspect;
		//Create the colors for the draw

		if(suspect->GetClassification() < 0)
		{
			// In training mode, classification is never set and ends up at -1
			// Make it a nice blue so it's clear that it hasn't classified
			color = QColor(0,0,255);
		}
		else if(suspect->GetClassification() < 0.5)
		{
			//at 0.5 QBrush is 255,255 (yellow), from 0->0.5 include more red until yellow
			color = QColor((int)(200*2*suspect->GetClassification()),200, 50);
		}
		else
		{
			//at 0.5 QBrush is 255,255 (yellow), at from 0.5->1.0 remove more green until QBrush is Red
			color = QColor(200,200-(int)(200*2*(suspect->GetClassification()-0.5)), 50);
		}
		brush.setColor(color);
		brush.setStyle(Qt::NoBrush);

		//Create the Suspect in list with info set alignment and color
		item = new QListWidgetItem(str,0);
		item->setTextAlignment(Qt::AlignLeft|Qt::AlignBottom);
		item->setForeground(brush);

		in_addr_t addr;
		int i = 0;
		if(ui.suspectList->count())
		{
			for(i = 0; i < ui.suspectList->count(); i++)
			{
				addr = inet_addr(ui.suspectList->item(i)->text().toStdString().c_str());
				if(SuspectTable[addr].suspect->GetClassification() < suspect->GetClassification())
				{
					break;
				}
			}
		}
		ui.suspectList->insertItem(i, item);

		//If Hostile
		if(suspect->GetIsHostile())
		{
			//Copy the item and add it to the list
			mainItem = new QListWidgetItem(str,0);
			mainItem->setTextAlignment(Qt::AlignLeft|Qt::AlignBottom);
			mainItem->setForeground(brush);

			i = 0;
			if(ui.hostileList->count())
			{
				for(i = 0; i < ui.hostileList->count(); i++)
				{
					addr = inet_addr(ui.hostileList->item(i)->text().toStdString().c_str());
					if(SuspectTable[addr].suspect->GetClassification() < suspect->GetClassification())
					{
						break;
					}
				}
			}
			ui.hostileList->insertItem(i, mainItem);
			it->second.mainItem = mainItem;
		}
		//Point to the new items
		it->second.item = item;
		//Reset the flags
		suspect->SetNeedsClassificationUpdate(false);
		it->second.suspect = suspect;
	}
	UpdateSuspectWidgets();
	pthread_rwlock_unlock(&lock);
	m_editingSuspectList = false;
}

//Updates the UI with the latest suspect information
//*NOTE This slot is not thread safe, make sure you set appropriate locks before sending a signal to this slot
void NovaGUI::DrawSuspect(in_addr_t suspectAddr)
{
	m_editingSuspectList = true;
	QString str;
	QBrush brush;
	QColor color;
	in_addr_t addr;

	pthread_rwlock_wrlock(&lock);
	suspectItem * sItem = &SuspectTable[suspectAddr];
	//Extract Information
	str = (QString) string(inet_ntoa(sItem->suspect->GetInAddr())).c_str();

	//Create the colors for the draw
	if(sItem->suspect->GetClassification() < 0)
	{
		// In training mode, classification is never set and ends up at -1
		// Make it a nice blue so it's clear that it hasn't classified
		color = QColor(0,0,255);
	}
	else if(sItem->suspect->GetClassification() < 0.5)
	{
		//at 0.5 QBrush is 255,255 (yellow), from 0->0.5 include more red until yellow
		color = QColor((int)(200*2*sItem->suspect->GetClassification()),200, 50);
	}
	else
	{
		//at 0.5 QBrush is 255,255 (yellow), at from 0.5->1.0 remove more green until QBrush is Red
		color = QColor(200,200-(int)(200*2*(sItem->suspect->GetClassification()-0.5)), 50);
	}
	brush.setColor(color);
	brush.setStyle(Qt::NoBrush);

	//If the item exists
	if(sItem->item != NULL)
	{
		sItem->item->setText(str);
		sItem->item->setForeground(brush);
		bool selected = false;
		int current_row = ui.suspectList->currentRow();

		//If this is our current selection flag it so we can update the selection if we change the index
		if(current_row == ui.suspectList->row(sItem->item))
		{
			selected = true;
		}

		ui.suspectList->removeItemWidget(sItem->item);

		int i = 0;
		if(ui.suspectList->count())
		{
			for(i = 0; i < ui.suspectList->count(); i++)
			{
				addr = inet_addr(ui.suspectList->item(i)->text().toStdString().c_str());
				if(SuspectTable[addr].suspect->GetClassification() < sItem->suspect->GetClassification())
				{
					break;
				}
			}
		}
		ui.suspectList->insertItem(i, sItem->item);

		//If we need to update the selection
		if(selected)
		{
			ui.suspectList->setCurrentRow(i);
		}
	}
	//If the item doesn't exist
	else
	{
		//Create the Suspect in list with info set alignment and color
		sItem->item = new QListWidgetItem(str,0);
		sItem->item->setTextAlignment(Qt::AlignLeft|Qt::AlignBottom);
		sItem->item->setForeground(brush);

		int i = 0;
		if(ui.suspectList->count())
		{
			for(i = 0; i < ui.suspectList->count(); i++)
			{
				addr = inet_addr(ui.suspectList->item(i)->text().toStdString().c_str());
				if(SuspectTable[addr].suspect->GetClassification() < sItem->suspect->GetClassification())
				{
					break;
				}
			}
		}
		ui.suspectList->insertItem(i, sItem->item);
	}

	//If the mainItem exists and suspect is hostile
	if((sItem->mainItem != NULL) && sItem->suspect->GetIsHostile())
	{
		sItem->mainItem->setText(str);
		sItem->mainItem->setForeground(brush);
		bool selected = false;
		int current_row = ui.hostileList->currentRow();

		//If this is our current selection flag it so we can update the selection if we change the index
		if(current_row == ui.hostileList->row(sItem->mainItem))
		{
			selected = true;
		}

		ui.hostileList->removeItemWidget(sItem->mainItem);
		int i = 0;
		if(ui.hostileList->count())
		{
			for(i = 0; i < ui.hostileList->count(); i++)
			{
				addr = inet_addr(ui.hostileList->item(i)->text().toStdString().c_str());
				if(SuspectTable[addr].suspect->GetClassification() < sItem->suspect->GetClassification())
				{
					break;
				}
			}
		}
		ui.hostileList->insertItem(i, sItem->mainItem);

		//If we need to update the selection
		if(selected)
		{
			ui.hostileList->setCurrentRow(i);
		}
		sItem->mainItem->setToolTip(QString(sItem->suspect->ToString().c_str()));
	}
	//Else if the mainItem exists and suspect is not hostile
	else if(sItem->mainItem != NULL)
	{
		ui.hostileList->removeItemWidget(sItem->mainItem);
	}
	//If the mainItem doesn't exist and suspect is hostile
	else if(sItem->suspect->GetIsHostile())
	{
		//Create the Suspect in list with info set alignment and color
		sItem->mainItem = new QListWidgetItem(str,0);
		sItem->mainItem->setTextAlignment(Qt::AlignLeft|Qt::AlignBottom);
		sItem->mainItem->setForeground(brush);

		sItem->mainItem->setToolTip(QString(sItem->suspect->ToString().c_str()));

		int i = 0;
		if(ui.hostileList->count())
		{
			for(i = 0; i < ui.hostileList->count(); i++)
			{
				addr = inet_addr(ui.hostileList->item(i)->text().toStdString().c_str());
				if(SuspectTable[addr].suspect->GetClassification() < sItem->suspect->GetClassification())
				{
					break;
				}
			}
		}
		ui.hostileList->insertItem(i, sItem->mainItem);
	}
	sItem->item->setToolTip(QString(sItem->suspect->ToString().c_str()));
	UpdateSuspectWidgets();
	pthread_rwlock_unlock(&lock);
	m_editingSuspectList = false;
}

void NovaGUI::UpdateSuspectWidgets()
{
	double hostileAcc = 0, benignAcc = 0, totalAcc = 0;

	for(SuspectGUIHashTable::iterator it = SuspectTable.begin() ; it != SuspectTable.end(); it++)
	{
		if(it->second.suspect->GetIsHostile())
		{
			hostileAcc += it->second.suspect->GetClassification();
			totalAcc += it->second.suspect->GetClassification();
		}
		else
		{
			benignAcc += 1-it->second.suspect->GetClassification();
			totalAcc += 1-it->second.suspect->GetClassification();
		}
	}

	int numBenign = ui.suspectList->count() - ui.hostileList->count();
	int numHostile = ui.hostileList->count();

	stringstream ss;
	ss << numBenign;
	ui.numBenignEdit->setText(QString::fromStdString(ss.str()));

	stringstream ssHostile;
	ssHostile << numHostile;
	ui.numHostileEdit->setText(QString::fromStdString(ssHostile.str()));

	if(numBenign)
	{
		benignAcc /= numBenign;
		ui.benignClassificationBar->setValue((int)(benignAcc*100));
		ui.benignSuspectClassificationBar->setValue((int)(benignAcc*100));
	}
	else
	{
		ui.benignClassificationBar->setValue(100);
		ui.benignSuspectClassificationBar->setValue(100);
	}


	if(ui.hostileList->count())
	{
		hostileAcc /= ui.hostileList->count();
		ui.hostileClassificationBar->setValue((int)(hostileAcc*100));
		ui.hostileSuspectClassificationBar->setValue((int)(hostileAcc*100));
	}
	else
	{
		ui.hostileClassificationBar->setValue(100);
		ui.hostileSuspectClassificationBar->setValue(100);
	}
	if(ui.suspectList->count())
	{
		totalAcc /= ui.suspectList->count();
		ui.overallSuspectClassificationBar->setValue((int)(totalAcc*100));
	}
	else
	{
		ui.overallSuspectClassificationBar->setValue(100);
	}
}

//Clears the suspect tables completely.
void NovaGUI::ClearSuspectList()
{
	pthread_rwlock_wrlock(&lock);
	this->ui.suspectList->clear();
	this->ui.hostileList->clear();
	//Since clearing permanently deletes the items we need to make sure the suspects point to null
	for(SuspectGUIHashTable::iterator it = SuspectTable.begin() ; it != SuspectTable.end(); it++)
	{
		it->second.item = NULL;
		it->second.mainItem = NULL;
	}
	pthread_rwlock_unlock(&lock);
}

/************************************************
 * Menu Signal Handlers
 ************************************************/

void NovaGUI::on_actionRunNova_triggered()
{
	if(IsNovadUp(false))
	{
		return;
	}
	StartNovad();
	ConnectGuiToNovad();
	StartHaystack();
}

void NovaGUI::on_actionRunNovaAs_triggered()
{
	Run_Popup *w = new Run_Popup(this);
	w->show();
}

void NovaGUI::on_actionStopNova_triggered()
{
	StopNovad();
	StopHaystack();

	// Were we in training mode?
	if(Config::Inst()->GetIsTraining())
	{
		m_prompter->DisplayPrompt(LAUNCH_TRAINING_MERGE,
			"ClassificationEngine was in training mode. Would you like to merge the capture file into the training database now?",
			ui.actionTrainingData, NULL);
	}
}

void NovaGUI::on_actionConfigure_triggered()
{
	NovaConfig *w = new NovaConfig(this, homePath);
	w->setWindowModality(Qt::WindowModal);
	w->show();
}

void  NovaGUI::on_actionExit_triggered()
{
	if(!CloseNovadConnection())
	{
		LOG(ERROR, "Did not close down connection to Novad cleanly", "CloseNovadConnection() failed");
	}
	QCoreApplication::exit(EXIT_SUCCESS);
}

void NovaGUI::on_actionClear_All_Suspects_triggered()
{
	m_editingSuspectList = true;
	ClearAllSuspects();

	pthread_rwlock_wrlock(&lock);
	SuspectTable.clear();
	pthread_rwlock_unlock(&lock);

	QFile::remove(QString::fromStdString(Config::Inst()->GetPathCESaveFile()));
	DrawAllSuspects();
	m_editingSuspectList = false;
}

void NovaGUI::on_actionClear_Suspect_triggered()
{
	QListWidget * list = NULL;
	if(ui.suspectList->hasFocus())
	{
		list = ui.suspectList;
	}
	else if(ui.hostileList->hasFocus())
	{
		list = ui.hostileList;
	}
	if(list->currentItem() != NULL && list->isItemSelected(list->currentItem()))
	{
		string suspectStr = list->currentItem()->text().toStdString();
		in_addr_t addr = inet_addr(suspectStr.c_str());
		HideSuspect(addr);
		pthread_rwlock_wrlock(&lock);
		SuspectTable.erase(addr);
		pthread_rwlock_unlock(&lock);
		ClearSuspect(addr);
	}
}

void NovaGUI::on_actionHide_Suspect_triggered()
{
	QListWidget * list = NULL;
	if(ui.suspectList->hasFocus())
	{
		list = ui.suspectList;
	}
	else if(ui.hostileList->hasFocus())
	{
		list = ui.hostileList;
	}
	if(list->currentItem() != NULL && list->isItemSelected(list->currentItem()))
	{
		in_addr_t addr = inet_addr(list->currentItem()->text().toStdString().c_str());
		HideSuspect(addr);
	}
}

void NovaGUI::on_actionSave_Suspects_triggered()
{
	QString filename = QFileDialog::getSaveFileName(this,
		tr("Save Suspect List"), QDir::currentPath(),
		tr("Documents (*.txt)"));

	if(filename.isNull())
	{
		return;
	}

	//TODO: Set the filename?
	SaveAllSuspects(filename.toStdString());
}

void NovaGUI::on_actionMakeDataFile_triggered()
{
	 QString data = QFileDialog::getOpenFileName(this,
			 tr("File to select classifications from"), QString::fromStdString(Config::Inst()->GetPathTrainingFile()), tr("NOVA Classification Database (*.db)"));

	if(data.isNull())
	{
		return;
	}

	trainingSuspectMap* map = TrainingData::ParseTrainingDb(data.toStdString());
	if(map == NULL)
	{
		m_prompter->DisplayPrompt(CONFIG_READ_FAIL, "Error parsing file "+ data.toStdString());
		return;
	}

	classifierPrompt* classifier = new classifierPrompt(map);

	if(classifier->exec() == QDialog::Rejected)
	{
		return;
	}

	string dataFileContent = TrainingData::MakaDataFile(*map);

	ofstream out (Config::Inst()->GetPathTrainingFile().data());
	out << dataFileContent;
	out.close();
}

void NovaGUI::on_actionTrainingData_triggered()
{
	 QString data = QFileDialog::getOpenFileName(this,
			 tr("Classification Engine Data Dump"), QString::fromStdString(Config::Inst()->GetPathTrainingFile()), tr("NOVA Classification Dump (*.dump)"));

	if(data.isNull())
	{
		return;
	}

	trainingDumpMap* trainingDump = TrainingData::ParseEngineCaptureFile(data.toStdString());

	if(trainingDump == NULL)
	{
		m_prompter->DisplayPrompt(CONFIG_READ_FAIL, "Error parsing file "+ data.toStdString());
		return;
	}

	TrainingData::ThinTrainingPoints(trainingDump, Config::Inst()->GetThinningDistance());

	classifierPrompt* classifier = new classifierPrompt(trainingDump);

	if(classifier->exec() == QDialog::Rejected)
	{
		return;
	}

	trainingSuspectMap* headerMap = classifier->getStateData();

	QString outputFile = QFileDialog::getSaveFileName(this,
			tr("Classification Database File"), QString::fromStdString(Config::Inst()->GetPathTrainingFile()), tr("NOVA Classification Database (*.db)"));

	if(outputFile.isNull())
	{
		return;
	}

	if(!TrainingData::CaptureToTrainingDb(outputFile.toStdString(), headerMap))
	{
		m_prompter->DisplayPrompt(CONFIG_READ_FAIL, "Error parsing the input files. Please see the logs for more details.");
	}
}

void  NovaGUI::on_actionHide_Old_Suspects_triggered()
{
	m_editingSuspectList = true;
	ClearSuspectList();
	m_editingSuspectList = false;
}

void  NovaGUI::on_actionShow_All_Suspects_triggered()
{
	m_editingSuspectList = true;
	DrawAllSuspects();
	m_editingSuspectList = false;
}

void NovaGUI::on_actionHelp_2_triggered()
{
	if(!m_isHelpUp)
	{
		Nova_Manual *wi = new Nova_Manual(this);
		wi->show();
		m_isHelpUp = true;
	}
}

void NovaGUI::on_actionLogger_triggered()
{
		LoggerWindow *wi = new LoggerWindow(this);
		wi->show();
}

/************************************************
 * View Signal Handlers
 ************************************************/

void NovaGUI::on_mainButton_clicked()
{
	this->ui.stackedWidget->setCurrentIndex(0);
	this->ui.mainButton->setFlat(true);
	this->ui.suspectButton->setFlat(false);
	this->ui.doppelButton->setFlat(false);
	this->ui.haystackButton->setFlat(false);
}

void NovaGUI::on_suspectButton_clicked()
{
	this->ui.stackedWidget->setCurrentIndex(1);
	this->ui.mainButton->setFlat(false);
	this->ui.suspectButton->setFlat(true);
	this->ui.doppelButton->setFlat(false);
	this->ui.haystackButton->setFlat(false);
}

void NovaGUI::on_doppelButton_clicked()
{
	this->ui.stackedWidget->setCurrentIndex(2);
	this->ui.mainButton->setFlat(false);
	this->ui.suspectButton->setFlat(false);
	this->ui.doppelButton->setFlat(true);
	this->ui.haystackButton->setFlat(false);
}

void NovaGUI::on_haystackButton_clicked()
{
	this->ui.stackedWidget->setCurrentIndex(3);
	this->ui.mainButton->setFlat(false);
	this->ui.suspectButton->setFlat(false);
	this->ui.doppelButton->setFlat(false);
	this->ui.haystackButton->setFlat(true);
}

/************************************************
 * Button Signal Handlers
 ************************************************/

void NovaGUI::on_runButton_clicked()
{
	if(IsNovadUp(false))
	{
		return;
	}
	StartNovad();
	ConnectGuiToNovad();
	StartHaystack();
}
void NovaGUI::on_stopButton_clicked()
{
	Q_EMIT on_actionStopNova_triggered();
}

void NovaGUI::on_systemStatusTable_itemSelectionChanged()
{
	int row = ui.systemStatusTable->currentRow();

	switch(row)
	{
		case COMPONENT_NOVAD:
		{
			if(IsNovadUp(false))
			{
				ui.systemStatStartButton->setDisabled(true);
				ui.systemStatStopButton->setDisabled(false);
			}
			else
			{
				ui.systemStatStartButton->setDisabled(false);
				ui.systemStatStopButton->setDisabled(true);
			}
			break;
		}
		case COMPONENT_HSH:
		{
			if(IsHaystackUp())
			{
				ui.systemStatStartButton->setDisabled(true);
				ui.systemStatStopButton->setDisabled(false);
			}
			else
			{
				ui.systemStatStartButton->setDisabled(false);
				ui.systemStatStopButton->setDisabled(true);
			}
			break;
		}
		default:
		{
			LOG(ERROR, "Invalid System Status Selection Row, ignoring.","");
			return;
		}
	}
}

void NovaGUI::on_actionSystemStatStop_triggered()
{
	int row = ui.systemStatusTable->currentRow();

	switch (row)
	{
		case COMPONENT_NOVAD:
		{
			StopNovad();
			break;
		}
		case COMPONENT_HSH:
		{
			StopHaystack();
			break;
		}
		default:
		{
			LOG(ERROR, "Invalid System Status Selection Row, ignoring","");
			break;
		}
	}
}

void NovaGUI::on_actionSystemStatStart_triggered()
{
	int row = ui.systemStatusTable->currentRow();

	switch (row)
	{
		case COMPONENT_NOVAD:
		{
			if(IsNovadUp(false))
			{
				return;
			}
			StartNovad();
			ConnectGuiToNovad();
			break;
		}
		case COMPONENT_HSH:
		{
			StartHaystack();
			break;
		}
		default:
		{
			return;
		}
	}

	UpdateSystemStatus();
}

void NovaGUI::on_actionSystemStatReload_triggered()
{
	ReclassifyAllSuspects();
}

void NovaGUI::on_systemStatStartButton_clicked()
{
	on_actionSystemStatStart_triggered();
}

void NovaGUI::on_systemStatStopButton_clicked()
{
	on_actionSystemStatStop_triggered();
}

void NovaGUI::on_clearSuspectsButton_clicked()
{
	on_actionClear_All_Suspects_triggered();
}

/************************************************
 * List Signal Handlers
 ************************************************/
void NovaGUI::on_suspectList_currentItemChanged(QListWidgetItem * current, QListWidgetItem * previous)
{
	if(!m_editingSuspectList)
	{
		pthread_rwlock_wrlock(&lock);
		if((ui.suspectList->currentItem() == current) && (current != NULL) && (current != previous))
		{
			in_addr_t addr = inet_addr(current->text().toStdString().c_str());
			ui.suspectFeaturesEdit->setText(QString(SuspectTable[addr].suspect->ToString().c_str()));
			SetFeatureDistances(SuspectTable[addr].suspect);
		}
		pthread_rwlock_unlock(&lock);
	}
}

void NovaGUI::SetFeatureDistances(Suspect* suspect)
{
	int row = 0;
	ui.suspectDistances->clear();
	for(int i = 0; i < DIM; i++)
	{
		if(m_featureEnabled[i])
		{
			QString featureLabel;
			switch (i)
			{
				case IP_TRAFFIC_DISTRIBUTION:
				{
					featureLabel = tr("IP Traffic Distribution");
					break;
				}
				case PORT_TRAFFIC_DISTRIBUTION:
				{
					featureLabel = tr("Port Traffic Distribution");
					break;
				}
				case HAYSTACK_EVENT_FREQUENCY:
				{
					featureLabel = tr("Haystack Event Frequency");
					break;
				}
				case PACKET_SIZE_MEAN:
				{
					featureLabel = tr("Packet Size Mean");
					break;
				}
				case PACKET_SIZE_DEVIATION:
				{
					featureLabel = tr("Packet Size Deviation");
					break;
				}
				case DISTINCT_IPS:
				{
					featureLabel = tr("IPs Contacted");
					break;
				}
				case DISTINCT_PORTS:
				{
					featureLabel = tr("Ports Contacted");
					break;
				}
				case PACKET_INTERVAL_MEAN:
				{
					featureLabel = tr("Packet Interval Mean");
					break;
				}
				case PACKET_INTERVAL_DEVIATION:
				{
					featureLabel = tr("Packet Interval Deviation");
					break;
				}
				default:
				{
					break;
				}
			}

			ui.suspectDistances->insertItem(row, featureLabel + tr("Accuracy"));
			QString formatString = "%p%| ";
			formatString.append(featureLabel);
			formatString.append(": ");

			row++;
			QProgressBar* bar = new QProgressBar();
			bar->setMinimum(0);
			bar->setMaximum(100);
			bar->setTextVisible(true);

			double d = suspect->GetFeatureAccuracy((featureIndex)i);
			stringstream ss;
			ss.str("");
			ss << "GUI got invalid feature accuracy of: " << d << " (range is 0-1).";

			if(suspect->GetFeatureAccuracy((featureIndex)i) < 0)
			{
				LOG(ERROR, ss.str(), "");
				suspect->SetFeatureAccuracy((featureIndex)i, 0);
			}
			else if(suspect->GetFeatureAccuracy((featureIndex)i) > 1)
			{
				LOG(ERROR, ss.str(), "");
				suspect->SetFeatureAccuracy((featureIndex)i, 1);
			}

			bar->setValue((int)((1 - suspect->GetFeatureAccuracy((featureIndex)i)/1.0)*100));
			bar->setStyleSheet("QProgressBar:horizontal {border: 1px solid gray;background: white;padding: 1px;} \
				 QProgressBar::chunk:horizontal {margin: 0.5px; background: qlineargradient(x1: 0, y1: 0.5, x2: 1,"
				" y2: 0.5, stop: 0 yellow, stop: 1 green);}");

			formatString.append(QString::number(suspect->GetFeatureSet(MAIN_FEATURES).m_features[i]));
			bar->setFormat(formatString);

			QListWidgetItem* item = new QListWidgetItem();
			ui.suspectDistances->insertItem(row, item);
			ui.suspectDistances->setItemWidget(item, bar);

			row++;
		}
	}
}

/************************************************
 * IPC Functions
 ************************************************/
void NovaGUI::HideSuspect(in_addr_t addr)
{
	pthread_rwlock_wrlock(&lock);
	m_editingSuspectList = true;

	if(!SuspectTable.keyExists(addr))
	{
		pthread_rwlock_unlock(&lock);
		m_editingSuspectList = false;
		return;
	}

	suspectItem *sItem = &SuspectTable[addr];
	if(sItem == NULL)
	{
		pthread_rwlock_unlock(&lock);
		m_editingSuspectList = false;
		return;
	}

	ui.suspectList->removeItemWidget(sItem->item);
	delete sItem->item;
	sItem->item = NULL;
	if(sItem->mainItem != NULL)
	{
		ui.hostileList->removeItemWidget(sItem->mainItem);
		delete sItem->mainItem;
		sItem->mainItem = NULL;
	}
	pthread_rwlock_unlock(&lock);
	m_editingSuspectList = false;
}

/*********************************************************
 ----------------- General Functions ---------------------
 *********************************************************/

namespace Nova
{

/************************************************
 * Thread Loops
 ************************************************/

void *StatusUpdate(void *ptr)
{
	while(true)
	{
		((NovaGUI*)ptr)->SystemStatusRefresh();

		sleep(2);
	}
	return NULL;
}

bool StartCallbackLoop(void *ptr)
{
	pthread_t callbackThread;
	pthread_create(&callbackThread, NULL, CallbackLoop, ptr);
	return true;
}

void *CallbackLoop(void *ptr)
{
	struct CallbackChange change;
	while(true)
	{
		change = ProcessCallbackMessage();
		switch(change.m_type)
		{
			case CALLBACK_ERROR:
			{
				//TODO: Die after X consecutive errors?
				LOG(ERROR, "Failed to connect to Novad", "Got a callback_error message");
				break;
			}
			case CALLBACK_HUNG_UP:
			{
				LOG(ERROR, "Novad hung up", "Got a callback_error message: CALLBACK_HUNG_UP");
				return NULL;
			}
			case CALLBACK_NEW_SUSPECT:
			{
				struct suspectItem suspectItem;
				suspectItem.suspect = change.m_suspect;
				suspectItem.item = NULL;
				suspectItem.mainItem = NULL;
				((NovaGUI*)ptr)->ProcessReceivedSuspect(suspectItem, true);
				break;
			}
			case CALLBACK_ALL_SUSPECTS_CLEARED:
			{
				((NovaGUI*)ptr)->ClearSuspectList();
				break;
			}
			case CALLBACK_SUSPECT_CLEARED:
			{
				((NovaGUI*)ptr)->HideSuspect(change.m_suspectIP);
				pthread_rwlock_wrlock(&lock);
				SuspectTable.erase(change.m_suspectIP);
				pthread_rwlock_unlock(&lock);
				break;
			}
			default:
			{
				break;
			}
		}
	}
	return NULL;
}

};
