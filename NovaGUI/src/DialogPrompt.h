//============================================================================
// Name        : DialogPrompt.h
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
// Description : 
//============================================================================
#ifndef DIALOGPROMPT_H
#define DIALOGPROMPT_H

#include <QtGui>
#include "NovaUtil.h"

using namespace std;
enum dialogType {
	notSet = 0,
	notifyPrompt,
	warningPrompt,
	warningPreventablePrompt,
	errorPrompt,
	notifyActionPrompt,
	warningActionPrompt
};



enum defaultChoice
{
	CHOICE_SHOW = 0,
	CHOICE_HIDE,
	CHOICE_DEFAULT,
	CHOICE_ALT,
};


class DialogPrompt : public QMessageBox
{
    Q_OBJECT

public:
    //Default Constructor
    DialogPrompt(QWidget *parent = 0);

    //Constructs a notification box, if it is a preventable warning then spawn the box before taking the action
    // Describe the side effects of the warning if the action is taken, if the user presses cancel, don't continue
    // If the error or notification will happen regardless simply display message and continue after the box is closed.
    DialogPrompt(dialogType type, const QString &title, const QString &text, QWidget *parent = 0);

    //Constructs a notification box that displays the problem, the default action and an alternative action
    // message type sets the level, title summarizes the problem, text describes the choices
    // default Action is performed on Yes, Alternative Action is performed on No
    // returns -1 so the action that caused the prompt can be prevented
    DialogPrompt(dialogType type, QAction * defaultAction, QAction * alternativeAction,
    		const QString &title, const QString &text, QWidget *parent = 0);

    ~DialogPrompt();

    // This returns the defaultAction to use if the checkbox is selected
    defaultChoice exec();

    void SetMessageType(dialogType type);
    void SetDefaultAction(QAction * action);
    void SetAlternativeAction(QAction * action);
    dialogType GetMessageType();
    QAction * GetDefaultAction();
    QAction * GetAlternativeAction();

    QCheckBox *checkBox;

private:
    dialogType type;
    QAction * defaultAct;
    QAction * alternativeAct;
	QLabel *checkBoxLabel;
};

#endif // DIALOGPROMPT_H
