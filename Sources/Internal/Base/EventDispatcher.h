#ifndef __DAVAENGINE_EVENTDISPATCHER_H__
#define __DAVAENGINE_EVENTDISPATCHER_H__

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "Base/Message.h"

namespace DAVA
{
/**
	\ingroup baseobjects
	\brief this class is a list of messages mapped to event ids
 */
class EventDispatcher : public BaseObject
{
protected:
    virtual ~EventDispatcher();

public:
    EventDispatcher();

    /**
		\brief Function to add event to event dispatcher.
		\param[in] eventId	event id we will use to perform messages
		\param[in] msg		message we assign for this eventId
	 */
    void AddEvent(int32 eventId, const Message& msg);
    /**
		\brief Function to remove event from event dispatcher.
		\param[in] eventId	event id 
		\param[in] msg		message we want to delete for given eventId
		\returns true if we removed something, false if not
	 */
    bool RemoveEvent(int32 eventId, const Message& msg);

    /**
	 \brief Function to remove all events from event dispatcher.
	 \returns true if we removed something, false if not
	 */
    bool RemoveAllEvents();

    /**
		\brief Function to perform all events with given id from this event dispatcher.
	 
		When this function called, it perform all messages linked to given eventId.
		It use this pointer for caller parameter of Message. caller variable is used when you want to 
		perform event but with different caller.

		\param[in] eventId	event id 
		\param[in] eventParam this param is used when we want to replace caller in message with another class 
	 */
    void PerformEvent(int32 eventId);
    void PerformEvent(int32 eventId, BaseObject* caller);

    /**
	 \brief Function to perform all events with given id from this event dispatcher.
	 
	 When this function called, it perform all messages linked to given eventId.
	 It use this pointer for caller parameter of Message. caller variable is used when you want to 
	 perform event but with different caller.
	 
	 \param[in] eventId	event id 
	 \param[in] eventParam this param is used when we want to replace caller in message with another class 
	 \param[in] callerData this param is used when we want to send some data from the caller
	 */
    void PerformEventWithData(int32 eventId, void* callerData);
    void PerformEventWithData(int32 eventId, BaseObject* caller, void* callerData);

    /**
		\brief Clone dispatcher make 100% copy of this dispatcher with reference count equal to 1
		\returns new EventDispatcher that contains the same data as first one
	 */
    EventDispatcher* CloneDispatcher();

    /**
		\brief This function copy all date from another dispatcher to this dispatcher
	 */
    void CopyDataFrom(EventDispatcher* srcDispatcher);

    int32 GetEventsCount() const;

protected:
    class Event
    {
    public:
        Event()
            : eventType(0)
            , needDelete(false)
        {
        }
        static bool IsEventToDelete(const Event& event)
        {
            return event.needDelete;
        }

        int32 eventType : 31;
        bool needDelete : 1;
        Message msg;
    };

    List<Event> events;
    bool eraseLocked = false;
    int32 eventsCount = 0; // actual events count
};

/**
	\ingroup baseobjects
	\brief	Helper to implement event dispatchers.
 
	In some cases we want to avoid inheritance from multiple objects.
	But some classes require to be both singletons & handle events.
	For these cases we've added this define to avoid multiple copying of 
	the same code.
 */
#define IMPLEMENT_EVENT_DISPATCHER(eventDispatcherName)	\
public:\
	void AddEvent(int32 eventType, const Message& msg){eventDispatcherName->AddEvent(eventType, msg); }; \
	bool RemoveEvent(int32 eventType, const Message& msg){return eventDispatcherName->RemoveEvent(eventType, msg); };\
	bool RemoveAllEvents(){return eventDispatcherName->RemoveAllEvents(); };\
    int32 GetEventsCount(){return eventDispatcherName->GetEventsCount(); };\
protected:\
	RefPtr<EventDispatcher> eventDispatcherName;
};

#endif