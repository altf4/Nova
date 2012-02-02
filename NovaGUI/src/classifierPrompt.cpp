#include "classifierPrompt.h"

classifierPrompt::classifierPrompt(QWidget *parent)
    : QDialog(parent)
{
	ui.setupUi(this);
}

classifierPrompt::classifierPrompt(trainingDumpMap* trainingDump, QWidget *parent)
{
	ui.setupUi(this);

	suspects = new trainingSuspectMap();
	suspects->set_empty_key("");

	int row = 0;
	for (trainingDumpMap::iterator it = trainingDump->begin(); it != trainingDump->end(); it++)
	{
		(*suspects)[it->first] = new trainingSuspect();
		(*suspects)[it->first]->isIncluded = true;
		(*suspects)[it->first]->isHostile = false;
		(*suspects)[it->first]->uid = it->first;
		(*suspects)[it->first]->description = "Description for " + it->first;

		makeRow((*suspects)[it->first], row);
	}

	allowDescriptionEdit = true;
}

classifierPrompt::classifierPrompt(trainingSuspectMap* map, QWidget *parent)
{
	ui.setupUi(this);
	suspects = map;

	int row = 0;
	for (trainingSuspectMap::iterator it = map->begin(); it != map->end(); it++)
	{
		makeRow((*suspects)[it->first], row);
		row++;
	}

	allowDescriptionEdit = false;
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
	if (!allowDescriptionEdit)
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
