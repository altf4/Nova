//============================================================================
// Name        : DialogPrompter.h
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

#ifndef DIALOGPROMPTER_H_
#define DIALOGPROMPTER_H_

#include <string>
#include <vector>
#include "DialogPrompt.h"

typedef int messageHandle;

struct messageType
{
	std::string descriptionUID;
	dialogType type;
	defaultChoice action;
};

class DialogPrompter
{
public:
	// Basic constructor
	//		configurationFilePath - Path to the "settings" file
	DialogPrompter(std::string configurationFilePath = "");

	// Registers a new message dialog with the dialog prompter.
	// Once a new message is registered, that dialog may be created by
	// passing the messageHandle returned from register to displayPrompt
	//		t - Description, dialog type, and default action for new message
	// Returns handle to use when calling other prompter methods, such as displayPrompt.
	messageHandle RegisterDialog(messageType type);

	// Display a predefined dialog message to the user (if default action is show)
	//		handle - Handle returned from registerDialog
	//		messageTxt - Text to display in dialog
	//		parent - Optional parent widget
	// Returns: true for an "okay/yes" user response, false otherwise
	defaultChoice DisplayPrompt(messageHandle handle, std::string messageTxt, QWidget *parent = 0);

	// Display a predefined dialog message to the user (if default action is show)
	//		handle - Handle returned from registerDialog
	//		messageTxt - Text to display in dialog
	//		defaultAction - Action to emit
	//		parent - Optional parent widget
	// Returns: true for an "okay/yes" user response, false otherwise
	defaultChoice DisplayPrompt(messageHandle handle, std::string messageTxt,
			QAction * defaultAction, QAction * alternativeAction, QWidget *parent = 0);

	// Saves a change in the default user actions for a setting in both the object state and config file
	//		handle - Handle returned from registerDialog
	//		action - Default action to take
	void SetDefaultAction(messageHandle handle, defaultChoice action);

	// Loads all of the default user actions for dialogs from the settings file
	void LoadDefaultActions();

	// List of registered message types
	std::vector<messageType> m_registeredMessageTypes;

private:
	// Translates a msg and action to a std::string for the settings file
	//		msg - Message handle
	//		action - Default action
	// Returns: std::string that can be written to the config file
	std::string MakeConfigurationLine(messageHandle msg, defaultChoice action);

	// Path to the settings file
	std::string m_configurationFile;

	// Configuration file prefixes defined here in case we want to change them later
	static const std::string m_showPrefix;
	static const std::string m_hidePrefix;
	static const std::string m_yesPrefix;
	static const std::string m_noPrefix;
};

#endif /* DIALOGPROMPTER_H_ */
