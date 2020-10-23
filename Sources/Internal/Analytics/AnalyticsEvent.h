#pragma once

#include "Base/Any.h"
#include "Base/BaseTypes.h"
#include "Time/DateTime.h"

namespace DAVA
{
namespace Analytics
{
/**
AnalyticsEvent represents information about analytics event.
Event has name, timestamp and any count of additional fields.
*/
struct AnalyticsEvent
{
    /** Construct a AnalyticsEvent. Event name can be non-unique. */
    AnalyticsEvent(const String& eventName)
        : name(eventName)
        , timestamp(DateTime::Now())
    {
    }

    /** Get additional field. Return pointer to the field if field exists, nullptr otherwise. */
    const Any* GetField(const String& field) const
    {
        auto iter = fields.find(field);
        return iter != fields.end() ? &iter->second : nullptr;
    }

    /** Event name. */
    String name;

    /** Event timestamp. */
    DateTime timestamp;

    /** Additional fields. */
    UnorderedMap<String, Any> fields;
};

} // namespace Analytics
} // namespace DAVA
