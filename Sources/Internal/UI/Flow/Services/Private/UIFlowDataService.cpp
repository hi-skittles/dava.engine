#include "UI/Flow/Services/UIFlowDataService.h"
#include "Engine/Engine.h"
#include "Engine/EngineContext.h"
#include "Logger/Logger.h"
#include "Reflection/ReflectionRegistrator.h"
#include "UI/DataBinding/UIDataBindingSystem.h"
#include "UI/Flow/UIFlowContext.h"
#include "UI/UIControlSystem.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIFlowDataService)
{
    ReflectionRegistrator<UIFlowDataService>::Begin()
    .ConstructorByPointer()
    .DestructorByPointer([](UIFlowDataService* s) { delete s; })
    .Method("SetDataDirty", &UIFlowDataService::SetDataDirty)
    .End();
}

void UIFlowDataService::SetDataDirty(const Reflection& ref)
{
    UIDataBindingSystem* dbs = Engine::Instance()->GetContext()->uiControlSystem->GetSystem<UIDataBindingSystem>();
    if (dbs)
    {
        dbs->SetDataDirty(ref.GetValueObject().GetVoidPtr());
    }
}
}
