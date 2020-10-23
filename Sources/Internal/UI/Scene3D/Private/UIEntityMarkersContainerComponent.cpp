#include "UI/Scene3D/UIEntityMarkersContainerComponent.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Scene3D/Entity.h"

ENUM_DECLARE(DAVA::UIEntityMarkersContainerComponent::OrderMode)
{
    ENUM_ADD_DESCR(DAVA::UIEntityMarkersContainerComponent::OrderMode::NearFront, "NearFront");
    ENUM_ADD_DESCR(DAVA::UIEntityMarkersContainerComponent::OrderMode::NearBack, "NearBack");
}

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIEntityMarkersContainerComponent)
{
    ReflectionRegistrator<UIEntityMarkersContainerComponent>::Begin()
    .ConstructorByPointer()
    .DestructorByPointer([](UIEntityMarkersContainerComponent* c) { SafeRelease(c); })
    .Field("enabled", &UIEntityMarkersContainerComponent::IsEnabled, &UIEntityMarkersContainerComponent::SetEnabled)
    .Field("syncVisibilityEnabled", &UIEntityMarkersContainerComponent::IsSyncVisibilityEnabled, &UIEntityMarkersContainerComponent::SetSyncVisibilityEnabled)
    .Field("syncPositionEnabled", &UIEntityMarkersContainerComponent::IsSyncPositionEnabled, &UIEntityMarkersContainerComponent::SetSyncPositionEnabled)
    .Field("syncScaleEnabled", &UIEntityMarkersContainerComponent::IsSyncScaleEnabled, &UIEntityMarkersContainerComponent::SetSyncScaleEnabled)
    .Field("scaleFactor", &UIEntityMarkersContainerComponent::GetScaleFactor, &UIEntityMarkersContainerComponent::SetScaleFactor)
    .Field("maxScale", &UIEntityMarkersContainerComponent::GetMaxScale, &UIEntityMarkersContainerComponent::SetMaxScale)
    .Field("minScale", &UIEntityMarkersContainerComponent::GetMinScale, &UIEntityMarkersContainerComponent::SetMinScale)
    .Field("syncOrderEnabled", &UIEntityMarkersContainerComponent::IsSyncOrderEnabled, &UIEntityMarkersContainerComponent::SetSyncOrderEnabled)
    .Field("orderMode", &UIEntityMarkersContainerComponent::GetOrderMode, &UIEntityMarkersContainerComponent::SetOrderMode)[M::EnumT<OrderMode>()]
    .Field("useCustomStrategy", &UIEntityMarkersContainerComponent::IsUseCustomStrategy, &UIEntityMarkersContainerComponent::SetUseCustomStrategy)
    .Field("customStrategy", &UIEntityMarkersContainerComponent::GetCustomStrategy, &UIEntityMarkersContainerComponent::SetCustomStrategy)[M::HiddenField()]
    .End();
}

IMPLEMENT_UI_COMPONENT(UIEntityMarkersContainerComponent);

UIEntityMarkersContainerComponent::UIEntityMarkersContainerComponent() = default;

UIEntityMarkersContainerComponent::~UIEntityMarkersContainerComponent() = default;

UIEntityMarkersContainerComponent::UIEntityMarkersContainerComponent(const UIEntityMarkersContainerComponent& src)
    : enabled(src.enabled)
    , syncVisibilityEnabled(src.syncVisibilityEnabled)
    , syncPositionEnabled(src.syncPositionEnabled)
    , syncScaleEnabled(src.syncScaleEnabled)
    , scaleFactor(src.scaleFactor)
    , minScale(src.minScale)
    , maxScale(src.maxScale)
    , syncOrderEnabled(src.syncOrderEnabled)
    , orderMode(src.orderMode)
    , useCustomStrategy(src.useCustomStrategy)
    , customStrategy(src.customStrategy)
{
}

UIEntityMarkersContainerComponent* UIEntityMarkersContainerComponent::Clone() const
{
    return new UIEntityMarkersContainerComponent(*this);
}

void UIEntityMarkersContainerComponent::SetEnabled(bool enable)
{
    enabled = enable;
}

void UIEntityMarkersContainerComponent::SetSyncVisibilityEnabled(bool absolute)
{
    syncVisibilityEnabled = absolute;
}

void UIEntityMarkersContainerComponent::SetSyncPositionEnabled(bool absolute)
{
    syncPositionEnabled = absolute;
}

void UIEntityMarkersContainerComponent::SetSyncScaleEnabled(bool absolute)
{
    syncScaleEnabled = absolute;
}

void UIEntityMarkersContainerComponent::SetScaleFactor(const Vector2& factor)
{
    scaleFactor = factor;
}

void UIEntityMarkersContainerComponent::SetMaxScale(const Vector2& s)
{
    maxScale = s;
}

void UIEntityMarkersContainerComponent::SetMinScale(const Vector2& s)
{
    minScale = s;
}

void UIEntityMarkersContainerComponent::SetSyncOrderEnabled(bool enable)
{
    syncOrderEnabled = enable;
}

void UIEntityMarkersContainerComponent::SetOrderMode(OrderMode mode)
{
    orderMode = mode;
}

void UIEntityMarkersContainerComponent::SetUseCustomStrategy(bool enable)
{
    useCustomStrategy = enable;
}

void UIEntityMarkersContainerComponent::SetCustomStrategy(const CustomStrategy& fn)
{
    customStrategy = fn;
}
}
