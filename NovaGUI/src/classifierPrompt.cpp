#include "classifierPrompt.h"

classifierPrompt::classifierPrompt(QWidget *parent)
    : QDialog(parent)
{
	ui.setupUi(this);
}

classifierPrompt::classifierPrompt(TrainingHashTable* trainingDump, QWidget *parent)
{
	ui.setupUi(this);
	suspects.set_empty_key("");
	updating = false;

	int row = 0;
	for (TrainingHashTable::iterator it = trainingDump->begin(); it != trainingDump->end(); it++)
	{
		// Set up the data in our struct
		suspects[it->first] = new suspectHeader();
		suspects[it->first]->isIncluded = true;
		suspects[it->first]->isHostile = false;
		suspects[it->first]->ip = it->first;
		suspects[it->first]->description = "Description for " + it->first;

		makeRow(suspects[it->first], row);
	}
}

void classifierPrompt::makeRow(suspectHeader* header, int row)
{
	updating = true;

	ui.tableWidget->insertRow(row);

	QTableWidgetItem* chkBoxItem = new QTableWidgetItem();
	chkBoxItem->setFlags(chkBoxItem->flags() &= ~Qt::ItemIsEditable);

	QTableWidgetItem* includeItem = new QTableWidgetItem();
	includeItem->setFlags(includeItem->flags() &= ~Qt::ItemIsEditable);

	QTableWidgetItem* ipItem = new QTableWidgetItem();
	ipItem->setFlags(ipItem->flags() &= ~Qt::ItemIsEditable);

	ui.tableWidget->setItem(row, 0, chkBoxItem);
	ui.tableWidget->setItem(row, 1, includeItem);
	ui.tableWidget->setItem(row, 2, ipItem);
	ui.tableWidget->setItem(row, 3, new QTableWidgetItem());

	updateRow(header, row);

	updating = false;
}

void classifierPrompt::updateRow(suspectHeader* header, int row)
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

	ui.tableWidget->item(row,2)->setText(QString::fromStdString(header->ip));
	ui.tableWidget->item(row,3)->setText(QString::fromStdString(header->description));

	updating = false;
}

void classifierPrompt::on_tableWidget_cellChanged(int row, int col)
{
	if (updating)
		return;

	suspectHeader* header = suspects[ui.tableWidget->item(row, 2)->text().toStdString()];

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

}

vector<string>* classifierPrompt::getHostiles()
{
    return NULL;
}
