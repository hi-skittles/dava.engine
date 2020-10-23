#include "UI/Joypad/UIJoypadComponent.h"

#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "Reflection/ReflectionRegistrator.h"
#include "UI/UIControl.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIJoypadComponent)
{
    ReflectionRegistrator<UIJoypadComponent>::Begin()
    .ConstructorByPointer()
    .DestructorByPointer([](UIJoypadComponent* c) { SafeRelease(c); })
    .Field("stickAreaControlPath", &UIJoypadComponent::GetStickAreaControlPath, &UIJoypadComponent::SetStickAreaControlPath)
    .Field("stickArmControlPath", &UIJoypadComponent::GetStickArmControlPath, &UIJoypadComponent::SetStickArmControlPath)
    .Field("stickArrowControlPath", &UIJoypadComponent::GetStickArrowControlPath, &UIJoypadComponent::SetStickArrowControlPath)
    .Field("stickAreaRadius", &UIJoypadComponent::GetStickAreaRadius, &UIJoypadComponent::SetStickAreaRadius)
    .Field("isActive", &UIJoypadComponent::IsActive, &UIJoypadComponent::SetActiveFlag)
    .Field("isDynamic", &UIJoypadComponent::IsDynamic, &UIJoypadComponent::SetDynamicFlag)
    .Field("coords", &UIJoypadComponent::GetOriginalCoords, &UIJoypadComponent::SetOriginalCoords)
    .Field("activationThreshold", &UIJoypadComponent::GetActivationThreshold, &UIJoypadComponent::SetActivationThreshold)
    .Field("initialPosition", &UIJoypadComponent::GetInitialPosition, &UIJoypadComponent::SetInitialPosition)
    .Field("cancelZone", &UIJoypadComponent::GetCancelZone, &UIJoypadComponent::SetCancelZone)
    .Field("cancelRadius", &UIJoypadComponent::GetCancelRadius, &UIJoypadComponent::SetCancelRadius)
    .Method("GetTransformedCoords", &UIJoypadComponent::GetTransformedCoords)
    .End();
}

IMPLEMENT_UI_COMPONENT(UIJoypadComponent);

UIJoypadComponent::UIJoypadComponent(const UIJoypadComponent& other)
    : UIComponent(other)
    , stickAreaControlPath(other.stickAreaControlPath)
    , stickArmControlPath(other.stickArmControlPath)
    , stickArrowControlPath(other.stickArrowControlPath)
    , coordsTransformFn(other.coordsTransformFn)
    , stickAreaRadius(other.stickAreaRadius)
    , isDynamic(other.isDynamic)
    , isActive(other.isActive)
    , coords(other.coords)
    , initialPosition(other.initialPosition)
    , activationThreshold(other.activationThreshold)
    , cancelZone(other.cancelZone)
    , cancelRadius(other.cancelRadius)
{
}

UIJoypadComponent* UIJoypadComponent::Clone() const
{
    return new UIJoypadComponent(*this);
}

bool UIJoypadComponent::IsDynamic() const
{
    return isDynamic;
}

void UIJoypadComponent::SetDynamicFlag(bool dynamic)
{
    isDynamic = dynamic;
}

const String& UIJoypadComponent::GetStickAreaControlPath() const
{
    return stickAreaControlPath;
}

void UIJoypadComponent::SetStickAreaControlPath(const String& areaControlPath)
{
    stickAreaControlPath = areaControlPath;
}

const String& UIJoypadComponent::GetStickArmControlPath() const
{
    return stickArmControlPath;
}

void UIJoypadComponent::SetStickArmControlPath(const String& armControlPath)
{
    stickArmControlPath = armControlPath;
}

const String& UIJoypadComponent::GetStickArrowControlPath() const
{
    return stickArrowControlPath;
}

void UIJoypadComponent::SetStickArrowControlPath(const String& arrowControlPath)
{
    stickArrowControlPath = arrowControlPath;
}

void UIJoypadComponent::SetCoordsTransformFunction(CoordsTransformFn fn)
{
    coordsTransformFn = fn;
}

const UIJoypadComponent::CoordsTransformFn& UIJoypadComponent::GetCoordsTransformFunction()
{
    return coordsTransformFn;
}

float32 UIJoypadComponent::GetStickAreaRadius() const
{
    return stickAreaRadius;
}

void UIJoypadComponent::SetStickAreaRadius(float32 radius)
{
    stickAreaRadius = radius;
}

bool UIJoypadComponent::IsActive() const
{
    return isActive;
}

void UIJoypadComponent::SetActiveFlag(bool active)
{
    isActive = active;
}

Vector2 UIJoypadComponent::GetOriginalCoords() const
{
    return coords;
}

void UIJoypadComponent::SetOriginalCoords(Vector2 coords_)
{
    coords = coords_;
}

Vector2 UIJoypadComponent::GetInitialPosition() const
{
    return initialPosition;
}

void UIJoypadComponent::SetInitialPosition(Vector2 position)
{
    initialPosition = position;
}

float32 UIJoypadComponent::GetActivationThreshold() const
{
    return activationThreshold;
}

void UIJoypadComponent::SetActivationThreshold(float32 threshold)
{
    activationThreshold = threshold;
}

float32 UIJoypadComponent::GetCancelRadius() const
{
    return cancelRadius;
}

void UIJoypadComponent::SetCancelRadius(float32 radius)
{
    cancelRadius = radius;
}

const Vector4& UIJoypadComponent::GetCancelZone() const
{
    return cancelZone;
}

void UIJoypadComponent::SetCancelZone(const Vector4& zone)
{
    cancelZone = zone;
}

Vector2 UIJoypadComponent::GetTransformedCoords() const
{
    if (coordsTransformFn)
    {
        Vector2 tCoords = coordsTransformFn(coords);
        DVASSERT(fabs(tCoords.x) <= 1.f && fabs(tCoords.y) <= 1.f);
        return tCoords;
    }

    return coords;
}

UIControl* UIJoypadComponent::GetStickArea()
{
    UIControl* c = GetControl();

    UIControl* stickArea = (!stickAreaControlPath.empty() && c != nullptr ? c->FindByPath(stickAreaControlPath) : nullptr);

    return stickArea;
}

UIControl* UIJoypadComponent::GetStickArm()
{
    UIControl* c = GetControl();

    UIControl* stickArm = (!stickArmControlPath.empty() && c != nullptr ? c->FindByPath(stickArmControlPath) : nullptr);

    return stickArm;
}

UIControl* UIJoypadComponent::GetStickArrow()
{
    UIControl* c = GetControl();

    UIControl* stickArrow = (!stickArrowControlPath.empty() && c != nullptr ? c->FindByPath(stickArrowControlPath) : nullptr);

    return stickArrow;
}
}