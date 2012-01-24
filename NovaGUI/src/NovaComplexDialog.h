#ifndef NOVACOMPLEXDIALOG_H
#define NOVACOMPLEXDIALOG_H

#include "novagui.h"
#include "ui_NovaComplexDialog.h"

enum whichDialog {MACDialog, PersonalityDialog};

class NovaConfig;
class NovaComplexDialog : public QDialog
{
    Q_OBJECT

public:
    //Default constructor, shouldn't be used
    NovaComplexDialog(QWidget *parent = 0);
    //Standard constructor, specify which dialog type you wish to create
    NovaComplexDialog(whichDialog type, QWidget *parent = 0);
    ~NovaComplexDialog();
    //Sets the help message displayed at the bottom of the window to provide the user hints on usage.
    void setHelpMsg(const QString& str);
    //Returns the help message displayed at the bottom of the window to provide the user hints on usage.
    QString getHelpMsg();
    //Sets the description at the top on the window to inform the user of the window's purpose.
    void setInfoMsg(const QString& str);
    //Returns the description at the top on the window to inform the user of the window's purpose.
    QString getInfoMsg();
    //Returns the object's 'type' enum
    whichDialog getType();

private:

    NovaConfig * novaParent;
    whichDialog type;
    Ui::NovaComplexDialogClass ui;

private slots:

	void on_searchEdit_textChanged();
	void on_cancelButton_clicked();
	void on_selectButton_clicked();
	void on_searchButton_clicked();
};

#endif // NOVACOMPLEXDIALOG_H
