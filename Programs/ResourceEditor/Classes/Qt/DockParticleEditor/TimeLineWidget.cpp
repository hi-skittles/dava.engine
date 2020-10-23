#include "TimeLineWidget.h"

#include <QPaintEvent>
#include <QPainter>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>

#include <Base/Introspection.h>

#define POINT_SIZE 4

#define LEGEND_WIDTH 12

#define LOCK_TEXT "Lock "
#define LOCK_WIDTH 45

#define MINIMUM_DISPLAYED_TIME 0.02f
#define ZOOM_STEP 0.1f
#define UI_RECTANGLE_OFFSET 1.5

TimeLineWidget::TimeLineWidget(QWidget* parent)
    :
    ScrollZoomWidget(parent)
{
    gridStyle = GRID_STYLE_LIMITS;

    setMouseTracking(true);

    UpdateSizePolicy();
}

TimeLineWidget::~TimeLineWidget()
{
}

void TimeLineWidget::paintEvent(QPaintEvent* e)
{
    QPainter painter(this);

#ifdef Q_OS_WIN
    QFont font("Courier", 8, QFont::Normal);
#else
    QFont font("Courier", 11, QFont::Normal);
#endif
    painter.setFont(font);

    painter.fillRect(this->rect(), palette().window());
    painter.setPen(Qt::black);
    painter.drawRect(QRect(0, 0, width() - 1, height() - 1));

    QRect graphRect = GetGraphRect();

    //draw legend
    if (lines.size())
    {
        if (sizeState == SIZE_STATE_MINIMIZED)
        {
            LINES_MAP::iterator iter = lines.begin();
            QString legend = iter->second.legend;

            painter.setPen(Qt::blue);

            QRect textRect = rect();
            textRect.adjust(3, 0, 0, 0);
            painter.drawText(textRect, Qt::AlignLeft, legend);
        }
        else
        {
            for (LINES_MAP::iterator iter = lines.begin(); iter != lines.end(); ++iter)
            {
                QRect lineEnableRect = GetLineEnableRect(iter->first);
                painter.setPen(iter->second.color);

                painter.drawRect(lineEnableRect);
                if (iter->second.line.size() == 0)
                {
                    painter.drawLine(lineEnableRect.topLeft(), lineEnableRect.bottomRight());
                    painter.drawLine(lineEnableRect.bottomLeft(), lineEnableRect.topRight());
                }

                QString legend = iter->second.legend;
                painter.drawText(QPoint(lineEnableRect.right() + 4, lineEnableRect.bottom()), legend);
            }
        }
    }

    //draw minimizebox
    if (sizeState == SIZE_STATE_MINIMIZED)
    {
        DrawUITriangle(painter, GetMinimizeRect(), 180);
    }
    else
    {
        DrawUITriangle(painter, GetMinimizeRect(), 0);
    }

    //draw maximize box
    {
        painter.setPen(Qt::black);
        QRect maximizeRect = GetMaximizeRect();
        painter.drawRect(maximizeRect);
        maximizeRect.adjust(2, 2, -2, -2);
        painter.drawRect(maximizeRect);
    }

    if (sizeState != SIZE_STATE_MINIMIZED)
    {
        //draw lock
        if (isLockEnable)
        {
            QRect lockRect(GetLockRect());
            painter.drawRect(lockRect);
            if (isLocked)
            {
                painter.drawLine(lockRect.topLeft() + QPoint(-1, -1), lockRect.center() + QPoint(-1, 4));
                painter.drawLine(lockRect.center() + QPoint(-1, 4), lockRect.topRight() + QPoint(4, -1));
            }

            lockRect.translate(lockRect.width() + 1, 0);
            lockRect.setWidth(LOCK_WIDTH);
            painter.drawText(lockRect.bottomLeft(), LOCK_TEXT);
        }

        //draw grid
        {
            painter.setPen(Qt::gray);

            float step = 18;
            float steps = (graphRect.height() / 2.f) / step;
            float valueCenter = (maxValue - minValue) / 2.f + minValue;
            float valueStep = ((maxValue - minValue) / 2.f) / steps;
            for (int i = 0; i < steps; i++)
            {
                int y = graphRect.center().y() - i * step;
                {
                    float value = valueCenter + i * valueStep;
                    painter.drawLine(graphRect.left(), y, graphRect.right(), y);
                    if (gridStyle == GRID_STYLE_ALL_POSITION)
                    {
                        QString strValue = float2QString(value);
                        QRect textRect(1, y - LEGEND_WIDTH / 2, graphRect.left() - 2, y - LEGEND_WIDTH / 2);
                        painter.drawText(textRect, Qt::AlignRight, strValue);
                    }
                }

                y = graphRect.center().y() + i * step;
                {
                    float value = valueCenter - i * valueStep;
                    painter.drawLine(graphRect.left(), y, graphRect.right(), y);
                    if (gridStyle == GRID_STYLE_ALL_POSITION)
                    {
                        QString strValue = float2QString(value);
                        QRect textRect(1, y - LEGEND_WIDTH / 2, graphRect.left() - 2, y - LEGEND_WIDTH / 2);
                        painter.drawText(textRect, Qt::AlignRight, strValue);
                    }
                }
            }

            steps = (graphRect.width()) / step;
            valueStep = (maxTime - minTime) / steps;
            bool drawText = false;
            for (int i = 0; i <= steps; i++)
            {
                int x = graphRect.left() + i * step;
                painter.drawLine(x, graphRect.top(), x, graphRect.bottom());

                drawText = !drawText;
                if (drawText && gridStyle == GRID_STYLE_ALL_POSITION)
                {
                    float value = minTime + i * valueStep;
                    QString strValue = float2QString(value);
                    int textWidth = painter.fontMetrics().width(strValue);
                    QRect textRect(x - textWidth / 2, graphRect.bottom(), textWidth, LEGEND_WIDTH + 3);
                    painter.drawText(textRect, Qt::AlignCenter, strValue);
                }
            }

            if (gridStyle == GRID_STYLE_LIMITS)
            {
                // Draw Y axe legend.
                QRect textRect = QRect(1, graphRect.top(), graphRect.left(), graphRect.height());
                QString value = QString("%1%2").arg(float2QString(minValue)).arg(yLegendMark);
                painter.drawText(textRect, Qt::AlignBottom | Qt::AlignHCenter, value);
                value = QString("%1%2").arg(float2QString(maxValue)).arg(yLegendMark);
                painter.drawText(textRect, Qt::AlignTop | Qt::AlignHCenter, value);

                // Draw X axe legend.
                textRect = QRect(graphRect.left(), graphRect.bottom() + 1, graphRect.width(), LEGEND_WIDTH);
                value = QString("%1%2").arg(float2QString(minTime)).arg(xLegendMark);
                painter.drawText(textRect, Qt::AlignLeft | Qt::AlignTop, value);
                value = QString("%1%2").arg(float2QString(maxTime)).arg(xLegendMark);
                painter.drawText(textRect, Qt::AlignRight | Qt::AlignTop, value);
            }
        }

        //draw graph border
        painter.setPen(Qt::black);
        painter.drawRect(graphRect);

        //draw lines
        bool isLineEnable = false;
        DAVA::uint32 lineCount = 0;
        for (LINES_MAP::iterator iter = lines.begin(); iter != lines.end(); ++iter, ++lineCount)
        {
            DAVA::uint32 lineId = iter->first;
            if (drawLine == -1 || drawLine == lineId)
                DrawLine(&painter, lineId);

            QPen pen;
            pen.setColor(iter->second.color);
            painter.setPen(pen);

            if (iter->second.line.size())
            {
                isLineEnable = true;
            }

            //draw drawed colors
            QRect rect = GetLineDrawRect();
            rect.translate(static_cast<int>(rect.width() * lineCount / lines.size()), 0);
            rect.setWidth(static_cast<int>(rect.width() / lines.size()));
            if (drawLine == -1)
                painter.fillRect(rect, iter->second.color);
            else
                painter.fillRect(rect, lines[drawLine].color);
        }

        if (!isLineEnable)
        {
            QFont font("Courier", 14, QFont::Bold);
            painter.setFont(font);
            painter.setPen(Qt::black);
            painter.drawText(graphRect, Qt::AlignVCenter | Qt::AlignHCenter, "Property is not enabled");
        }
    }

    if (selectedPoint == -1 && selectedLine != -1)
    {
        QBrush pointBrush;
        pointBrush.setColor(lines[selectedLine].color);
        pointBrush.setStyle(Qt::SolidPattern);

        painter.fillRect(GetPointRect(GetDrawPoint(newPoint)), pointBrush);
    }

    ScrollZoomWidget::paintEvent(e);
}

void TimeLineWidget::DrawLine(QPainter* painter, DAVA::uint32 lineId)
{
    if (lines[lineId].line.size() == 0)
    {
        return;
    }

    if (FLOAT_EQUAL(generalMaxTime, generalMinTime)) //in case of zero life time
    {
        return;
    }

    QBrush pointBrush;
    pointBrush.setColor(lines[lineId].color);
    pointBrush.setStyle(Qt::SolidPattern);
    QPen pen;
    pen.setColor(lines[lineId].color);
    painter->setPen(pen);

    QRect graphRect = GetGraphRect();

    QPoint prevPoint = GetDrawPoint(lines[lineId].line[0]);
    prevPoint.setX(graphRect.x());

    for (uint i = 0; i < lines[lineId].line.size(); ++i)
    {
        QPoint point = GetDrawPoint(lines[lineId].line[i]);

        ePositionRelativelyToDrawRect leftPosition = GetPointPositionFromDrawingRect(prevPoint);
        ePositionRelativelyToDrawRect rightPosition = GetPointPositionFromDrawingRect(point);

        // if line(leftPosition, rightPosition) is outside of drawing rect skip it
        if (!((leftPosition == rightPosition) && (leftPosition != POSITION_INSIDE)))
        {
            QPoint firstPoint = prevPoint;
            QPoint secondPoint = point;

            GetCrossingPoint(prevPoint, point, firstPoint, secondPoint);

            painter->drawLine(firstPoint, secondPoint);

            //draw rects only if they are inside of drawingRect
            if (rightPosition == POSITION_INSIDE)
            {
                if (selectedPoint == i && selectedLine == lineId)
                {
                    painter->fillRect(GetPointRect(point), pointBrush);
                }
                else
                {
                    painter->drawRect(GetPointRect(point));
                }
            }
        }
        prevPoint = point;
    }

    QPoint point = GetDrawPoint(lines[lineId].line[lines[lineId].line.size() - 1]);
    point.setX(graphRect.x() + graphRect.width());

    //cut horizontal axis to boundaries
    if (prevPoint.x() < graphRect.x())
    {
        prevPoint.setX(graphRect.x());
    }
    else if (prevPoint.x() > graphRect.x() + graphRect.width())
    {
        prevPoint.setX(graphRect.x() + graphRect.width());
    }

    painter->drawLine(prevPoint, point);
}

QPoint TimeLineWidget::GetDrawPoint(const DAVA::Vector2& point) const
{
    DAVA::float32 time = maxTime - minTime;
    DAVA::float32 value = maxValue - minValue;
    if (time < 0.01f || value < 0.01f)
        return QPoint();

    QRect graphRect = GetGraphRect();
    float x = graphRect.x() + graphRect.width() * (point.x - minTime) / time;

    float y = graphRect.bottom() - graphRect.height() * (point.y - minValue) / value;

    return QPoint(x, y);
}

DAVA::Vector2 TimeLineWidget::GetLogicPoint(const QPoint& point) const
{
    QRect graphRect = GetGraphRect();

    DAVA::float32 x = (point.x() - graphRect.x()) / static_cast<DAVA::float32>(graphRect.width());
    x = minTime + x * (maxTime - minTime);
    DAVA::float32 y = (graphRect.bottom() - point.y()) / static_cast<DAVA::float32>(graphRect.height());
    y = minValue + y * (maxValue - minValue);

    if (isInteger)
    {
        y = GetIntValue(y);
    }

    return DAVA::Vector2(x, y);
}

QRect TimeLineWidget::GetPointRect(const QPoint& point) const
{
    return QRect(point.x() - POINT_SIZE, point.y() - POINT_SIZE, POINT_SIZE * 2, POINT_SIZE * 2);
}

bool TimeLineWidget::SortPoints(const DAVA::Vector2& i, const DAVA::Vector2& j)
{
    return (i.x < j.x);
}

void TimeLineWidget::Init(DAVA::float32 minT, DAVA::float32 maxT, bool updateSizeState, bool aliasLinePoint, bool allowDeleteLine, bool integer, DAVA::int32 valueDecimalsPrecision)
{
    lines.clear();

    this->updateSizeState = updateSizeState;
    this->aliasLinePoint = aliasLinePoint;
    this->allowDeleteLine = allowDeleteLine;
    this->valueDecimalsPrecision = valueDecimalsPrecision;

    this->isInteger = integer;
    ScrollZoomWidget::Init(minT, maxT);
}

void TimeLineWidget::Init(DAVA::float32 minT, DAVA::float32 maxT, DAVA::float32 generalMinT, DAVA::float32 generalMaxT, bool updateSizeState, bool aliasLinePoint, bool allowDeleteLine, bool integer, DAVA::int32 valueDecimalsPrecision_)
{
    Init(minT, maxT, updateSizeState, aliasLinePoint, allowDeleteLine, integer, valueDecimalsPrecision_);
    this->minTime = minT;
    this->maxTime = maxT;
    this->generalMinTime = generalMinT;
    this->generalMaxTime = generalMaxT;
    this->initialTimeInterval = generalMaxTime - generalMinTime;
    scale = (generalMaxT - generalMinT) / (maxT - minT);

    UpdateScrollBarSlider();
    UpdateZoomSlider();
}

void TimeLineWidget::SetMinLimits(DAVA::float32 minV)
{
    minValueLimit = minV;
}

void TimeLineWidget::SetMaxLimits(DAVA::float32 maxV)
{
    maxValueLimit = maxV;
}

void TimeLineWidget::AddLine(DAVA::uint32 lineId, const DAVA::Vector<DAVA::PropValue<DAVA::float32>>& line, const QColor& color, const QString& legend)
{
    LOGIC_POINTS desLine;
    for (DAVA::uint32 i = 0; i < line.size(); ++i)
        desLine.push_back(DAVA::Vector2(line[i].t, line[i].v));
    if (desLine.size() == 1) //force correct min time
    {
        desLine[0].x = minTime;
    }

    lines[lineId].line = desLine;
    lines[lineId].color = color;
    lines[lineId].legend = legend;

    PostAddLine();
}

void TimeLineWidget::AddLines(const DAVA::Vector<DAVA::PropValue<DAVA::Vector2>>& lines, const DAVA::Vector<QColor>& colors, const DAVA::Vector<QString>& legends)
{
    if (colors.size() < 2 || legends.size() < 2)
    {
        DAVA::Logger::FrameworkDebug("incorrect number of input arguments");
        return;
    }

    LOGIC_POINTS desLine[2];
    for (size_t i = 0; i < lines.size(); ++i)
    {
        desLine[0].push_back(DAVA::Vector2(lines[i].t, lines[i].v.x));
        desLine[1].push_back(DAVA::Vector2(lines[i].t, lines[i].v.y));
    }
    for (size_t i = 0; i < 2; ++i)
    {
        if (desLine[i].size() == 1) //force correct min time
        {
            desLine[i][0].x = minTime;
        }
    }

    for (DAVA::uint32 i = 0; i < 2; i++)
    {
        DAVA::uint32 id = static_cast<DAVA::uint32>(this->lines.size());
        // no panic, this->lines - is map<uint32, struct>
        // so, looks like we are just adding another element here
        this->lines[id].line = desLine[i];
        this->lines[id].color = colors[i];
        this->lines[id].legend = legends[i];
    }

    PostAddLine();
}

void TimeLineWidget::AddLines(const DAVA::Vector<DAVA::PropValue<DAVA::Vector3>>& lines, const DAVA::Vector<QColor>& colors, const DAVA::Vector<QString>& legends)
{
    if (colors.size() < 3 || legends.size() < 3)
    {
        DAVA::Logger::FrameworkDebug("incorrect number of input arguments");
        return;
    }

    LOGIC_POINTS desLine[3];
    for (DAVA::uint32 i = 0; i < lines.size(); ++i)
    {
        desLine[0].push_back(DAVA::Vector2(lines[i].t, lines[i].v.x));
        desLine[1].push_back(DAVA::Vector2(lines[i].t, lines[i].v.y));
        desLine[2].push_back(DAVA::Vector2(lines[i].t, lines[i].v.z));
    }
    for (DAVA::int32 i = 0; i < 3; ++i)
    {
        if (desLine[i].size() == 1) //force correct min time
        {
            desLine[i][0].x = minTime;
        }
    }

    for (DAVA::int32 i = 0; i < 3; i++)
    {
        DAVA::uint32 id = static_cast<DAVA::uint32>(this->lines.size());
        // no panic, this->lines - is map<uint32, struct>
        // so, looks like we are just adding another element here
        this->lines[id].line = desLine[i];
        this->lines[id].color = colors[i];
        this->lines[id].legend = legends[i];
    }

    PostAddLine();
}

void TimeLineWidget::PostAddLine()
{
    if (updateSizeState)
    {
        sizeState = SIZE_STATE_MINIMIZED;
        for (LINES_MAP::const_iterator iter = lines.begin(); iter != lines.end(); ++iter)
        {
            if (iter->second.line.size())
            {
                sizeState = SIZE_STATE_NORMAL;
                break;
            }
        }
    }

    UpdateLimits();
    UpdateSizePolicy();
}

void TimeLineWidget::UpdateLimits()
{
    DAVA::float32 newMinValue = std::numeric_limits<DAVA::float32>::infinity();
    DAVA::float32 newMaxValue = -std::numeric_limits<DAVA::float32>::infinity();

    for (LINES_MAP::iterator iter = lines.begin(); iter != lines.end(); ++iter)
    {
        for (DAVA::uint32 i = 0; i < iter->second.line.size(); ++i)
        {
            newMaxValue = DAVA::Max(iter->second.line[i].y, newMaxValue);
            newMinValue = DAVA::Min(iter->second.line[i].y, newMinValue);
            /*
            maxTime = Max(iter->second.line[i].x, maxTime);
            minTime = Min(iter->second.line[i].x, minTime);*/
        }
    }

    if (newMinValue == std::numeric_limits<DAVA::float32>::infinity() ||
        newMaxValue == -std::numeric_limits<DAVA::float32>::infinity())
    {
        newMinValue = newMaxValue = 0;
    }

    newMinValue = DAVA::Max(newMinValue, minValueLimit);
    newMaxValue = DAVA::Min(newMaxValue, maxValueLimit);

    DAVA::float32 limitDelta = 0;
    limitDelta = (newMaxValue - newMinValue) * 0.2f;
    if (limitDelta < 0.01f)
        limitDelta = newMaxValue * 0.2;
    if (limitDelta < 0.01f)
        limitDelta = 1.f;

    if (DAVA::Abs(maxValue) > DAVA::Abs(newMaxValue) * 1.2 ||
        DAVA::Abs(minValue) < DAVA::Abs(newMinValue) * 1.2 ||
        newMinValue < minValue ||
        newMaxValue > maxValue)
    {
        minValue = newMinValue - limitDelta;
        maxValue = newMaxValue + limitDelta;
    }

    if (isInteger)
    {
        minValue = GetIntValue(minValue);
        maxValue = GetIntValue(maxValue);

        if (minValue >= newMinValue)
        {
            minValue = GetIntValue(newMinValue - 1.f);
        }
        if (maxValue <= newMaxValue)
        {
            maxValue = GetIntValue(newMaxValue + 1.f);
        }
    }
}

bool TimeLineWidget::GetValue(DAVA::uint32 lineId, DAVA::Vector<DAVA::PropValue<DAVA::float32>>* line) const
{
    LINES_MAP::const_iterator iter = lines.find(lineId);
    if (iter == lines.end())
        return false;

    for (DAVA::uint32 i = 0; i < iter->second.line.size(); ++i)
    {
        line->push_back(DAVA::PropValue<DAVA::float32>(iter->second.line[i].x, iter->second.line[i].y));
    }

    return true;
}

bool TimeLineWidget::GetValues(DAVA::Vector<DAVA::PropValue<DAVA::Vector2>>* lines)
{
    LINES_MAP::const_iterator iter = this->lines.begin();
    if (iter == this->lines.end())
        return false;

    for (DAVA::uint32 i = 0; i < iter->second.line.size(); ++i)
    {
        DAVA::Vector2 value;
        value.x = this->lines[0].line[i].y;
        value.y = this->lines[1].line[i].y;
        lines->push_back(DAVA::PropValue<DAVA::Vector2>(this->lines[0].line[i].x, value));
    }
    return true;
}

bool TimeLineWidget::GetValues(DAVA::Vector<DAVA::PropValue<DAVA::Vector3>>* lines)
{
    LINES_MAP::const_iterator iter = this->lines.begin();
    if (iter == this->lines.end())
        return false;

    for (DAVA::uint32 i = 0; i < iter->second.line.size(); ++i)
    {
        DAVA::Vector3 value;
        value.x = this->lines[0].line[i].y;
        value.y = this->lines[1].line[i].y;
        value.z = this->lines[2].line[i].y;
        lines->push_back(DAVA::PropValue<DAVA::Vector3>(this->lines[0].line[i].x, value));
    }
    return true;
}

void TimeLineWidget::AddPoint(DAVA::uint32 lineId, const DAVA::Vector2& point)
{
    if (aliasLinePoint)
    {
        for (LINES_MAP::iterator iter = lines.begin(); iter != lines.end(); ++iter)
        {
            if ((isLockEnable && isLocked) || iter->first == lineId)
                iter->second.line.push_back(DAVA::Vector2(point.x, point.y));
            else
            {
                DAVA::float32 y = GetYFromX(iter->first, point.x);
                if (isInteger)
                {
                    y = GetIntValue(y);
                }
                iter->second.line.push_back(DAVA::Vector2(point.x, y));
            }
            std::sort(iter->second.line.begin(), iter->second.line.end(), TimeLineWidget::SortPoints);
        }
    }
    else if (lines.find(lineId) != lines.end())
    {
        lines[lineId].line.push_back(point);
        std::sort(lines[lineId].line.begin(), lines[lineId].line.end(), TimeLineWidget::SortPoints);
    }
    this->update();
}

bool TimeLineWidget::DeletePoint(DAVA::uint32 lineId, DAVA::uint32 pointId)
{
    if (!allowDeleteLine &&
        lines[lineId].line.size() < 2)
        return false;

    selectedLine = -1;
    if (aliasLinePoint)
    {
        for (LINES_MAP::iterator iter = lines.begin(); iter != lines.end(); ++iter)
        {
            if (pointId < iter->second.line.size())
                iter->second.line.erase(iter->second.line.begin() + pointId);
        }
    }
    else if (lines.find(lineId) != lines.end())
    {
        if (pointId < lines[lineId].line.size())
            lines[lineId].line.erase(lines[lineId].line.begin() + pointId);
    }
    this->update();
    return true;
}

DAVA::float32 TimeLineWidget::GetYFromX(DAVA::uint32 lineId, DAVA::float32 x)
{
    LOGIC_POINTS& points = lines.at(lineId).line;

    if (points.empty())
        return 0.f;

    DAVA::uint32 right = static_cast<DAVA::uint32>(-1);
    for (DAVA::uint32 i = 0; i < points.size(); ++i)
    {
        if (points[i].x > x)
        {
            right = i;
            break;
        }
    }

    DAVA::Vector2 leftPoint;
    DAVA::Vector2 rightPoint;
    if (right == static_cast<DAVA::uint32>(-1))
    {
        leftPoint = points.back();
        rightPoint = points.back() + DAVA::Vector2(x, 0);
    }
    else
    {
        rightPoint = points[right];
        if (right > 0)
            leftPoint = points[right - 1];
        else
        {
            leftPoint = rightPoint;
            leftPoint.x = 0;
        }
    }
    DAVA::float32 y = DAVA::Interpolation::Linear(leftPoint.y, rightPoint.y, leftPoint.x, x, rightPoint.x);

    return y;
}

QRect TimeLineWidget::GetGraphRect() const
{
    QRect graphRect = this->rect();
    graphRect.setX(graphRect.x() + 40);
    /*if (IsLegendEmpty())
        graphRect.setY(graphRect.y() + 5);
    else
        graphRect.setY(graphRect.y() + 2 + LEGEND_WIDTH);
    graphRect.setWidth(graphRect.width() - 5);
    if (sizeState == SizeStateMinimized)
        graphRect.setHeight(0);
    else
        graphRect.setHeight(graphRect.height() - 30);*/

    graphRect.setWidth(graphRect.width() - 5);
    graphRect.setY(GetLegendHeight());
    if (sizeState == SIZE_STATE_MINIMIZED)
    {
        graphRect.setHeight(0);
    }
    else
    {
        graphRect.setHeight(this->height() - graphRect.y() - LEGEND_WIDTH - 1 - SCROLL_BAR_HEIGHT);
    }

    return graphRect;
}

void TimeLineWidget::mousePressEvent(QMouseEvent* event)
{
    QWidget::mousePressEvent(event);

    //check click on draw color rect
    if (event->button() == Qt::LeftButton)
    {
        if (isLockEnable && GetLockRect().contains(event->pos()))
        {
            isLocked = !isLocked;
        }
        else if (GetMinimizeRect().contains(event->pos()))
        {
            if (sizeState == SIZE_STATE_MINIMIZED)
                sizeState = SIZE_STATE_NORMAL;
            else
                sizeState = SIZE_STATE_MINIMIZED;
            UpdateSizePolicy();
            return;
        }
        else if (GetMaximizeRect().contains(event->pos()))
        {
            if (sizeState == SIZE_STATE_NORMAL)
                sizeState = SIZE_STATE_DOUBLE;
            else
                sizeState = SIZE_STATE_NORMAL;
            UpdateSizePolicy();
            return;
        }
        else if (GetLineDrawRect().contains(event->pos()))
        {
            drawLine++;
            if (drawLine >= static_cast<DAVA::int32>(lines.size()))
                drawLine = -1;
        }
        else
        {
            for (LINES_MAP::iterator iter = lines.begin(); iter != lines.end(); ++iter)
            {
                QRect rect = GetLineEnableRect(iter->first);
                if (rect.contains(event->pos()))
                {
                    if (aliasLinePoint)
                    {
                        for (LINES_MAP::iterator iter = lines.begin(); iter != lines.end(); ++iter)
                            if (iter->second.line.size())
                                iter->second.line.clear(); //clear existing line
                            else
                                iter->second.line.push_back(DAVA::Vector2(minTime, (minValue + maxValue) / 2)); //init default
                    }
                    else
                    {
                        if (iter->second.line.size())
                            iter->second.line.clear(); //clear existing line
                        else
                            iter->second.line.push_back(DAVA::Vector2(minTime, (minValue + maxValue) / 2)); //init default
                    }
                    emit ValueChanged();
                    break;
                }
            }
        }
    }

    ScrollZoomWidget::mousePressEvent(event);
    if (sizeState != SIZE_STATE_MINIMIZED)
        GraphRectClick(event);

    update();
}

void TimeLineWidget::GraphRectClick(QMouseEvent* event)
{
    DAVA::int32 pointId = -1;
    DAVA::int32 lineId = -1;
    QPoint point = event->pos();

    GetClickedPoint(point, pointId, lineId);

    if (event->button() == Qt::LeftButton)
    {
        if (pointId != -1)
        {
            this->selectedPoint = pointId;
            this->selectedLine = lineId;
        }
        else if (selectedLine != -1)
        {
            QRect graphRect = GetGraphRect();
            if (graphRect.contains(point))
            {
                if (isInteger)
                {
                    newPoint.y = GetIntValue(newPoint.y);
                }
                AddPoint(selectedLine, newPoint);
                //find add point
                for (DAVA::uint32 i = 0; i < lines[selectedLine].line.size(); ++i)
                {
                    if (lines[selectedLine].line[i].x == newPoint.x)
                    {
                        selectedPoint = i;
                        break;
                    }
                }
            }
        }
    }
    else if (event->button() == Qt::RightButton && pointId != -1)
    {
        DeletePoint(selectedLine, pointId);
        emit ValueChanged();
    }

    update();
}

void TimeLineWidget::mouseMoveEvent(QMouseEvent* event)
{
    QWidget::mouseMoveEvent(event);

    if (sizeState == SIZE_STATE_MINIMIZED)
        return;

    DAVA::Vector2 point = GetLogicPoint(event->pos());
    if (selectedPoint == -1)
    {
        selectedLine = -1;
        //get selected line
        for (LINES_MAP::iterator iter = lines.begin(); iter != lines.end(); ++iter)
        {
            DAVA::uint32 lineId = iter->first;
            if (drawLine != -1 && drawLine != lineId)
                continue;

            const LOGIC_POINTS& line = iter->second.line;
            if (line.size() == 0)
                continue;

            DAVA::Vector2 prevPoint = line[0];
            prevPoint.x = minTime;
            for (DAVA::uint32 i = 0; i < line.size() + 1; ++i)
            {
                DAVA::Vector2 nextPoint;
                if (i < line.size())
                    nextPoint = line[i];
                else
                {
                    nextPoint = prevPoint;
                    nextPoint.x = maxTime;
                }

                if (prevPoint.x < point.x && point.x < nextPoint.x)
                {
                    DAVA::float32 y = 0;
                    if ((nextPoint.x - prevPoint.x) < 0.01f)
                        y = prevPoint.y;
                    else
                        y = (point.x - prevPoint.x) * (nextPoint.y - prevPoint.y) / (nextPoint.x - prevPoint.x) + prevPoint.y;

                    QRect rect = GetPointRect(GetDrawPoint(DAVA::Vector2(point.x, y)));
                    if (rect.contains(event->pos()))
                    {
                        newPoint = DAVA::Vector2(point.x, y);
                        selectedLine = lineId;
                        break;
                    }
                    else
                    {
                        ScrollZoomWidget::mouseMoveEvent(event);
                    }
                }
                prevPoint = nextPoint;
            }

            if (selectedLine != -1)
                break;
        }
    }
    else
    {
        SetPointValue(selectedLine, selectedPoint, point, false);
    }

    update();
}

void TimeLineWidget::mouseReleaseEvent(QMouseEvent* event)
{
    QWidget::mouseReleaseEvent(event);

    if ((mouseStartPos - event->pos()).manhattanLength() > 3)
    {
        if (event->button() == Qt::LeftButton)
        {
            if (selectedLine != -1 && selectedPoint != -1)
            {
                DAVA::Vector2 point = GetLogicPoint(event->pos());
                SetPointValue(selectedLine, selectedPoint, point, true);
                emit ValueChanged();
            }
        }
    }
    selectedPoint = -1;
    selectedLine = -1;

    ScrollZoomWidget::mouseReleaseEvent(event);
}

void TimeLineWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    QWidget::mouseDoubleClickEvent(event);

    if (event->button() == Qt::LeftButton)
    {
        DAVA::int32 pointId = -1;
        DAVA::int32 lineId = -1;
        GetClickedPoint(event->pos(), pointId, lineId);

        if (lineId != -1)
        {
            ChangePointValueDialog(pointId, lineId);
        }
    }
}

void TimeLineWidget::SetPointValue(DAVA::uint32 lineId, DAVA::uint32 pointId, DAVA::Vector2 value, bool deleteSamePoints)
{
    if (lineId >= lines.size())
    {
        return;
    }

    if (pointId > 0)
        value.x = DAVA::Max(lines[lineId].line[pointId - 1].x, value.x);
    if (pointId < (lines[lineId].line.size() - 1))
        value.x = DAVA::Min(lines[lineId].line[pointId + 1].x, value.x);

    value.x = DAVA::Max(minTime, DAVA::Min(maxTime, value.x));
    value.y = DAVA::Max(minValueLimit, DAVA::Min(maxValueLimit, value.y));

    if (aliasLinePoint)
    {
        for (LINES_MAP::iterator iter = lines.begin(); iter != lines.end(); ++iter)
        {
            if ((isLockEnable && isLocked) || iter->first == lineId)
                iter->second.line[pointId] = value;
            else
                iter->second.line[pointId].x = value.x;
        }
    }
    else
        lines[lineId].line[pointId] = value;

    if (deleteSamePoints)
    {
        //delete same time point
        for (DAVA::uint32 i = 1; i < lines[lineId].line.size(); ++i)
        {
            float x1 = lines[lineId].line[i - 1].x;
            float x2 = lines[lineId].line[i].x;

            if ((x2 - x1) < (maxTime - minTime) * 0.001)
            {
                if (i < lines[lineId].line.size() - 1)
                {
                    if (DAVA::Abs(x2 - value.x) < 0.01f)
                    {
                        //lines[lineId].line[i - 1].y = value.y;
                        for (LINES_MAP::iterator iter = lines.begin(); iter != lines.end(); ++iter)
                        {
                            if ((isLockEnable && isLocked) || iter->first == lineId)
                                iter->second.line[i - 1].y = value.y;
                        }
                    }
                    //remove next point
                    //lines[lineId].line.erase(lines[lineId].line.begin() + i);
                    DeletePoint(lineId, i);
                }
                else
                {
                    if (DAVA::Abs(x1 - value.x) < 0.01f)
                    {
                        //lines[lineId].line[i].y = value.y;
                        for (LINES_MAP::iterator iter = lines.begin(); iter != lines.end(); ++iter)
                        {
                            if ((isLockEnable && isLocked) || iter->first == lineId)
                                iter->second.line[i].y = value.y;
                        }
                    }

                    DeletePoint(lineId, static_cast<DAVA::uint32>(lines[lineId].line.size() - 2));
                }
                i = 0;
            }
        }
    }

    if (deleteSamePoints)
        UpdateLimits();
    update();
}

void TimeLineWidget::leaveEvent(QEvent*)
{
    selectedLine = -1;

    update();
}

QRect TimeLineWidget::GetLineEnableRect(DAVA::uint32 lineId) const
{
    /*uint32 lineCount = 0;
    for (LINES_MAP::const_iterator iter = lines.begin(); iter != lines.end(); ++iter, ++lineCount)
    {
        if (iter->first == lineId)
            break;
    }
    
    QRect graphRect = GetGraphRect();
    int rectSize = 10;
    QRect lineEnableRect(0, 0, rectSize, rectSize);
    lineEnableRect.translate(graphRect.left() + 50 + rectSize * 2 * lineCount, this->rect().bottom() - 15);
    return lineEnableRect;*/

    DAVA::uint32 lineCount = 0;
    for (LINES_MAP::const_iterator iter = lines.begin(); iter != lines.end(); ++iter, ++lineCount)
    {
        if (iter->first == lineId)
            break;
    }
    int rectSize = 10;
    QRect lineEnableRect(0, 0, rectSize, rectSize);
    lineEnableRect.translate(50, 2 + (rectSize + 3) * lineCount);
    return lineEnableRect;
}

int TimeLineWidget::GetLegendHeight() const
{
    return GetLineEnableRect(-1).top();
}

QRect TimeLineWidget::GetLineDrawRect() const
{
    /*	QRect graphRect = GetGraphRect();
    QRect lineDrawRect(0, 0, 40, 10);
    lineDrawRect.translate(graphRect.left() + 5, this->rect().bottom() - 15);*/

    QRect lineDrawRect(5, 2, 40, LEGEND_WIDTH);
    return lineDrawRect;
}

QRect TimeLineWidget::GetMinimizeRect() const
{
    QRect rect = GetMaximizeRect();
    rect.translate(-rect.width() * UI_RECTANGLE_OFFSET, 0);
    return rect;
}

QRect TimeLineWidget::GetIncreaseRect() const
{
    QRect rect = GetScaleRect();
    rect.translate(-rect.width() * UI_RECTANGLE_OFFSET, 0);
    return rect;
}

QRect TimeLineWidget::GetScaleRect() const
{
    QRect rect(GetLockRect());
    rect.translate(-SCALE_WIDTH, 0);
    return rect;
}

QRect TimeLineWidget::GetDecreaseRect() const
{
    QRect rect = GetSliderRect();
    int sideLength = LEGEND_WIDTH - 2;
    rect.translate(-sideLength * UI_RECTANGLE_OFFSET, 0);
    rect.setWidth(sideLength);
    rect.setHeight(sideLength);
    return rect;
}

QRect TimeLineWidget::GetSliderRect() const
{
    QRect rect = GetIncreaseRect();
    rect.translate(-(ZOOM_SLIDER_LENGTH + 5), 0);
    rect.setWidth(ZOOM_SLIDER_LENGTH);
    rect.setHeight(rect.height() + SLIDER_HEIGHT_EXPAND);
    return rect;
}

QRect TimeLineWidget::GetMaximizeRect() const
{
    return QRect(this->width() - LEGEND_WIDTH - 2, 2, LEGEND_WIDTH - 2, LEGEND_WIDTH - 2);
}

QRect TimeLineWidget::GetLockRect() const
{
    QRect rect(GetMinimizeRect());
    rect.translate(-LOCK_WIDTH, 0);
    return rect;
}

void TimeLineWidget::UpdateSizePolicy()
{
    switch (sizeState)
    {
    case SIZE_STATE_MINIMIZED:
    {
        setMinimumHeight(16);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        horizontalScrollBar->setEnabled(false);
        horizontalScrollBar->setVisible(false);
        zoomSlider->setEnabled(false);
    }
    break;
    case SIZE_STATE_NORMAL:
    {
        setMinimumHeight(GetLegendHeight() + GRAPH_HEIGHT + SCROLL_BAR_HEIGHT);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        horizontalScrollBar->setEnabled(true);
        horizontalScrollBar->setVisible(true);

        zoomSlider->setEnabled(true);
    }
    break;
    case SIZE_STATE_DOUBLE:
    {
        int height = GetLegendHeight() + GRAPH_HEIGHT;
        height *= 2;
        setMinimumHeight(height + SCROLL_BAR_HEIGHT);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        horizontalScrollBar->setEnabled(true);
        horizontalScrollBar->setVisible(true);

        zoomSlider->setEnabled(true);
    }
    break;
    }

    update();
}

void TimeLineWidget::GetClickedPoint(const QPoint& point, DAVA::int32& pointId, DAVA::int32& lineId) const
{
    //find point
    for (LINES_MAP::const_iterator iter = lines.begin(); iter != lines.end(); ++iter)
    {
        if (drawLine != -1 && drawLine != iter->first)
            continue;

        for (DAVA::uint32 j = 0; j < iter->second.line.size(); ++j)
        {
            QRect rect = GetPointRect(GetDrawPoint(iter->second.line[j]));
            if (rect.contains(point))
            {
                pointId = j;
                break;
            }
        }
        if (pointId != -1)
        {
            lineId = iter->first;
            return;
        }
    }
    pointId = -1;
    lineId = -1;
}

void TimeLineWidget::ChangePointValueDialog(DAVA::uint32 pointId, DAVA::int32 lineId)
{
    LINES_MAP::iterator iter = lines.find(lineId);
    if (iter == lines.end())
        return;
    if (iter->second.line.size() <= pointId)
        return;

    SetPointValueDlg dialog(iter->second.line[pointId].x, minTime, maxTime, iter->second.line[pointId].y, minValueLimit, maxValueLimit, this, isInteger, valueDecimalsPrecision);
    if (dialog.exec())
    {
        DAVA::float32 value = dialog.GetValue();
        if (isInteger)
        {
            value = GetIntValue(value);
        }

        SetPointValue(iter->first, pointId, DAVA::Vector2(dialog.GetTime(), value), true);
        UpdateLimits();
        emit ValueChanged();
        update();
    }
}

void TimeLineWidget::EnableLock(bool enable)
{
    isLockEnable = enable;
}

void TimeLineWidget::SetVisualState(DAVA::KeyedArchive* visualStateProps)
{
    if (!visualStateProps)
        return;

    isLocked = visualStateProps->GetBool("IS_LOCKED", false);
    sizeState = (eSizeState)visualStateProps->GetInt32("SIZE_STATE", SIZE_STATE_NORMAL);
    drawLine = visualStateProps->GetInt32("DRAW_LINE", -1);

    UpdateSizePolicy();
}

void TimeLineWidget::GetVisualState(DAVA::KeyedArchive* visualStateProps)
{
    if (!visualStateProps)
        return;

    visualStateProps->SetBool("IS_LOCKED", isLocked);
    visualStateProps->SetInt32("SIZE_STATE", sizeState);
    visualStateProps->SetInt32("DRAW_LINE", drawLine);
}

void TimeLineWidget::PerformZoom(float newScale, bool moveSlider)
{
    if (sizeState != SIZE_STATE_NORMAL)
    {
        return;
    }
    ScrollZoomWidget::PerformZoom(newScale, moveSlider);
}

void TimeLineWidget::PerformOffset(int value, bool moveScroll)
{
    if (sizeState != SIZE_STATE_NORMAL)
    {
        return;
    }
    ScrollZoomWidget::PerformOffset(value, moveScroll);
}

void TimeLineWidget::DrawUITriangle(QPainter& painter, const QRect& rect, int rotateDegree)
{
    painter.setPen(Qt::black);
    //QRect rect = GetOffsetRightRect();
    painter.drawRect(rect);
    painter.save();
    painter.translate(rect.center() + QPoint(1, 1));
    QPolygon polygon;

    painter.rotate(rotateDegree);

    polygon.append(QPoint(0, -rect.height() * 0.25 - 1));
    polygon.append(QPoint(rect.width() * 0.25 + 1, rect.height() * 0.25 + 1));
    polygon.append(QPoint(-rect.width() * 0.25 - 1, rect.height() * 0.25 + 1));

    QPainterPath painterPath;
    painterPath.addPolygon(polygon);
    painter.fillPath(painterPath, Qt::black);
    painter.restore();
}

//find out two points (leftBorderCrossPoint, rightBorderCrossPoint) in wich
// line(firstPoint;secondPoint) cross left and right boundariaes of drawing rectangle
void TimeLineWidget::GetCrossingPoint(const QPoint& firstPoint, const QPoint& secondPoint, QPoint& leftBorderCrossPoint, QPoint& rightBorderCrossPoint)
{
    QRect graphRect = GetGraphRect();
    if (rightBorderCrossPoint.x() < graphRect.x())
    {
        rightBorderCrossPoint.setX(graphRect.x());
    }

    if (!(firstPoint.x() < secondPoint.x()))
    {
        return;
    }

    if (FLOAT_EQUAL(secondPoint.x() - firstPoint.x(), 0.0f))
    {
        return;
    }

    ePositionRelativelyToDrawRect leftPosition = GetPointPositionFromDrawingRect(firstPoint);
    ePositionRelativelyToDrawRect rightPosition = GetPointPositionFromDrawingRect(secondPoint);

    //calc Y value of points through arctangens
    if (leftPosition == POSITION_LEFT)
    {
        float angleRad = atan((float)(secondPoint.y() - firstPoint.y()) / (secondPoint.x() - firstPoint.x()));
        float b = graphRect.x() - firstPoint.x();

        float a = tan(angleRad) * b;

        leftBorderCrossPoint.setX(graphRect.x());
        leftBorderCrossPoint.setY(firstPoint.y() + a);
    }
    if (rightPosition == POSITION_RIGHT)
    {
        float angleRad = atan((float)(firstPoint.y() - secondPoint.y()) / (secondPoint.x() - firstPoint.x()));
        float b = secondPoint.x() - (graphRect.x() + graphRect.width());

        float a = tan(angleRad) * b;

        rightBorderCrossPoint.setX(graphRect.x() + graphRect.width());
        rightBorderCrossPoint.setY(secondPoint.y() + a);
    }
}

// Add the mark to X/Y legend values (like 'deg' or 'pts').
void TimeLineWidget::SetXLegendMark(const QString& value)
{
    this->xLegendMark = value;
}

void TimeLineWidget::SetYLegendMark(const QString& value)
{
    this->yLegendMark = value;
}

SetPointValueDlg::SetPointValueDlg(DAVA::float32 time, DAVA::float32 minTime, DAVA::float32 maxTime, DAVA::float32 value,
                                   DAVA::float32 minValue, DAVA::float32 maxValue, QWidget* parent, bool integer, DAVA::int32 valueDecimalsPrecision)
    : QDialog(parent)
    , isInteger(integer)
{
    QVBoxLayout* mainBox = new QVBoxLayout;
    setLayout(mainBox);

    QHBoxLayout* valueBox = new QHBoxLayout;
    timeSpin = new EventFilterDoubleSpinBox(this);

    if (isInteger)
        valueSpinInt = new QSpinBox(this);
    else
        valueSpin = new EventFilterDoubleSpinBox(this);

    valueBox->addWidget(new QLabel("T:"));
    valueBox->addWidget(timeSpin);
    valueBox->addWidget(new QLabel("V:"));

    if (isInteger)
        valueBox->addWidget(valueSpinInt);
    else
        valueBox->addWidget(valueSpin);

    mainBox->addLayout(valueBox);

    QHBoxLayout* btnBox = new QHBoxLayout;
    QPushButton* btnCancel = new QPushButton("Cancel", this);
    QPushButton* btnOk = new QPushButton("Ok", this);
    btnBox->addWidget(btnCancel);
    btnBox->addWidget(btnOk);
    mainBox->addLayout(btnBox);

    timeSpin->setMinimum(minTime);
    timeSpin->setMaximum(maxTime);
    timeSpin->setValue(time);

    maxValue = DAVA::Min(maxValue, 1000000.f);

    if (isInteger)
    {
        valueSpinInt->setMinimum(static_cast<DAVA::int32>(minValue));
        valueSpinInt->setMaximum(static_cast<DAVA::int32>(maxValue));
        valueSpinInt->setValue(static_cast<DAVA::int32>(value));
    }
    else
    {
        valueSpin->setDecimals(valueDecimalsPrecision);
        valueSpin->setMinimum(minValue);
        valueSpin->setMaximum(maxValue);
        valueSpin->setValue(value);
    }

    connect(btnOk,
            SIGNAL(clicked(bool)),
            this,
            SLOT(accept()));
    connect(btnCancel,
            SIGNAL(clicked(bool)),
            this,
            SLOT(reject()));

    btnOk->setDefault(true);
    if (isInteger)
    {
        valueSpinInt->setFocus();
        valueSpinInt->selectAll();
    }
    else
    {
        valueSpin->setFocus();
        valueSpin->selectAll();
    }
}

DAVA::float32 SetPointValueDlg::GetTime() const
{
    return timeSpin->value();
}

DAVA::float32 SetPointValueDlg::GetValue() const
{
    if (isInteger)
        return valueSpinInt->value();

    return valueSpin->value();
}
