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

messageType dialogPrompter::registerDialog(dialogMessageType t)
{
	for (uint i = 0; i < registeredMessageTypes.size(); i++)
	{
		if (!registeredMessageTypes[i].descriptionUID.compare(t.descriptionUID) && registeredMessageTypes[i].type == t.type)
			return i;
	}
	registeredMessageTypes.push_back(t);
	return (registeredMessageTypes.size() - 1);
}

void dialogPrompter::loadDefaultActions()
{
	ifstream config(configurationFile.data());
	string line;

	string description, prefix;
	dialogType type;
	defaultAction action;

	if (config.is_open())
	{
		while (config.good())
		{
			if (!getline(config, line))
				continue;

			// Extract the info from the file
			if (!line.substr(0, showPrefix.length()).compare(showPrefix))
			{
				action = CHOICE_SHOW;
				prefix = showPrefix;
			}
			else if (!line.substr(0, hidePrefix.length()).compare(hidePrefix))
			{
				action = CHOICE_HIDE;
				prefix = hidePrefix;
			}
			else if (!line.substr(0, yesPrefix.length()).compare(yesPrefix))
			{
				action = CHOICE_DEFAULT;
				prefix = yesPrefix;
			}
			else if (!line.substr(0, noPrefix.length()).compare(noPrefix))
			{
				action = CHOICE_ALT;
				prefix = noPrefix;
			}
			else
			{
				continue;
			}

			line = line.substr(showPrefix.length() + 1);
			type = (dialogType)atoi(line.substr(0, line.find_first_of(" ")).c_str());
			description = line.substr(line.find_first_of(" ") + 1);

			dialogMessageType* t = new dialogMessageType();
			t->action = action;
			t->descriptionUID = description;
			t->type = type;

			registeredMessageTypes.push_back(*t);
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
	registeredMessageTypes[msg].action = action;

	// Pull in the old config file
	ifstream config(configurationFile.data());
	string line, trimmedLine, description, prefix;
	dialogType type;
	stringstream ss;

	// Try to change existing lines first. If !found, we append at the end
	bool found = false;

	if (config.is_open())
	{
		while (config.good())
		{
			if (!getline(config, line))
				continue;

			// Extract the info from the file
			if (!line.substr(0, showPrefix.length()).compare(showPrefix))
				prefix = showPrefix;
			else if (!line.substr(0, hidePrefix.length()).compare(hidePrefix))
				prefix = hidePrefix;
			else if (!line.substr(0, yesPrefix.length()).compare(yesPrefix))
				prefix = yesPrefix;
			else if (!line.substr(0, noPrefix.length()).compare(noPrefix))
				prefix = noPrefix;
			else
				prefix = "";

			if (prefix.compare(""))
			{
				// Trim off the prefix
				trimmedLine = line.substr(showPrefix.length() + 1);

				type = (dialogType)atoi(trimmedLine.substr(0, trimmedLine.find_first_of(" ")).c_str());
				description = trimmedLine.substr(trimmedLine.find_first_of(" ") + 1);

				// Found an entry
				if (!description.compare(registeredMessageTypes[msg].descriptionUID) && type == registeredMessageTypes[msg].type)
				{
					found = true;
					ss << makeConfigurationLine(msg, action);
				}
				else
					ss << line << endl;
			}
			else
				ss << line << endl;

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
		ss << showPrefix;
		break;
	case CHOICE_HIDE:
		ss << hidePrefix;
		break;
	case CHOICE_DEFAULT:
		ss << yesPrefix;
		break;
	case CHOICE_ALT:
		ss << noPrefix;
		break;
	}

	ss  << " " << registeredMessageTypes[msg].type << " " <<  registeredMessageTypes[msg].descriptionUID << endl;

	return ss.str();
}

defaultAction dialogPrompter::displayPrompt(messageType msg, string text /*= ""*/)
{
	// Do we have a default action for this messageType?
	if (registeredMessageTypes[msg].action == CHOICE_HIDE)
		return CHOICE_DEFAULT;
	else if (registeredMessageTypes[msg].action == CHOICE_DEFAULT)
		return CHOICE_DEFAULT;
	else if (registeredMessageTypes[msg].action == CHOICE_ALT)
		return CHOICE_ALT;


	dialogType dialog = registeredMessageTypes[msg].type;

	dialogPrompt *dialogBox = new dialogPrompt(dialog, QString::fromStdString(text), QString::fromStdString(""));

	// Prompt the user
	defaultAction action = dialogBox->exec();

	if (dialogBox->checkBox->isChecked())
				setDefaultAction(msg, action);

	return action;
}
