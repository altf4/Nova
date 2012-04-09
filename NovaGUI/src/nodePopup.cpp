//============================================================================
// Name        : nodePopup.cpp
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
// Description : Popup for creating and editing nodes
//============================================================================
#include "nodePopup.h"
#include "novaconfig.h"
#include "novagui.h"

using namespace std;
using namespace Nova;

//Parent window pointers, allows us to call functions from the parent
NovaConfig * novaParent;

/************************************************
 * Construct and Initialize GUI
 ************************************************/

node editNode;
nodePopup::nodePopup(QWidget * parent, node * n)
    : QMainWindow(parent)
{
	//homePath = GetHomePath();
	ui.setupUi(this);
	novaParent = (NovaConfig *)parent;
	editNode = *n;
	m_ethernetEdit = new HexMACSpinBox(this, editNode.MAC, macSuffix);
	m_prefixEthEdit = new HexMACSpinBox(this, editNode.MAC, macPrefix);
	ui.ethernetHBox->insertWidget(0, m_ethernetEdit);
	ui.ethernetHBox->insertWidget(0, m_prefixEthEdit);
	LoadNode();
}

nodePopup::~nodePopup()
{

}


/************************************************
 * Load and Save changes to the node's profile
 ************************************************/

//saves the changes to a node
void nodePopup::SaveNode()
{
	editNode.MAC = m_prefixEthEdit->text().toStdString() +":"+m_ethernetEdit->text().toStdString();
	editNode.realIP = (ui.ipSpinBox0->value() << 24) +(ui.ipSpinBox1->value() << 16)
			+ (ui.ipSpinBox2->value() << 8) + (ui.ipSpinBox3->value());
	in_addr inTemp;
	inTemp.s_addr = htonl(editNode.realIP);
	editNode.IP = inet_ntoa(inTemp);
}

//loads the selected node's options
void nodePopup::LoadNode()
{
	subnet s = novaParent->m_honeydConfig->m_subnets[editNode.sub];
	profile p = novaParent->m_honeydConfig->m_profiles[editNode.pfile];

	ui.ethernetVendorEdit->setText((QString)p.ethernet.c_str());
	if(editNode.MAC.length() == 17)
	{
		QString prefixStr = QString(editNode.MAC.substr(0, 8).c_str()).toLower();
		prefixStr = prefixStr.remove(':');
		m_prefixEthEdit->setValue(prefixStr.toInt(NULL, 16));

		QString suffixStr = QString(editNode.MAC.substr(9, 8).c_str()).toLower();
		suffixStr = suffixStr.remove(':');
		m_ethernetEdit->setValue(suffixStr.toInt(NULL, 16));
	}
	else
	{
		m_prefixEthEdit->setValue(0);
		m_ethernetEdit->setValue(0);
	}

	int count = 0;
	int numBits = 32 - s.maskBits;

	in_addr_t base = ::pow(2, numBits);
	in_addr_t flatConst = ::pow(2,32)- base;

	flatConst = flatConst & editNode.realIP;
	in_addr_t flatBase = flatConst;
	count = s.maskBits/8;

	while(count < 4)
	{
		switch(count)
		{
			case 0:
			{
				ui.ipSpinBox0->setReadOnly(false);
				if(numBits > 32)
					numBits = 32;
				base = ::pow(2,32)-1;
				flatBase = flatConst & base;
				base = ::pow(2, numBits)-1;
				ui.ipSpinBox0->setRange((flatBase >> 24), (flatBase+base) >> 24);
				break;
			}
			case 1:
			{
				ui.ipSpinBox1->setReadOnly(false);
				if(numBits > 24)
					numBits = 24;
				base = ::pow(2, 24)-1;
				flatBase = flatConst & base;
				base = ::pow(2, numBits)-1;
				ui.ipSpinBox1->setRange(flatBase >> 16, (flatBase+base) >> 16);
				break;
			}
			case 2:
			{
				ui.ipSpinBox2->setReadOnly(false);
				if(numBits > 16)
					numBits = 16;
				base = ::pow(2, 16)-1;
				flatBase = flatConst & base;
				base = ::pow(2, numBits)-1;
				ui.ipSpinBox2->setRange(flatBase >> 8, (flatBase+base) >> 8);
				break;
			}
			case 3:
			{
				ui.ipSpinBox3->setReadOnly(false);
				if(numBits > 8)
					numBits = 8;
				base = ::pow(2, 8)-1;
				flatBase = flatConst & base;
				base = ::pow(2, numBits)-1;
				ui.ipSpinBox3->setRange(flatBase, (flatBase+base));
				break;
			}
		}
		count++;
	}
	ui.ipSpinBox3->setValue(editNode.realIP & 255);
	ui.ipSpinBox2->setValue((editNode.realIP >> 8) & 255);
	ui.ipSpinBox1->setValue((editNode.realIP >> 16) & 255);
	ui.ipSpinBox0->setValue((editNode.realIP >> 24) & 255);
}

/************************************************
 * Data Transfer Functions
 ************************************************/

//Copies the data from parent novaconfig and adjusts the pointers
void nodePopup::PullData()
{
	editNode = novaParent->m_honeydConfig->m_nodes[editNode.name];
}

//Copies the data to parent novaconfig and adjusts the pointers
void nodePopup::PushData()
{
	novaParent->m_loading->lock();
	novaParent->m_honeydConfig->m_nodes[editNode.name] = editNode;
	novaParent->SyncAllNodesWithProfiles();

	//The node may have a new name after updateNodeTypes depending on changes made and profile type
	if(novaParent->m_honeydConfig->m_profiles[editNode.pfile].type == staticDHCP)
		editNode.name = editNode.MAC;
	if(novaParent->m_honeydConfig->m_profiles[editNode.pfile].type == static_IP)
		editNode.name = editNode.IP;
	novaParent->m_loading->unlock();
	novaParent->LoadAllNodes();
}

/************************************************
 * General GUI Signal Handling
 ************************************************/

//Closes the window without saving any changes
void nodePopup::on_cancelButton_clicked()
{
	this->close();
}

void nodePopup::on_okButton_clicked()
{
	NovaGUI * mainwindow  = (NovaGUI*)novaParent->parent();
	SaveNode();
	int ret = ValidateNodeSettings();
	switch(ret)
	{
		case 0:
			PushData();
			this->close();
			break;
		case 1:
			mainwindow->m_prompter->DisplayPrompt(mainwindow->NODE_LOAD_FAIL,
					"This Node requires a unique IP address");
			break;
		case 2:
			mainwindow->m_prompter->DisplayPrompt(mainwindow->NODE_LOAD_FAIL,
					"DHCP Enabled nodes requires a unique MAC Address.");
			break;
	}
	on_restoreButton_clicked();
}

void nodePopup::on_restoreButton_clicked()
{
	PullData();
	LoadNode();
}

void nodePopup::on_applyButton_clicked()
{
	NovaGUI * mainwindow  = (NovaGUI*)novaParent->parent();
	SaveNode();
	int ret = ValidateNodeSettings();
	switch(ret)
	{
		case 0:
			PushData();
			break;
		case 1:
			mainwindow->m_prompter->DisplayPrompt(mainwindow->NODE_LOAD_FAIL,
					"This Node requires a unique IP address");
			break;
		case 2:
			mainwindow->m_prompter->DisplayPrompt(mainwindow->NODE_LOAD_FAIL,
					"DHCP Enabled nodes requires a unique MAC Address.");
			break;
	}
	on_restoreButton_clicked();

}

void nodePopup::on_generateButton_clicked()
{
	editNode.MAC = novaParent->GenerateUniqueMACAddress(ui.ethernetVendorEdit->text().toStdString());
	LoadNode();
	SaveNode();
}

int nodePopup::ValidateNodeSettings()
{
	novaParent->m_loading->lock();
	bool ipConflict = false;
	bool macConflict = false;
	for(NodeTable::iterator it = novaParent->m_honeydConfig->m_nodes.begin(); it != novaParent->m_honeydConfig->m_nodes.end(); it++)
	{
		if(it->second.name.compare(editNode.name))
		{
			if(it->second.realIP == editNode.realIP)
			{
				ipConflict = true;
				break;
			}
			if(!it->second.MAC.compare(editNode.MAC))
			{
				macConflict = true;
				break;
			}
		}
	}
	if(novaParent->m_honeydConfig->m_subnets[editNode.sub].base == editNode.realIP)
	{
		ipConflict = true;
	}

	int ret = 0;
	if(ipConflict)
	{
		ret = 1;
	}
	else if(macConflict)
	{
		ret = 2;
	}
	novaParent->m_loading->unlock();
	return ret;
}
