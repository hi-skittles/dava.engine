#include "UI/Components/UIControlSourceComponent.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIControlSourceComponent)
{
    ReflectionRegistrator<UIControlSourceComponent>::Begin()[M::HiddenField()]
    .ConstructorByPointer()
    .DestructorByPointer([](UIControlSourceComponent* o) { o->Release(); })
    .Field("packagePath", &UIControlSourceComponent::GetPackagePath, &UIControlSourceComponent::SetPackagePath)
    .Field("controlName", &UIControlSourceComponent::GetControlName, &UIControlSourceComponent::SetControlName)
    .Field("prototypeName", &UIControlSourceComponent::GetPrototypeName, &UIControlSourceComponent::SetPrototypeName)
    .End();
}

IMPLEMENT_UI_COMPONENT(UIControlSourceComponent);

UIControlSourceComponent::UIControlSourceComponent() = default;
UIControlSourceComponent::~UIControlSourceComponent() = default;

UIControlSourceComponent::UIControlSourceComponent(const UIControlSourceComponent& src)
    : packagePath(src.packagePath)
    , controlName(src.controlName)
    , prototypeName(src.prototypeName)
{
}

UIControlSourceComponent* UIControlSourceComponent::Clone() const
{
    return new UIControlSourceComponent(*this);
}

void UIControlSourceComponent::SetPackagePath(const String& path)
{
    packagePath = path;
}

void UIControlSourceComponent::SetControlName(const String& name)
{
    controlName = name;
}

void UIControlSourceComponent::SetPrototypeName(const String& name)
{
    prototypeName = name;
}
}
