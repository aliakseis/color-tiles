#include "tiles.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Tiles w;
    w.readPositionSettings();
    w.show();
    return a.exec();
}
