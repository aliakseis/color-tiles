#include "TilesArea.h"

#include <QPainter>
#include <QTimer>
#include <QApplication>
#include <QMessageBox>
#include <QPixmap>
#include <QLabel>
#include <QMovie>

#include <ctime>
#include <algorithm>
#include <memory>

enum
{
    clrRed,
    clrGreen,
    clrBlue,
    clrYellow,
    clrAqua,
    clrPurple,
};

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
    srand((unsigned)time(nullptr));

    random();
}


TilesArea::~TilesArea()
= default;


void TilesArea::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    QRect clientRect = rect();

    for (int i = 0; i < DIM; ++i) {
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
}

void TilesArea::random()
{
    stopReplay();

    for (auto & i : board) {
        for (int j = 0; j < DIM; ++j) {
            i[j] = rand() % 6;
        }
    }

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


void TilesArea::onColor(int color, bool fromUser /*= true*/)
{
    int prevColor = board[0][0];
    if (prevColor == color) {
        return;
    }

    doOnColor(color, prevColor, 0, 0);

    update();

    ++currentStep;
    emit onStep(currentStep);

    enum { MAX_STEPS_NUMBER = 25 };
    if (fromUser && currentStep >= MAX_STEPS_NUMBER && !isSolved())
    {
        // https://stackoverflow.com/a/50028443/10472202
        QMessageBox msg;
        // create Label
        msg.setIconPixmap(QPixmap(":/Tiles/Resources/bad.gif").scaledToWidth(100));
        auto icon_label = msg.findChild<QLabel*>("qt_msgboxex_icon_label");
        auto movie = new QMovie(":/Tiles/Resources/bad.gif", {}, &msg);
        // avoid garbage collector
        //setattr(msg, 'icon_label', movie)
        icon_label->setMovie(movie);
        movie->start();

        msg.setText(tr("Too bad."));
        //msg.setWindowTitle(" ");
        msg.setStandardButtons(QMessageBox::Ok);

        msg.exec();
    }
}

void TilesArea::doOnColor(int color, int prevColor, int i, int j)
{
    if (i < 0 || i >= DIM || j < 0 || j >= DIM
        || board[i][j] != prevColor) {
        return;
    }

    board[i][j] = color;

    doOnColor(color, prevColor, i - 1, j);
    doOnColor(color, prevColor, i + 1, j);
    doOnColor(color, prevColor, i, j - 1);
    doOnColor(color, prevColor, i, j + 1);
}

bool TilesArea::isSolved() const
{
    auto comparand = board[0][0];

    for (auto& row : board) {
        for (auto v : row) {
            if (v != comparand) {
                return false;
            }
        }
    }
    return true;
}

void TilesArea::solve()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);

    solution = DoSolve(board, 100000);//NUM_COURSES);

    QApplication::restoreOverrideCursor();

    if (solution.empty()) {
        return;
    }

    std::reverse(solution.begin(), solution.end());

    int color = solution.back();
    solution.pop_back();
    if (!solution.empty())
    {
        timer = std::make_unique<QTimer>(this);
        connect(timer.get(), SIGNAL(timeout()), this, SLOT(step()));
        timer->start(1000);
    }

    onColor(color, false);
}

void TilesArea::step()
{
    int color = solution.back();
    solution.pop_back();
    if (solution.empty())
    {
        timer.reset();
    }

    onColor(color, false);
}

void TilesArea::stopReplay()
{
    timer.reset();
    solution.clear();
}
