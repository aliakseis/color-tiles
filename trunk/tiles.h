#ifndef TILES_H
#define TILES_H

#include <QtGui/QMainWindow>
#include "ui_tiles.h"

class TilesArea;

class Tiles : public QMainWindow
{
    Q_OBJECT

public:
    Tiles(QWidget *parent = 0, Qt::WFlags flags = 0);
    ~Tiles();

public slots:
    void onStep(int step);

private:
    void createActions();

private:
    Ui::TilesClass ui;
    TilesArea* tilesArea;
};

#endif // TILES_H
