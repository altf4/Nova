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
NovaConfig *novaParent;

/************************************************
 * Construct and Initialize GUI
 ************************************************/

nodePopup::nodePopup(QWidget *parent, Nova::Node *n, bool editingNode)
    : QMainWindow(parent)
{
	ui.setupUi(this);

	//Set up member var references to parent and dialog prompt object
	novaParent = (NovaConfig *)parent;
	m_prompter = novaParent->m_mainwindow->m_prompter;

	//Init params into member vars
	m_parentNode = n;
	m_editingNode = editingNode;

	//init and display MAC editing Spin boxes
	m_ethernetEdit = new HexMACSpinBox(this, m_editNode.MAC, macSuffix);
	m_prefixEthEdit = new HexMACSpinBox(this, m_editNode.MAC, macPrefix);

	vector<string> profiles = novaParent->m_honeydConfig->GetProfileNames();
	while(!profiles.empty())
	{
		ui.nodeProfileComboBox->addItem(QString::fromStdString(profiles.back()));
		profiles.pop_back();
	}
	ui.nodeProfileComboBox->setCurrentIndex(ui.nodeProfileComboBox->findText(QString(m_parentNode->pfile.c_str())));
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
	m_editNode.pfile = ui.nodeProfileComboBox->currentText().toStdString();

	if(ui.isDHCP->isChecked())
	{
		m_editNode.IP = "DHCP";
	}
	else
	{
		m_editNode.realIP = (ui.ipSpinBox0->value() << 24) + (ui.ipSpinBox1->value() << 16)
				+ (ui.ipSpinBox2->value() << 8) + (ui.ipSpinBox3->value());
		in_addr inTemp;
		inTemp.s_addr = htonl(m_editNode.realIP);
		m_editNode.IP = inet_ntoa(inTemp);
	}
	if(ui.isRandomMAC->isChecked())
	{
		m_editNode.MAC = "RANDOM";
	}
	else
	{
		m_editNode.MAC = m_prefixEthEdit->text().toStdString() + ":"+ m_ethernetEdit->text().toStdString();
	}
	m_editNode.name = m_editNode.IP + " - " + m_editNode.MAC;
}

void nodePopup::on_isDHCP_stateChanged()
{
	if(ui.isDHCP->isChecked())
	{
		ui.ipHBox->removeWidget(ui.ipSpinBox0);
		ui.ipHBox->removeWidget(ui.ipSpinBox1);
		ui.ipHBox->removeWidget(ui.ipSpinBox2);
		ui.ipHBox->removeWidget(ui.ipSpinBox3);
		ui.ipSpinBox0->setVisible(false);
		ui.ipSpinBox1->setVisible(false);
		ui.ipSpinBox2->setVisible(false);
		ui.ipSpinBox3->setVisible(false);
	}
	else
	{
		ui.ipHBox->addWidget(ui.ipSpinBox0);
		ui.ipHBox->addWidget(ui.ipSpinBox1);
		ui.ipHBox->addWidget(ui.ipSpinBox2);
		ui.ipHBox->addWidget(ui.ipSpinBox3);
		ui.ipSpinBox0->setVisible(true);
		ui.ipSpinBox1->setVisible(true);
		ui.ipSpinBox2->setVisible(true);
		ui.ipSpinBox3->setVisible(true);
	}
}

void nodePopup::on_isRandomMAC_stateChanged()
{
	if(ui.isRandomMAC->isChecked())
	{
		ui.ethernetHBox->removeWidget(m_prefixEthEdit);
		ui.ethernetHBox->removeWidget(m_ethernetEdit);
		ui.ethernetHBox->removeWidget(ui.generateButton);
		m_prefixEthEdit->setVisible(false);
		m_ethernetEdit->setVisible(false);
		ui.generateButton->setVisible(false);
	}
	else
	{
		ui.ethernetHBox->addWidget(m_prefixEthEdit);
		ui.ethernetHBox->addWidget(m_ethernetEdit);
		ui.ethernetHBox->addWidget(ui.generateButton);
		m_prefixEthEdit->setVisible(true);
		m_ethernetEdit->setVisible(true);
		ui.generateButton->setVisible(true);
	}

}

//loads the selected node's options
void nodePopup::LoadNode()
{
	m_editNode = *m_parentNode;
	subnet s = novaParent->m_honeydConfig->m_subnets[m_editNode.sub];
	profile p = novaParent->m_honeydConfig->m_profiles[m_editNode.pfile];

	if(m_editNode.MAC.length() == 17)
	{
		QString prefixStr = QString(m_editNode.MAC.substr(0, 8).c_str()).toLower();
		prefixStr = prefixStr.remove(':');
		m_prefixEthEdit->setValue(prefixStr.toInt(NULL, 16));

		QString suffixStr = QString(m_editNode.MAC.substr(9, 8).c_str()).toLower();
		suffixStr = suffixStr.remove(':');
		m_ethernetEdit->setValue(suffixStr.toInt(NULL, 16));
	}
	else
	{
		m_prefixEthEdit->setValue(0);
		m_ethernetEdit->setValue(0);
	}

	//See if we're using a random mac and update the window to reflect which case it is
	ui.isRandomMAC->setChecked(!m_editNode.MAC.compare("RANDOM"));
	if(ui.isRandomMAC->isChecked())
	{
		ui.ethernetHBox->removeWidget(ui.generateButton);
		ui.generateButton->setVisible(false);
	}
	else
	{
		ui.ethernetHBox->addWidget(m_prefixEthEdit);
		ui.ethernetHBox->addWidget(m_ethernetEdit);
		m_prefixEthEdit->setVisible(true);
		m_ethernetEdit->setVisible(true);
	}

	int count = 0;
	int numBits = 32 - s.maskBits;

	in_addr_t base = ::pow(2, numBits);
	in_addr_t flatConst = ::pow(2,32)- base;

	flatConst = flatConst & s.base;
	in_addr_t flatBase = flatConst;
	count = s.maskBits/8;

	//See if we're using dhcp and update the window to reflect which case it is
	ui.isDHCP->setChecked(!m_editNode.IP.compare("DHCP"));
	if(ui.isDHCP->isChecked())
	{
		ui.ipHBox->removeWidget(ui.ipSpinBox0);
		ui.ipHBox->removeWidget(ui.ipSpinBox1);
		ui.ipHBox->removeWidget(ui.ipSpinBox2);
		ui.ipHBox->removeWidget(ui.ipSpinBox3);
		ui.ipSpinBox0->setVisible(false);
		ui.ipSpinBox1->setVisible(false);
		ui.ipSpinBox2->setVisible(false);
		ui.ipSpinBox3->setVisible(false);
		//We take the previous 'realIP' and apply the subnets mask to assert the ip is within the subnet
		m_editNode.realIP = (m_editNode.realIP & (base-1)) + flatBase;
	}

	while(count < 4)
	{
		switch(count)
		{
			case 0:
			{
				ui.ipSpinBox0->setReadOnly(false);
				if(numBits > 32)
				{
					numBits = 32;
				}
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
				{
					numBits = 24;
				}
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
				{
					numBits = 16;
				}
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
				{
					numBits = 8;
				}
				base = ::pow(2, 8)-1;
				flatBase = flatConst & base;
				base = ::pow(2, numBits)-1;
				ui.ipSpinBox3->setRange(flatBase, (flatBase+base));
				break;
			}
		}
		count++;
	}

	ui.ipSpinBox3->setValue(m_editNode.realIP & 255);
	ui.ipSpinBox2->setValue((m_editNode.realIP >> 8) & 255);
	ui.ipSpinBox1->setValue((m_editNode.realIP >> 16) & 255);
	ui.ipSpinBox0->setValue((m_editNode.realIP >> 24) & 255);
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
	on_applyButton_clicked();

	novaParent->LoadAllNodes();
	this->close();
}

void nodePopup::on_restoreButton_clicked()
{
	LoadNode();
}

void nodePopup::on_applyButton_clicked()
{
	SaveNode();

	NovaGUI *mainwindow  = (NovaGUI*)novaParent->parent();
	nodeConflictType ret = ValidateNodeSettings();
	switch(ret)
	{
		case NO_CONFLICT:
		{
			//If we're editing we don't need to add a new node
			if(m_editingNode)
			{
				//If we have a new name, clean up the old node
				if(!m_editNode.name.compare(m_parentNode->name))
				{
					novaParent->m_honeydConfig->m_nodes[m_editNode.name] = m_editNode;
					break;
				}
				novaParent->m_honeydConfig->DeleteNode(m_parentNode->name);
			}
			novaParent->m_honeydConfig->AddNewNode(m_editNode.pfile, m_editNode.IP, m_editNode.MAC, m_editNode.interface, m_editNode.sub);

			// Make our new node the current selection
			novaParent->SetSelectedNode(m_editNode.name);

			break;
		}
		case IP_CONFLICT:
		{
			m_prompter->DisplayPrompt(mainwindow->NODE_LOAD_FAIL,
					"This Node requires a unique IP address");
			break;
		}
		case MAC_CONFLICT:
		{
			m_prompter->DisplayPrompt(mainwindow->NODE_LOAD_FAIL,
					"DHCP Enabled nodes requires a unique MAC Address.");
			break;
		}
	}

}

void nodePopup::on_generateButton_clicked()
{
	if (ui.isRandomMAC->isChecked())
	{
		return;
	}

	m_editNode.MAC = novaParent->m_honeydConfig->GenerateUniqueMACAddress(novaParent->m_honeydConfig->m_profiles[m_editNode.pfile].ethernet);
	QString prefixStr = QString(m_editNode.MAC.substr(0, 8).c_str()).toLower();
	prefixStr = prefixStr.remove(':');
	m_prefixEthEdit->setValue(prefixStr.toInt(NULL, 16));

	QString suffixStr = QString(m_editNode.MAC.substr(9, 8).c_str()).toLower();
	suffixStr = suffixStr.remove(':');
	m_ethernetEdit->setValue(suffixStr.toInt(NULL, 16));
}

nodeConflictType nodePopup::ValidateNodeSettings()
{
	novaParent->m_loading->lock();

	bool ipConflict = false;
	bool macConflict = false;
	// If the IP or MAC hasn't changed and we aren't using DHCP and/or RANDOM
	// then the IsUsed checks will return true (ie conflict) when it finds itself
	// Thus we skip validation only in the case that 1. the IP and/or MAC remains unchanged
	// and 2. We are editing a Node, not creating one.

	//If were editing the node and the IP hasn't changed that's ok.
	if(!m_editingNode || m_editNode.IP.compare(m_parentNode->IP))
	{
		//If the IP is DHCP we don't care if theres a conflict
		if(m_editNode.IP != "DHCP")
		{
			ipConflict = novaParent->m_honeydConfig->IsIPUsed(m_editNode.IP);
			if(novaParent->m_honeydConfig->m_subnets[m_editNode.sub].base == m_editNode.realIP)
			{
				ipConflict = true;
			}
		}
	}
	//If were editing the node and the MAC hasn't changed that's ok.
	if(!m_editingNode || m_editNode.MAC.compare(m_parentNode->MAC))
	{
		if(m_editNode.MAC != "RANDOM")
		{
			macConflict = novaParent->m_honeydConfig->IsMACUsed(m_editNode.MAC);
		}
	}


	nodeConflictType ret = NO_CONFLICT;
	if(ipConflict)
	{
		ret = IP_CONFLICT;
	}
	else if(macConflict)
	{
		ret = MAC_CONFLICT;
	}
	novaParent->m_loading->unlock();
	return ret;
}
