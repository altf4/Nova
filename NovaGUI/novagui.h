#ifndef NOVAGUI_H
#define NOVAGUI_H

#include <QtGui/QMainWindow>
#include "ui_novagui.h"
#include <tr1/unordered_map>
#include <sys/un.h>
#include <arpa/inet.h>
#include <Suspect.h>

///	Filename of the file to be used as an Classification Engine IPC key
#define CE_FILENAME "/etc/CEKey"

///The maximum message, as defined in /proc/sys/kernel/msgmax
#define MAX_MSG_SIZE 65535

class NovaGUI : public QMainWindow
{
    Q_OBJECT

public:

    NovaGUI(QWidget *parent = 0);
    ~NovaGUI();
    Ui::NovaGUIClass ui;

    ///Receive a input from Classification Engine.
    bool ReceiveCE(int socket);

    ///Processes the recieved suspect in the suspect table
    void updateSuspect(Nova::ClassificationEngine::Suspect* suspect);

    ///Updates the UI with the latest suspect information
    void drawSuspects();

private slots:
    void on_CEUpdate_clicked();

private:

};

using namespace Nova;
using namespace ClassificationEngine;

typedef std::tr1::unordered_map<in_addr_t, Suspect*> SuspectHashTable;

/// This is a blocking function. If nothing is received, then wait on this thread for an answer
void *CEListen(void *ptr);

///Socket closing workaround for namespace issue.
void sclose(int sock);



#endif // NOVAGUI_H
