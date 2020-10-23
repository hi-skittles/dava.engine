#include "Reflection/ReflectionRegistrator.h"
#include "UI/Script/UIScriptComponent.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIScriptComponent)
{
    ReflectionRegistrator<UIScriptComponent>::Begin()
    .ConstructorByPointer()
    .DestructorByPointer([](UIScriptComponent* c) { SafeRelease(c); })
    .Field("reflectionTypeName", &UIScriptComponent::GetReflectionTypeName, &UIScriptComponent::SetReflectionTypeName)
    .Field("luaScriptPath", &UIScriptComponent::GetLuaScriptPath, &UIScriptComponent::SetLuaScriptPath)
    .Field("parameters", &UIScriptComponent::GetParameters, &UIScriptComponent::SetParameters)
    .End();
}
IMPLEMENT_UI_COMPONENT(UIScriptComponent);

UIScriptComponent::UIScriptComponent() = default;

UIScriptComponent::UIScriptComponent(const UIScriptComponent& dst) = default;

UIScriptComponent::~UIScriptComponent() = default;

UIScriptComponent* UIScriptComponent::Clone() const
{
    return new UIScriptComponent(*this);
}

void UIScriptComponent::SetReflectionTypeName(const String& typeName)
{
    reflectionTypeName = typeName;
    luaScriptPath = "";
    SetModifiedScripts(true);
}

void UIScriptComponent::SetLuaScriptPath(const FilePath& filePath)
{
    luaScriptPath = filePath;
    reflectionTypeName = "";
    SetModifiedScripts(true);
}

void UIScriptComponent::SetParameters(const String& value)
{
    parameters = value;
    SetModifiedParameters(true);
}
bool UIScriptComponent::GetModifiedParameters() const
{
    return modifiedParameters;
}
void UIScriptComponent::SetModifiedParameters(bool value)
{
    modifiedParameters = value;
}
bool UIScriptComponent::GetModifiedScripts() const
{
    return modifiedScripts;
}
void UIScriptComponent::SetModifiedScripts(bool value)
{
    modifiedScripts = value;
}
}
