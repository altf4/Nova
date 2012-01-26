//============================================================================
// Name        : DialogPrompter.cpp
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
// Description : Class to help display error messages to the user via the GUI
//============================================================================
#include "DialogPrompter.h"
#include "NovaUtil.h"
#include <sstream>
#include <QtGui>

// Prefixes for the configuration file
const string DialogPrompter::showPrefix = "message show";
const string DialogPrompter::hidePrefix = "message hide";
const string DialogPrompter::yesPrefix = "message default yes";
const string DialogPrompter::noPrefix = "message default no";


DialogPrompter::DialogPrompter(string configurationFilePath /*= ""*/)
{
	if (!configurationFilePath.compare(""))
		configurationFile = GetHomePath() + "/settings";
	LoadDefaultActions();
}


messageHandle DialogPrompter::RegisterDialog(messageType t)
{
	for (uint i = 0; i < registeredMessageTypes.size(); i++)
	{
		if (!registeredMessageTypes[i].descriptionUID.compare(t.descriptionUID) && registeredMessageTypes[i].type == t.type)
			return i;
	}
	registeredMessageTypes.push_back(t);
	return (registeredMessageTypes.size() - 1);
}


void DialogPrompter::LoadDefaultActions()
{
	ifstream config(configurationFile.data());
	string line;

	string description, prefix;
	dialogType type;
	defaultChoice action;

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

			messageType* t = new messageType();
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


void DialogPrompter::SetDefaultAction(messageHandle msg, defaultChoice action)
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
					ss << MakeConfigurationLine(msg, action);
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
		ss << MakeConfigurationLine(msg, action);

	config.close();

	// Write out string version to the file
	ofstream outFile;
	outFile.open(configurationFile.data());
	outFile << ss.str();
	outFile.close();
}


string DialogPrompter::MakeConfigurationLine(messageHandle msg, defaultChoice action)
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


defaultChoice DialogPrompter::DisplayPrompt(messageHandle handle, string messageTxt, QAction * defaultAction, QAction * alternativeAction, QWidget *parent /*= 0*/)
{
	// Do we have a default action for this messageType?
	if (registeredMessageTypes[handle].action == CHOICE_HIDE)
	{
		return CHOICE_DEFAULT;
	}
	else if (registeredMessageTypes[handle].action == CHOICE_DEFAULT)
	{
		if (defaultAction != NULL)
			defaultAction->trigger();
		return CHOICE_DEFAULT;
	}
	else if (registeredMessageTypes[handle].action == CHOICE_ALT)
	{
		if (alternativeAction != NULL)
			alternativeAction->trigger();
		return CHOICE_ALT;
	}

	dialogType dialog = registeredMessageTypes[handle].type;

	DialogPrompt *dialogBox = new DialogPrompt(dialog, defaultAction, alternativeAction,
			QString::fromStdString(messageTxt),
			QString::fromStdString(registeredMessageTypes[handle].descriptionUID), parent);

	// Prompt the user
	defaultChoice action = dialogBox->exec();

	if (dialogBox->checkBox->isChecked())
				SetDefaultAction(handle, action);

	return action;
}

defaultChoice DialogPrompter::DisplayPrompt(messageHandle handle, string messageTxt, QWidget *parent /* = 0*/)
{
	return DisplayPrompt(handle, messageTxt, NULL, NULL, parent);
}
