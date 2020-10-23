#pragma once

#include "Base/Token.h"

namespace DAVA
{
struct SignalBase
{
public:
    virtual ~SignalBase() = default;

    void Watch(TrackedObject* object)
    {
        object->watchers.insert(this);
    }

    void Unwatch(TrackedObject* object)
    {
        object->watchers.erase(this);
    }

    virtual void Disconnect(Token token) = 0;
    virtual void Track(Token token, TrackedObject* tracked) = 0;
    virtual void OnTrackedObjectDestroyed(TrackedObject* object) = 0;
};
} // namespace DAVA
