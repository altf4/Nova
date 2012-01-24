#include "NovaComplexDialog.h"

NovaComplexDialog::NovaComplexDialog(QWidget *parent)
    : QDialog(parent)
{
	novaParent = (NovaConfig *)parent;
	ui.setupUi(this);
	type = MACDialog;
}
NovaComplexDialog::NovaComplexDialog(whichDialog type, QWidget *parent)
	: QDialog(parent)
{
	novaParent = (NovaConfig *)parent;
	this->type = type;
	ui.setupUi(this);
	switch(type)
	{
		case PersonalityDialog:
			this->setWindowTitle((const QString&)"Select an Operating System");
			ui.displayLabel->setText((const QString&)"Select which operating system you would like to emulate");
			ui.helpLabel->setText((const QString&)"Navigate down the tree, select an operating system and press 'Select'");
			ui.searchEdit->setEnabled(false);
			ui.searchEdit->setVisible(false);
			ui.selectButton->setDefault(true);
			ui.searchButton->setEnabled(false);
			ui.searchButton->setVisible(false);
			break;
		case MACDialog:
			ui.searchButton->setDefault(true);
			this->setWindowTitle((const QString&)"Select a MAC Vendor");
		default:
			break;
	}
	on_searchButton_clicked();
}

NovaComplexDialog::~NovaComplexDialog()
{

}

//Sets the help message displayed at the bottom of the window to provide the user hints on usage.
void NovaComplexDialog::setHelpMsg(const QString& str)
{
	ui.helpLabel->setText(str);
}

//Returns the help message displayed at the bottom of the window to provide the user hints on usage.
QString NovaComplexDialog::getHelpMsg()
{
	return ui.helpLabel->text();
}

//Sets the description at the top on the window to inform the user of the window's purpose.
void NovaComplexDialog::setInfoMsg(const QString& str)
{
	ui.displayLabel->setText(str);
}

//Returns the description at the top on the window to inform the user of the window's purpose.
QString NovaComplexDialog::getInfoMsg()
{
	return ui.displayLabel->text();
}

//Returns the object's 'type' enum
whichDialog NovaComplexDialog::getType()
{
	return type;
}

void NovaComplexDialog::on_cancelButton_clicked()
{
	this->close();
}

void NovaComplexDialog::on_selectButton_clicked()
{
	if(ui.treeWidget->currentItem() != NULL)
	{
		string ret = ui.treeWidget->currentItem()->text(0).toStdString();
		novaParent->retVal = ret;
		this->close();
	}
}

void NovaComplexDialog::on_searchButton_clicked()
{
	QTreeWidgetItem * item = NULL;
	VendorToMACTable * table = &novaParent->VendorMACTable;
	string filterStr = ui.searchEdit->text().toStdString();
	bool matched = false;
	ui.treeWidget->clear();
	for(VendorToMACTable::iterator it = table->begin(); it != table->end(); it++)
	{
		matched = true;
		for(uint i = 0; i < filterStr.size(); i++)
		{
			if(it->first[i] != filterStr[i])
			{
				matched = false;
				break;
			}
		}
		if(matched)
		{
			item = new QTreeWidgetItem(ui.treeWidget);
			item->setText(0, it->first.data());
		}
	}
	ui.treeWidget->sortItems(0, Qt::AscendingOrder);
}

void NovaComplexDialog::on_searchEdit_textChanged()
{
	QTreeWidgetItem * item = NULL;
	VendorToMACTable * table = &novaParent->VendorMACTable;
	string filterStr = ui.searchEdit->text().toStdString();
	bool matched = false;
	ui.treeWidget->clear();
	for(VendorToMACTable::iterator it = table->begin(); it != table->end(); it++)
	{
		matched = true;
		for(uint i = 0; i < filterStr.size(); i++)
		{
			if(it->first[i] != filterStr[i])
			{
				matched = false;
				break;
			}
		}
		if(matched)
		{
			item = new QTreeWidgetItem(ui.treeWidget);
			item->setText(0, it->first.data());
		}
	}
	ui.treeWidget->sortItems(0, Qt::AscendingOrder);
}
