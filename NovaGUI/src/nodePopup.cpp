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
	ethernetEdit = new HexMACSpinBox(this, editNode.MAC);
	ui.ethernetHBox->insertWidget(0, ethernetEdit);
	loadNode();
}

nodePopup::~nodePopup()
{

}


/************************************************
 * Load and Save changes to the node's profile
 ************************************************/

//saves the changes to a node
void nodePopup::saveNode()
{
	editNode.MAC = ethernetEdit->text().toStdString();
	editNode.realIP = (ui.ipSpinBox0->value() << 24) +(ui.ipSpinBox1->value() << 16)
			+ (ui.ipSpinBox2->value() << 8) + (ui.ipSpinBox3->value());
	in_addr inTemp;
	inTemp.s_addr = htonl(editNode.realIP);
	editNode.IP = inet_ntoa(inTemp);
}

//loads the selected node's options
void nodePopup::loadNode()
{
	subnet s = novaParent->subnets[editNode.sub];
	profile p = novaParent->profiles[editNode.pfile];

	ui.ethernetVendorEdit->setText((QString)p.ethernet.c_str());
	QString t = QString(editNode.MAC.substr(0, 9).c_str());
	ethernetEdit->setPrefix(t);
	QString suffixStr = QString(editNode.MAC.substr(9, editNode.MAC.size()).c_str());
	suffixStr = suffixStr.remove(':');
	ethernetEdit->setValue(suffixStr.toInt(NULL, 16));

	int count = 0;
	int numBits = 32 - s.maskBits;

	in_addr_t base = pow(2, numBits);
	in_addr_t flatConst = pow(2,32)- base;

	flatConst = flatConst & editNode.realIP;
	in_addr_t flatBase = flatConst;
	count = s.maskBits/8;

	switch(count)
	{
		case 0:
			ui.ipSpinBox0->setReadOnly(false);
			if(numBits > 32)
				numBits = 32;
			base = pow(2,32)-1;
			flatBase = flatConst & base;
			base = pow(2, numBits)-1;
			ui.ipSpinBox0->setRange((flatBase >> 24), (flatBase+base) >> 24);

		case 1:

			ui.ipSpinBox1->setReadOnly(false);
			if(numBits > 24)
				numBits = 24;
			base = pow(2, 24)-1;
			flatBase = flatConst & base;
			base = pow(2, numBits)-1;
			ui.ipSpinBox1->setRange(flatBase >> 16, (flatBase+base) >> 16);

		case 2:
			ui.ipSpinBox2->setReadOnly(false);
			if(numBits > 16)
				numBits = 16;
			base = pow(2, 16)-1;
			flatBase = flatConst & base;
			base = pow(2, numBits)-1;
			ui.ipSpinBox2->setRange(flatBase >> 8, (flatBase+base) >> 8);

		case 3:
			ui.ipSpinBox3->setReadOnly(false);
			if(numBits > 8)
				numBits = 8;
			base = pow(2, 8)-1;
			flatBase = flatConst & base;
			base = pow(2, numBits)-1;
			ui.ipSpinBox3->setRange(flatBase, (flatBase+base));
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
void nodePopup::pullData()
{
	editNode = novaParent->nodes[editNode.name];
}

//Copies the data to parent novaconfig and adjusts the pointers
void nodePopup::pushData()
{
	novaParent->nodes[editNode.name] = editNode;
	novaParent->updateNodeTypes();
	novaParent->loadAllNodes();
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
	saveNode();
	pushData();
	this->close();
}

void nodePopup::on_restoreButton_clicked()
{
	pullData();
	loadNode();
}

void nodePopup::on_applyButton_clicked()
{
	saveNode();
	pushData();
	pullData();
	loadNode();
}

