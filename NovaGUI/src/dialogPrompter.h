/*
 * dialogPrompter.h
 *
 *  Created on: Jan 17, 2012
 *      Author: root
 */

#ifndef DIALOGPROMPTER_H_
#define DIALOGPROMPTER_H_

#include <string>

using namespace std;

// If you add a new message, be sure to update numberOfMessageTypes
#define numberOfMessageTypes 3
enum messageType
{
	CONFIG_READ_FAIL,
	CONFIG_WRITE_FAIL,
	DELETE_USED_PROFILE,
};

enum userAction
{
	CHOICE_SHOW,
	CHOICE_HIDE,
	CHOICE_ALWAYS_YES,
	CHOICE_ALWAYS_NO,
};

class dialogPrompter
{
public:
	dialogPrompter();
	dialogPrompter(string configurationFile);

	virtual ~dialogPrompter();

	void loadConfig(string configurationFile);

	bool displayPrompt(messageType msg);

	userAction actionToTake[numberOfMessageTypes];
};

#endif /* DIALOGPROMPTER_H_ */
