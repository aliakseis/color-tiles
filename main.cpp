#include "tiles.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("Fusion"));
    Tiles w;
    w.readPositionSettings();
    w.show();
    return QApplication::exec();
}
