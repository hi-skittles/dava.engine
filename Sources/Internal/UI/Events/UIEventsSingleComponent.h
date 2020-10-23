#pragma once

#include <Base/FastName.h>
#include <Base/RefPtr.h>
#include <UI/Components/UISingleComponent.h>

namespace DAVA
{
class UIControl;

struct DefferedEvent
{
    DefferedEvent(UIControl* control, const FastName& event, const Any& data, bool broadcast_);
    DefferedEvent(const DefferedEvent&) = default;
    DefferedEvent() = default;
    ~DefferedEvent() = default;

    RefPtr<UIControl> control;
    FastName event;
    Any data;
    bool broadcast;
};

/** Single component for . */
struct UIEventsSingleComponent : public UISingleComponent
{
    /** Events queue */
    Deque<DefferedEvent> events;

    void ResetState() override;

    /** Send event through controls graph. */
    bool SendEvent(UIControl* control, const FastName& event, const Any& data);

    /** Broadcast event to control children. */
    bool SendBroadcastEvent(UIControl* control, const FastName& event, const Any& data);
};
}
