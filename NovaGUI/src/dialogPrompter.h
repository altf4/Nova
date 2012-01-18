//============================================================================
// Name        : dialogPrompter.h
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Class to help display error messages to the user via the GUI
//============================================================================/*

#ifndef DIALOGPROMPTER_H_
#define DIALOGPROMPTER_H_

#include <string>

using namespace std;

// If you add a new message, be sure to update numberOfMessageTypes and messageTypeStrings
#define numberOfMessageTypes 10
enum messageType
{
	CONFIG_READ_FAIL = 0,
	CONFIG_WRITE_FAIL,
	DELETE_USED_PROFILE,
	HONEYD_FILE_READ_FAIL,
	UNEXPECTED_FILE_ENTRY,
	HONEYD_LOAD_SUBNETS_FAIL,
	HONEYD_LOAD_NODES_FAIL,
	HONEYD_LOAD_PROFILES_FAIL,
	HONEYD_LOAD_PROFILESET_FAIL,
	HONEYD_NODE_INVALID_SUBNET,
};

enum defaultAction
{
	CHOICE_SHOW = 0,
	CHOICE_HIDE,
	CHOICE_ALWAYS_YES,
	CHOICE_ALWAYS_NO,
};

enum dialogType
{
	DIALOG_NOTIFICATION,
	DIALOG_YES_NO,
};

class dialogPrompter
{
public:
	// Basic constructor
	dialogPrompter();

	// Basic constructor
	//		configurationFilePath - Path to the "settings" file
	dialogPrompter(string configurationFilePath);

	// Basic destructor
	virtual ~dialogPrompter();

	// Display a predefined dialog message to the user
	//		msg - Which message to display
	//		arg - Some of the predefined messages can allow for additional info to be included,
	//			such as a file path or return error value.
	bool displayPrompt(messageType msg, string arg = "");

	// Saves a change in the default user actions for a setting in both the object state and config file
	//
	//		msg - Message to change setting of
	//		action - New default action
	void setDefaultAction(messageType msg, defaultAction action);

	// Loads all of the default user actions for dialogs from the settings file
	void loadDefaultActions();

	// Default actions array
	defaultAction defaultActionToTake[numberOfMessageTypes];

	static const char* messageTypeStrings[];
	static const dialogType messageTypeTypes[];

private:
	// Translates a msg type and action to a string for the settings file
	//		msg - Message type
	//		action - Default action
	// Returns: string that can be written to the config file
	string makeConfigurationLine(messageType msg, defaultAction action);


	// Path to the configuration file
	string configurationFile;

	// Configuration file prefixes defined here in case we want to change them later
	static const string showPrefix;
	static const string hidePrefix;
	static const string defaultPrefix;

};

#endif /* DIALOGPROMPTER_H_ */
