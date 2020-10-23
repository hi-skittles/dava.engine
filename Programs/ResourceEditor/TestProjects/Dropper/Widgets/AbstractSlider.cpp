#include "AbstractSlider.h"

#include <QPainter>
#include <QResizeEvent>

#include "../Helpers/MouseHelper.h"

#include "../ColorPicker/PaletteHSV.h"

AbstractSlider::AbstractSlider(QWidget* parent)
    : QWidget(parent)
    , mouse(new MouseHelper(this))
{
    connect(mouse, SIGNAL(mousePress(const QPoint&)), SLOT(OnMousePress(const QPoint&)));
    connect(mouse, SIGNAL(mouseMove(const QPoint&)), SLOT(OnMouseMove(const QPoint&)));
    connect(mouse, SIGNAL(mouseRelease(const QPoint&)), SLOT(OnMouseRelease(const QPoint&)));

    setFocusPolicy(Qt::ClickFocus);
}

AbstractSlider::~AbstractSlider()
{
}

QPointF AbstractSlider::PosF() const
{
    return posF;
}

void AbstractSlider::SetPosF(const QPointF& _posF)
{
    posF = _posF;
    update();
}

void AbstractSlider::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);

    QPainter p(this);

    p.save();
    DrawBackground(&p);
    p.restore();
    p.save();
    DrawForeground(&p);
    p.restore();
}

void AbstractSlider::resizeEvent(QResizeEvent* e)
{
    update();
}

void AbstractSlider::DrawBackground(QPainter* p) const
{
    Q_UNUSED(p);
}

void AbstractSlider::DrawForeground(QPainter* p) const
{
    Q_UNUSED(p);
}

QRect AbstractSlider::PosArea() const
{
    return QRect(0, 0, width(), height());
}

QPoint AbstractSlider::Pos() const
{
    const QRect& rc = PosArea();
    const int x = rc.width() * posF.x();
    const int y = rc.height() * posF.y();
    const QPoint pos = QPoint(x, y) + rc.topLeft();

    return pos;
}

MouseHelper* AbstractSlider::Mouse() const
{
    return mouse;
}

void AbstractSlider::OnMousePress(const QPoint& _pos)
{
    SetPos(_pos);
    pressPos = Pos();
    emit started(PosF());
}

void AbstractSlider::OnMouseMove(const QPoint& _pos)
{
    if (mouse->IsPressed())
    {
        SetPos(_pos);
        emit changing(PosF());
    }
}

void AbstractSlider::OnMouseRelease(const QPoint& _pos)
{
    SetPos(_pos);
    if (pressPos != _pos)
    {
        emit changed(PosF());
    }
    else
    {
        emit canceled();
    }
}

void AbstractSlider::SetPos(const QPoint& _pos)
{
    const QRect& area = PosArea();
    const QRect& rc = area.adjusted(0, 0, 1, 1);
    QPoint pos = _pos;

    if (!rc.contains(pos))
    {
        if (pos.x() < rc.left())
        {
            pos.setX(rc.left());
        }
        if (pos.x() > rc.right())
        {
            pos.setX(rc.right());
        }
        if (pos.y() < rc.top())
        {
            pos.setY(rc.top());
        }
        if (pos.y() > rc.bottom())
        {
            pos.setY(rc.bottom());
        }
    }

    pos -= rc.topLeft();

    const qreal xF = double(pos.x()) / double(area.width());
    const qreal yF = double(pos.y()) / double(area.height());
    posF = QPointF(xF, yF);

    update();
}
