#pragma once

#include "Base/Set.h"

namespace DAVA
{
struct SignalBase;

/**
    \ingroup functional
    TrackedObject is class used for Signal automatic connection management.
*/
struct TrackedObject
{
    virtual ~TrackedObject();
    void DisconnectAll();

private:
    friend struct SignalBase;
    Set<SignalBase*> watchers;
};

} // namespace DAVA
