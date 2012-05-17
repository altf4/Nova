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
#include "Logger.h"
#include "Config.h"

#include <QAction>
#include <sstream>
#include <fstream>

using namespace std;
using namespace Nova;

// Prefixes for the configuration file
const string DialogPrompter::m_showPrefix = "message show";
const string DialogPrompter::m_hidePrefix = "message hide";
const string DialogPrompter::m_yesPrefix = "message default yes";
const string DialogPrompter::m_noPrefix = "message default no";

DialogPrompter::DialogPrompter(string configurationFilePath)
{
	if(!configurationFilePath.compare(""))
	{
		m_configurationFile = Config::Inst()->GetPathHome() + "/settings";
	}
	LoadDefaultActions();
}

messageHandle DialogPrompter::RegisterDialog(messageType t)
{
	for(uint i = 0; i < m_registeredMessageTypes.size(); i++)
	{
		if(!m_registeredMessageTypes[i].descriptionUID.compare(t.descriptionUID)
			&& m_registeredMessageTypes[i].type == t.type)
		{
			return i;
		}
	}
	m_registeredMessageTypes.push_back(t);
	return (m_registeredMessageTypes.size() - 1);
}

void DialogPrompter::LoadDefaultActions()
{
	ifstream config(m_configurationFile.data());
	string line;

	string description, prefix;
	dialogType type;
	defaultChoice action;

	if(config.is_open())
	{
		while (config.good())
		{
			if(!getline(config, line))
			{
				continue;
			}

			// Extract the info from the file
			if(!line.substr(0, m_showPrefix.length()).compare(m_showPrefix))
			{
				action = CHOICE_SHOW;
				prefix = m_showPrefix;
			}
			else if(!line.substr(0, m_hidePrefix.length()).compare(m_hidePrefix))
			{
				action = CHOICE_HIDE;
				prefix = m_hidePrefix;
			}
			else if(!line.substr(0, m_yesPrefix.length()).compare(m_yesPrefix))
			{
				action = CHOICE_DEFAULT;
				prefix = m_yesPrefix;
			}
			else if(!line.substr(0, m_noPrefix.length()).compare(m_noPrefix))
			{
				action = CHOICE_ALT;
				prefix = m_noPrefix;
			}
			else
			{
				continue;
			}

			line = line.substr(m_showPrefix.length() + 1);
			type = (dialogType)atoi(line.substr(0, line.find_first_of(" ")).c_str());
			description = line.substr(line.find_first_of(" ") + 1);

			messageType* t = new messageType();
			t->action = action;
			t->descriptionUID = description;
			t->type = type;

			m_registeredMessageTypes.push_back(*t);
		}
	}
	else
	{
		LOG(ERROR, "Unable to open settings file: "+m_configurationFile,"");
	}
}

void DialogPrompter::SetDefaultAction(messageHandle msg, defaultChoice action)
{
	// Set in our local sate
	m_registeredMessageTypes[msg].action = action;

	// Pull in the old config file
	ifstream config(m_configurationFile.data());
	string line, trimmedLine, description, prefix;
	dialogType type;
	stringstream ss;

	// Try to change existing lines first. If !found, we append at the end
	bool found = false;

	if(config.is_open())
	{
		while (config.good())
		{
			if(!getline(config, line))
			{
				continue;
			}
			// Extract the info from the file
			if(!line.substr(0, m_showPrefix.length()).compare(m_showPrefix))
			{
				prefix = m_showPrefix;
			}
			else if(!line.substr(0, m_hidePrefix.length()).compare(m_hidePrefix))
			{
				prefix = m_hidePrefix;
			}
			else if(!line.substr(0, m_yesPrefix.length()).compare(m_yesPrefix))
			{
				prefix = m_yesPrefix;
			}
			else if(!line.substr(0, m_noPrefix.length()).compare(m_noPrefix))
			{
				prefix = m_noPrefix;
			}
			else
			{
				prefix = "";
			}

			if(prefix.compare(""))
			{
				// Trim off the prefix
				trimmedLine = line.substr(m_showPrefix.length() + 1);

				type = (dialogType)atoi(trimmedLine.substr(0, trimmedLine.find_first_of(" ")).c_str());
				description = trimmedLine.substr(trimmedLine.find_first_of(" ") + 1);

				// Found an entry
				if(!description.compare(m_registeredMessageTypes[msg].descriptionUID) && type == m_registeredMessageTypes[msg].type)
				{
					found = true;
					ss << MakeConfigurationLine(msg, action);
				}
				else
				{
					ss << line << endl;
				}
			}
			else
			{
				ss << line << endl;
			}
		}
	}

	// Append a new line if we didn't find one to edit
	if(!found)
	{
		ss << MakeConfigurationLine(msg, action);
	}

	config.close();

	// Write out string version to the file
	ofstream outFile;
	outFile.open(m_configurationFile.data());
	outFile << ss.str();
	outFile.close();
}

string DialogPrompter::MakeConfigurationLine(messageHandle msg, defaultChoice action)
{
	stringstream ss;
	switch (action)
	{
		case CHOICE_SHOW:
		{
			ss << m_showPrefix;
			break;
		}
		case CHOICE_HIDE:
		{
			ss << m_hidePrefix;
			break;
		}
		case CHOICE_DEFAULT:
		{
			ss << m_yesPrefix;
			break;
		}
		case CHOICE_ALT:
		{
			ss << m_noPrefix;
			break;
		}
		default:
		{
			break;
		}
	}
	ss  << " " << m_registeredMessageTypes[msg].type << " " <<  m_registeredMessageTypes[msg].descriptionUID << endl;
	return ss.str();
}

defaultChoice DialogPrompter::DisplayPrompt(messageHandle handle, string messageTxt,
		QAction *defaultAction, QAction *alternativeAction, QWidget *parent /*= 0*/)
{
	// Do we have a default action for this messageType?
	if(m_registeredMessageTypes[handle].action == CHOICE_HIDE)
	{
		return CHOICE_DEFAULT;
	}
	else if(m_registeredMessageTypes[handle].action == CHOICE_DEFAULT)
	{
		if(defaultAction != NULL)
		{
			defaultAction->trigger();
		}
		return CHOICE_DEFAULT;
	}
	else if(m_registeredMessageTypes[handle].action == CHOICE_ALT)
	{
		if(alternativeAction != NULL)
		{
			alternativeAction->trigger();
		}
		return CHOICE_ALT;
	}

	dialogType dialog = m_registeredMessageTypes[handle].type;

	DialogPrompt *dialogBox = new DialogPrompt(dialog, defaultAction, alternativeAction,
		QString::fromStdString(messageTxt),
		QString::fromStdString(m_registeredMessageTypes[handle].descriptionUID), parent);

	// Prompt the user
	defaultChoice action = dialogBox->exec();

	if(dialogBox->m_checkBox->isChecked())
	{
		SetDefaultAction(handle, action);
	}
	return action;
}

defaultChoice DialogPrompter::DisplayPrompt(messageHandle handle, string messageTxt, QWidget *parent /* = 0*/)
{
	return DisplayPrompt(handle, messageTxt, NULL, NULL, parent);
}
