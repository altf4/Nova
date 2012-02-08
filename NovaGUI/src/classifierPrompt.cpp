//============================================================================
// Name        : classifierPrompt.cpp
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
// Description : Prompt for displaying and editing training data
//============================================================================

#include "classifierPrompt.h"
#include <QtGui>

classifierPrompt::classifierPrompt(QWidget *parent)
    : QDialog(parent)
{
	ui.setupUi(this);
	groups = 0;
	menu = new QMenu(this);
}

classifierPrompt::classifierPrompt(trainingDumpMap* trainingDump, QWidget *parent)
{
	parent = parent;
	ui.setupUi(this);
	groups = 0;
	menu = new QMenu(this);

	allowDescriptionEdit = true;

	suspects = new trainingSuspectMap();
	suspects->set_empty_key("");
	suspects->set_deleted_key("!");

	int row = 0;
	for (trainingDumpMap::iterator it = trainingDump->begin(); it != trainingDump->end(); it++)
	{
		(*suspects)[it->first] = new trainingSuspect();
		(*suspects)[it->first]->isIncluded = false;
		(*suspects)[it->first]->isHostile = false;
		(*suspects)[it->first]->uid = it->first;
		(*suspects)[it->first]->description = "Description for " + it->first;
		(*suspects)[it->first]->points = it->second;
	}

	DisplaySuspectEntries();
}

classifierPrompt::classifierPrompt(trainingSuspectMap* map, QWidget *parent)
{
	parent = parent;
	ui.setupUi(this);
	groups = 0;
	menu = new QMenu(this);
	suspects = map;
	allowDescriptionEdit = false;

	DisplaySuspectEntries();
}

void classifierPrompt::DisplaySuspectEntries()
{
	//ui.tableWidget->clear();
	for (int i=ui.tableWidget->rowCount()-1; i>=0; --i)
	  ui.tableWidget->removeRow(i);


	int row = 0;
	for (trainingSuspectMap::iterator it = suspects->begin(); it != suspects->end(); it++)
	{
		makeRow((*suspects)[it->first], row);
		row++;
	}
}

void classifierPrompt::contextMenuEvent(QContextMenuEvent * event)
{
	menu->clear();

	// Only display in edit mode
	if (allowDescriptionEdit)
		menu->addAction(ui.actionCombineEntries);

	menu->exec(event->globalPos());
}

void classifierPrompt::on_actionCombineEntries_triggered()
{
	vector<string> idsToMerge;

	// Figure out what's selected
	QList<QTableWidgetSelectionRange> selectRanges = ui.tableWidget->selectedRanges();
	for (int i =0; i != selectRanges.size(); ++i) {
		QTableWidgetSelectionRange range = selectRanges.at(i);
		int top = range.topRow();
		int bottom = range.bottomRow();
		for (int i = top; i <= bottom; ++i) {
			idsToMerge.push_back(ui.tableWidget->item(i, 2)->text().toStdString());
		}
	}

	// Return if less than 2 entries are trying to be combined
	if (idsToMerge.size() < 2)
		return;

	stringstream ss;
	ss << "Group " << groups;
	string rootuid = ss.str();
	groups++;
	(*suspects)[rootuid] = new trainingSuspect();
	(*suspects)[rootuid]->points = new vector<string>();
	(*suspects)[rootuid]->isIncluded = true;
	(*suspects)[rootuid]->isHostile = true;
	(*suspects)[rootuid]->uid = rootuid;
	(*suspects)[rootuid]->description = "User specified group";

	for (uint i = 0; i < idsToMerge.size(); i++)
	{
		string id = idsToMerge.at(i);

		if ((*suspects)[id]->points == NULL)
		{
			cout << "Null points" << endl;
			continue;
		}

		// Append all the old suspect's points to our new group entry
		for(uint j = 0; j < (*suspects)[id]->points->size(); j++)
		{
			(*suspects)[rootuid]->points->push_back((*suspects)[id]->points->at(j));
		}

		delete (*suspects)[id]->points;
		delete (*suspects)[id];
		suspects->erase(id);
	}

	DisplaySuspectEntries();
}

void classifierPrompt::makeRow(trainingSuspect* header, int row)
{
	updating = true;

	ui.tableWidget->insertRow(row);

	QTableWidgetItem* chkBoxItem = new QTableWidgetItem();
	chkBoxItem->setFlags(chkBoxItem->flags() &= ~Qt::ItemIsEditable);

	QTableWidgetItem* includeItem = new QTableWidgetItem();
	includeItem->setFlags(includeItem->flags() &= ~Qt::ItemIsEditable);

	QTableWidgetItem* ipItem = new QTableWidgetItem();
	ipItem->setFlags(ipItem->flags() &= ~Qt::ItemIsEditable);

	QTableWidgetItem* descriptionItem = new QTableWidgetItem();

	if (allowDescriptionEdit)
		descriptionItem->setFlags(descriptionItem->flags() |= Qt::ItemIsEditable);
	else
		descriptionItem->setFlags(descriptionItem->flags() &= ~Qt::ItemIsEditable);


	ui.tableWidget->setItem(row, 0, chkBoxItem);
	ui.tableWidget->setItem(row, 1, includeItem);
	ui.tableWidget->setItem(row, 2, ipItem);
	ui.tableWidget->setItem(row, 3, descriptionItem);

	updateRow(header, row);

	updating = false;
}

void classifierPrompt::updateRow(trainingSuspect* header, int row)
{
	updating = true;

	if (!header->isIncluded)
		ui.tableWidget->item(row,0)->setCheckState(Qt::Unchecked);
	else
		ui.tableWidget->item(row,0)->setCheckState(Qt::Checked);

	if (!header->isHostile)
		ui.tableWidget->item(row,1)->setCheckState(Qt::Unchecked);
	else
		ui.tableWidget->item(row,1)->setCheckState(Qt::Checked);

	ui.tableWidget->item(row,2)->setText(QString::fromStdString(header->uid));
	ui.tableWidget->item(row,3)->setText(QString::fromStdString(header->description));

	updating = false;
}

void classifierPrompt::on_tableWidget_cellChanged(int row, int col)
{
	if (updating)
		return;

	trainingSuspect* header = (*suspects)[ui.tableWidget->item(row, 2)->text().toStdString()];

	switch (col)
	{
	case 0:
		if (ui.tableWidget->item(row,0)->checkState() == Qt::Checked)
			header->isIncluded = true;
		else
			header->isIncluded = false;

		break;
	case 1:
		if (ui.tableWidget->item(row,1)->checkState() == Qt::Checked)
			header->isHostile = true;
		else
			header->isHostile = false;

		break;
	case 3:
		header->description = ui.tableWidget->item(row, 3)->text().toStdString();
		break;
	}
}

classifierPrompt::~classifierPrompt()
{
	for (trainingSuspectMap::iterator it = suspects->begin(); it != suspects->end(); it++)
		delete it->second;
}

trainingSuspectMap* classifierPrompt::getStateData()
{
    return suspects;
}

void classifierPrompt::on_okayButton_clicked()
{
	accept();
}


void classifierPrompt::on_cancelButton_clicked()
{
	reject();
}
