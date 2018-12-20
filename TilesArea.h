#pragma once

#include <QWidget>

#include "solver.h"

#include <memory>

class TilesArea :
    public QWidget
{
    Q_OBJECT

public:
    TilesArea(QWidget *parent = 0);
    ~TilesArea();


signals:
    void onStep(int step);

public slots:
    void onRed();
    void onGreen();
    void onBlue();
    void onYellow();
    void onAqua();
    void onPurple();

    void random();
    void solve();

protected slots:
    void step();

protected:
    virtual void paintEvent(QPaintEvent *event) override;

    void onColor(int color);
    void doOnColor(int color, int prevColor, int i, int j);
    void stopReplay();

private:
    Board board;
    std::vector<int> solution;
    std::unique_ptr<QTimer> timer;

    int currentStep;
};

