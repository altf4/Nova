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

private slots:
	void on_tableWidget_cellChanged(int row, int col);
	void on_okayButton_clicked();
	void on_cancelButton_clicked();

private:
    void updateRow(trainingSuspect* header, int row);
    void makeRow(trainingSuspect* header, int row);

    bool allowDescriptionEdit;
    bool updating;
	trainingSuspectMap* suspects;
    Ui::classifierPromptClass ui;
};

#endif // CLASSIFIERPROMPT_H
