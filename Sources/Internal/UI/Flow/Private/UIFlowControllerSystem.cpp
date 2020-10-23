#include "UI/Flow/UIFlowControllerSystem.h"
#include "Base/TemplateHelpers.h"
#include "Reflection/ReflectedTypeDB.h"
#include "Scripting/LuaScript.h"
#include "UI/Flow/Private/UIFlowLuaController.h"
#include "UI/Flow/UIFlowControllerComponent.h"
#include "UI/Flow/UIFlowContext.h"
#include "UI/Flow/UIFlowController.h"
#include "UI/UIControl.h"

namespace DAVA
{
UIFlowControllerSystem::~UIFlowControllerSystem() = default;

void UIFlowControllerSystem::RegisterControl(UIControl* control)
{
    UIFlowControllerComponent* c = control->GetComponent<UIFlowControllerComponent>();
    if (c)
    {
        AddControllerLink(c);
    }
}

void UIFlowControllerSystem::UnregisterControl(UIControl* control)
{
    UIFlowControllerComponent* c = control->GetComponent<UIFlowControllerComponent>();
    if (c)
    {
        RemoveControllerLink(c);
    }
}

void UIFlowControllerSystem::RegisterComponent(UIControl* control, UIComponent* component)
{
    if (component->GetType() == Type::Instance<UIFlowControllerComponent>())
    {
        UIFlowControllerComponent* c = DynamicTypeCheck<UIFlowControllerComponent*>(component);
        AddControllerLink(c);
    }
}

void UIFlowControllerSystem::UnregisterComponent(UIControl* control, UIComponent* component)
{
    if (component->GetType() == Type::Instance<UIFlowControllerComponent>())
    {
        UIFlowControllerComponent* c = DynamicTypeCheck<UIFlowControllerComponent*>(component);
        RemoveControllerLink(c);
    }
}

void UIFlowControllerSystem::Process(float32 elapsedTime)
{
    for (ControllerLink& link : links)
    {
        if (link.controller)
        {
            link.controller->Process(elapsedTime);
        }
    }
}

UIFlowController* UIFlowControllerSystem::GetController(UIFlowControllerComponent* component) const
{
    auto it = std::find_if(links.begin(), links.end(), [&](const ControllerLink& l) {
        return l.component == component;
    });
    if (it != links.end())
    {
        return it->controller.get();
    }
    return nullptr;
}

void UIFlowControllerSystem::AddControllerLink(UIFlowControllerComponent* component)
{
    ControllerLink l;
    l.component = component;
    links.push_back(std::move(l));
}

void UIFlowControllerSystem::RemoveControllerLink(UIFlowControllerComponent* component)
{
    auto it = std::remove_if(links.begin(), links.end(), [&](const ControllerLink& l) { return l.component == component; });
    links.erase(it, links.end());
}

UIFlowController* UIFlowControllerSystem::InitController(UIFlowControllerComponent* component, UIFlowContext* context)
{
    auto it = std::find_if(links.begin(), links.end(), [&](const ControllerLink& l) {
        return l.component == component;
    });
    if (it != links.end())
    {
        const String& nativeTypeName = component->GetReflectionTypeName();
        if (!nativeTypeName.empty())
        {
            const ReflectedType* type = ReflectedTypeDB::GetByPermanentName(nativeTypeName);
            if (type)
            {
                const Any& object = type->CreateObject(ReflectedType::CreatePolicy::ByPointer);
                it->controller.reset(object.Cast<UIFlowController*>());
                it->controller->Init(context);
                return it->controller.get();
            }
            else
            {
                Logger::Warning("Controller with type `%s` not found!", nativeTypeName.c_str());
            }
        }

        const FilePath& luaPath = component->GetLuaScriptPath();
        if (!luaPath.IsEmpty())
        {
            if (luaPath.Exists())
            {
                it->controller = std::make_shared<UIFlowLuaController>(luaPath);
                it->controller->Init(context);
                return it->controller.get();
            }
            else
            {
                Logger::Warning("Lua controller with path `%s` not found!", luaPath.GetStringValue().c_str());
            }
        }
    }
    return nullptr;
}

void UIFlowControllerSystem::ReleaseController(UIFlowControllerComponent* component, UIFlowContext* context)
{
    auto it = std::find_if(links.begin(), links.end(), [&](const ControllerLink& l) {
        return l.component == component;
    });
    if (it != links.end())
    {
        if (it->controller)
        {
            it->controller->Release(context);
            it->controller.reset();
        }
    }
}

void UIFlowControllerSystem::LoadController(UIFlowControllerComponent* component, UIFlowContext* context, UIControl* view)
{
    auto it = std::find_if(links.begin(), links.end(), [&](const ControllerLink& l) {
        return l.component == component;
    });
    if (it != links.end())
    {
        if (it->controller)
        {
            it->controller->LoadResources(context, view);
        }
    }
}

void UIFlowControllerSystem::UnloadController(UIFlowControllerComponent* component, UIFlowContext* context, UIControl* view)
{
    auto it = std::find_if(links.begin(), links.end(), [&](const ControllerLink& l) {
        return l.component == component;
    });
    if (it != links.end())
    {
        if (it->controller)
        {
            it->controller->UnloadResources(context, view);
        }
    }
}

UIFlowController* UIFlowControllerSystem::ActivateController(UIFlowControllerComponent* component, UIFlowContext* context, UIControl* view)
{
    auto it = std::find_if(links.begin(), links.end(), [&](const ControllerLink& l) {
        return l.component == component;
    });
    if (it != links.end())
    {
        if (it->controller)
        {
            it->controller->Activate(context, view);
        }
        return it->controller.get();
    }
    return nullptr;
}

void UIFlowControllerSystem::DeactivateController(UIFlowControllerComponent* component, UIFlowContext* context, UIControl* view)
{
    auto it = std::find_if(links.begin(), links.end(), [&](const ControllerLink& l) {
        return l.component == component;
    });
    if (it != links.end())
    {
        if (it->controller)
        {
            it->controller->Deactivate(context, view);
        }
    }
}
}
