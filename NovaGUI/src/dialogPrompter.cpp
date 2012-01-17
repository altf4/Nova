/*
 * dialogPrompter.cpp
 *
 *  Created on: Jan 17, 2012
 *      Author: root
 */

#include "dialogPrompter.h"
#include "NovaUtil.h"


dialogPrompter::dialogPrompter()
{
	string path = GetHomePath() + "/settings";
	loadConfig(path);

}

dialogPrompter::dialogPrompter(string configurationFile)
{
	loadConfig(configurationFile);
}


dialogPrompter::~dialogPrompter()
{
	// TODO Auto-generated destructor stub
}

void dialogPrompter::loadConfig(string configurationFile)
{
	ifstream config(configurationFile.data());
	string line;

	string type;
	messageType enumType;
	userAction action;

	string showPrefix = "message show";
	string hidePrefix = "message hide";
	string defaultPrefix = "message default";

	if (config.is_open())
	{
		while (config.good())
		{
			getline(config, line);

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
				type = line.substr(showPrefix.length() + 1, line.find_first_of(" ") - 1);
				string defaultAnswer = line.substr(line.find_last_of(" ") + 1, line.length());

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

			if (!type.compare("CONFIG_READ_FAIL"))
			{
				enumType = CONFIG_READ_FAIL;
			}
			else if  (!type.compare("CONFIG_WRITE_FAIL"))
			{
				enumType = CONFIG_WRITE_FAIL;
			}
			else if  (!type.compare("DELETE_USED_PROFILE"))
			{
				enumType = DELETE_USED_PROFILE;
			}

			actionToTake[enumType] = action;
		}
	}
	else
	{
		cout << "Error loading settings file " << configurationFile << endl;
		//syslog(SYSL_ERR, "Unable to open settings file " + configurationFile);
	}
}

bool dialogPrompter::displayPrompt(messageType msg)
{
	if (actionToTake[msg] == CHOICE_HIDE)
		return true;
	else if (actionToTake[msg] == CHOICE_ALWAYS_YES)
		return true;
	else if (actionToTake[msg] == CHOICE_ALWAYS_NO)
		return false;

	// Display the actual message


	switch (msg)
	{
	case CONFIG_READ_FAIL:
		break;

	case CONFIG_WRITE_FAIL:
		break;

	case DELETE_USED_PROFILE:
		break;
	}

}
