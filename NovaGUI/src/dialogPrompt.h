#ifndef DIALOGPROMPT_H
#define DIALOGPROMPT_H

#include <QtGui>
#include "NovaUtil.h"

enum messageType {notSet = 0, notifyPrompt, warningPrompt, warningPreventablePrompt, errorPrompt, notifyActionPrompt, warningActionPrompt};

class dialogPrompt : public QMessageBox
{
    Q_OBJECT

public:
    //Default Constructor
    dialogPrompt(QWidget *parent = 0);

    //Constructs a notification box, if it is a preventable warning then spawn the box before taking the action
    // Describe the side effects of the warning if the action is taken, if the user presses cancel, don't continue
    // If the error or notification will happen regardless simply display message and continue after the box is closed.
    dialogPrompt(enum messageType, const QString &title, const QString &text = "", QWidget *parent = 0);

    //Constructs a notification box that displays the problem, the default action and an alternative action
    // message type sets the level, title summarizes the problem, text describes the choices
    // default Action is performed on Yes, Alternative Action is performed on No
    // returns -1 so the action that caused the prompt can be prevented
    dialogPrompt(enum messageType, QAction * defaultAction, QAction * alternativeAction,
    		const QString &title, const QString &text = "", QWidget *parent = 0);
    ~dialogPrompt();

    int exec();
    void setMessageType(messageType type);
    void setDefaultAction(QAction * action);
    void setAlternativeAction(QAction * action);
    messageType getMessageType();
    QAction * getDefaultAction();
    QAction * getAlternativeAction();


private:
    messageType type;
    QAction * defaultAct;
    QAction * alternativeAct;
};

#endif // DIALOGPROMPT_H
