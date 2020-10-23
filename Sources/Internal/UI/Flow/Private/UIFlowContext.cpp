#include "UI/Flow/UIFlowContext.h"
#include "Engine/EngineContext.h"
#include "Logger/Logger.h"
#include "Reflection/ReflectedTypeDB.h"
#include "Reflection/ReflectionRegistrator.h"
#include "UI/Flow/UIFlowService.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIFlowContext)
{
    ReflectionRegistrator<UIFlowContext>::Begin()
    .DestructorByPointer([](UIFlowContext* c) { delete c; })
    .Field("data", &UIFlowContext::GetData, nullptr)
    .Method<UIFlowService* (UIFlowContext::*)(const FastName&) const>("GetService", &UIFlowContext::GetService)
    .End();
}

UIFlowContext::UIFlowContext()
    : data(new KeyedArchive())
{
}

KeyedArchive* UIFlowContext::GetData() const
{
    return data.Get();
}

UIFlowService* UIFlowContext::GetService(const FastName& name) const
{
    auto it = services.find(name);
    if (it != services.end())
    {
        return it->second.service.get();
    }
    return nullptr;
}

void UIFlowContext::InitServiceByType(const FastName& name, const String& typeName)
{
    auto it = services.find(name);
    if (it != services.end())
    {
        it->second.initCount++;
        return;
    }

    const ReflectedType* rType = ReflectedTypeDB::GetByPermanentName(typeName);
    if (rType)
    {
        Any obj = rType->CreateObject(ReflectedType::CreatePolicy::ByPointer);
        if (obj.CanCast<UIFlowService*>())
        {
            UIFlowService* service = obj.Cast<UIFlowService*>();
            service->Activate(this);
            ServiceLink link;
            link.service = std::unique_ptr<UIFlowService>(service);
            link.initCount = 1;
            services[name] = link;
        }
        else
        {
            Logger::Warning("Class with type `%s` not UIService!", typeName.c_str());
        }
    }
    else
    {
        Logger::Warning("Service with type `%s` not found!", typeName.c_str());
    }
}

void UIFlowContext::ReleaseService(const FastName& name)
{
    auto it = services.find(name);
    if (it != services.end())
    {
        it->second.initCount--;
        if (it->second.initCount <= 0)
        {
            it->second.service->Deactivate(this);
            services.erase(it);
        }
    }
}
}
