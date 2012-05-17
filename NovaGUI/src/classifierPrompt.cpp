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
#include "Logger.h"

#include <QContextMenuEvent>
#include <QtGui/QMenu>
#include <sstream>

using namespace std;
using namespace Nova;

classifierPrompt::classifierPrompt(trainingDumpMap *trainingDump, QWidget *parent)
: QDialog(parent)
{
	ui.setupUi(this);
	m_groups = 0;
	m_menu = new QMenu(this);

	m_allowDescriptionEdit = true;

	m_suspects = new trainingSuspectMap();
	m_suspects->set_empty_key("");
	m_suspects->set_deleted_key("!");

	for(trainingDumpMap::iterator it = trainingDump->begin(); it != trainingDump->end(); it++)
	{
		(*m_suspects)[it->first] = new trainingSuspect();
		(*m_suspects)[it->first]->isIncluded = false;
		(*m_suspects)[it->first]->isHostile = false;
		(*m_suspects)[it->first]->uid = it->first;
		(*m_suspects)[it->first]->description = "Description for " + it->first;
		(*m_suspects)[it->first]->points = it->second;
	}

	DisplaySuspectEntries();
}

classifierPrompt::classifierPrompt(trainingSuspectMap *map, QWidget *parent)
: QDialog(parent)
{
	ui.setupUi(this);
	m_groups = 0;
	m_menu = new QMenu(this);
	m_suspects = map;
	m_allowDescriptionEdit = false;

	DisplaySuspectEntries();
}

void classifierPrompt::DisplaySuspectEntries()
{
	//ui.tableWidget->clear();
	for(int i=ui.tableWidget->rowCount()-1; i>=0; --i)
	{
	  ui.tableWidget->removeRow(i);
	}


	int row = 0;
	for(trainingSuspectMap::iterator it = m_suspects->begin(); it != m_suspects->end(); it++)
	{
		makeRow((*m_suspects)[it->first], row);
		row++;
	}
}

void classifierPrompt::contextMenuEvent(QContextMenuEvent *event)
{
	m_menu->clear();

	// Only display in edit mode
	if(m_allowDescriptionEdit)
	{
		m_menu->addAction(ui.actionCombineEntries);
	}

	m_menu->exec(event->globalPos());
}

void classifierPrompt::on_actionCombineEntries_triggered()
{
	vector<string> idsToMerge;

	// Figure out what's selected
	QList<QTableWidgetSelectionRange> selectRanges = ui.tableWidget->selectedRanges();
	for(int i =0; i != selectRanges.size(); ++i)
	{
		QTableWidgetSelectionRange range = selectRanges.at(i);
		int top = range.topRow();
		int bottom = range.bottomRow();
		for(int i = top; i <= bottom; ++i)
		{
			idsToMerge.push_back(ui.tableWidget->item(i, 2)->text().toStdString());
		}
	}

	// Return if less than 2 entries are trying to be combined
	if(idsToMerge.size() < 2)
	{
		return;
	}

	stringstream ss;
	ss << "Group " << m_groups;
	string rootuid = ss.str();
	m_groups++;
	(*m_suspects)[rootuid] = new trainingSuspect();
	(*m_suspects)[rootuid]->points = new vector<string>();
	(*m_suspects)[rootuid]->isIncluded = true;
	(*m_suspects)[rootuid]->isHostile = true;
	(*m_suspects)[rootuid]->uid = rootuid;
	(*m_suspects)[rootuid]->description = "User specified group";

	for(uint i = 0; i < idsToMerge.size(); i++)
	{
		string id = idsToMerge.at(i);

		if((*m_suspects)[id]->points == NULL)
		{
			cout << "Null points" << endl;
			continue;
		}

		// Append all the old suspect's points to our new group entry
		for(uint j = 0; j < (*m_suspects)[id]->points->size(); j++)
		{
			(*m_suspects)[rootuid]->points->push_back((*m_suspects)[id]->points->at(j));
		}

		delete (*m_suspects)[id]->points;
		delete (*m_suspects)[id];
		m_suspects->erase(id);
	}

	DisplaySuspectEntries();
}

void classifierPrompt::makeRow(trainingSuspect *header, int row)
{
	m_updating = true;

	ui.tableWidget->insertRow(row);

	QTableWidgetItem* chkBoxItem = new QTableWidgetItem();
	chkBoxItem->setFlags(chkBoxItem->flags() &= ~Qt::ItemIsEditable);

	QTableWidgetItem* includeItem = new QTableWidgetItem();
	includeItem->setFlags(includeItem->flags() &= ~Qt::ItemIsEditable);

	QTableWidgetItem* ipItem = new QTableWidgetItem();
	ipItem->setFlags(ipItem->flags() &= ~Qt::ItemIsEditable);

	QTableWidgetItem* descriptionItem = new QTableWidgetItem();

	if(m_allowDescriptionEdit)
	{
		descriptionItem->setFlags(descriptionItem->flags() |= Qt::ItemIsEditable);
	}
	else
	{
		descriptionItem->setFlags(descriptionItem->flags() &= ~Qt::ItemIsEditable);
	}

	ui.tableWidget->setItem(row, 0, chkBoxItem);
	ui.tableWidget->setItem(row, 1, includeItem);
	ui.tableWidget->setItem(row, 2, ipItem);
	ui.tableWidget->setItem(row, 3, descriptionItem);

	updateRow(header, row);

	m_updating = false;
}

void classifierPrompt::updateRow(trainingSuspect *header, int row)
{
	m_updating = true;

	if(!header->isIncluded)
	{
		ui.tableWidget->item(row,0)->setCheckState(Qt::Unchecked);
	}
	else
	{
		ui.tableWidget->item(row,0)->setCheckState(Qt::Checked);
	}

	if(!header->isHostile)
	{
		ui.tableWidget->item(row,1)->setCheckState(Qt::Unchecked);
	}
	else
	{
		ui.tableWidget->item(row,1)->setCheckState(Qt::Checked);
	}

	ui.tableWidget->item(row,2)->setText(QString::fromStdString(header->uid));
	ui.tableWidget->item(row,3)->setText(QString::fromStdString(header->description));

	m_updating = false;
}

void classifierPrompt::on_tableWidget_cellChanged(int row, int col)
{
	if(m_updating)
	{
		return;
	}

	trainingSuspect* header = (*m_suspects)[ui.tableWidget->item(row, 2)->text().toStdString()];

	switch(col)
	{
		case 0:
		{
			if(ui.tableWidget->item(row,0)->checkState() == Qt::Checked)
			{
				header->isIncluded = true;
			}
			else
			{
				header->isIncluded = false;
			}
			break;
		}
		case 1:
		{
			if(ui.tableWidget->item(row,1)->checkState() == Qt::Checked)
			{
				header->isHostile = true;
			}
			else
			{
				header->isHostile = false;
			}
			break;
		}
		case 3:
		{
			header->description = ui.tableWidget->item(row, 3)->text().toStdString();
			break;
		}
		default:
		{
			break;
		}
	}
}

classifierPrompt::~classifierPrompt()
{
	for(trainingSuspectMap::iterator it = m_suspects->begin(); it != m_suspects->end(); it++)
	{
		delete it->second;
	}
}

trainingSuspectMap *classifierPrompt::getStateData()
{
    return m_suspects;
}

void classifierPrompt::on_okayButton_clicked()
{
	accept();
}


void classifierPrompt::on_cancelButton_clicked()
{
	reject();
}
