#include "tiles.h"

#include "TilesArea.h"

Tiles::Tiles(QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
{
    ui.setupUi(this);

//    TilesArea* tilesArea = new TilesArea();
//    setCentralWidget(tilesArea);

    createActions();
}

Tiles::~Tiles()
{
}

void Tiles::createActions()
{
}

void Tiles::onStep(int step)
{
    QString str;
    str.sprintf("Step: %d", step);
    statusBar()->showMessage(str);
}
