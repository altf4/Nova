#include "HaystackConfigWizard.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    HaystackConfigWizard w;
    w.show();
    return a.exec();
}
