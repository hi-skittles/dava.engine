#include "UI/Script/UIScriptSystem.h"
#include "Base/TemplateHelpers.h"
#include "Reflection/ReflectedTypeDB.h"
#include "Scripting/LuaScript.h"
#include "UI/Script/Private/UILuaScriptComponentController.h"
#include "UI/Script/UIScriptComponent.h"
#include "UI/Script/UIScriptComponentController.h"
#include "UI/UIControl.h"

namespace DAVA
{
UIScriptSystem::~UIScriptSystem() = default;

void UIScriptSystem::RegisterControl(UIControl* control)
{
    UIScriptComponent* c = control->GetComponent<UIScriptComponent>();
    if (c)
    {
        AddScriptLink(c);
    }
}

void UIScriptSystem::UnregisterControl(UIControl* control)
{
    UIScriptComponent* c = control->GetComponent<UIScriptComponent>();
    if (c)
    {
        RemoveScriptLink(c);
    }
}

void UIScriptSystem::RegisterComponent(UIControl* control, UIComponent* component)
{
    if (component->GetType() == Type::Instance<UIScriptComponent>())
    {
        UIScriptComponent* c = DynamicTypeCheck<UIScriptComponent*>(component);
        AddScriptLink(c);
    }
}

void UIScriptSystem::UnregisterComponent(UIControl* control, UIComponent* component)
{
    if (component->GetType() == Type::Instance<UIScriptComponent>())
    {
        UIScriptComponent* c = DynamicTypeCheck<UIScriptComponent*>(component);
        RemoveScriptLink(c);
    }
}

void UIScriptSystem::Process(float32 elapsedTime)
{
    for (ScriptLink& link : links)
    {
        if (link.component)
        {
            if (link.component->GetModifiedScripts())
            {
                UpdateController(link);
            }
            if (link.controller && link.component->GetControl()->GetVisibilityFlag())
            {
                if (link.component->GetModifiedParameters())
                {
                    link.controller->ParametersChanged(link.component);
                    link.component->SetModifiedParameters(false);
                }
                if (!pauseProcessing)
                {
                    link.controller->Process(link.component, elapsedTime);
                }
            }
        }
    }
}

bool UIScriptSystem::ProcessEvent(UIControl* control, const FastName& event, const Any& data)
{
    UIScriptComponent* component = control->GetComponent<UIScriptComponent>();
    if (component)
    {
        auto it = std::find_if(links.begin(), links.end(), [&](const ScriptLink& l) {
            return l.component == component;
        });
        if (it != links.end())
        {
            if (it->controller)
            {
                Vector<Any> controllerData;
                controllerData.push_back(data);
                return it->controller->ProcessEvent(component, event, controllerData);
            }
        }
    }
    return false;
}

UIScriptComponentController* UIScriptSystem::GetController(UIScriptComponent* component) const
{
    auto it = std::find_if(links.begin(), links.end(), [&](const ScriptLink& l) {
        return l.component == component;
    });
    if (it != links.end())
    {
        return it->controller.get();
    }
    return nullptr;
}

void UIScriptSystem::AddScriptLink(UIScriptComponent* component)
{
    ScriptLink l;
    l.component = component;
    UpdateController(l);
    links.push_back(std::move(l));
}

void UIScriptSystem::UpdateController(DAVA::UIScriptSystem::ScriptLink& l)
{
    if (l.controller)
    {
        l.controller->Release(l.component);
        l.controller.reset();
    }
    DAVA::UIScriptComponent* component = l.component;
    const String& nativeTypeName = component->GetReflectionTypeName();
    if (!nativeTypeName.empty())
    {
        const ReflectedType* type = ReflectedTypeDB::GetByPermanentName(nativeTypeName);
        if (type)
        {
            const Any& object = type->CreateObject(ReflectedType::CreatePolicy::ByPointer);
            l.controller.reset(object.Cast<UIScriptComponentController*>());
            l.controller->Init(component);
        }
        else
        {
            Logger::Warning("Controller with type `%s` not found!", nativeTypeName.c_str());
        }
    }

    const FilePath& luaPath = component->GetLuaScriptPath();
    if (luaPath.Exists())
    {
        l.controller = std::make_shared<UILuaScriptComponentController>(luaPath);
        l.controller->Init(component);
    }
    component->SetModifiedScripts(false);
}

void UIScriptSystem::RemoveScriptLink(UIScriptComponent* component)
{
    auto it = std::find_if(links.begin(), links.end(), [&](const ScriptLink& l) { return l.component == component; });
    if (it != links.end())
    {
        if (it->controller)
        {
            it->controller->Release(component);
            it->controller.reset();
        }
        links.erase(it);
    }
}

void UIScriptSystem::SetPauseProcessing(bool value)
{
    pauseProcessing = value;
}
}
