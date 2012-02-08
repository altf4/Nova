//============================================================================
// Name        : DialogPrompt.cpp
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
// Description : General purpose warning/error/notification dialog prompts
//============================================================================

#include "DialogPrompt.h"


//Default constructor, used for instantiation only
DialogPrompt::DialogPrompt(QWidget *parent)
    : QMessageBox(parent)
{
	type = notSet;
	defaultAct = NULL;
	alternativeAct = NULL;
	checkBoxLabel = NULL;
	checkBox = NULL;
}

//Constructs a notification box, if it is a preventable warning then spawn the box before taking the action
// Describe the side effects of the warning if the action is taken, if the user presses cancel, don't continue
// If the error or notification will happen regardless simply display message and continue after the box is closed.
DialogPrompt::DialogPrompt(dialogType type, const QString &title, const QString &text, QWidget *parent)
    : QMessageBox(parent)
{
	this->setText(title);
	this->setWindowTitle(text);
	checkBoxLabel = new QLabel();
	checkBox = new QCheckBox();

	SetMessageType(type);
}

//Constructs a notification box that displays the problem, the default action and an alternative action
// message type sets the level, title summarizes the problem, text describes the choices
// default Action is performed on Yes, Alternative Action is performed on No
// returns -1 so the action that caused the prompt can be prevented
DialogPrompt::DialogPrompt(dialogType type, QAction * defaultAction, QAction * alternativeAction,
		const QString &title, const QString &text, QWidget *parent)
    : QMessageBox(parent)
{
	this->setText(title);
	this->setWindowTitle(text);
	this->defaultAct = defaultAction;
	this->alternativeAct = alternativeAction;
	this->checkBoxLabel = new QLabel();
	this->checkBox = new QCheckBox();

	SetMessageType(type);
}

DialogPrompt::~DialogPrompt()
{

}

defaultChoice DialogPrompt::exec()
{
	// This is a bit hackish, but Qt doesn't have a yes/no dialog box with a "always do this" checkbox,
	// so this inserts one and shifts the buttons below it
	QLayoutItem *buttons = ((QGridLayout*)layout())->itemAtPosition(2, 0);
	((QGridLayout*)layout())->addWidget(checkBoxLabel, 2, 1);
	((QGridLayout*)layout())->addWidget(checkBox, 2, 0);
	((QGridLayout*)layout())->addWidget(buttons->widget(), 3, 0, 1, 2);

	int selectedButton = QMessageBox::exec();

	switch(this->type)
	{
		case notifyPrompt:
		case warningPrompt:
		case errorPrompt:
			return CHOICE_HIDE;

		case warningPreventablePrompt:
			if (selectedButton == QMessageBox::Ok)
				return CHOICE_DEFAULT;
			else
				return CHOICE_ALT;

		case notifyActionPrompt:
		case warningActionPrompt:
			if (selectedButton == QMessageBox::Yes)
			{
				if (defaultAct != NULL)
					defaultAct->trigger();
				return CHOICE_DEFAULT;
			}
			else if (selectedButton == QMessageBox::No)
			{
				if (alternativeAct != NULL)
					alternativeAct->trigger();
				return CHOICE_ALT;
			}
			else
				return CHOICE_SHOW;
		default:
			return CHOICE_SHOW;
	}
}

//Sets the message type, if a question based type or preventable warning, if the user clicks cancel
// a value of -1 will be returned as a check to prevent the action.
void DialogPrompt::SetMessageType(dialogType type)
{
	this->type = type;
	switch(this->type)
	{
		case notifyPrompt:
			this->setIcon(QMessageBox::Information);
			setStandardButtons(QMessageBox::Ok);
			checkBoxLabel->setText(QString::fromStdString("Do not show this message again"));
			break;
		case warningPrompt:
			this->setIcon(QMessageBox::Warning);
			setStandardButtons(QMessageBox::Ok);
			checkBoxLabel->setText(QString::fromStdString("Do not show this warning again"));
			break;
		case errorPrompt:
			this->setIcon(QMessageBox::Critical);
			setStandardButtons(QMessageBox::Ok);
			checkBoxLabel->setText(QString::fromStdString("Do not show this message again"));
			break;
		case warningPreventablePrompt:
			this->setIcon(QMessageBox::Warning);
			this->setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
			checkBoxLabel->setText(QString::fromStdString("Always take this action"));
			break;
		case notifyActionPrompt:
			this->setIcon(QMessageBox::Question);
			QMessageBox::setStandardButtons(QMessageBox::No | QMessageBox::Yes | QMessageBox::Cancel);
			checkBoxLabel->setText(QString::fromStdString("Always take this option"));
			break;
		case warningActionPrompt:
			this->setIcon(QMessageBox::Warning);
			QMessageBox::setStandardButtons(QMessageBox::No | QMessageBox::Yes | QMessageBox::Cancel);
			checkBoxLabel->setText(QString::fromStdString("Always take this option"));
			break;
		default:
			this->setIcon(QMessageBox::NoIcon);
			break;
	}
}

//Sets the default action to take, create and action ahead of time and when the user clicks
// yes on a question based prompt that action will occur
void DialogPrompt::SetDefaultAction(QAction * action)
{
	defaultAct = action;
}

//Sets the alternative action to take, create and action ahead of time and when the user clicks
// No on a question based prompt that action will occur
void DialogPrompt::SetAlternativeAction(QAction * action)
{
	alternativeAct = action;
}

//Returns the message type a value of 0 here means the is currently no message type.
dialogType DialogPrompt::GetMessageType()
{
	return type;
}

//Returns a pointer to the default action
QAction * DialogPrompt::GetDefaultAction()
{
	return defaultAct;
}

//Returns a pointer to the alternative action
QAction * DialogPrompt::GetAlternativeAction()
{
	return alternativeAct;
}
