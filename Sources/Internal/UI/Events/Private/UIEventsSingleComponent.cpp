#include "UI/Events/UIEventsSingleComponent.h"

#include <UI/UIControl.h>

namespace DAVA
{
DefferedEvent::DefferedEvent(UIControl* control_, const FastName& event_, const Any& data_, bool broadcast_)
    : event(event_)
    , data(data_)
    , broadcast(broadcast_)
{
    control = control_;
}

void UIEventsSingleComponent::ResetState()
{
}

bool UIEventsSingleComponent::SendEvent(UIControl* control, const FastName& event, const Any& data)
{
    if (event.IsValid())
    {
        events.emplace_back(control, event, data, false);
        return true;
    }
    return false;
}

bool UIEventsSingleComponent::SendBroadcastEvent(UIControl* control, const FastName& event, const Any& data)
{
    if (event.IsValid())
    {
        events.emplace_back(control, event, data, true);
        return true;
    }
    return false;
}
}
