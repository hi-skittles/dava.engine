#include "UI/Scene3D/UISceneComponent.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "Reflection/ReflectionRegistrator.h"
#include "UI/UIControl.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UISceneComponent)
{
    ReflectionRegistrator<UISceneComponent>::Begin()[M::HiddenField()]
    .ConstructorByPointer()
    .DestructorByPointer([](UISceneComponent* o) { o->Release(); })
    .End();
}

IMPLEMENT_UI_COMPONENT(UISceneComponent);

UISceneComponent::UISceneComponent() = default;

UISceneComponent::~UISceneComponent() = default;

UISceneComponent::UISceneComponent(const UISceneComponent& src)
    : UIComponent(src)
{
}

UISceneComponent* UISceneComponent::Clone() const
{
    return new UISceneComponent(*this);
}
}
