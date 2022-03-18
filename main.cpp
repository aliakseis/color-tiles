#include "tiles.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Tiles w;
    w.show();
    return a.exec();
}
