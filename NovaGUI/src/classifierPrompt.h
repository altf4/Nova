#ifndef CLASSIFIERPROMPT_H
#define CLASSIFIERPROMPT_H

#include <QtGui/QDialog>
#include "ui_classifierPrompt.h"
#include "NovaUtil.h"


struct _trainingSuspectHeader
{
	bool isHostile;
	bool isIncluded;
	string ip;
	string description;
};

typedef struct _trainingSuspectHeader trainingSuspectHeader;

typedef google::dense_hash_map<string, trainingSuspectHeader*, tr1::hash<string>, eqstr > TrainingHeaderMap;


class classifierPrompt : public QDialog
{
    Q_OBJECT

public:
    classifierPrompt(QWidget *parent = 0);
    classifierPrompt(TrainingHashTable* trainingDump, QWidget *parent = 0);

    TrainingHeaderMap* getStateData();

    ~classifierPrompt();

private slots:
	void on_tableWidget_cellChanged(int row, int col);

private:
    void updateRow(trainingSuspectHeader* header, int row);
    void makeRow(trainingSuspectHeader* header, int row);

    bool updating;
	TrainingHeaderMap suspects;
    Ui::classifierPromptClass ui;
};

#endif // CLASSIFIERPROMPT_H
