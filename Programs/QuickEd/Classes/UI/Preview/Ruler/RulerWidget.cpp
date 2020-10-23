#include "RulerWidget.h"

#include <TArc/Core/ContextAccessor.h>
#include <TArc/SharedModules/ThemesModule/ThemesModule.h>

#include <QtTools/Updaters/LazyUpdater.h>

#include <Debug/DVAssert.h>

#include <QMenu>
#include <QPainter>
#include <QApplication>
#include <QDrag>
#include <QMimeData>

#include <cmath>
#include <sstream>

RulerWidget::RulerWidget(DAVA::ContextAccessor* accessor_, IRulerListener* listener_, QWidget* parent)
    : QWidget(parent)
    , accessor(accessor_)
    , listener(listener_)
    , canvasDataAdapter(accessor)
{
    DVASSERT(listener != nullptr);

    setMouseTracking(true);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, &RulerWidget::ShowContextMenu);

    DAVA::Function<void()> updateRuler(this, static_cast<void (QWidget::*)(void)>(&RulerWidget::update));
    lazyUpdater = new LazyUpdater(updateRuler, this);

    UpdateDoubleBufferImage();
}

void RulerWidget::SetRulerOrientation(Qt::Orientation orientation_)
{
    orientation = orientation_;
    UpdateDoubleBufferImage();
    update();
}

void RulerWidget::OnRulerSettingsChanged(const RulerSettings& rulerSettings)
{
    settings = rulerSettings;

    UpdateDoubleBufferImage();
    update();
}

void RulerWidget::OnMarkerPositionChanged(int position)
{
    markerPosition = position;
    lazyUpdater->Update();
}

void RulerWidget::ShowContextMenu(const QPoint& point)
{
    QMenu menu(this);

    QList<QAction*> actions = listener->GetActions(GetMousePos(point), &menu);
    menu.addActions(actions);

    if (menu.actions().isEmpty() == false)
    {
        menu.exec(mapToGlobal(point));
    }
}

void RulerWidget::paintEvent(QPaintEvent* paintEvent)
{
    UpdateDoubleBufferImage();
    QPainter painter(this);
    painter.drawPixmap(0, 0, doubleBuffer);
    if (markerPosition == 0)
    {
        return;
    }

    // Draw the marker.
    QColor penColor(palette().color(QPalette::Text));
    QPen pen(penColor, 1);
    pen.setStyle(Qt::DashLine);
    painter.setPen(pen);

    if (orientation == Qt::Horizontal)
    {
        painter.drawLine(markerPosition, 0, markerPosition, rect().height());
    }
    else
    {
        painter.drawLine(0, markerPosition, rect().width(), markerPosition);
    }

    QWidget::paintEvent(paintEvent);
}

QSize RulerWidget::minimumSizeHint() const
{
    static const int minimumSize = 25;
    return QSize(minimumSize, minimumSize);
}

void RulerWidget::DrawScale(QPainter& painter, int tickStep, int tickStartPos, int tickEndPos,
                            bool drawValues)
{
    using namespace DAVA;
    DVASSERT(tickStep > 0);

    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const QFont& font = painter.font();
    int fontSize = font.pixelSize();

    QFontMetrics fm(font, this);
    int digitHeight = fm.boundingRect('5').height();

    int fontPos = 0;
    int endPos = 0;
    if (orientation == Qt::Horizontal)
    {
        endPos = rect().width();
        fontPos = fm.boundingRect('5').height();
    }
    else
    {
        endPos = rect().height();
        fontPos = digitHeight;
    }

    char nextDigit[2] = { 0x00, 0x00 };

    Vector2::eAxis axis = orientation == Qt::Horizontal ? Vector2::AXIS_X : Vector2::AXIS_Y;

    float32 absValue = canvasDataAdapter.MapFromScreenToRoot(0.0f, axis);
    float32 scale = canvasDataAdapter.GetScale();
    float32 positionStep = tickStep * scale;
    float32 startValue = std::ceilf(absValue / tickStep) * tickStep;

    //i and endPos is a visual positions
    for (int i = 0; i < endPos / scale; i += tickStep)
    {
        float32 currentValue = i + startValue;

        int curPos = static_cast<int>(std::round(canvasDataAdapter.MapFromRootToScreen(currentValue, axis) + EPSILON));
        int curPosValue = static_cast<int>(currentValue + EPSILON);
        const int textOffset = 5;

        if (orientation == Qt::Horizontal)
        {
            painter.drawLine(QLine(curPos, tickStartPos, curPos, tickEndPos));
            if (drawValues)
            {
                painter.drawText(curPos + textOffset, fontPos + textOffset, QString::number(curPosValue));
            }
        }
        else
        {
            painter.drawLine(QLine(tickStartPos, curPos, tickEndPos, curPos));
            if (drawValues)
            {
                int digitPos = curPos + fontSize - 1;
                painter.save();
                painter.translate(tickStartPos, curPos);
                painter.rotate(-90);
                painter.drawText(textOffset, fontPos + textOffset, QString::number(curPosValue));
                painter.restore();
            }
        }
    }
}

void RulerWidget::UpdateDoubleBufferImage()
{
    doubleBuffer = QPixmap(size());
    doubleBuffer.fill();

    DAVA::ThemesSettings* themeSettings = accessor->GetGlobalContext()->GetData<DAVA::ThemesSettings>();
    QColor rulerBackgroundColor = themeSettings->GetRulerWidgetBackgroungColor();

    const QColor rulerTicksColor = themeSettings->GetRulerTextColor();
    static const int rulerFontSize = 10;

    static const int borderWidth = 2;
    static const int borderOffset = borderWidth / 2;
    static const int ticksWidth = 1;

    QPainter painter(&doubleBuffer);
    painter.fillRect(rect(), rulerBackgroundColor);

    // Draw the thick right and top borders.
    QPen curPen(rulerTicksColor, borderWidth);
    painter.setPen(curPen);

    if (orientation == Qt::Horizontal)
    {
        painter.drawLine(borderOffset, 0, borderOffset, rect().height());
        curPen.setWidth(ticksWidth);
        painter.setPen(curPen);
        painter.drawLine(0, rect().height() - ticksWidth, rect().width(), rect().height() - ticksWidth);
    }
    else
    {
        painter.drawLine(0, borderOffset, rect().width(), borderOffset);
        curPen.setWidth(ticksWidth);
        painter.setPen(curPen);

        painter.drawLine(rect().width() - ticksWidth, 0, rect().width() - ticksWidth, rect().height());
    }

    // Calculate the tick coords depending on orientation.
    int smallTickStart = 0;
    int bigTickStart = 0;
    int tickEnd = 0;

    if (orientation == Qt::Horizontal)
    {
        smallTickStart = (rect().height() / 5) * 3;
        tickEnd = rect().height();
    }
    else
    {
        smallTickStart = (rect().width() / 5) * 3;
        tickEnd = rect().width();
    }

    // Draw small ticks.
    DrawScale(painter, settings.smallTicksDelta, smallTickStart, tickEnd, false);

    // Draw big ticks and values.
    QFont font = painter.font();
    font.setPixelSize(rulerFontSize);
    painter.setFont(font);

    DrawScale(painter, settings.bigTicksDelta, bigTickStart, tickEnd, true);
}

void RulerWidget::resizeEvent(QResizeEvent* resizeEvent)
{
    UpdateDoubleBufferImage();

    QWidget::resizeEvent(resizeEvent);
}

void RulerWidget::mouseMoveEvent(QMouseEvent* mouseEvent)
{
    listener->OnMouseMove(GetMousePos(mouseEvent->pos()));
    QWidget::mouseMoveEvent(mouseEvent);
}

void RulerWidget::leaveEvent(QEvent* event)
{
    listener->OnMouseLeave();
    QWidget::leaveEvent(event);
}

void RulerWidget::mousePressEvent(QMouseEvent* mouseEvent)
{
    if (mouseEvent->button() == Qt::LeftButton)
    {
        listener->OnMousePress(GetMousePos(mouseEvent->pos()));
    }
    QWidget::mousePressEvent(mouseEvent);
}

void RulerWidget::mouseReleaseEvent(QMouseEvent* mouseEvent)
{
    if (mouseEvent->button() == Qt::LeftButton)
    {
        listener->OnMouseRelease(GetMousePos(mouseEvent->pos()));
    }
    QWidget::mouseReleaseEvent(mouseEvent);
}

DAVA::float32 RulerWidget::GetMousePos(const QPoint& pos) const
{
    return (orientation == Qt::Horizontal) ? pos.x() : pos.y();
}
