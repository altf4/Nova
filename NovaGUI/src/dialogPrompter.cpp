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


const string dialogPrompter::showPrefix = "message show";
const string dialogPrompter::hidePrefix = "message hide";
const string dialogPrompter::defaultPrefix = "message default";

// These are string versions of the enum names used in the settings file
// If you add a messageType enum, you must add a string name for it here (same as enum name is fine)
const char* dialogPrompter::messageTypeStrings[] = {
		"CONFIG_READ_FAIL",
		"CONFIG_WRITE_FAIL",
		"DELETE_USED_PROFILE",
		"HONEYD_FILE_READ_FAIL",
		"UNEXPECTED_FILE_ENTRY",
		"HONEYD_LOAD_SUBNETS_FAIL",
		"HONEYD_LOAD_NODES_FAIL",
		"HONEYD_LOAD_PROFILES_FAIL",
		"HONEYD_LOAD_PROFILESET_FAIL",
		"HONEYD_NODE_INVALID_SUBNET",
};

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
				type = line.substr(showPrefix.length() + 1, line.length());
			}
			else if (!line.substr(0, hidePrefix.length()).compare(hidePrefix))
			{
				action = CHOICE_HIDE;
				type = line.substr(showPrefix.length() + 1, line.length());
			}
			else if (!line.substr(0, defaultPrefix.length()).compare(defaultPrefix))
			{
				string tmp = line.substr(defaultPrefix.length() + 1);
				type = tmp.substr(0, tmp.find_first_of(' '));
				string defaultAnswer = tmp.substr(tmp.find_first_of(' ') + 1);

				if (!defaultAnswer.compare("yes"))
				{
					action = CHOICE_ALWAYS_YES;
				}
				else if (!defaultAnswer.compare("no"))
				{
					action = CHOICE_ALWAYS_NO;
				}
				else
				{
					action = CHOICE_SHOW;
				}
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
				syslog(SYSL_ERR, "Line: %d Error in settings file, message type does not exist: %s", __LINE__, type.c_str());
			}
		}
	}
	else
	{
		syslog(SYSL_ERR, "Line: %d Unable to open settings file: %s", __LINE__, configurationFile.c_str());
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

	// Try to change existing values first. If !found, we append at the end
	bool found = false;

	if (config.is_open())
	{
		while (config.good())
		{
			getline(config, line);

			if (!line.substr(0, showPrefix.length()).compare(showPrefix))
				type = line.substr(showPrefix.length() + 1);
			else if (!line.substr(0, hidePrefix.length()).compare(hidePrefix))
				type = line.substr(hidePrefix.length() + 1);
			else if (!line.substr(0, defaultPrefix.length()).compare(defaultPrefix))
			{
				string tmp = line.substr(defaultPrefix.length() + 1);
				type = tmp.substr(0, tmp.find_first_of(' '));
			}

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
		ss << showPrefix << " " << messageTypeStrings[msg] << endl;
		break;
	case CHOICE_HIDE:
		ss << hidePrefix << " " << messageTypeStrings[msg] << endl;
		break;
	case CHOICE_ALWAYS_YES:
		ss << defaultPrefix << " " << messageTypeStrings[msg] << " yes" << endl;
		break;
	case CHOICE_ALWAYS_NO:
		ss << defaultPrefix << " " << messageTypeStrings[msg] << " no" << endl;
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

	dialogType dialog = DIALOG_NOTIFICATION;
	stringstream errorMessage;

	// Put new predefined error message strings here and define their dialogType
	switch (msg)
	{
	case CONFIG_READ_FAIL:
		errorMessage << "Error: Unable to read NOVA configuration file " << arg;
		break;
	case CONFIG_WRITE_FAIL:
		errorMessage << "Error: Unable to write to NOVA configuration file "  << arg;
		break;
	case DELETE_USED_PROFILE:
		errorMessage << "Error: This profile is currently in use. Are you sure you want to delete it?";
		dialog = DIALOG_YES_NO;
		break;
	case HONEYD_FILE_READ_FAIL:
		errorMessage << "Error: Unable to read Honeyd configuration file " << arg;
		break;
	case UNEXPECTED_FILE_ENTRY:
		errorMessage << "Error: Unexpected entry in file " << arg;
		break;
	case HONEYD_LOAD_SUBNETS_FAIL:
		errorMessage << "Error: Unable to load subnets " << arg;
		break;
	case HONEYD_LOAD_NODES_FAIL:
		errorMessage << "Error: Unable to load nodes " << arg;
		break;
	case HONEYD_LOAD_PROFILES_FAIL:
		errorMessage << "Error: Unable to load profiles " << arg;
		break;
	case HONEYD_LOAD_PROFILESET_FAIL:
		errorMessage << "Error: Unable to load profile sets " << arg;
		break;
	case HONEYD_NODE_INVALID_SUBNET:
		errorMessage << "Node at IP: " << arg << "is outside all valid subnet ranges";
		break;
	}

	// Configure the buttons and checkbox text
	switch (dialog)
	{
	case DIALOG_YES_NO:
		checkBoxLabel->setText("Always take this action");
		dialogBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
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
		syslog(SYSL_ERR, "Line: %d Shouldn't get here. displayPrompt's result was invalid", __LINE__);
		return false;
	}
}
