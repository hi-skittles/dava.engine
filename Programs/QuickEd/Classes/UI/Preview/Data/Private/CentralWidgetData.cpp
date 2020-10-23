#include "UI/Preview/Data/CentralWidgetData.h"

#include <QWidget>

DAVA::FastName CentralWidgetData::viewSizePropertyName{ "view size" };
DAVA::FastName CentralWidgetData::guidesPosPropertyName{ "guides position" };
DAVA::FastName CentralWidgetData::guidesSizePropertyName{ "guides size" };
DAVA::FastName CentralWidgetData::guidesRelativePosPropertyName{ "guides relative pos" };

DAVA_VIRTUAL_REFLECTION_IMPL(CentralWidgetData)
{
    DAVA::ReflectionRegistrator<CentralWidgetData>::Begin()
    .Field(viewSizePropertyName.c_str(), &CentralWidgetData::GetViewSize, nullptr)
    .Field(guidesPosPropertyName.c_str(), &CentralWidgetData::GetGuidesPos, nullptr)
    .Field(guidesSizePropertyName.c_str(), &CentralWidgetData::GetGuidesSize, nullptr)
    .Field(guidesRelativePosPropertyName.c_str(), &CentralWidgetData::GetGuidesRelativePos, nullptr)
    .End();
}

CentralWidgetData::CentralWidgetData(QWidget* renderWidget_, QWidget* hRulerWidget_, QWidget* vRulerWidget_)
    : renderWidget(renderWidget_)
    , hRulerWidget(hRulerWidget_)
    , vRulerWidget(vRulerWidget_)
{
}

DAVA::Vector2 CentralWidgetData::GetViewSize() const
{
    using namespace DAVA;

    QSize size = renderWidget->size();
    return Vector2(static_cast<float32>(size.width()),
                   static_cast<float32>(size.height()));
}

DAVA::Vector2 CentralWidgetData::GetGuidesPos() const
{
    using namespace DAVA;

    return Vector2(static_cast<float32>(hRulerWidget->pos().y()),
                   static_cast<float32>(vRulerWidget->pos().x()));
}

DAVA::Vector2 CentralWidgetData::GetGuidesSize() const
{
    using namespace DAVA;
    int top = hRulerWidget->geometry().top();
    int bottom = renderWidget->geometry().bottom();

    int left = vRulerWidget->geometry().left();
    int right = renderWidget->geometry().right();

    return Vector2(static_cast<float32>(bottom - top),
                   static_cast<float32>(right - left));
}

DAVA::Vector2 CentralWidgetData::GetGuidesRelativePos() const
{
    using namespace DAVA;
    QPoint hRulerPos = hRulerWidget->pos();
    QPoint vRulerPos = vRulerWidget->pos();
    return Vector2(static_cast<float32>(hRulerPos.x()),
                   static_cast<float32>(vRulerPos.y()));
}
