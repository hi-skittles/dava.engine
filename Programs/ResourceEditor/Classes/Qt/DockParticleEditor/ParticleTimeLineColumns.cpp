#include "ParticleTimeLineColumns.h"

ParticlesExtraInfoColumn::ParticlesExtraInfoColumn(const ParticleTimeLineWidget* timeLineWidget,
                                                   QWidget* parent)
    :
    QWidget(parent)
{
    this->timeLineWidget = timeLineWidget;
}

void ParticlesExtraInfoColumn::paintEvent(QPaintEvent*)
{
    if (!this->timeLineWidget)
    {
        return;
    }

    QPainter painter(this);
    painter.setPen(Qt::black);

    QRect ourRect = rect();
    ourRect.adjust(0, 0, -1, -1);
    painter.drawRect(ourRect);

    // Draw the header.
    painter.setFont(timeLineWidget->nameFont);
    painter.setPen(Qt::black);
    QRect textRect(0, 0, rect().width(), TOP_INDENT);
    painter.drawRect(textRect);
    painter.setPen(Qt::white);
    painter.drawText(textRect, Qt::AlignHCenter | Qt::AlignVCenter, GetExtraInfoHeader());

    // Draw the per-layer particles count.
    OnBeforeGetExtraInfoLoop();

    QFontMetrics fontMetrics(timeLineWidget->nameFont);
    painter.setFont(timeLineWidget->nameFont);

    DAVA::int32 i = 0;
    for (ParticleTimeLineWidget::LINE_MAP::const_iterator iter = timeLineWidget->lines.begin();
         iter != timeLineWidget->lines.end(); ++iter, ++i)
    {
        const ParticleTimeLineWidget::LINE& line = iter->second;

        painter.setPen(QPen(line.color, LINE_WIDTH));
        int startY = i * LINE_STEP + LINE_STEP / 2;
        QRect textRect(EXTRA_INFO_LEFT_PADDING, TOP_INDENT + startY,
                       rect().width() - EXTRA_INFO_LEFT_PADDING, LINE_STEP);
        painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter,
                         GetExtraInfoForLayerLine(timeLineWidget->selectedEffect, line));
    }

    OnAfterGetExtraInfoLoop();

    // Draw the "Total" box.
    QPoint totalPoint(EXTRA_INFO_LEFT_PADDING, rect().bottom() - 3);
    QFont totalFont = timeLineWidget->nameFont;
    totalFont.setBold(true);

    painter.setPen(QPen(Qt::white, LINE_WIDTH));
    painter.drawText(totalPoint, GetExtraInfoFooter());
}

QString ParticlesExtraInfoColumn::FormatFloat(DAVA::float32 value)
{
    QString strValue;
    if (fabs(value) < 10)
    {
        strValue = "%.4f";
    }
    else if (fabs(value) < 100)
    {
        strValue = "%.2f";
    }
    else
    {
        strValue = "%.0f";
    }

    strValue.sprintf(strValue.toLatin1(), value);
    return strValue;
}

////////////////////////////////////////////////////////////////////////////////////

ParticlesExtraInfoCumulativeColumn::ParticlesExtraInfoCumulativeColumn(const ParticleTimeLineWidget* timeLineWidget,
                                                                       QWidget* parent)
    : ParticlesExtraInfoColumn(timeLineWidget, parent)
{
    CleanupCumulativeData();
}

void ParticlesExtraInfoCumulativeColumn::OnLayersListChanged()
{
    CleanupCumulativeData();
}

void ParticlesExtraInfoCumulativeColumn::UpdateCumulativeData(DAVA::ParticleLayer* layer, DAVA::float32 value)
{
    if (!layer)
    {
        return;
    }

    if (cumulativeData.find(layer) == cumulativeData.end())
    {
        cumulativeData[layer] = value;
    }
    else
    {
        cumulativeData[layer] += value;
    }
}

void ParticlesExtraInfoCumulativeColumn::UpdateCumulativeDataIfMaximum(DAVA::ParticleLayer* layer, DAVA::float32 value)
{
    if (!layer)
    {
        return;
    }

    if ((cumulativeData.find(layer) == cumulativeData.end()) ||
        (value > cumulativeData[layer]))
    {
        cumulativeData[layer] = value;
    }
}

void ParticlesExtraInfoCumulativeColumn::CleanupCumulativeData()
{
    this->totalParticlesCount = 0;
    this->totalUpdatesCount = 0;
    this->totalParticlesArea = 0.0f;

    cumulativeData.clear();
}

////////////////////////////////////////////////////////////////////////////////////
ParticlesCountColumn::ParticlesCountColumn(const ParticleTimeLineWidget* timeLineWidget,
                                           QWidget* parent)
    :
    ParticlesExtraInfoColumn(timeLineWidget, parent)
{
    this->totalParticlesCount = 0;
}

void ParticlesCountColumn::OnBeforeGetExtraInfoLoop()
{
    this->totalParticlesCount = 0;
}

QString ParticlesCountColumn::GetExtraInfoForLayerLine(DAVA::ParticleEffectComponent* effect, const ParticleTimeLineWidget::LINE& line)
{
    if (!line.layer)
    {
        return QString();
    }

    DAVA::int32 particlesNumber = effect->GetLayerActiveParticlesCount(line.layer);
    this->totalParticlesCount += particlesNumber;

    return QString::number(particlesNumber);
}

QString ParticlesCountColumn::GetExtraInfoHeader()
{
    return "Count";
}

QString ParticlesCountColumn::GetExtraInfoFooter()
{
    return QString::number(this->totalParticlesCount);
}

////////////////////////////////////////////////////////////////////////////////////
ParticlesAverageCountColumn::ParticlesAverageCountColumn(const ParticleTimeLineWidget* timeLineWidget,
                                                         QWidget* parent)
    :
    ParticlesExtraInfoCumulativeColumn(timeLineWidget, parent)
{
}

void ParticlesAverageCountColumn::Reset()
{
    CleanupCumulativeData();
}

QString ParticlesAverageCountColumn::GetExtraInfoForLayerLine(DAVA::ParticleEffectComponent* effect, const ParticleTimeLineWidget::LINE& line)
{
    if (!line.layer)
    {
        return QString();
    }

    // Calculate the cumulative info.
    this->totalUpdatesCount++;

    DAVA::int32 particlesNumber = effect->GetLayerActiveParticlesCount(line.layer);
    UpdateCumulativeData(line.layer, particlesNumber);
    this->totalParticlesCount += particlesNumber;

    return FormatFloat(cumulativeData[line.layer] / (float)totalUpdatesCount);
}

QString ParticlesAverageCountColumn::GetExtraInfoHeader()
{
    return "Avg Cnt";
}

QString ParticlesAverageCountColumn::GetExtraInfoFooter()
{
    if (this->totalUpdatesCount == 0)
    {
        return FormatFloat(0);
    }
    else
    {
        return FormatFloat((float)totalParticlesCount / (float)totalUpdatesCount);
    }
}

////////////////////////////////////////////////////////////////////////////////////

ParticlesMaxCountColumn::ParticlesMaxCountColumn(const ParticleTimeLineWidget* timeLineWidget,
                                                 QWidget* parent)
    : ParticlesExtraInfoCumulativeColumn(timeLineWidget, parent)
{
    Reset();
}

void ParticlesMaxCountColumn::OnLayersListChanged()
{
    ParticlesExtraInfoCumulativeColumn::OnLayersListChanged();
    Reset();
}

void ParticlesMaxCountColumn::Reset()
{
    CleanupCumulativeData();
    this->maxParticlesCount = 0;
    this->totalParticlesCountOnThisLoop = 0;
}

void ParticlesMaxCountColumn::OnBeforeGetExtraInfoLoop()
{
    this->totalParticlesCountOnThisLoop = 0;
}

QString ParticlesMaxCountColumn::GetExtraInfoForLayerLine(DAVA::ParticleEffectComponent* effect, const ParticleTimeLineWidget::LINE& line)
{
    if (!line.layer)
    {
        return QString();
    }

    // Calculate the cumulative info.

    DAVA::int32 particlesNumber = effect->GetLayerActiveParticlesCount(line.layer);
    UpdateCumulativeDataIfMaximum(line.layer, particlesNumber);
    totalParticlesCountOnThisLoop += particlesNumber;

    return QString::number((int)cumulativeData[line.layer]);
}

void ParticlesMaxCountColumn::OnAfterGetExtraInfoLoop()
{
    if (maxParticlesCount < totalParticlesCountOnThisLoop)
    {
        maxParticlesCount = totalParticlesCountOnThisLoop;
    }
}

QString ParticlesMaxCountColumn::GetExtraInfoHeader()
{
    return "Max Cnt";
}

QString ParticlesMaxCountColumn::GetExtraInfoFooter()
{
    return QString::number((int)maxParticlesCount);
}

////////////////////////////////////////////////////////////////////////////////////

ParticlesAreaColumn::ParticlesAreaColumn(const ParticleTimeLineWidget* timeLineWidget,
                                         QWidget* parent)
    :
    ParticlesExtraInfoColumn(timeLineWidget, parent)
{
    this->totalParticlesArea = 0.0f;
}

void ParticlesAreaColumn::OnBeforeGetExtraInfoLoop()
{
    this->totalParticlesArea = 0;
}

QString ParticlesAreaColumn::GetExtraInfoForLayerLine(DAVA::ParticleEffectComponent* effect, const ParticleTimeLineWidget::LINE& line)
{
    if (!line.layer)
    {
        return QString();
    }

    DAVA::float32 area = effect->GetLayerActiveParticlesSquare(line.layer);
    this->totalParticlesArea += area;

    return FormatFloat(area);
}

QString ParticlesAreaColumn::GetExtraInfoHeader()
{
    return "Area";
}

QString ParticlesAreaColumn::GetExtraInfoFooter()
{
    return FormatFloat(this->totalParticlesArea);
}

////////////////////////////////////////////////////////////////////////////////////

ParticlesAverageAreaColumn::ParticlesAverageAreaColumn(const ParticleTimeLineWidget* timeLineWidget,
                                                       QWidget* parent)
    :
    ParticlesExtraInfoCumulativeColumn(timeLineWidget, parent)
{
}

void ParticlesAverageAreaColumn::Reset()
{
    CleanupCumulativeData();
}

QString ParticlesAverageAreaColumn::GetExtraInfoForLayerLine(DAVA::ParticleEffectComponent* effect, const ParticleTimeLineWidget::LINE& line)
{
    if (!line.layer)
    {
        return QString();
    }

    // Calculate the cumulative info.
    this->totalUpdatesCount++;

    DAVA::float32 area = effect->GetLayerActiveParticlesSquare(line.layer);
    UpdateCumulativeData(line.layer, area);
    this->totalParticlesArea += area;

    return FormatFloat(cumulativeData[line.layer] / (float)totalUpdatesCount);
}

QString ParticlesAverageAreaColumn::GetExtraInfoHeader()
{
    return "Avg Area";
}

QString ParticlesAverageAreaColumn::GetExtraInfoFooter()
{
    if (this->totalUpdatesCount == 0)
    {
        return FormatFloat(0);
    }
    else
    {
        return FormatFloat(totalParticlesArea / (float)totalUpdatesCount);
    }
}

////////////////////////////////////////////////////////////////////////////////////

ParticlesMaxAreaColumn::ParticlesMaxAreaColumn(const ParticleTimeLineWidget* timeLineWidget,
                                               QWidget* parent)
    :
    ParticlesExtraInfoCumulativeColumn(timeLineWidget, parent)
{
    Reset();
}

void ParticlesMaxAreaColumn::OnLayersListChanged()
{
    ParticlesExtraInfoCumulativeColumn::OnLayersListChanged();
    Reset();
}

void ParticlesMaxAreaColumn::Reset()
{
    CleanupCumulativeData();
    maxParticlesArea = 0;
    totalParticlesAreaOnThisLoop = 0;
}

void ParticlesMaxAreaColumn::OnBeforeGetExtraInfoLoop()
{
    totalParticlesAreaOnThisLoop = 0;
}

QString ParticlesMaxAreaColumn::GetExtraInfoForLayerLine(DAVA::ParticleEffectComponent* effect, const ParticleTimeLineWidget::LINE& line)
{
    if (!line.layer)
    {
        return QString();
    }

    // Calculate the cumulative info.
    DAVA::float32 area = effect->GetLayerActiveParticlesSquare(line.layer);
    UpdateCumulativeDataIfMaximum(line.layer, area);
    totalParticlesAreaOnThisLoop += area;

    return FormatFloat((float)cumulativeData[line.layer]);
}

void ParticlesMaxAreaColumn::OnAfterGetExtraInfoLoop()
{
    if (maxParticlesArea < totalParticlesAreaOnThisLoop)
    {
        maxParticlesArea = totalParticlesAreaOnThisLoop;
    }
}

QString ParticlesMaxAreaColumn::GetExtraInfoHeader()
{
    return "Max Area";
}

QString ParticlesMaxAreaColumn::GetExtraInfoFooter()
{
    return FormatFloat(maxParticlesArea);
}
