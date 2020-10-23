#include "UI/Flow/UIFlowViewComponent.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIFlowViewComponent)
{
    ReflectionRegistrator<UIFlowViewComponent>::Begin()[M::DisplayName("Flow View")]
    .ConstructorByPointer()
    .DestructorByPointer([](UIFlowViewComponent* c) { SafeRelease(c); })
    .Field("viewYaml", &UIFlowViewComponent::GetViewYaml, &UIFlowViewComponent::SetViewYaml)[M::DisplayName("View YAML")]
    .Field("controlName", &UIFlowViewComponent::GetControlName, &UIFlowViewComponent::SetControlName)[M::DisplayName("Control Name")]
    .Field("containerPath", &UIFlowViewComponent::GetContainerPath, &UIFlowViewComponent::SetContainerPath)[M::DisplayName("Container Path")]
    .Field("modelName", &UIFlowViewComponent::GetModelName, &UIFlowViewComponent::SetModelName)[M::DisplayName("Model Name")]
    .Field("modelScope", &UIFlowViewComponent::GetModelScope, &UIFlowViewComponent::SetModelScope)[M::DisplayName("Model Scope")]
    .End();
}

IMPLEMENT_UI_COMPONENT(UIFlowViewComponent);

UIFlowViewComponent::UIFlowViewComponent() = default;

UIFlowViewComponent::UIFlowViewComponent(const UIFlowViewComponent& dst) = default;

UIFlowViewComponent::~UIFlowViewComponent() = default;

UIFlowViewComponent* UIFlowViewComponent::Clone() const
{
    return new UIFlowViewComponent(*this);
}

void UIFlowViewComponent::SetViewYaml(const FilePath& path)
{
    viewYamlPath = path;
}

void UIFlowViewComponent::SetControlName(const String& name)
{
    controlName = name;
}

void UIFlowViewComponent::SetContainerPath(const String& path)
{
    containerPath = path;
}

void UIFlowViewComponent::SetModelName(const String& name)
{
    modelName = name;
}

void UIFlowViewComponent::SetModelScope(const String& scope)
{
    modelScope = scope;
}
}
