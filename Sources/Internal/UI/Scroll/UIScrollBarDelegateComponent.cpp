#include "UI/Scroll/UIScrollBarDelegateComponent.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIScrollBarDelegateComponent)
{
    ReflectionRegistrator<UIScrollBarDelegateComponent>::Begin()[M::DisplayName("Scroll Bar Delegate"), M::Group("Input")]
    .ConstructorByPointer()
    .DestructorByPointer([](UIScrollBarDelegateComponent* o) { o->Release(); })
    .Field("delegate", &UIScrollBarDelegateComponent::GetPathToDelegate, &UIScrollBarDelegateComponent::SetPathToDelegate)[M::DisplayName("Delegate")]
    .End();
}

IMPLEMENT_UI_COMPONENT(UIScrollBarDelegateComponent);

UIScrollBarDelegateComponent::UIScrollBarDelegateComponent()
{
}

UIScrollBarDelegateComponent::UIScrollBarDelegateComponent(const UIScrollBarDelegateComponent& src)
    : pathToDelegate(src.pathToDelegate)
{
}

UIScrollBarDelegateComponent::~UIScrollBarDelegateComponent()
{
}

UIScrollBarDelegateComponent* UIScrollBarDelegateComponent::Clone() const
{
    return new UIScrollBarDelegateComponent(*this);
}

const String& UIScrollBarDelegateComponent::GetPathToDelegate() const
{
    return pathToDelegate;
}

void UIScrollBarDelegateComponent::SetPathToDelegate(const String& path)
{
    pathToDelegate = path;
    pathToDelegateDirty = true;
}

bool UIScrollBarDelegateComponent::IsPathToDelegateDirty() const
{
    return pathToDelegateDirty;
}

void UIScrollBarDelegateComponent::ResetPathToDelegateDirty()
{
    pathToDelegateDirty = false;
}
}
