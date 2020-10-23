#include "EyeDropper.h"

#include <QApplication>
#include <QCursor>
#include <QDesktopWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QKeyEvent>
#include <QDebug>

#include "../Helpers/MouseHelper.h"

EyeDropper::EyeDropper(QWidget* parent)
    : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    , mouse(new MouseHelper(this))
    , cursorSize(69, 69)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setCursor(Qt::BlankCursor);

    connect(mouse, SIGNAL(mouseMove(const QPoint&)), SLOT(OnMouseMove(const QPoint&)));
    connect(mouse, SIGNAL(mouseRelease(const QPoint&)), SLOT(OnClicked(const QPoint&)));
}

EyeDropper::~EyeDropper()
{
}

void EyeDropper::Exec()
{
    CreateShade();
    show();
    setFocus();
    update();
}

void EyeDropper::OnMouseMove(const QPoint& pos)
{
    const int sx = cursorSize.width() / 2;
    const int sy = cursorSize.height() / 2;
    QRect rcOld(QPoint(cursorPos.x() - sx, cursorPos.y() - sy), cursorSize);
    rcOld.adjust(-1, -1, 2, 2);
    QRect rcNew(QPoint(pos.x() - sx, pos.y() - sy), cursorSize);
    rcNew.adjust(-1, -1, 2, 2);
    const QRect rc = rcOld.united(rcNew);

    cursorPos = pos;
    repaint(rc);

    emit moved(GetPixel(pos));
}

void EyeDropper::OnClicked(const QPoint& pos)
{
    emit picked(GetPixel(pos));
    close();
}

void EyeDropper::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);

    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing, false);
    p.drawImage(0, 0, cache);
    DrawCursor(cursorPos, &p);
}

void EyeDropper::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape)
    {
        close();
    }

    QWidget::keyPressEvent(e);
}

void EyeDropper::DrawCursor(const QPoint& pos, QPainter* p)
{
    const int sx = cursorSize.width() / 2 - 1;
    const int sy = cursorSize.height() / 2 - 1;
    const QColor c = GetPixel(pos);

    QRect rc(QPoint(pos.x() - sx, pos.y() - sy), QPoint(pos.x() + sx, pos.y() + sy));

    const int fc = 4;
    QRect rcZoom(QPoint(pos.x() - sx / fc, pos.y() - sy / fc), QPoint(pos.x() + sx / fc, pos.y() + sy / fc));
    const QImage& zoomed = cache.copy(rcZoom).scaled(rc.size(), Qt::KeepAspectRatio, Qt::FastTransformation);

    p->drawImage(rc, zoomed);
    p->setPen(QPen(Qt::black, 1.0));

    const int midX = (rc.left() + rc.right()) / 2;
    const int midY = (rc.bottom() + rc.top()) / 2;

    p->drawLine(rc.left(), midY, rc.right(), midY);
    p->drawLine(midX, rc.top(), midX, rc.bottom());
    p->fillRect(pos.x() - 1, pos.y() - 1, 3, 3, c);

    p->setPen(Qt::white);
    p->drawRect(rc);
    rc.adjust(-1, -1, 1, 1);
    p->setPen(Qt::black);
    p->drawRect(rc);
}

void EyeDropper::CreateShade()
{
    QDesktopWidget* desktop = QApplication::desktop();
    const int n = desktop->screenCount();
    QRect rc;

    for (int i = 0; i < n; i++)
    {
        const QRect screenRect = desktop->screenGeometry(i);
        rc = rc.united(screenRect);
    }

    cache = QPixmap::grabWindow(QApplication::desktop()->winId(), rc.left(), rc.top(), rc.width(), rc.height()).toImage();
    resize(rc.size());
    move(rc.topLeft());
    cursorPos = mapFromGlobal(QCursor::pos());
}

QColor EyeDropper::GetPixel(const QPoint& pos) const
{
    const QColor c = cache.pixel(pos);
    return c;
}
