#include "novagui.h"
#include "ui_novagui.h"
#include <QtGui>
#include <QApplication>
#include <main.h>

using namespace std;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    NovaGUI w;
    w.show();
    a.exec();
}
