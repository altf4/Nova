#include "dialogPrompt.h"

//Default constructor, used for instantiation only
dialogPrompt::dialogPrompt(QWidget *parent)
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
dialogPrompt::dialogPrompt(dialogType type, const QString &title, const QString &text, QWidget *parent)
    : QMessageBox(parent)
{
	this->setText(title);
	this->setInformativeText(text);
	checkBoxLabel = new QLabel();
	checkBox = new QCheckBox();

	setMessageType(type);
}

//Constructs a notification box that displays the problem, the default action and an alternative action
// message type sets the level, title summarizes the problem, text describes the choices
// default Action is performed on Yes, Alternative Action is performed on No
// returns -1 so the action that caused the prompt can be prevented
dialogPrompt::dialogPrompt(enum dialogType, QAction * defaultAction, QAction * alternativeAction,
		const QString &title, const QString &text, QWidget *parent)
    : QMessageBox(parent)
{
	this->setText(title);
	this->setInformativeText(text);
	this->defaultAct = defaultAction;
	this->alternativeAct = alternativeAction;
	this->checkBoxLabel = new QLabel();
	this->checkBox = new QCheckBox();

	setMessageType(type);
}

dialogPrompt::~dialogPrompt()
{

}

defaultAction dialogPrompt::exec()
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
				return CHOICE_DEFAULT;
			if (selectedButton == QMessageBox::No)
				return CHOICE_ALT;
			else
				return CHOICE_SHOW;
		default:
			return CHOICE_SHOW;
	}
}

//Sets the message type, if a question based type or preventable warning, if the user clicks cancel
// a value of -1 will be returned as a check to prevent the action.
void dialogPrompt::setMessageType(dialogType type)
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
void dialogPrompt::setDefaultAction(QAction * action)
{
	defaultAct = action;
}

//Sets the alternative action to take, create and action ahead of time and when the user clicks
// No on a question based prompt that action will occur
void dialogPrompt::setAlternativeAction(QAction * action)
{
	alternativeAct = action;
}

//Returns the message type a value of 0 here means the is currently no message type.
dialogType dialogPrompt::getMessageType()
{
	return type;
}

//Returns a pointer to the default action
QAction * dialogPrompt::getDefaultAction()
{
	return defaultAct;
}

//Returns a pointer to the alternative action
QAction * dialogPrompt::getAlternativeAction()
{
	return alternativeAct;
}
