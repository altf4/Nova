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

using namespace std;
using namespace Nova;

//Parent window pointers, allows us to call functions from the parent
NovaConfig * nParent;
string subName;
in_addr_t subnetRealIP;

/************************************************
 * Construct and Initialize GUI
 ************************************************/

subnetPopup::subnetPopup(QWidget * parent, subnet * s)
    : QMainWindow(parent)
{
	ui.setupUi(this);
	nParent = (NovaConfig *)parent;
	editSubnet = *s;
	subName = editSubnet.name;
	maskEdit = new MaskSpinBox(this, s);
	ui.maskHBox->insertWidget(0, maskEdit);
	loadSubnet();
}

subnetPopup::~subnetPopup()
{

}


/************************************************
 * Load and Save changes to the Subnet's profile
 ************************************************/

//saves the changes to a Subnet
void subnetPopup::saveSubnet()
{
	editSubnet.name = ui.interfaceEdit->text().toStdString();
	editSubnet.maskBits = maskEdit->value();

	in_addr_t temp = (ui.ipSpinBox0->value() << 24) +(ui.ipSpinBox1->value() << 16)
			+ (ui.ipSpinBox2->value() << 8) + (ui.ipSpinBox3->value());
	in_addr_t subNewIP = temp;

	bool conflict = false;
	in_addr inTemp;
	inTemp.s_addr = htonl(temp);

	stringstream ss;
	ss << inet_ntoa(inTemp) << '/' << editSubnet.maskBits;
	editSubnet.address = ss.str();

	editSubnet.mask = maskEdit->text().toStdString();

	in_addr_t maskTemp = ntohl(inet_addr(editSubnet.mask.c_str()));
	editSubnet.base = (temp & maskTemp);
	editSubnet.max = editSubnet.base + ~maskTemp;

	vector<string> addList;
	addList.clear();
	while(editSubnet.nodes.size())
	{
		conflict = false;
		node tempNode = nParent->nodes[editSubnet.nodes.back()];
		editSubnet.nodes.pop_back();

		tempNode.interface = editSubnet.name;
		tempNode.sub = editSubnet.name;

		tempNode.realIP = (tempNode.realIP & ~maskTemp);
		//If the subnet has been modified to take the IP of a current node,
		// attempt to give the node the IP the subnet was using previously
		if(tempNode.realIP == (subNewIP & ~maskTemp))
			tempNode.realIP = (subnetRealIP & ~maskTemp);

		for(uint i = 0; i < editSubnet.nodes.size(); i++)
			if(tempNode.realIP == (nParent->nodes[editSubnet.nodes[i]].realIP & ~maskTemp))
				conflict = true;

		for(uint i = 0; i < addList.size(); i++)
			if(tempNode.realIP == (nParent->nodes[addList[i]].realIP & ~maskTemp))
				conflict = true;
		if((tempNode.realIP == 0) || (tempNode.realIP == ~maskTemp))
			conflict = true;
		tempNode.realIP += editSubnet.base;

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
				for(uint i = 0; i < editSubnet.nodes.size(); i++)
				{
					if(tempNode.realIP == (nParent->nodes[editSubnet.nodes[i]].realIP & ~maskTemp))
					{
						conflict = true;
						tempNode.realIP++;
						break;
					}
				}
				if(!conflict) for(uint i = 0; i < addList.size(); i++)
				{
					if(tempNode.realIP == (nParent->nodes[addList[i]].realIP & ~maskTemp))
					{
						conflict = true;
						tempNode.realIP++;
						break;
					}
				}
			}
			tempNode.realIP += editSubnet.base;
			if(conflict)
			{
				nParent->nodes.erase(tempNode.name);
				continue;
			}
		}
		inTemp.s_addr = htonl(tempNode.realIP);
		tempNode.IP = inet_ntoa(inTemp);
		if(nParent->profiles[tempNode.pfile].type == static_IP)
		{
			nParent->nodes.erase(tempNode.name);
			tempNode.name = tempNode.IP;
			nParent->nodes[tempNode.name] = tempNode;
		}
		else
			nParent->nodes[tempNode.name] = tempNode;

		addList.push_back(tempNode.name);
	}

	editSubnet.nodes = addList;
	subnetRealIP = subNewIP;
}

//loads the selected Subnet's options
void subnetPopup::loadSubnet()
{
	maskEdit->setValue(editSubnet.maskBits);
	ui.interfaceEdit->setText(editSubnet.name.c_str());

	in_addr_t temp = ntohl(inet_addr(editSubnet.address.substr(0,editSubnet.address.find('/',0)).c_str()));
	subnetRealIP = temp;
	ui.ipSpinBox3->setValue(temp & 255);
	ui.ipSpinBox2->setValue((temp >> 8) & 255);
	ui.ipSpinBox1->setValue((temp >> 16) & 255);
	ui.ipSpinBox0->setValue((temp >> 24) & 255);
	//number of nodes + network address, gateway and broadcast address
	int n = editSubnet.nodes.size() + 3;
	int count = 0;
	while((n/=2) > 0)
	{
		count++;
	}
	maskEdit->setRange(0, 31-count);
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
void subnetPopup::pullData()
{
	editSubnet = nParent->subnets[subName];
}

//Copies the data to parent novaconfig and adjusts the pointers
void subnetPopup::pushData()
{
	nParent->loading->lock();
	nParent->subnets.erase(subName);
	nParent->subnets[editSubnet.name] = editSubnet;
	nParent->loading->unlock();
	nParent->loadAllNodes();

	subName = editSubnet.name;
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
	saveSubnet();
	pushData();
	this->close();
}

void subnetPopup::on_restoreButton_clicked()
{
	pullData();
	loadSubnet();
}

void subnetPopup::on_applyButton_clicked()
{
	saveSubnet();
	pushData();
	pullData();
	loadSubnet();
}

