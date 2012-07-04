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

/****************************
 Construct and Initialize GUI
 ****************************/

subnetPopup::subnetPopup(QWidget *parent, Nova::Subnet *s)
    : QMainWindow(parent)
{
	ui.setupUi(this);
	nParent = (NovaConfig*)parent;
	m_editSubnet = *s;
	subName = m_editSubnet.m_name;
	m_maskEdit = new MaskSpinBox(this, s);
	ui.maskHBox->insertWidget(0, m_maskEdit);
	LoadSubnet();
}

subnetPopup::~subnetPopup()
{

}


/*********************************************
 Load and Save changes to the Subnet's profile
 *********************************************/

//saves the changes to a Subnet
void subnetPopup::SaveSubnet()
{
	//Get subnet name and mask bits
	m_editSubnet.m_name = ui.interfaceEdit->text().toStdString();
	m_editSubnet.m_maskBits = m_maskEdit->value();

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
	ss << inet_ntoa(inTemp) << '/' << m_editSubnet.m_maskBits;
	m_editSubnet.m_address = ss.str();

	m_editSubnet.m_mask = m_maskEdit->text().toStdString();

	in_addr_t maskTemp = ntohl(inet_addr(m_editSubnet.m_mask.c_str()));
	m_editSubnet.m_base = (temp & maskTemp);
	m_editSubnet.m_max = m_editSubnet.m_base + ~maskTemp;

	vector<string> addList;
	addList.clear();

	//Search for nodes that need to reflect any changes
	while(m_editSubnet.m_nodes.size())
	{
		conflict = false;
		Node tempNode = nParent->m_honeydConfig->m_nodes[m_editSubnet.m_nodes.back()];
		m_editSubnet.m_nodes.pop_back();

		tempNode.m_interface = m_editSubnet.m_name;
		tempNode.m_sub = m_editSubnet.m_name;

		tempNode.m_realIP = (tempNode.m_realIP & ~maskTemp);
		//If the subnet has been modified to take the IP of a current node,
		// attempt to give the node the IP the subnet was using previously
		if(tempNode.m_realIP == (subNewIP & ~maskTemp))
		{
			tempNode.m_realIP = (subnetRealIP & ~maskTemp);
		}

		for(uint i = 0; i < m_editSubnet.m_nodes.size(); i++)
		{
			if(tempNode.m_realIP == (nParent->m_honeydConfig->m_nodes[m_editSubnet.m_nodes[i]].m_realIP & ~maskTemp))
			{
				conflict = true;
			}
		}

		for(uint i = 0; i < addList.size(); i++)
		{
			if(tempNode.m_realIP == (nParent->m_honeydConfig->m_nodes[addList[i]].m_realIP & ~maskTemp))
			{
				conflict = true;
			}
		}

		if((tempNode.m_realIP == 0) || (tempNode.m_realIP == ~maskTemp))
		{
			conflict = true;
		}

		tempNode.m_realIP += m_editSubnet.m_base;

		if((tempNode.m_realIP == subNewIP) || conflict)
		{
			conflict = true;
			// 0 == editSubnet.base & ~maskTemp
			tempNode.m_realIP = 1;
			while(conflict && (tempNode.m_realIP < ~maskTemp))
			{
				conflict = false;
				if(tempNode.m_realIP == (subNewIP & ~maskTemp))
				{
					conflict = true;
					tempNode.m_realIP++;
					continue;
				}
				if((tempNode.m_realIP == 0) || (tempNode.m_realIP == ~maskTemp))
				{
					conflict = true;
					tempNode.m_realIP++;
					continue;
				}
				for(uint i = 0; i < m_editSubnet.m_nodes.size(); i++)
				{
					if(tempNode.m_realIP == (nParent->m_honeydConfig->m_nodes[m_editSubnet.m_nodes[i]].m_realIP & ~maskTemp))
					{
						conflict = true;
						tempNode.m_realIP++;
						break;
					}
				}
				if(!conflict) for(uint i = 0; i < addList.size(); i++)
				{
					if(tempNode.m_realIP == (nParent->m_honeydConfig->m_nodes[addList[i]].m_realIP & ~maskTemp))
					{
						conflict = true;
						tempNode.m_realIP++;
						break;
					}
				}
			}

			tempNode.m_realIP += m_editSubnet.m_base;

			if(conflict)
			{
				nParent->m_honeydConfig->m_nodes.erase(tempNode.m_name);
				continue;
			}
		}
		inTemp.s_addr = htonl(tempNode.m_realIP);

		if (tempNode.m_IP != "DHCP")
		{
			tempNode.m_IP = inet_ntoa(inTemp);
		}

		//If node has a static IP it's name needs to change with it's IP
		//the only exception to this is the Doppelganger, it's name is always the same.
		if(tempNode.m_name.compare("Doppelganger") && tempNode.m_IP.length() && tempNode.m_IP != "DHCP")
		{
			nParent->m_honeydConfig->m_nodes.erase(tempNode.m_name);
			tempNode.m_name = tempNode.m_IP + " - " + tempNode.m_MAC;
		}

		nParent->m_honeydConfig->m_nodes[tempNode.m_name] = tempNode;


		addList.push_back(tempNode.m_name);
	}

	m_editSubnet.m_nodes = addList;
	subnetRealIP = subNewIP;
}

//loads the selected Subnet's options
void subnetPopup::LoadSubnet()
{
	m_maskEdit->setValue(m_editSubnet.m_maskBits);
	ui.interfaceEdit->setText(m_editSubnet.m_name.c_str());

	in_addr_t temp = ntohl(inet_addr(m_editSubnet.m_address.substr(0,m_editSubnet.m_address.find('/',0)).c_str()));
	subnetRealIP = temp;
	ui.ipSpinBox3->setValue(temp & 255);
	ui.ipSpinBox2->setValue((temp >> 8) & 255);
	ui.ipSpinBox1->setValue((temp >> 16) & 255);
	ui.ipSpinBox0->setValue((temp >> 24) & 255);
	//number of nodes + network address, gateway and broadcast address
	int n = m_editSubnet.m_nodes.size() + 3;
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

/***********************
 Data Transfer Functions
 ***********************/

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
	nParent->m_honeydConfig->m_subnets[m_editSubnet.m_name] = m_editSubnet;
	nParent->m_loading->unlock();

	nParent->LoadAllNodes();
	subName = m_editSubnet.m_name;
}

/***************************
 General GUI Signal Handling
 ***************************/

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

