#include "UI/Flow/Services/UIFlowEngineService.h"
#include "Engine/Engine.h"
#include "Engine/EngineContext.h"
#include "Logger/Logger.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIFlowEngineService)
{
    ReflectionRegistrator<UIFlowEngineService>::Begin()
    .ConstructorByPointer()
    .DestructorByPointer([](UIFlowEngineService* s) { delete s; })
    .Field("engine", &UIFlowEngineService::GetEngine, nullptr)
    .Field("engineContext", &UIFlowEngineService::GetEngineContext, nullptr)
    .End();
}

const Engine* UIFlowEngineService::GetEngine() const
{
    return Engine::Instance();
}

const EngineContext* UIFlowEngineService::GetEngineContext() const
{
    return GetEngine()->GetContext();
}
}