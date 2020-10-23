#include "pipetkawidget.h"
#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    PipetkaWidget w;
    w.show();
    return a.exec();
}
