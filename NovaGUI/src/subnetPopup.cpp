//============================================================================
// Name        : subnetPopup.h
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
// Description : Popup for creating and editing subnets
//============================================================================
#include "subnetPopup.h"
#include "novaconfig.h"
#include "novagui.h"

#include <arpa/inet.h>

using namespace std;
using namespace Nova;

//Parent window pointers, allows us to call functions from the parent
NovaConfig *nParent;
string subName;
in_addr_t subnetRealIP;

/************************************************
 * Construct and Initialize GUI
 ************************************************/

subnetPopup::subnetPopup(QWidget *parent, Nova::subnet *s)
    : QMainWindow(parent)
{
	ui.setupUi(this);
	nParent = (NovaConfig*)parent;
	m_editSubnet = *s;
	subName = m_editSubnet.name;
	m_maskEdit = new MaskSpinBox(this, s);
	ui.maskHBox->insertWidget(0, m_maskEdit);
	LoadSubnet();
}

subnetPopup::~subnetPopup()
{

}


/************************************************
 * Load and Save changes to the Subnet's profile
 ************************************************/

//saves the changes to a Subnet
void subnetPopup::SaveSubnet()
{
	//Get subnet name and mask bits
	m_editSubnet.name = ui.interfaceEdit->text().toStdString();
	m_editSubnet.maskBits = m_maskEdit->value();

	//Extract IP address
	in_addr_t temp = (ui.ipSpinBox0->value() << 24) +(ui.ipSpinBox1->value() << 16)
			+ (ui.ipSpinBox2->value() << 8) + (ui.ipSpinBox3->value());
	in_addr_t subNewIP = temp;

	//Init some values
	bool conflict = false;
	in_addr inTemp;
	inTemp.s_addr = htonl(temp);

	//Format IP address
	stringstream ss;
	ss << inet_ntoa(inTemp) << '/' << m_editSubnet.maskBits;
	m_editSubnet.address = ss.str();

	m_editSubnet.mask = m_maskEdit->text().toStdString();

	in_addr_t maskTemp = ntohl(inet_addr(m_editSubnet.mask.c_str()));
	m_editSubnet.base = (temp & maskTemp);
	m_editSubnet.max = m_editSubnet.base + ~maskTemp;

	vector<string> addList;
	addList.clear();

	//Search for nodes that need to reflect any changes
	while(m_editSubnet.nodes.size())
	{
		conflict = false;
		Node tempNode = nParent->m_honeydConfig->m_nodes[m_editSubnet.nodes.back()];
		m_editSubnet.nodes.pop_back();

		tempNode.interface = m_editSubnet.name;
		tempNode.sub = m_editSubnet.name;

		tempNode.realIP = (tempNode.realIP & ~maskTemp);
		//If the subnet has been modified to take the IP of a current node,
		// attempt to give the node the IP the subnet was using previously
		if(tempNode.realIP == (subNewIP & ~maskTemp))
		{
			tempNode.realIP = (subnetRealIP & ~maskTemp);
		}

		for(uint i = 0; i < m_editSubnet.nodes.size(); i++)
		{
			if(tempNode.realIP == (nParent->m_honeydConfig->m_nodes[m_editSubnet.nodes[i]].realIP & ~maskTemp))
			{
				conflict = true;
			}
		}

		for(uint i = 0; i < addList.size(); i++)
		{
			if(tempNode.realIP == (nParent->m_honeydConfig->m_nodes[addList[i]].realIP & ~maskTemp))
			{
				conflict = true;
			}
		}

		if((tempNode.realIP == 0) || (tempNode.realIP == ~maskTemp))
		{
			conflict = true;
		}

		tempNode.realIP += m_editSubnet.base;

		if((tempNode.realIP == subNewIP) || conflict)
		{
			conflict = true;
			// 0 == editSubnet.base & ~maskTemp
			tempNode.realIP = 1;
			while(conflict && (tempNode.realIP < ~maskTemp))
			{
				conflict = false;
				if(tempNode.realIP == (subNewIP & ~maskTemp))
				{
					conflict = true;
					tempNode.realIP++;
					continue;
				}
				if((tempNode.realIP == 0) || (tempNode.realIP == ~maskTemp))
				{
					conflict = true;
					tempNode.realIP++;
					continue;
				}
				for(uint i = 0; i < m_editSubnet.nodes.size(); i++)
				{
					if(tempNode.realIP == (nParent->m_honeydConfig->m_nodes[m_editSubnet.nodes[i]].realIP & ~maskTemp))
					{
						conflict = true;
						tempNode.realIP++;
						break;
					}
				}
				if(!conflict) for(uint i = 0; i < addList.size(); i++)
				{
					if(tempNode.realIP == (nParent->m_honeydConfig->m_nodes[addList[i]].realIP & ~maskTemp))
					{
						conflict = true;
						tempNode.realIP++;
						break;
					}
				}
			}

			tempNode.realIP += m_editSubnet.base;

			if(conflict)
			{
				nParent->m_honeydConfig->m_nodes.erase(tempNode.name);
				continue;
			}
		}
		inTemp.s_addr = htonl(tempNode.realIP);

		if (tempNode.IP != "DHCP")
		{
			tempNode.IP = inet_ntoa(inTemp);
		}

		//If node has a static IP it's name needs to change with it's IP
		//the only exception to this is the Doppelganger, it's name is always the same.
		if(tempNode.name.compare("Doppelganger") && tempNode.IP.length() && tempNode.IP != "DHCP")
		{
			nParent->m_honeydConfig->m_nodes.erase(tempNode.name);
			tempNode.name = tempNode.IP + " - " + tempNode.MAC;
		}

		nParent->m_honeydConfig->m_nodes[tempNode.name] = tempNode;


		addList.push_back(tempNode.name);
	}

	m_editSubnet.nodes = addList;
	subnetRealIP = subNewIP;
}

//loads the selected Subnet's options
void subnetPopup::LoadSubnet()
{
	m_maskEdit->setValue(m_editSubnet.maskBits);
	ui.interfaceEdit->setText(m_editSubnet.name.c_str());

	in_addr_t temp = ntohl(inet_addr(m_editSubnet.address.substr(0,m_editSubnet.address.find('/',0)).c_str()));
	subnetRealIP = temp;
	ui.ipSpinBox3->setValue(temp & 255);
	ui.ipSpinBox2->setValue((temp >> 8) & 255);
	ui.ipSpinBox1->setValue((temp >> 16) & 255);
	ui.ipSpinBox0->setValue((temp >> 24) & 255);
	//number of nodes + network address, gateway and broadcast address
	int n = m_editSubnet.nodes.size() + 3;
	int count = 0;
	while((n/=2) > 0)
	{
		count++;
	}
	m_maskEdit->setRange(0, 31-count);
	temp = 0;
	int i;
	for(i = 0; i < 31-count; i++)
	{
		temp++;
		temp = temp << 1;
	}
	temp = temp << (31-i);
	in_addr mask;
	mask.s_addr = ntohl(temp);
	ui.maxMaskLabel->setText(QString(inet_ntoa(mask)).prepend("Maximum Mask: "));
}

/************************************************
 * Data Transfer Functions
 ************************************************/

//Copies the data from parent novaconfig and adjusts the pointers
void subnetPopup::PullData()
{
	m_editSubnet = nParent->m_honeydConfig->m_subnets[subName];
}

//Copies the data to parent novaconfig and adjusts the pointers
void subnetPopup::PushData()
{
	nParent->m_loading->lock();
	nParent->m_honeydConfig->m_subnets.erase(subName);
	nParent->m_honeydConfig->m_subnets[m_editSubnet.name] = m_editSubnet;
	nParent->m_loading->unlock();

	nParent->LoadAllNodes();
	subName = m_editSubnet.name;
}

/************************************************
 * General GUI Signal Handling
 ************************************************/

//Closes the window without saving any changes
void subnetPopup::on_cancelButton_clicked()
{
	this->close();
}

void subnetPopup::on_okButton_clicked()
{
	SaveSubnet();
	PushData();
	this->close();
}

void subnetPopup::on_restoreButton_clicked()
{
	PullData();
	LoadSubnet();
}

void subnetPopup::on_applyButton_clicked()
{
	SaveSubnet();
	PushData();
	PullData();
	LoadSubnet();
}

