#ifndef TILES_H
#define TILES_H

#include <QMainWindow>
#include "ui_tiles.h"

class TilesArea;

class Tiles : public QMainWindow
{
    Q_OBJECT

public:
    Tiles(QWidget *parent = 0, Qt::WindowFlags flags = 0);
    ~Tiles();

    void writePositionSettings();
    void readPositionSettings();

public slots:
    void onStep(int step);

private:
    void createActions();

    void moveEvent(QMoveEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void closeEvent(QCloseEvent*) override;

private:
    Ui::TilesClass ui;
    TilesArea* tilesArea;
};

#endif // TILES_H
