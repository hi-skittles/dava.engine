#include "dropper.h"
#include <QtWidgets/QApplication>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    Dropper w;
    w.show();
    return a.exec();
}
