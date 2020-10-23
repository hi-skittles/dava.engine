#include "UI/Events/UIEventBindingComponent.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Utils/Utils.h"
#include "Utils/StringUtils.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIEventBindingComponent)
{
    ReflectionRegistrator<UIEventBindingComponent>::Begin()[M::DisplayName("Event Binding"), M::Group("Input")]
    .ConstructorByPointer()
    .DestructorByPointer([](UIEventBindingComponent* o) { o->Release(); })
    .End();
}
IMPLEMENT_UI_COMPONENT(UIEventBindingComponent);

UIEventBindingComponent::UIEventBindingComponent()
{
}

UIEventBindingComponent::UIEventBindingComponent(const UIEventBindingComponent& src)
{
}

UIEventBindingComponent::~UIEventBindingComponent()
{
}

UIEventBindingComponent* UIEventBindingComponent::Clone() const
{
    return new UIEventBindingComponent(*this);
}

UIActionMap& UIEventBindingComponent::GetActionMap()
{
    return actionMap;
}

void UIEventBindingComponent::BindAction(const FastName& eventName, const UIActionMap::SimpleAction& action)
{
    actionMap.Put(eventName, action);
}

void UIEventBindingComponent::UnbindAction(const FastName& eventName)
{
    actionMap.Remove(eventName);
}
}
