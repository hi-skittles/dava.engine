#include "UI/Flow/Services/UIFlowSystemService.h"
#include "Engine/EngineContext.h"
#include "Logger/Logger.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Render/2D/Sprite.h"
#include "Render/Texture.h"
#include "UI/Flow/UIFlowStateComponent.h"
#include "UI/Flow/UIFlowStateSystem.h"
#include "UI/Render/UIRenderSystem.h"
#include "UI/UIControlSystem.h"
#include "UI/UIScreen.h"
#include "UI/UIScreenshoter.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIFlowSystemService)
{
    ReflectionRegistrator<UIFlowSystemService>::Begin()
    .ConstructorByPointer()
    .DestructorByPointer([](UIFlowSystemService* s) { delete s; })
    .Method("ActivateState", &UIFlowSystemService::ActivateState)
    .Method("DeactivateState", &UIFlowSystemService::DeactivateState)
    .Method("PreloadState", &UIFlowSystemService::PreloadState)
    .Method("IsStateLoaded", &UIFlowSystemService::IsStateLoaded)
    .Method("IsStateActive", &UIFlowSystemService::IsStateActive)
    .Method("HasTransitions", &UIFlowSystemService::HasTransitions)
    .Method("GetCurrentSingleState", &UIFlowSystemService::GetCurrentSingleState)
    .Method("GetCurrentMultipleStates", &UIFlowSystemService::GetCurrentMultipleStates)
    .End();
}

void UIFlowSystemService::ActivateState(const String& path, bool background)
{
    UIFlowStateSystem* sys = GetEngineContext()->uiControlSystem->GetSystem<UIFlowStateSystem>();
    sys->ActivateState(sys->FindStateByPath(path), background);
}

void UIFlowSystemService::DeactivateState(const String& path, bool background)
{
    UIFlowStateSystem* sys = GetEngineContext()->uiControlSystem->GetSystem<UIFlowStateSystem>();
    sys->DeactivateState(sys->FindStateByPath(path), background);
}

void UIFlowSystemService::PreloadState(const String& path, bool background)
{
    UIFlowStateSystem* sys = GetEngineContext()->uiControlSystem->GetSystem<UIFlowStateSystem>();
    sys->PreloadState(sys->FindStateByPath(path), background);
}

bool UIFlowSystemService::IsStateLoaded(const String& path)
{
    UIFlowStateSystem* sys = GetEngineContext()->uiControlSystem->GetSystem<UIFlowStateSystem>();
    return sys->IsStateLoaded(sys->FindStateByPath(path));
}

bool UIFlowSystemService::IsStateActive(const String& path)
{
    UIFlowStateSystem* sys = GetEngineContext()->uiControlSystem->GetSystem<UIFlowStateSystem>();
    return sys->IsStateActive(sys->FindStateByPath(path));
}

bool UIFlowSystemService::HasTransitions()
{
    UIFlowStateSystem* sys = GetEngineContext()->uiControlSystem->GetSystem<UIFlowStateSystem>();
    return sys->HasTransitions();
}

UIFlowStateComponent* UIFlowSystemService::GetCurrentSingleState()
{
    UIFlowStateSystem* sys = GetEngineContext()->uiControlSystem->GetSystem<UIFlowStateSystem>();
    return sys->GetCurrentSingleState();
}

const Vector<UIFlowStateComponent*>& UIFlowSystemService::GetCurrentMultipleStates()
{
    UIFlowStateSystem* sys = GetEngineContext()->uiControlSystem->GetSystem<UIFlowStateSystem>();
    return sys->GetCurrentMultipleStates();
}
}
