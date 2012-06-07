#include "autoconfigwizard.h"

#include <QtGui>
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    AutoConfigWizard w;
    w.show();
    return a.exec();
}
