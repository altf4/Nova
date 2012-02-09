//============================================================================
// Name        : classifierPrompt.h
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
// Description : Prompt for displaying and editing training data
//============================================================================

#ifndef CLASSIFIERPROMPT_H
#define CLASSIFIERPROMPT_H

#include <QtGui/QDialog>
#include "ui_classifierPrompt.h"
#include "NovaUtil.h"

class classifierPrompt : public QDialog
{
    Q_OBJECT

public:
    classifierPrompt(QWidget *parent = 0);
    classifierPrompt(trainingDumpMap* trainingDump, QWidget *parent = 0);
    classifierPrompt(trainingSuspectMap* map, QWidget *parent = 0);

    trainingSuspectMap* getStateData();

    ~classifierPrompt();

protected:
    void contextMenuEvent(QContextMenuEvent * event);

private slots:
	void on_tableWidget_cellChanged(int row, int col);
	void on_okayButton_clicked();
	void on_cancelButton_clicked();
	void on_actionCombineEntries_triggered();

private:
    void updateRow(trainingSuspect* header, int row);
    void makeRow(trainingSuspect* header, int row);
    void DisplaySuspectEntries();

    QMenu * menu;
    int groups;

    bool allowDescriptionEdit;
    bool updating;
	trainingSuspectMap* suspects;
    Ui::classifierPromptClass ui;
};

#endif // CLASSIFIERPROMPT_H
