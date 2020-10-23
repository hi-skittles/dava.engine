#include "Functional/TrackedObject.h"
#include "Functional/Private/SignalBase.h"

namespace DAVA
{
TrackedObject::~TrackedObject()
{
    DisconnectAll();
}

void TrackedObject::DisconnectAll()
{
    auto it = watchers.begin();
    auto end = watchers.end();
    while (it != end)
    {
        SignalBase* watcher = *it;
        it++;
        watcher->OnTrackedObjectDestroyed(this);
    }

    watchers.clear();
}
} // namespace DAVA
