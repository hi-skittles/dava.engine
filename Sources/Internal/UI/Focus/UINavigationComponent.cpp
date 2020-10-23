#include "UI/Focus/UINavigationComponent.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UINavigationComponent)
{
    ReflectionRegistrator<UINavigationComponent>::Begin()[M::DisplayName("Navigation"), M::Group("Input")]
    .ConstructorByPointer()
    .DestructorByPointer([](UINavigationComponent* o) { o->Release(); })
    .Field("left", &UINavigationComponent::GetNextFocusLeft, &UINavigationComponent::SetNextFocusLeft)[M::DisplayName("Left")]
    .Field("right", &UINavigationComponent::GetNextFocusRight, &UINavigationComponent::SetNextFocusRight)[M::DisplayName("Rigth")]
    .Field("up", &UINavigationComponent::GetNextFocusUp, &UINavigationComponent::SetNextFocusUp)[M::DisplayName("Up")]
    .Field("down", &UINavigationComponent::GetNextFocusDown, &UINavigationComponent::SetNextFocusDown)[M::DisplayName("Down")]
    .End();
}
IMPLEMENT_UI_COMPONENT(UINavigationComponent);

UINavigationComponent::UINavigationComponent()
{
}

UINavigationComponent::UINavigationComponent(const UINavigationComponent& src)
{
    for (int i = 0; i < DIRECTION_COUNT; i++)
    {
        nextFocusPath[i] = src.nextFocusPath[i];
    }
}

UINavigationComponent::~UINavigationComponent()
{
}

UINavigationComponent* UINavigationComponent::Clone() const
{
    return new UINavigationComponent(*this);
}

const String& UINavigationComponent::GetNextFocusLeft() const
{
    return nextFocusPath[LEFT];
}

void UINavigationComponent::SetNextFocusLeft(const String& val)
{
    nextFocusPath[LEFT] = val;
}

const String& UINavigationComponent::GetNextFocusRight() const
{
    return nextFocusPath[RIGHT];
}

void UINavigationComponent::SetNextFocusRight(const String& val)
{
    nextFocusPath[RIGHT] = val;
}

const String& UINavigationComponent::GetNextFocusUp() const
{
    return nextFocusPath[UP];
}

void UINavigationComponent::SetNextFocusUp(const String& val)
{
    nextFocusPath[UP] = val;
}

const String& UINavigationComponent::GetNextFocusDown() const
{
    return nextFocusPath[DOWN];
}

void UINavigationComponent::SetNextFocusDown(const String& val)
{
    nextFocusPath[DOWN] = val;
}

const String& UINavigationComponent::GetNextControlPathInDirection(Direction dir)
{
    DVASSERT(0 <= dir && dir < DIRECTION_COUNT);
    return nextFocusPath[dir];
}
}
