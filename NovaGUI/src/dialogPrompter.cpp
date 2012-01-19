//============================================================================
// Name        : dialogPrompter.cpp
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Class to help display error messages to the user via the GUI
//============================================================================/*

#include "dialogPrompter.h"
#include "NovaUtil.h"
#include <sstream>
#include <QtGui>

// These are string messages corresponding to the messageType enums
// If you add a messageType enum, you must add a string for it here
// Note: You should format in such a way to allow for an extra argument string to be appended,
// e.g., "Unable to load file: " can be called with an arg to displayMessage with a filename to be appended
const char* dialogPrompter::messageTypeStrings[] = {
		"Error: Unable to read NOVA configuration file ",
		"Error: Unable to write to NOVA configuration file ",
		"This profile is currently in use. Are you sure you want to delete it?",
		"Error: Unable to read Honeyd configuration file ",
		"Error: Unexpected entry in file ",
		"Error: Unable to load subnets ",
		"Error: Unable to load nodes ",
		"Error: Unable to load profiles ",
		"Error: Unable to load profile sets ",
		"Error: Node at IP is outside all valid subnet ranges: ",
};

// Type of dialog message you want displayed
const dialogType dialogPrompter::messageTypeTypes[] = {
		DIALOG_NOTIFICATION,
		DIALOG_NOTIFICATION,
		DIALOG_YES_NO,
		DIALOG_NOTIFICATION,
		DIALOG_NOTIFICATION,
		DIALOG_NOTIFICATION,
		DIALOG_NOTIFICATION,
		DIALOG_NOTIFICATION,
		DIALOG_NOTIFICATION,
		DIALOG_NOTIFICATION,
};


// Prefixes for the configuration file
const string dialogPrompter::showPrefix = "message show";
const string dialogPrompter::hidePrefix = "message hide";
const string dialogPrompter::yesPrefix = "message default yes";
const string dialogPrompter::noPrefix = "message default no";

dialogPrompter::dialogPrompter()
{
	configurationFile = GetHomePath() + "/settings";
	loadDefaultActions();
}

dialogPrompter::dialogPrompter(string configurationFilePath)
{
	configurationFile = configurationFilePath;
	loadDefaultActions();
}


dialogPrompter::~dialogPrompter()
{
	// TODO Auto-generated destructor stub
}

void dialogPrompter::loadDefaultActions()
{
	ifstream config(configurationFile.data());
	string line;

	string type;
	messageType enumType;
	bool validType;
	defaultAction action;

	// Set everything to always show in case the settings file has no entry for one
	for (int i = 0; i < numberOfMessageTypes; i++)
		defaultActionToTake[i] = CHOICE_SHOW;

	if (config.is_open())
	{
		while (config.good())
		{
			if (!getline(config, line))
				continue;

			validType = false;

			// Extract the info from the file
			if (!line.substr(0, showPrefix.length()).compare(showPrefix))
			{
				action = CHOICE_SHOW;
				type = line.substr(showPrefix.length() + 2, line.length());
			}
			else if (!line.substr(0, hidePrefix.length()).compare(hidePrefix))
			{
				action = CHOICE_HIDE;
				type = line.substr(showPrefix.length() + 2, line.length());
			}
			else if (!line.substr(0, yesPrefix.length()).compare(yesPrefix))
			{
				action = CHOICE_ALWAYS_YES;
				type = line.substr(yesPrefix.length() + 2, line.length());
			}
			else if (!line.substr(0, noPrefix.length()).compare(noPrefix))
			{
				action = CHOICE_ALWAYS_NO;
				type = line.substr(noPrefix.length() + 2, line.length());
			}
			else
			{
				continue;
			}

			// Map the string to the messageType enum
			for (int i = 0; i < numberOfMessageTypes; i++)
				if (!type.compare(messageTypeStrings[i]))
				{
					enumType = (messageType)i;
					validType = true;
				}

			if (validType)
				defaultActionToTake[enumType] = action;
			else
			{
				syslog(SYSL_ERR, "File: %s Line: %d Error in settings file, message type does not exist: %s", __FILE__, __LINE__, type.c_str());
			}
		}
	}
	else
	{
		syslog(SYSL_ERR, "File: %s Line: %d Unable to open settings file: %s", __FILE__, __LINE__, configurationFile.c_str());
	}
}

void dialogPrompter::setDefaultAction(messageType msg, defaultAction action)
{
	// Set in our local sate
	defaultActionToTake[msg] = action;

	// Pull in the old config file
	ifstream config(configurationFile.data());
	string line, type;
	stringstream ss;

	// Try to change existing lines first. If !found, we append at the end
	bool found = false;

	if (config.is_open())
	{
		while (config.good())
		{
			if (!getline(config, line))
				continue;

			if (!line.substr(0, showPrefix.length()).compare(showPrefix))
				type = line.substr(showPrefix.length() + 2);
			else if (!line.substr(0, hidePrefix.length()).compare(hidePrefix))
				type = line.substr(hidePrefix.length() + 2);
			else if (!line.substr(0, noPrefix.length()).compare(noPrefix))
				type = line.substr(noPrefix.length() + 2);
			else if (!line.substr(0, yesPrefix.length()).compare(yesPrefix))
				type = line.substr(yesPrefix.length() + 2);

			// Found an entry
			if (!type.compare(messageTypeStrings[msg]))
			{
				found = true;
				ss << makeConfigurationLine(msg, action);
			}
			else
			{
				ss << line << endl;
			}
		}
	}

	// Append a new line if we didn't find one to edit
	if (!found)
		ss << makeConfigurationLine(msg, action);

	config.close();

	// Write out string version to the file
	ofstream outFile;
	outFile.open(configurationFile.data());
	outFile << ss.str();
	outFile.close();
}

string dialogPrompter::makeConfigurationLine(messageType msg, defaultAction action)
{
	stringstream ss;

	switch (action)
	{
	case CHOICE_SHOW:
		ss << showPrefix << " |" << messageTypeStrings[msg] << endl;
		break;
	case CHOICE_HIDE:
		ss << hidePrefix << " |" << messageTypeStrings[msg] << endl;
		break;
	case CHOICE_ALWAYS_YES:
		ss << yesPrefix << " |" << messageTypeStrings[msg] << endl;
		break;
	case CHOICE_ALWAYS_NO:
		ss << noPrefix << " |" << messageTypeStrings[msg] << endl;
		break;
	}

	return ss.str();
}

bool dialogPrompter::displayPrompt(messageType msg, string arg /*= ""*/)
{
	// Do we have a default action for this messageType?
	if (defaultActionToTake[msg] == CHOICE_HIDE)
		return true;
	else if (defaultActionToTake[msg] == CHOICE_ALWAYS_YES)
		return true;
	else if (defaultActionToTake[msg] == CHOICE_ALWAYS_NO)
		return false;


	// No default action, display the actual message
	QMessageBox *dialogBox = new QMessageBox();
	QLabel *checkBoxLabel = new QLabel();
	QCheckBox *checkBox = new QCheckBox();
	stringstream errorMessage;

	dialogType dialog = messageTypeTypes[msg];
	errorMessage << messageTypeStrings[msg] << arg;

	// Configure the buttons and checkbox text
	switch (dialog)
	{
	case DIALOG_YES_NO:
		checkBoxLabel->setText("Always take this action");
		dialogBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		break;
	case DIALOG_NOTIFICATION:
		checkBoxLabel->setText("Never show this type of error again");
		dialogBox->setStandardButtons(QMessageBox::Ok);
		break;
	}

	dialogBox->setText(errorMessage.str().c_str());

	// This is a bit hackish, but Qt doesn't have a yes/no dialog box with a "always do this" checkbox,
	// so this inserts one and shifts the buttons below it
	QLayoutItem *buttons = ((QGridLayout*)dialogBox->layout())->itemAtPosition(2, 0);
	((QGridLayout*)dialogBox->layout())->addWidget(checkBox, 2, 0);
	((QGridLayout*)dialogBox->layout())->addWidget(checkBoxLabel, 2, 1);
	((QGridLayout*)dialogBox->layout())->addWidget(buttons->widget(), 3, 0, 1, 2);

	// Prompt the user
	dialogBox->exec();

	// Note: If the user hits the X button, QMessageBox still sets result() automagically

	// Set the default action of needed, return true for okay/yes responses
	if (dialogBox->result() == QMessageBox::Ok)
	{
		if (checkBox->isChecked())
			setDefaultAction(msg, CHOICE_HIDE);
		return true;
	}
	else if (dialogBox->result() == QMessageBox::Yes)
	{
		if (checkBox->isChecked())
			setDefaultAction(msg, CHOICE_ALWAYS_YES);
		return true;
	}
	else if (dialogBox->result() == QMessageBox::No)
	{
		if (checkBox->isChecked())
			setDefaultAction(msg, CHOICE_ALWAYS_NO);
		return false;
	}
	else /* Shouldn't get here */
	{
		syslog(SYSL_ERR, "File: %s Line: %d Shouldn't get here. displayPrompt's result was invalid", __FILE__, __LINE__);
		return false;
	}
}
