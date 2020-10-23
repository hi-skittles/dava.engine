#include "UI/Preview/Guides/GuideLabel.h"

#include <QPainter>

GuideLabel::GuideLabel(DAVA::Vector2::eAxis orientation_, QWidget* parent)
    : QWidget(parent)
    , orientation(orientation_)
{
    setAttribute(Qt::WA_TranslucentBackground); // Indicates that the background will be transparent
}

void GuideLabel::SetValue(int arg)
{
    value = arg;
}

int GuideLabel::GetValue() const
{
    return value;
}

void GuideLabel::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    QFont painterFont = painter.font();
    painterFont.setPixelSize(10);
    painter.setFont(painterFont);

    QPalette palette;
    QColor rectColor = palette.color(QPalette::Window);
    rectColor = rectColor.lighter();
    rectColor.setAlpha(220);

    painter.save();
    painter.setBrush(QBrush(rectColor));
    painter.setPen(rectColor);

    const int radius = 3;
    painter.drawRoundedRect(rect(), radius, radius);

    painter.restore();

    QRect textRect = rect();
    if (orientation == DAVA::Vector2::AXIS_Y)
    {
        painter.translate(0, height());
        painter.rotate(-90);
        textRect.setWidth(height());
        textRect.setHeight(width());
    }
    painter.drawText(textRect, Qt::AlignCenter, QString::number(value));
}
