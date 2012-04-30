//============================================================================
// Name        : NovaComplexDialog.cpp
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
// Description : Provides dialogs for MAC and personality selection
//============================================================================
#include "NovaComplexDialog.h"
#include "NovaGuiTypes.h"
#include "OsPersonalityDb.h"

using namespace std;

class NovaConfig;

NovaComplexDialog::NovaComplexDialog(QWidget *parent)
    : QDialog(parent)
{
	m_retVal = NULL;
	m_novaParent = (NovaConfig *)parent;
	ui.setupUi(this);
	m_type = MACDialog;
}
NovaComplexDialog::NovaComplexDialog(whichDialog type,string* retval, QWidget *parent, string filter)
	: QDialog(parent)
{
	m_retVal = retval;
	m_novaParent = (NovaConfig *)parent;
	this->m_type = type;
	ui.setupUi(this);
	switch(type)
	{
		case PersonalityDialog:
		{
			this->setWindowTitle((const QString&)"Select an Operating System");
			ui.displayLabel->setText((const QString&)"Select which operating system you would like to emulate");
			ui.helpLabel->setText((const QString&)"Navigate down the tree, select an operating system and press 'Select'");
			ui.searchButton->setDefault(true);
			DrawPersonalities("");
			break;
		}
		case MACDialog:
		{
			ui.searchButton->setDefault(true);
			this->setWindowTitle((const QString&)"Select a MAC Vendor");
			this->blockSignals(true);
			ui.searchEdit->setText((QString)filter.c_str());
			this->blockSignals(false);
			on_searchButton_clicked();
			break;
		}
		default:
		{
			break;
		}
	}
	ui.searchEdit->setFocus();
}

NovaComplexDialog::~NovaComplexDialog()
{

}

//Sets the help message displayed at the bottom of the window to provide the user hints on usage.
void NovaComplexDialog::SetHelpMsg(const QString& str)
{
	ui.helpLabel->setText(str);
}

//Returns the help message displayed at the bottom of the window to provide the user hints on usage.
QString NovaComplexDialog::GetHelpMsg()
{
	return ui.helpLabel->text();
}

//Sets the description at the top on the window to inform the user of the window's purpose.
void NovaComplexDialog::SetInfoMsg(const QString& str)
{
	ui.displayLabel->setText(str);
}

//Returns the description at the top on the window to inform the user of the window's purpose.
QString NovaComplexDialog::GetInfoMsg()
{
	return ui.displayLabel->text();
}

//Returns the object's 'type' enum
whichDialog NovaComplexDialog::GetType()
{
	return m_type;
}

void NovaComplexDialog::DrawPersonalities(string filterStr)
{
	ui.treeWidget->clear();
	QTreeWidgetItem * item = NULL;
	QTreeWidgetItem * index = NULL;

	OsPersonalityDb db;
	db.LoadNmapPersonalitiesFromFile();
	vector<pair<string,string> > * printList = &db.m_nmapPersonalities;

	string fprint;
	string printClass;
	string temp;
	//Populate the list
	for(uint i = 0; i < printList->size(); i++)
	{

		//Get fingerprint and Class strings
		fprint = printList->at((int)i).first;
		printClass = printList->at((int)i).second;
		//find the first portion of the class string
		temp = printClass.substr(0,printClass.find(' '));
		bool matched = true;
		if(strcasestr(temp.c_str(), filterStr.c_str()) == NULL)
		{
			matched = false;
		}
		if(!matched)
			continue;

		//Get the current top level children
		QObjectList list = ui.treeWidget->children();
		bool found = false;

		//Attempt to locate the proper node, if none found create it
		for(int i = 0; i < ui.treeWidget->topLevelItemCount(); i++)
		{
			index = ui.treeWidget->topLevelItem(i);
			if(!index->text(0).toStdString().compare(temp))
			{
				found = true;
				break;
			}
		}
		if(!found)
		{
			index = new QTreeWidgetItem(ui.treeWidget);
			index->setText(0,(QString)temp.c_str());
		}


		//While the class is not resolved, find the next level node or create it.
		while(printClass.find("| ") != string::npos)
		{
			found = false;
			printClass = printClass.substr(printClass.find("| ")+2,printClass.size());
			if(printClass.find(' ') != string::npos)
				temp = printClass.substr(0,printClass.find(' '));
			else
				temp = printClass;

			if(index->childCount())
			{
				int i;
				for(i = 0; i < index->childCount(); i++)
				{
					item = index->child(i);
					if(!item->text(0).toStdString().compare(temp))
					{
						found = true;
						break;
					}
				}
			}
			if(!found)
			{
				index = new QTreeWidgetItem(index);
				index->setText(0,(QString)temp.c_str());
			}
		}
		//Look for a duplicate fingerprint, if not found create it.
		found = false;
		if(index->childCount())
		{
			int i;
			for(i = 0; i < index->childCount(); i++)
			{
				item = index->child(i);
				if(!item->text(0).toStdString().compare(fprint))
				{
					found = true;
					break;
				}
			}
		}
		if(!found)
		{
			index = new QTreeWidgetItem(index);
			index->setText(0,(QString)fprint.c_str());
		}
	}
}

void NovaComplexDialog::on_cancelButton_clicked()
{
	*m_retVal = "";
	this->close();
}

void NovaComplexDialog::on_selectButton_clicked()
{
	if((ui.treeWidget->currentItem() != NULL) && !ui.treeWidget->currentItem()->childCount())
	{
		string ret = ui.treeWidget->currentItem()->text(0).toStdString();
		*m_retVal = ret;
		this->close();
	}
}

void NovaComplexDialog::on_searchButton_clicked()
{
	QTreeWidgetItem * item = NULL;
	string filterStr = ui.searchEdit->text().toStdString();

	if(m_type == MACDialog)
	{
		VendorMacDb db;
		db.LoadPrefixFile();
		vector<string> matches = db.SearchVendors(filterStr);
		ui.treeWidget->clear();
		for(vector<string>::iterator it = matches.begin(); it != matches.end(); it++)
		{
				item = new QTreeWidgetItem(ui.treeWidget);
				item->setText(0, it->data());
		}
		ui.treeWidget->sortItems(0, Qt::AscendingOrder);
	}
	if(m_type == PersonalityDialog)
	{
		DrawPersonalities(filterStr);
	}
}

void NovaComplexDialog::on_searchEdit_textChanged()
{
	on_searchButton_clicked();
}
