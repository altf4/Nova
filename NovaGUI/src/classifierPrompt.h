#ifndef CLASSIFIERPROMPT_H
#define CLASSIFIERPROMPT_H

#include <QtGui/QDialog>
#include "ui_classifierPrompt.h"
#include "NovaUtil.h"


struct _suspectHeader
{
	bool isHostile;
	bool isIncluded;
	string ip;
	string description;
};


typedef struct _suspectHeader suspectHeader;

typedef google::dense_hash_map<string, suspectHeader*, tr1::hash<string>, eqstr > ipToHeader;


class classifierPrompt : public QDialog
{
    Q_OBJECT

public:
    classifierPrompt(QWidget *parent = 0);
    classifierPrompt(TrainingHashTable* trainingDump, QWidget *parent = 0);

    vector<string>* getHostiles();

    ~classifierPrompt();

private slots:
	void on_tableWidget_cellChanged(int row, int col);

private:
    void updateRow(suspectHeader* header, int row);
    void makeRow(suspectHeader* header, int row);

    bool updating;
	ipToHeader suspects;
    Ui::classifierPromptClass ui;
};

#endif // CLASSIFIERPROMPT_H
