//============================================================================
// Name        : dialogPrompter.h
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Class to help display error messages to the user via the GUI
//============================================================================/*

#ifndef DIALOGPROMPTER_H_
#define DIALOGPROMPTER_H_

#include <string>
#include <vector>
#include "dialogPrompt.h"

using namespace std;

// If you add a new dialog in this enum, be sure to,
//	Increment dialogPrompter.h/numberOfMessageTypes
//	Set a message string in dialogPrompter.cpp/messageTypeStrings
//	Set the dialog type in dialogPrompter.cpp/messageTypeTypes
#define numberOfMessageTypes 10

typedef int messageType;

struct dialogMessageType
{
	string descriptionUID;
	dialogType type;
	defaultAction action;
};

class dialogPrompter
{
public:
	// Basic constructor
	dialogPrompter();

	// Basic constructor
	//		configurationFilePath - Path to the "settings" file
	dialogPrompter(string configurationFilePath);

	messageType registerDialog(dialogMessageType t);

	// Basic destructor
	virtual ~dialogPrompter();

	// Display a predefined dialog message to the user
	//		msg - Which message to display
	//		arg - Some of the predefined messages can allow for additional info to be included,
	//			such as a file path or return error value.
	// Returns: true for an "okay/yes" user response, false otherwise
	defaultAction displayPrompt(messageType msg, string arg = "");

	// Saves a change in the default user actions for a setting in both the object state and config file
	//
	//		msg - Message to change setting of
	//		action - New default action
	void setDefaultAction(messageType msg, defaultAction action);

	// Loads all of the default user actions for dialogs from the settings file
	void loadDefaultActions();

	vector<dialogMessageType> registeredMessageTypes;

private:
	// Translates a msg type and action to a string for the settings file
	//		msg - Message type
	//		action - Default action
	// Returns: string that can be written to the config file
	string makeConfigurationLine(messageType msg, defaultAction action);

	// Path to the settings file
	string configurationFile;

	// Configuration file prefixes defined here in case we want to change them later
	static const string showPrefix;
	static const string hidePrefix;
	static const string yesPrefix;
	static const string noPrefix;
};

#endif /* DIALOGPROMPTER_H_ */
