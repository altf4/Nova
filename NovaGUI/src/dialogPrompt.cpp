#include "dialogPrompt.h"

//Default constructor, used for instantiation only
dialogPrompt::dialogPrompt(QWidget *parent)
    : QMessageBox(parent)
{
	type = notSet;
	defaultAct = NULL;
	alternativeAct = NULL;
}

//Constructs a notification box, if it is a preventable warning then spawn the box before taking the action
// Describe the side effects of the warning if the action is taken, if the user presses cancel, don't continue
// If the error or notification will happen regardless simply display message and continue after the box is closed.
dialogPrompt::dialogPrompt(messageType type, const QString &title, const QString &text, QWidget *parent)
    : QMessageBox(parent)
{
	this->type = type;
	this->setText(title);
	this->setInformativeText(text);
	setStandardButtons(QMessageBox::Ok);
	switch(this->type)
	{
		case notifyPrompt:
			this->setIcon(QMessageBox::Information);
			break;
		case warningPrompt:
			this->setIcon(QMessageBox::Warning);
			break;
		case errorPrompt:
			this->setIcon(QMessageBox::Critical);
			break;
		case warningPreventablePrompt:
			this->setIcon(QMessageBox::Warning);
			this->setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
			break;
		default:
			this->setIcon(QMessageBox::NoIcon);
			break;
	}
}

//Constructs a notification box that displays the problem, the default action and an alternative action
// message type sets the level, title summarizes the problem, text describes the choices
// default Action is performed on Yes, Alternative Action is performed on No
// returns -1 so the action that caused the prompt can be prevented
dialogPrompt::dialogPrompt(enum messageType, QAction * defaultAction, QAction * alternativeAction,
		const QString &title, const QString &text, QWidget *parent)
    : QMessageBox(parent)
{
	this->type = type;
	this->setText(title);
	this->setInformativeText(text);
	QMessageBox::setStandardButtons(QMessageBox::No | QMessageBox::Yes | QMessageBox::Cancel);
	this->defaultAct = defaultAction;
	this->alternativeAct = alternativeAction;

	switch(this->type)
	{
		case notifyActionPrompt:
			this->setIcon(QMessageBox::Question);
			break;
		case warningActionPrompt:
			this->setIcon(QMessageBox::Warning);
			break;
		default:
			this->setIcon(QMessageBox::NoIcon);
			break;
	}
}

dialogPrompt::~dialogPrompt()
{

}

int dialogPrompt::exec()
{
	switch(QMessageBox::exec())
	{
		case QMessageBox::No:
			alternativeAct->trigger();
			return 1;
		case QMessageBox::Yes:
			defaultAct->trigger();
		case QMessageBox::Ok:
			return 0;
		case QMessageBox::Cancel:
		default:
			return -1;
	}
}

//Sets the message type, if a question based type or preventable warning, if the user clicks cancel
// a value of -1 will be returned as a check to prevent the action.
void dialogPrompt::setMessageType(messageType type)
{
	this->type = type;
	switch(this->type)
	{
		case notifyPrompt:
			this->setIcon(QMessageBox::Information);
			setStandardButtons(QMessageBox::Ok);
			break;
		case warningPrompt:
			this->setIcon(QMessageBox::Warning);
			setStandardButtons(QMessageBox::Ok);
			break;
		case errorPrompt:
			this->setIcon(QMessageBox::Critical);
			setStandardButtons(QMessageBox::Ok);
			break;
		case warningPreventablePrompt:
			this->setIcon(QMessageBox::Warning);
			this->setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
			break;
		case notifyActionPrompt:
			this->setIcon(QMessageBox::Question);
			QMessageBox::setStandardButtons(QMessageBox::No | QMessageBox::Yes | QMessageBox::Cancel);
			break;
		case warningActionPrompt:
			this->setIcon(QMessageBox::Warning);
			QMessageBox::setStandardButtons(QMessageBox::No | QMessageBox::Yes | QMessageBox::Cancel);
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
messageType dialogPrompt::getMessageType()
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
