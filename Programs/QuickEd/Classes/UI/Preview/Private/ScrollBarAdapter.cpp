#include "Classes/UI/Preview/ScrollBarAdapter.h"

#include "Classes/Modules/CanvasModule/CanvasData.h"

#include <TArc/Core/ContextAccessor.h>

#include <Reflection/ReflectedTypeDB.h>

DAVA::FastName ScrollBarAdapter::positionPropertyName{ "position" };
DAVA::FastName ScrollBarAdapter::minPosPropertyName{ "minimum position" };
DAVA::FastName ScrollBarAdapter::maxPosPropertyName{ "maximum position" };
DAVA::FastName ScrollBarAdapter::pageStepPropertyName{ "page step" };
DAVA::FastName ScrollBarAdapter::enabledPropertyName{ "enabled" };
DAVA::FastName ScrollBarAdapter::visiblePropertyName{ "visible" };
DAVA::FastName ScrollBarAdapter::orientationPropertyName{ "orientation" };

DAVA_REFLECTION_IMPL(ScrollBarAdapter)
{
    DAVA::ReflectionRegistrator<ScrollBarAdapter>::Begin()
    .Field(positionPropertyName.c_str(), &ScrollBarAdapter::GetPosition, &ScrollBarAdapter::SetPosition)
    .Field(minPosPropertyName.c_str(), &ScrollBarAdapter::GetMinPos, nullptr)
    .Field(maxPosPropertyName.c_str(), &ScrollBarAdapter::GetMaxPos, nullptr)
    .Field(pageStepPropertyName.c_str(), &ScrollBarAdapter::GetPageStep, nullptr)
    .Field(enabledPropertyName.c_str(), &ScrollBarAdapter::IsEnabled, nullptr)
    .Field(visiblePropertyName.c_str(), &ScrollBarAdapter::IsVisible, nullptr)
    .Field(orientationPropertyName.c_str(), &ScrollBarAdapter::GetOrientation, nullptr)
    .End();
}

ScrollBarAdapter::ScrollBarAdapter(DAVA::Vector2::eAxis orientation_, DAVA::ContextAccessor* accessor_)
    : orientation(orientation_)
    , accessor(accessor_)
    , canvasDataAdapter(accessor)
{
}

int ScrollBarAdapter::GetPosition() const
{
    using namespace DAVA;

    DataContext* activeContext = accessor->GetActiveContext();
    if (activeContext == nullptr)
    {
        return 0;
    }

    CanvasData* canvasData = activeContext->GetData<CanvasData>();

    float32 minPos = GetMinPos();
    float32 maxPos = GetMaxPos();
    float32 position = canvasDataAdapter.GetPosition()[orientation];

    return minPos + maxPos - position;
}

void ScrollBarAdapter::SetPosition(int value)
{
    using namespace DAVA;

    DataContext* activeContext = accessor->GetActiveContext();
    if (activeContext == nullptr)
    {
        return;
    }

    int position = GetPosition();
    Vector2 delta(0.0f, 0.0f);
    delta[orientation] = position - value;
    canvasDataAdapter.MoveScene(delta);
}

int ScrollBarAdapter::GetMinPos() const
{
    DAVA::float32 realMinPos = canvasDataAdapter.GetMinPos()[orientation];
    return realMinPos;
}

int ScrollBarAdapter::GetMaxPos() const
{
    DAVA::float32 realMaxPos = canvasDataAdapter.GetMaxPos()[orientation];
    return realMaxPos;
}

int ScrollBarAdapter::GetPageStep() const
{
    return canvasDataAdapter.GetViewSize()[orientation];
}

bool ScrollBarAdapter::IsEnabled() const
{
    return accessor->GetActiveContext() != nullptr;
}

bool ScrollBarAdapter::IsVisible() const
{
    using namespace DAVA;
    DAVA::DataContext* active = accessor->GetActiveContext();
    if (active == nullptr)
    {
        return false;
    }

    return GetMaxPos() - GetMinPos() > 0;
}

Qt::Orientation ScrollBarAdapter::GetOrientation() const
{
    return orientation == DAVA::Vector2::AXIS_X ? Qt::Horizontal : Qt::Vertical;
}
