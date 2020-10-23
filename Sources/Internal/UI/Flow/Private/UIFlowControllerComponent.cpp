#include "UI/Flow/UIFlowControllerComponent.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIFlowControllerComponent)
{
    ReflectionRegistrator<UIFlowControllerComponent>::Begin()[M::DisplayName("Flow Controller")]
    .ConstructorByPointer()
    .DestructorByPointer([](UIFlowControllerComponent* c) { SafeRelease(c); })
    .Field("reflectionTypeName", &UIFlowControllerComponent::GetReflectionTypeName, &UIFlowControllerComponent::SetReflectionTypeName)[M::DisplayName("Type Name")]
    .Field("luaScriptPath", &UIFlowControllerComponent::GetLuaScriptPath, &UIFlowControllerComponent::SetLuaScriptPath)[M::DisplayName("Lua Script")]
    .End();
}

IMPLEMENT_UI_COMPONENT(UIFlowControllerComponent);

UIFlowControllerComponent::UIFlowControllerComponent() = default;

UIFlowControllerComponent::UIFlowControllerComponent(const UIFlowControllerComponent& dst) = default;

UIFlowControllerComponent::~UIFlowControllerComponent() = default;

UIFlowControllerComponent* UIFlowControllerComponent::Clone() const
{
    return new UIFlowControllerComponent(*this);
}

void UIFlowControllerComponent::SetReflectionTypeName(const String& typeName)
{
    reflectionTypeName = typeName;
}

void UIFlowControllerComponent::SetLuaScriptPath(const FilePath& filePath)
{
    luaScriptPath = filePath;
}
}
