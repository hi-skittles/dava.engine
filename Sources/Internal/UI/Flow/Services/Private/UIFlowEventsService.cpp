#include "UI/Flow/Services/UIFlowEventsService.h"
#include "Engine/Engine.h"
#include "Engine/EngineContext.h"
#include "Reflection/ReflectionRegistrator.h"
#include "UI/Events/UIEventsSingleComponent.h"
#include "UI/UIControl.h"
#include "UI/UIControlSystem.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIFlowEventsService)
{
    ReflectionRegistrator<UIFlowEventsService>::Begin()
    .ConstructorByPointer()
    .DestructorByPointer([](UIFlowEventsService* s) { delete s; })
    .Method("Send", &UIFlowEventsService::Send)
    .End();
}

void UIFlowEventsService::Send(UIControl* control, const FastName& event)
{
    UIEventsSingleComponent* events = Engine::Instance()->GetContext()->uiControlSystem->GetSingleComponent<UIEventsSingleComponent>();
    if (events)
    {
        events->SendEvent(control, event, Any());
    }
}
}
