#include "ParticleTimeLineWidget.h"
#include "ParticleTimeLineColumns.h"
#include "Classes/Qt/Scene/SceneSignals.h"

#include <REPlatform/Commands/ParticleEditorCommands.h>
#include <REPlatform/DataNodes/SceneData.h>
#include <REPlatform/DataNodes/SelectionData.h>
#include <REPlatform/Scene/Systems/EditorParticlesSystem.h>

#include <TArc/Core/FieldBinder.h>
#include <TArc/Core/Deprecated.h>

#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

ParticleTimeLineWidget::ParticleTimeLineWidget(QWidget* parent /* = 0*/)
    : ScrollZoomWidget(parent)
    , selectedPoint(-1, -1)
#ifdef Q_OS_WIN
    , nameFont("Courier", 8, QFont::Normal)
#else
    , nameFont("Courier", 12, QFont::Normal)
#endif
{
    gridStyle = GRID_STYLE_LIMITS;

    selectionFieldBinder.reset(new DAVA::FieldBinder(DAVA::Deprecated::GetAccessor()));
    {
        DAVA::FieldDescriptor fieldDescr;
        fieldDescr.type = DAVA::ReflectedTypeDB::Get<DAVA::SelectionData>();
        fieldDescr.fieldName = DAVA::FastName(DAVA::SelectionData::selectionPropertyName);
        selectionFieldBinder->BindField(fieldDescr, DAVA::MakeFunction(this, &ParticleTimeLineWidget::OnSelectionChanged));
    }

    auto dispatcher = SceneSignals::Instance();

    // Get the notification about changes in Particle Editor items.
    connect(dispatcher, &SceneSignals::ParticleEmitterValueChanged, this, &ParticleTimeLineWidget::OnParticleEmitterValueChanged);
    connect(dispatcher, &SceneSignals::ParticleLayerValueChanged, this, &ParticleTimeLineWidget::OnParticleLayerValueChanged);

    // Particle Effect Started/Stopped notification is also needed to set/reset stats.
    connect(dispatcher, &SceneSignals::ParticleEffectStateChanged, this, &ParticleTimeLineWidget::OnParticleEffectStateChanged);

    // Particle Emitter Loaded notification is needed to re-initialize the Timeline.
    connect(dispatcher, &SceneSignals::ParticleEmitterLoaded, this, &ParticleTimeLineWidget::OnParticleEmitterLoaded);

    // Notifications about structure changes are needed to re-initialize the Timeline.
    connect(dispatcher, &SceneSignals::ParticleLayerAdded, this, &ParticleTimeLineWidget::OnParticleLayerAdded);
    connect(dispatcher, &SceneSignals::ParticleLayerRemoved, this, &ParticleTimeLineWidget::OnParticleLayerRemoved);

    Init(0, 0);

    // Init and start updating the particles grid.
    infoColumns.push_back(new ParticlesCountColumn(this, this));
    infoColumns.push_back(new ParticlesAverageCountColumn(this, this));
    infoColumns.push_back(new ParticlesMaxCountColumn(this, this));

    infoColumns.push_back(new ParticlesAreaColumn(this, this));
    infoColumns.push_back(new ParticlesAverageAreaColumn(this, this));
    infoColumns.push_back(new ParticlesMaxAreaColumn(this, this));

    connect(&updateTimer, &QTimer::timeout, this, &ParticleTimeLineWidget::OnUpdateLayersExtraInfoNeeded);
    updateTimer.start(UPDATE_LAYERS_EXTRA_INFO_PERIOD);
}

ParticleTimeLineWidget::~ParticleTimeLineWidget()
{
    updateTimer.stop();

    for (DAVA::List<ParticlesExtraInfoColumn*>::iterator iter = infoColumns.begin();
         iter != infoColumns.end(); iter++)
    {
        DAVA::SafeDelete(*iter);
    }

    infoColumns.clear();
}

void ParticleTimeLineWidget::HandleEmitterSelected(DAVA::ParticleEffectComponent* effect, DAVA::ParticleEmitterInstance* instance, DAVA::ParticleLayer* layer)
{
    if (instance == nullptr)
    {
        CleanupTimelines();
        emit ChangeVisible(false);
        return;
    }

    selectedEmitter = instance;
    selectedLayer = layer;
    selectedEffect = effect;

    auto emitter = instance->GetEmitter();

    DAVA::float32 minTime = 0;
    DAVA::float32 maxTime = emitter->lifeTime;

    Init(minTime, maxTime);
    QColor colors[3] = { Qt::blue, Qt::darkGreen, Qt::red };
    DAVA::uint32 colorsCount = sizeof(colors) / sizeof(*colors);

    if (!layer)
    {
        for (DAVA::uint32 i = 0; i < emitter->layers.size(); ++i)
        {
            AddLayerLine(i, minTime, maxTime, colors[i % colorsCount], emitter->layers[i]);
        }
    }
    else
    {
        // Add the particular layer only.
        int layerIndex = 0;
        for (DAVA::uint32 i = 0; i < emitter->layers.size(); i++)
        {
            if (emitter->layers[i] == layer)
            {
                layerIndex = i;
                break;
            }
        }

        AddLayerLine(layerIndex, minTime, maxTime, colors[layerIndex % colorsCount], layer);
    }

    UpdateSizePolicy();
    NotifyLayersExtraInfoChanged();
    UpdateLayersExtraInfoPosition();

    if (lines.size())
    {
        emit ChangeVisible(true);
        update();
    }
    else
    {
        emit ChangeVisible(false);
    }
}

QRect ParticleTimeLineWidget::GetSliderRect() const
{
    QRect rect = GetIncreaseRect();
    rect.translate(-(ZOOM_SLIDER_LENGTH + 5), 0);
    rect.setWidth(ZOOM_SLIDER_LENGTH);
    rect.setHeight(rect.height() + 4);
    return rect;
}

QRect ParticleTimeLineWidget::GetIncreaseRect() const
{
    QRect rect = GetScaleRect();
    rect.translate(-12, 0);
    rect.setWidth(8);
    rect.setHeight(8);
    return rect;
}

QRect ParticleTimeLineWidget::GetScaleRect() const
{
    QRect rect = GetScrollBarRect();
    rect.translate(-SCALE_WIDTH, 0);
    return rect;
}

QRect ParticleTimeLineWidget::GetDecreaseRect() const
{
    QRect rect = GetSliderRect();
    rect.translate(-12, 0);
    rect.setWidth(8);
    rect.setHeight(8);
    return rect;
}

void ParticleTimeLineWidget::OnParticleEffectSelected(DAVA::ParticleEffectComponent* effect)
{
    selectedEffect = effect;
    selectedEmitter = NULL;
    selectedLayer = NULL;

    DAVA::float32 minTime = 0;
    DAVA::float32 maxTime = 0;
    if (effect)
    {
        DAVA::int32 count = effect->GetEmittersCount();
        for (DAVA::int32 i = 0; i < count; ++i)
        {
            maxTime = DAVA::Max(maxTime, effect->GetEmitterInstance(i)->GetEmitter()->lifeTime);
        }
    }
    Init(minTime, maxTime);
    if (effect)
    {
        QColor colors[3] = { Qt::blue, Qt::darkGreen, Qt::red };
        DAVA::int32 count = effect->GetEmittersCount();
        DAVA::int32 iLines = 0;
        for (DAVA::int32 iEmitter = 0; iEmitter < count; ++iEmitter)
        {
            const DAVA::Vector<DAVA::ParticleLayer*>& layers = effect->GetEmitterInstance(iEmitter)->GetEmitter()->layers;
            for (DAVA::uint32 iLayer = 0; iLayer < layers.size(); ++iLayer)
            {
                DAVA::float32 startTime = DAVA::Max(minTime, layers[iLayer]->startTime);
                DAVA::float32 endTime = DAVA::Min(maxTime, layers[iLayer]->endTime);
                DAVA::float32 deltaTime = layers[iLayer]->deltaTime;
                DAVA::float32 loopEndTime = layers[iLayer]->loopEndTime;
                bool isLooped = layers[iLayer]->isLooped;
                bool hasLoopVariation = (layers[iLayer]->loopVariation > 0) ||
                (layers[iLayer]->deltaVariation > 0);
                AddLine(iLines, startTime, endTime, deltaTime, loopEndTime, isLooped, hasLoopVariation,
                        colors[iLines % 3], QString::fromStdString(layers[iLayer]->layerName), layers[iLayer]);
                iLines++;
            }
        }
    }

    UpdateSizePolicy();
    if (lines.size())
    {
        emit ChangeVisible(true);
        update();
    }
    else
    {
        emit ChangeVisible(false);
    }
}

void ParticleTimeLineWidget::Init(DAVA::float32 minTime, DAVA::float32 maxTime)
{
    ScrollZoomWidget::Init(minTime, maxTime);
    lines.clear();
}

void ParticleTimeLineWidget::AddLayerLine(DAVA::uint32 layerLineID, DAVA::float32 minTime, DAVA::float32 maxTime,
                                          const QColor& layerColor, DAVA::ParticleLayer* layer)
{
    if (!layer)
    {
        return;
    }

    DAVA::float32 startTime = DAVA::Max(minTime, layer->startTime);
    DAVA::float32 endTime = DAVA::Min(maxTime, layer->endTime);
    DAVA::float32 deltaTime = layer->deltaTime;
    DAVA::float32 loopEndTime = layer->loopEndTime;
    bool isLooped = layer->isLooped;
    bool hasLoopVariation = (layer->loopVariation > 0) || (layer->deltaVariation > 0);

    AddLine(layerLineID, startTime, endTime, deltaTime, loopEndTime, isLooped, hasLoopVariation, layerColor,
            QString::fromStdString(layer->layerName), layer);
}

void ParticleTimeLineWidget::AddLine(DAVA::uint32 lineId, DAVA::float32 startTime, DAVA::float32 endTime, DAVA::float32 deltaTime, DAVA::float32 loopEndTime,
                                     bool isLooped, bool hasLoopVariation, const QColor& color, const QString& legend, DAVA::ParticleLayer* layer)
{
    LINE line;
    line.startTime = startTime;
    line.endTime = endTime;
    line.deltaTime = deltaTime;
    line.loopEndTime = loopEndTime;
    line.isLooped = isLooped;
    line.hasLoopVariation = hasLoopVariation;
    line.color = color;
    line.legend = legend;
    line.layer = layer;
    lines[lineId] = line;
}

void ParticleTimeLineWidget::paintEvent(QPaintEvent* e)
{
    QPainter painter(this);

    QFont font("Courier", 11, QFont::Normal);
    painter.setFont(font);

    painter.fillRect(this->rect(), palette().window());

    QRect graphRect = GetGraphRect();

    //draw grid
    {
        painter.setPen(Qt::gray);

        float step = 18;
        float steps = graphRect.width() / step;
        float valueStep = (maxTime - minTime) / steps;
        bool drawText = false;
        for (int i = 0; i <= steps; i++)
        {
            int x = graphRect.left() + i * step;
            painter.drawLine(x, graphRect.top(), x, graphRect.bottom());
            drawText = !drawText;
            if (!drawText)
                continue;

            if (gridStyle == GRID_STYLE_ALL_POSITION)
            {
                float value = minTime + i * valueStep;
                QString strValue = float2QString(value);
                int textWidth = painter.fontMetrics().width(strValue);
                QRect textRect(x - textWidth / 2, graphRect.bottom(), textWidth, BOTTOM_INDENT);
                painter.drawText(textRect, Qt::AlignCenter, strValue);
            }
        }

        if (gridStyle == GRID_STYLE_LIMITS)
        {
            QRect textRect(graphRect.left(), graphRect.bottom(), graphRect.width(), BOTTOM_INDENT);
            painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, float2QString(minTime));
            painter.drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, float2QString(maxTime));
        }
    }

    painter.setFont(nameFont);

    painter.setPen(Qt::black);
    painter.drawRect(graphRect);

    DAVA::uint32 i = 0;
    for (LINE_MAP::const_iterator iter = lines.begin(); iter != lines.end(); ++iter, ++i)
    {
        const LINE& line = iter->second;

        QRect startRect;
        QRect endRect;
        bool drawStartRect = true;
        bool drawEndRect = true;
        GetLineRect(iter->first, startRect, endRect);
        ePositionRelativelyToDrawRect startPosition = GetPointPositionFromDrawingRect(startRect.center());
        ePositionRelativelyToDrawRect endPosition = GetPointPositionFromDrawingRect(endRect.center());
        if (startPosition == POSITION_LEFT)
        {
            drawStartRect = false;
            startRect.moveTo(graphRect.x() - RECT_SIZE, startRect.y());
        }
        else if (startPosition == POSITION_RIGHT)
        {
            drawStartRect = false;
            startRect.moveTo(graphRect.x() + graphRect.width() - RECT_SIZE, startRect.y());
        }

        if (endPosition == POSITION_LEFT)
        {
            drawEndRect = false;
            endRect.moveTo(graphRect.x() - RECT_SIZE, endRect.y());
        }
        else if (endPosition == POSITION_RIGHT)
        {
            drawEndRect = false;
            endRect.moveTo(graphRect.x() + graphRect.width() - RECT_SIZE, endRect.y());
        }

        painter.setPen(QPen(line.color, 1));
        painter.drawLine(QPoint(graphRect.left(), startRect.center().y()), QPoint(graphRect.right(), startRect.center().y()));

        int textMaxWidth = graphRect.left() - LEFT_INDENT - painter.fontMetrics().width("WW");
        QString legend;
        for (int i = 0; i < line.legend.length(); ++i)
        {
            legend += line.legend.at(i);
            int textWidth = painter.fontMetrics().width(legend);
            if (textWidth > textMaxWidth)
            {
                legend.remove(legend.length() - 3, 3);
                legend += "...";
                break;
            }
        }
        painter.drawText(QPoint(LEFT_INDENT, startRect.bottom()), legend);
        painter.setPen(QPen(line.hasLoopVariation ? Qt::gray : line.color, LINE_WIDTH));
        if (selectedPoint.x() == iter->first)
        {
            QBrush brush(line.color);
            if (selectedPoint.y() == 0)
                painter.fillRect(startRect, brush);
            else
                painter.fillRect(endRect, brush);
        }

        QPoint startPoint(startRect.center());
        startPoint.setX(startPoint.x() + 3);
        QPoint endPoint(endRect.center());
        endPoint.setX(endPoint.x() - 3);

        if (drawStartRect)
        {
            painter.drawRect(startRect);
        }
        if (drawEndRect)
        {
            painter.drawRect(endRect);
        }
        if (!(startPosition == endPosition && startPosition != POSITION_INSIDE))
        {
            if (selectedLine == iter->first)
                painter.setPen(QPen(line.color, LINE_WIDTH + 2));

            painter.drawLine(startPoint, endPoint);
        }
        // Draw additional lines if layer is looped
        if (line.isLooped)
        {
            DAVA::float32 loopEndTime = line.loopEndTime;
            DAVA::float32 deltaTime = line.deltaTime;

            DAVA::float32 durationTime = line.endTime - line.startTime;
            DAVA::float32 currentStartTime = line.endTime;
            DAVA::float32 currentEndTime;

            bool useDeltaTime = (line.deltaTime > 0.0f);
            bool useDurationTime = (durationTime > 0.0f);
            bool timeCanBeIncremented = useDeltaTime || useDurationTime;

            Qt::PenStyle lineStyle;
            QColor lineColor;

            while (timeCanBeIncremented && (currentStartTime < loopEndTime))
            {
                // Use gray color for layer which has time variations.
                if (line.hasLoopVariation)
                {
                    lineColor = Qt::gray;
                }
                else
                {
                    lineColor = line.color;
                }
                // If delta time is used - we should use different calculations
                // and draw dotted line
                if (useDeltaTime)
                {
                    currentEndTime = currentStartTime + deltaTime;
                    lineStyle = Qt::DotLine;
                    useDeltaTime = false;
                }
                else
                {
                    currentEndTime = currentStartTime + durationTime;
                    lineStyle = Qt::SolidLine;
                    useDeltaTime = (line.deltaTime > 0);
                }

                // We should not exceed loopEnd time
                currentEndTime = DAVA::Min(currentEndTime, loopEndTime);

                GetLoopedLineRect(iter->first, startRect, endRect, currentStartTime, currentEndTime);
                // We should start next line section from current End time
                currentStartTime = currentEndTime;

                startPosition = GetPointPositionFromDrawingRect(startRect.center());
                endPosition = GetPointPositionFromDrawingRect(endRect.center());

                QPoint startPoint(startRect.center());
                startPoint.setX(startPoint.x() + 3);
                QPoint endPoint(endRect.center());
                endPoint.setX(endPoint.x() - 3);

                if (!(startPosition == endPosition && startPosition != POSITION_INSIDE))
                {
                    painter.setPen(QPen(lineColor, LINE_WIDTH));
                    painter.drawRect(endRect);

                    painter.setPen(QPen(lineColor, LINE_WIDTH, lineStyle));
                    painter.drawLine(startPoint, endPoint);
                }
            }
        }

        UpdateLayersExtraInfoPosition();
    }

    ScrollZoomWidget::paintEvent(e);
}

bool ParticleTimeLineWidget::GetLineRect(DAVA::uint32 id, QRect& startRect, QRect& endRect) const
{
    DAVA::uint32 i = 0;
    QRect grapRect = GetGraphRect();
    for (LINE_MAP::const_iterator iter = lines.begin(); iter != lines.end(); ++iter, ++i)
    {
        if (iter->first != id)
            continue;

        const LINE& line = iter->second;

        QPoint startPoint(grapRect.left() + (line.startTime - minTime) / (maxTime - minTime) * grapRect.width(), grapRect.top() + (i + 1) * LINE_STEP);
        QPoint endPoint(grapRect.left() + (line.endTime - minTime) / (maxTime - minTime) * grapRect.width(), grapRect.top() + (i + 1) * LINE_STEP);
        startRect = QRect(startPoint - QPoint(RECT_SIZE, RECT_SIZE), startPoint + QPoint(RECT_SIZE, RECT_SIZE));
        endRect = QRect(endPoint - QPoint(RECT_SIZE, RECT_SIZE), endPoint + QPoint(RECT_SIZE, RECT_SIZE));
        return true;
    }
    return false;
}

bool ParticleTimeLineWidget::GetLoopedLineRect(DAVA::uint32 id, QRect& startRect, QRect& endRect, DAVA::float32 startTime, DAVA::float32 endTime) const
{
    DAVA::uint32 i = 0;
    QRect grapRect = GetGraphRect();
    for (LINE_MAP::const_iterator iter = lines.begin(); iter != lines.end(); ++iter, ++i)
    {
        if (iter->first != id)
            continue;

        QPoint startPoint(grapRect.left() + (startTime - minTime) / (maxTime - minTime) * grapRect.width(), grapRect.top() + (i + 1) * LINE_STEP);
        QPoint endPoint(grapRect.left() + (endTime - minTime) / (maxTime - minTime) * grapRect.width(), grapRect.top() + (i + 1) * LINE_STEP);
        startRect = QRect(startPoint - QPoint(RECT_SIZE, RECT_SIZE), startPoint + QPoint(RECT_SIZE, RECT_SIZE));
        endRect = QRect(endPoint - QPoint(RECT_SIZE, RECT_SIZE), endPoint + QPoint(RECT_SIZE, RECT_SIZE));
        return true;
    }
    return false;
}

QRect ParticleTimeLineWidget::GetGraphRect() const
{
    QFontMetrics metrics(nameFont);

    int legendWidth = 0;
    for (LINE_MAP::const_iterator iter = lines.begin(); iter != lines.end(); ++iter)
    {
        int width = metrics.width(iter->second.legend);
        width += LEFT_INDENT;
        width += metrics.width(" ");
        legendWidth = DAVA::Max(legendWidth, width);
    }
    legendWidth = DAVA::Min(legendWidth, (width() - LEFT_INDENT * 2) / 6);

    QRect rect = QRect(QPoint(LEFT_INDENT + legendWidth, TOP_INDENT),
                       QSize(width() - LEFT_INDENT * 2 - legendWidth - RIGHT_INDENT,
                             height() - (BOTTOM_INDENT + TOP_INDENT) + LINE_STEP / 2 - SCROLL_BAR_HEIGHT));

    return rect;
}

void ParticleTimeLineWidget::UpdateLayersExtraInfoPosition()
{
    if (lines.size() == 0)
    {
        ShowLayersExtraInfoValues(false);
        return;
    }

    ShowLayersExtraInfoValues(true);
    QRect graphRect = GetGraphRect();

    DAVA::List<ParticlesExtraInfoColumn*>::iterator firstIter = infoColumns.begin();
    ParticlesExtraInfoColumn* firstColumn = (*firstIter);

    QRect extraInfoRect(graphRect.right() + PARTICLES_INFO_CONTROL_OFFSET, 0,
                        firstColumn->GetColumnWidth(), graphRect.height() + TOP_INDENT + 1);
    firstColumn->setGeometry(extraInfoRect);

    firstIter++;
    for (DAVA::List<ParticlesExtraInfoColumn*>::iterator iter = firstIter;
         iter != infoColumns.end(); iter++)
    {
        int curRight = extraInfoRect.right();
        extraInfoRect.setLeft(curRight);
        extraInfoRect.setRight(curRight + (*iter)->GetColumnWidth());

        (*iter)->setGeometry(extraInfoRect);
    }
}

void ParticleTimeLineWidget::ShowLayersExtraInfoValues(bool isVisible)
{
    for (DAVA::List<ParticlesExtraInfoColumn*>::iterator iter = infoColumns.begin();
         iter != infoColumns.end(); iter++)
    {
        (*iter)->setVisible(isVisible);
    }
}

void ParticleTimeLineWidget::UpdateLayersExtraInfoValues()
{
    // Just invalidate and repaint the columns.
    for (DAVA::List<ParticlesExtraInfoColumn*>::iterator iter = infoColumns.begin();
         iter != infoColumns.end(); iter++)
    {
        (*iter)->update();
    }
}

void ParticleTimeLineWidget::ResetLayersExtraInfoValues()
{
    // Just invalidate and repaint the columns.
    for (DAVA::List<ParticlesExtraInfoColumn*>::iterator iter = infoColumns.begin();
         iter != infoColumns.end(); iter++)
    {
        (*iter)->Reset();
    }
}

void ParticleTimeLineWidget::NotifyLayersExtraInfoChanged()
{
    // Just invalidate and repaint the columns.
    for (DAVA::List<ParticlesExtraInfoColumn*>::iterator iter = infoColumns.begin();
         iter != infoColumns.end(); iter++)
    {
        (*iter)->OnLayersListChanged();
    }
}

void ParticleTimeLineWidget::UpdateSizePolicy()
{
    //setFixedHeight((lines.size() + 1) * LINE_STEP + BOTTOM_INDENT + TOP_INDENT + PARTICLES_INFO_CONTROL_OFFSET);
    updateGeometry();
    update();
}

void ParticleTimeLineWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (selectedPoint.x() != -1)
    {
        LINE_MAP::iterator iter = lines.find(selectedPoint.x());
        if (iter == lines.end())
            return;

        LINE& line = iter->second;

        QRect graphRect = GetGraphRect();
        DAVA::float32 value = (event->pos().x() - graphRect.left()) / static_cast<DAVA::float32>(graphRect.width()) * (maxTime - minTime) + minTime;
        value = DAVA::Max(minTime, DAVA::Min(maxTime, value));
        if (selectedPoint.y() == 0) //start point selected
        {
            line.startTime = DAVA::Min(value, line.endTime);
        }
        else
        {
            line.endTime = DAVA::Max(value, line.startTime);
        }
        update();
        return;
    }
    else if (selectedLine != -1)
    {
        LINE_MAP::iterator iter = lines.find(selectedLine);
        if (iter == lines.end())
            return;
        LINE& line = iter->second;
        DAVA::int32 delta = event->pos().x() - selectedLineOrigin;
        selectedLineOrigin = event->pos().x();

        QRect graphRect = GetGraphRect();

        DAVA::float32 offset = delta / static_cast<DAVA::float32>(graphRect.width()) * (maxTime - minTime);
        offset = DAVA::Max(generalMinTime - line.startTime, DAVA::Min(generalMaxTime - line.endTime, offset));
        line.startTime += offset;
        line.endTime += offset;
        update();
        return;
    }

    ScrollZoomWidget::mouseMoveEvent(event);
}

void ParticleTimeLineWidget::mousePressEvent(QMouseEvent* event)
{
    selectedPoint = GetPoint(event->pos());
    if (selectedPoint.x() == -1) //try selecting line only if no point selected endpoint
    {
        DAVA::uint32 i = 0;
        QRect grapRect = GetGraphRect();

        for (LINE_MAP::const_iterator iter = lines.begin(); iter != lines.end(); ++iter, ++i)
        {
            const LINE& line = iter->second;

            QPoint startPoint(grapRect.left() + (line.startTime - minTime) / (maxTime - minTime) * grapRect.width(), grapRect.top() + (i + 1) * LINE_STEP);
            QPoint endPoint(grapRect.left() + (line.endTime - minTime) / (maxTime - minTime) * grapRect.width(), grapRect.top() + (i + 1) * LINE_STEP);
            QRect lineRect = QRect(startPoint - QPoint(RECT_SIZE, RECT_SIZE), endPoint + QPoint(RECT_SIZE, RECT_SIZE));

            if (lineRect.contains(event->pos()))
            {
                selectedLine = iter->first;
                selectedLineOrigin = event->pos().x();
                break;
            }
        }
    }

    ScrollZoomWidget::mousePressEvent(event);
    update();
}

void ParticleTimeLineWidget::mouseReleaseEvent(QMouseEvent* e)
{
    if (selectedPoint.x() != -1 &&
        selectedPoint.y() != -1)
    {
        OnValueChanged(selectedPoint.x());
    }
    if (selectedLine != -1)
    {
        OnValueChanged(selectedLine);
    }

    selectedLine = -1;
    selectedPoint = QPoint(-1, -1);
    ScrollZoomWidget::mouseReleaseEvent(e);
    update();
}

void ParticleTimeLineWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    QPoint point = GetPoint(event->pos());
    LINE_MAP::iterator iter = lines.find(point.x());
    if (iter != lines.end())
    {
        LINE& line = iter->second;

        if (line.hasLoopVariation)
            return;

        DAVA::float32 value = point.y() == 0 ? line.startTime : line.endTime;
        DAVA::float32 minValue = point.y() == 0 ? minTime : line.startTime;
        DAVA::float32 maxValue = point.y() == 0 ? line.endTime : maxTime;
        SetPointValueDlg dlg(value, minValue, maxValue, this);
        if (dlg.exec())
        {
            if (point.y() == 0)
                line.startTime = dlg.GetValue();
            else
                line.endTime = dlg.GetValue();

            OnValueChanged(iter->first);
        }
    }
    update();
}

QPoint ParticleTimeLineWidget::GetPoint(const QPoint& pos) const
{
    QPoint point = QPoint(-1, -1);
    for (LINE_MAP::const_iterator iter = lines.begin(); iter != lines.end(); ++iter)
    {
        QRect startRect;
        QRect endRect;
        if (!GetLineRect(iter->first, startRect, endRect))
            continue;

        //startRect.translate(-LINE_WIDTH, -LINE_WIDTH);
        //endRect.translate(-LINE_WIDTH, -LINE_WIDTH);
        startRect.adjust(-LINE_WIDTH, -LINE_WIDTH, LINE_WIDTH, LINE_WIDTH);
        endRect.adjust(-LINE_WIDTH, -LINE_WIDTH, LINE_WIDTH, LINE_WIDTH);
        point.setX(iter->first);
        if (startRect.contains(pos))
        {
            point.setY(0);
            break;
        }

        if (endRect.contains(pos))
        {
            point.setY(1);
            break;
        }
    }
    if (point.y() == -1)
        point.setX(-1);
    return point;
}

DAVA::float32 ParticleTimeLineWidget::SetPointValueDlg::GetValue() const
{
    return valueSpin->value();
}

void ParticleTimeLineWidget::OnValueChanged(int lineId)
{
    LINE_MAP::iterator iter = lines.find(lineId);
    if (iter == lines.end())
        return;

    if (activeScene)
    {
        std::unique_ptr<DAVA::CommandUpdateParticleLayerTime> cmd(new DAVA::CommandUpdateParticleLayerTime(iter->second.layer));
        cmd->Init(iter->second.startTime, iter->second.endTime);
        activeScene->Exec(std::move(cmd));
        activeScene->MarkAsChanged();
    }

    emit ValueChanged();
}

void ParticleTimeLineWidget::OnUpdate()
{
    if (selectedEmitter)
    {
        HandleEmitterSelected(selectedEffect, selectedEmitter, selectedLayer);
    }
    else if (selectedEffect)
    {
        OnParticleEffectSelected(selectedEffect);
    }
}

void ParticleTimeLineWidget::OnUpdateLayersExtraInfoNeeded()
{
    UpdateLayersExtraInfoValues();
}

void ParticleTimeLineWidget::OnParticleEffectStateChanged(DAVA::SceneEditor2* scene, DAVA::Entity* effect, bool isStarted)
{
    // The particle effect was started, stopped or restarted. Reset all the extra info.
    ResetLayersExtraInfoValues();
}

void ParticleTimeLineWidget::OnSelectionChanged(const DAVA::Any& selectionAny)
{
    if (selectionAny.CanGet<DAVA::SelectableGroup>())
    {
        const DAVA::SelectableGroup& selection = selectionAny.Get<DAVA::SelectableGroup>();

        DAVA::SceneData* sceneData = DAVA::Deprecated::GetActiveDataNode<DAVA::SceneData>();
        DAVA::SceneEditor2* scene = sceneData->GetScene().Get();

        ProcessSelection(scene, selection);
    }
    else
    {
        activeScene = nullptr;
        CleanupTimelines();
        emit ChangeVisible(false);
    }
}

void ParticleTimeLineWidget::ProcessSelection(DAVA::SceneEditor2* scene, const DAVA::SelectableGroup& selection)
{
    bool shouldReset = true;
    SCOPE_EXIT
    {
        if (shouldReset)
        {
            activeScene = nullptr;
            CleanupTimelines();
            emit ChangeVisible(false);
        }
    };

    if (selection.GetSize() != 1)
        return;

    DAVA::EditorParticlesSystem* system = scene->GetSystem<DAVA::EditorParticlesSystem>();
    activeScene = scene;
    const DAVA::Selectable& obj = selection.GetFirst();
    if (obj.CanBeCastedTo<DAVA::Entity>())
    {
        auto entity = obj.AsEntity();
        auto effect = entity->GetComponent<DAVA::ParticleEffectComponent>();
        if (effect != nullptr)
        {
            shouldReset = false;
            OnParticleEffectSelected(effect);
        }
    }
    else if (obj.CanBeCastedTo<DAVA::ParticleEmitterInstance>())
    {
        shouldReset = false;
        DAVA::ParticleEmitterInstance* instance = obj.Cast<DAVA::ParticleEmitterInstance>();
        DAVA::ParticleEffectComponent* effect = system->GetEmitterOwner(instance);
        HandleEmitterSelected(effect, instance, nullptr);
    }
    else if (obj.CanBeCastedTo<DAVA::ParticleLayer>())
    {
        DAVA::ParticleLayer* layer = obj.Cast<DAVA::ParticleLayer>();
        DAVA::ParticleEmitterInstance* instance = scene->GetSystem<DAVA::EditorParticlesSystem>()->GetRootEmitterLayerOwner(layer);
        if (instance != nullptr)
        {
            shouldReset = false;
            DAVA::ParticleEffectComponent* effect = system->GetEmitterOwner(instance);
            HandleEmitterSelected(effect, instance, layer);
        }
    }
}

void ParticleTimeLineWidget::OnParticleEmitterValueChanged(DAVA::SceneEditor2* /*scene*/, DAVA::ParticleEmitterInstance* emitter)
{
    if (emitter == nullptr)
        return;

    // Update the timeline parameters which are related to the whole emitter.
    if (maxTime != emitter->GetEmitter()->lifeTime)
    {
        maxTime = emitter->GetEmitter()->lifeTime;
        update();
    }
}

void ParticleTimeLineWidget::OnParticleLayerValueChanged(DAVA::SceneEditor2* scene, DAVA::ParticleLayer* layer)
{
    // Update the params related to the particular layer.
    int lineIndex = 0;
    for (LINE_MAP::iterator iter = lines.begin(); iter != lines.end(); ++iter, ++lineIndex)
    {
        LINE& line = iter->second;
        if (line.layer != layer)
        {
            continue;
        }

        if (line.startTime != layer->startTime || line.endTime != layer->endTime)
        {
            line.startTime = layer->startTime;
            line.endTime = layer->endTime;
            update();
        }

        if (line.isLooped != layer->isLooped)
        {
            line.isLooped = layer->isLooped;
            update();
        }

        if (line.deltaTime != layer->deltaTime || line.loopEndTime != layer->loopEndTime)
        {
            line.deltaTime = layer->deltaTime;
            line.loopEndTime = layer->loopEndTime;
            update();
        }

        bool hasLoopVariation = (layer->loopVariation > 0) || (layer->deltaVariation > 0);
        if (line.hasLoopVariation != hasLoopVariation)
        {
            line.hasLoopVariation = hasLoopVariation;
            update();
        }

        break;
    }
}

void ParticleTimeLineWidget::OnParticleEmitterLoaded(DAVA::SceneEditor2* scene, DAVA::ParticleEmitterInstance* emitter)
{
    if (!emitter)
    {
        return;
    }

    // Handle in the same way as new emitter is selected.
    activeScene = scene;
    HandleEmitterSelected(selectedEffect, emitter, NULL);
}

void ParticleTimeLineWidget::OnParticleLayerAdded(DAVA::SceneEditor2* scene, DAVA::ParticleEmitterInstance* emitter, DAVA::ParticleLayer* layer)
{
    if (!layer)
    {
        return;
    }

    // Handle in the same way as new emitter is selected.
    activeScene = scene;
    HandleEmitterSelected(selectedEffect, emitter, NULL);
}

void ParticleTimeLineWidget::OnParticleLayerRemoved(DAVA::SceneEditor2* scene, DAVA::ParticleEmitterInstance* emitter)
{
    if (!emitter)
    {
        return;
    }

    // Handle in the same way as new emitter is selected.
    activeScene = scene;
    HandleEmitterSelected(selectedEffect, emitter, NULL);
}

void ParticleTimeLineWidget::CleanupTimelines()
{
    lines.clear();
    UpdateSizePolicy();
    NotifyLayersExtraInfoChanged();
    UpdateLayersExtraInfoPosition();
}

ParticleTimeLineWidget::SetPointValueDlg::SetPointValueDlg(DAVA::float32 value, DAVA::float32 minValue, DAVA::float32 maxValue, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Set time");
    // DF-1248 fix - Remove help button
    Qt::WindowFlags flags = windowFlags();
    flags &= ~Qt::WindowContextHelpButtonHint;
    setWindowFlags(flags);

    QVBoxLayout* mainBox = new QVBoxLayout;
    setLayout(mainBox);

    valueSpin = new EventFilterDoubleSpinBox(this);
    mainBox->addWidget(valueSpin);

    QHBoxLayout* btnBox = new QHBoxLayout;
    QPushButton* btnCancel = new QPushButton("Cancel", this);
    QPushButton* btnOk = new QPushButton("Ok", this);
    btnBox->addWidget(btnCancel);
    btnBox->addWidget(btnOk);
    mainBox->addLayout(btnBox);

    valueSpin->setMinimum(minValue);
    valueSpin->setMaximum(maxValue);
    valueSpin->setValue(value);
    valueSpin->setSingleStep(0.01);

    connect(btnOk,
            SIGNAL(clicked(bool)),
            this,
            SLOT(accept()));
    connect(btnCancel,
            SIGNAL(clicked(bool)),
            this,
            SLOT(reject()));

    btnOk->setDefault(true);
    valueSpin->setFocus();
    valueSpin->selectAll();
}
