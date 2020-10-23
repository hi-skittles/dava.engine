#include "Base/EventDispatcher.h"
#include <iterator>

namespace DAVA
{
EventDispatcher::EventDispatcher()
{
}

EventDispatcher::~EventDispatcher()
{
}

void EventDispatcher::AddEvent(int32 eventType, const Message& msg)
{
    Event ev;
    ev.msg = msg;
    ev.eventType = eventType;
    events.push_back(ev);
    ++eventsCount;
}

bool EventDispatcher::RemoveEvent(int32 eventType, const Message& msg)
{
    List<Event>::iterator it = events.begin();
    for (; it != events.end(); it++)
    {
        if ((it->msg == msg) && (it->eventType == eventType) && !it->needDelete)
        {
            it->needDelete = true;
            --eventsCount;
            DVASSERT(eventsCount >= 0);
            if (!eraseLocked)
                events.erase(it);
            return true;
        }
    }
    return false;
}

bool EventDispatcher::RemoveAllEvents()
{
    size_t removedEventsCount = 0;
    if (!eraseLocked)
    {
        removedEventsCount = events.size();
        events.clear();
        eventsCount = 0;
    }
    else
    {
        List<Event>::iterator it = events.begin();
        List<Event>::const_iterator end = events.end();
        for (; it != end; ++it)
        {
            if (!(*it).needDelete)
            {
                (*it).needDelete = true;
                --eventsCount;
                ++removedEventsCount;
            }
        }
    }
    DVASSERT(eventsCount == 0);
    return (removedEventsCount != 0);
}

void EventDispatcher::PerformEvent(int32 eventType)
{
    PerformEventWithData(eventType, this, NULL);
}

void EventDispatcher::PerformEvent(int32 eventType, BaseObject* eventParam)
{
    PerformEventWithData(eventType, eventParam, NULL);
}

void EventDispatcher::PerformEventWithData(int32 eventType, void* callerData)
{
    PerformEventWithData(eventType, this, callerData);
}

void EventDispatcher::PerformEventWithData(int32 eventType, BaseObject* eventParam, void* callerData)
{
    if (events.empty())
        return;

    bool needEraseEvents = false;
    bool eraseLockedOldValue = true;
    std::swap(eraseLocked, eraseLockedOldValue); // set eraseLocked in TRUE, remember old value of eraseLocked in eraseLockedOldValue;

    List<Event>::const_iterator it = events.begin();
    List<Event>::const_iterator end = events.end();
    for (; it != end; ++it)
    {
        if ((*it).eventType == eventType && !(*it).needDelete)
        {
            (*it).msg(eventParam, callerData);
        }

        needEraseEvents |= (*it).needDelete;
    }
    std::swap(eraseLocked, eraseLockedOldValue); // restore old value for eraseLocked

    if (!eraseLocked && needEraseEvents)
    {
        events.erase(std::remove_if(events.begin(), events.end(), Event::IsEventToDelete), events.end());
    }
}

void EventDispatcher::CopyDataFrom(EventDispatcher* srcDispatcher)
{
    events.clear();
    List<Event>::iterator it = srcDispatcher->events.begin();
    for (; it != srcDispatcher->events.end(); it++)
    {
        events.push_back(*it);
    }
    eventsCount = srcDispatcher->eventsCount;
}

int32 EventDispatcher::GetEventsCount() const
{
    return eventsCount;
}
}