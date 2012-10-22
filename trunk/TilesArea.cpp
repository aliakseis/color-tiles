#include "TilesArea.h"

#include <QPainter>
#include <QTimer>
#include <QApplication>

#include <time.h>

enum
{
    clrRed,
    clrGreen,
    clrBlue,
    clrYellow,
    clrAqua,
    clrPurple,
} enmColors;

QColor colors[] =
{
    QColor(255, 0, 0),
    QColor(0, 255, 0),
    QColor(0, 0, 255),
    QColor(255, 255, 0),
    QColor(0, 255, 255),
    QColor(255, 0, 255),
};

TilesArea::TilesArea(QWidget *parent /*= 0*/)
    : QWidget(parent)
    , currentStep(0)
{
    // Seed the random-number generator with the current time so that
    // the numbers will be different every time we run.
    srand( (unsigned)time( NULL ) );

    random();
}


TilesArea::~TilesArea(void)
{
}


void TilesArea::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    QRect clientRect = rect();

    for (int i = 0; i < DIM; ++i)
        for (int j = 0; j < DIM; ++j)
        {
            QRect rect;
            rect.setLeft(clientRect.left() + clientRect.width() * j / DIM);
            rect.setRight(clientRect.left() + clientRect.width() * (j + 1) / DIM);
            rect.setTop(clientRect.top() + clientRect.height() * i / DIM);
            rect.setBottom(clientRect.top() + clientRect.height() * (i + 1) / DIM);

            QBrush brush(colors[board[i][j]]);

            painter.setBrush(brush);
            painter.drawRect(rect);
        }
}

void TilesArea::random()
{
    stopReplay();

    for (int i = 0; i < DIM; ++i)
        for (int j = 0; j < DIM; ++j)
            board[i][j] = rand() % 6;

    currentStep = 0;
    emit onStep(0);

    update();
}

void TilesArea::onRed() 
{
    stopReplay();
    onColor(clrRed);
}

void TilesArea::onGreen()
{
    stopReplay();
    onColor(clrGreen);
}

void TilesArea::onBlue()
{
    stopReplay();
    onColor(clrBlue);
}

void TilesArea::onYellow()
{
    stopReplay();
    onColor(clrYellow);
}

void TilesArea::onAqua()
{
    stopReplay();
    onColor(clrAqua);
}

void TilesArea::onPurple()
{
    stopReplay();
    onColor(clrPurple);
}


void TilesArea::onColor(int color)
{
    int prevColor = board[0][0];
    if (prevColor == color)
        return;

    doOnColor(color, prevColor, 0, 0);

    update();

    ++currentStep;
    emit onStep(currentStep);
}

void TilesArea::doOnColor(int color, int prevColor, int i, int j)
{
    if (i < 0 || i >= DIM || j < 0 || j >= DIM
            || board[i][j] != prevColor)
        return;

    board[i][j] = color;

    doOnColor(color, prevColor, i - 1, j);
    doOnColor(color, prevColor, i + 1, j);
    doOnColor(color, prevColor, i, j - 1);
    doOnColor(color, prevColor, i, j + 1);
}


void TilesArea::solve()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);

    solution = DoSolve(board, 100000);//NUM_COURSES);

    QApplication::restoreOverrideCursor();    

    if (solution.empty())
        return;

    std::reverse(solution.begin(), solution.end());

    int color = solution.back();
    solution.pop_back();
    if (!solution.empty())
    {
        timer.reset(new QTimer(this));
        connect(timer.get(), SIGNAL(timeout()), this, SLOT(step()));
        timer->start(1000);
    }

    onColor(color);
}

void TilesArea::step()
{
    int color = solution.back();
    solution.pop_back();
    if (solution.empty())
    {
        timer.reset();
    }

    onColor(color);
}

void TilesArea::stopReplay()
{
    timer.reset();
    solution.clear();
}
