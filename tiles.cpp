#include "tiles.h"

#include "TilesArea.h"

#include <QSettings>

Tiles::Tiles(QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
{
    ui.setupUi(this);

    //    TilesArea* tilesArea = new TilesArea();
    //    setCentralWidget(tilesArea);

    createActions();
}

Tiles::~Tiles()
= default;

void Tiles::createActions()
{
}

void Tiles::onStep(int step)
{
    statusBar()->showMessage(QString("Step: %1").arg(step));
}

// https://stackoverflow.com/questions/74690/how-do-i-store-the-window-size-between-sessions-in-qt
void Tiles::writePositionSettings()
{
    QSettings qsettings("noname", "Tiles");

    qsettings.beginGroup("mainwindow");

    qsettings.setValue("geometry", saveGeometry());
    qsettings.setValue("savestate", saveState());
    qsettings.setValue("maximized", isMaximized());
    if (!isMaximized()) {
        qsettings.setValue("pos", pos());
        qsettings.setValue("size", size());
    }

    qsettings.endGroup();
}

void Tiles::readPositionSettings()
{
    QSettings qsettings("noname", "Tiles");

    qsettings.beginGroup("mainwindow");

    restoreGeometry(qsettings.value("geometry", saveGeometry()).toByteArray());
    restoreState(qsettings.value("savestate", saveState()).toByteArray());
    move(qsettings.value("pos", pos()).toPoint());
    resize(qsettings.value("size", size()).toSize());
    if (qsettings.value("maximized", isMaximized()).toBool()) {
        showMaximized();
    }

    qsettings.endGroup();
}

void Tiles::moveEvent(QMoveEvent* /*event*/)
{
    writePositionSettings();
}

void Tiles::resizeEvent(QResizeEvent* /*event*/)
{
    writePositionSettings();
}

void Tiles::closeEvent(QCloseEvent* /*event*/)
{
    writePositionSettings();
}
