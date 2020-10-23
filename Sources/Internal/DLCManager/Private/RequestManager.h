#pragma once

#include "Base/String.h"
#include "Base/Vector.h"
#include "Base/UnordererSet.h"
#include "Base/List.h"

namespace DAVA
{
class DLCManagerImpl;
class PackRequest;

class RequestManager
{
public:
    explicit RequestManager(DLCManagerImpl& packManager_)
        : packManager(packManager_)
    {
    }

    void Start();
    void Stop();
    void Update(bool inBackground);
    bool Empty() const;
    size_t GetNumRequests() const;
    bool IsInQueue(const String& packName) const;
    PackRequest* Find(const String& requestedPackName) const;
    PackRequest* Top() const;
    void Push(PackRequest*);
    void Pop();
    void SetPriorityToRequest(PackRequest* request);
    void Remove(PackRequest* request);
    void SwapPointers(PackRequest* newPointer, PackRequest* oldInvalidPointer);
    const Vector<PackRequest*>& GetRequests() const;
    void Clear();

private:
    void FireStartLoadingSignal(PackRequest& request, bool inBackground);
    void FireUpdateSignal(PackRequest& request, bool inBackground);
    void OneUpdateIteration(bool inBackground);
    void FireStartLoadingWhileInactiveSignals();
    void FireUpdateWhileInactiveSignals();
    bool IsQueueOrderChangedDuringLastIteration() const
    {
        return isQueueChanged;
    }

    DLCManagerImpl& packManager;
    Vector<PackRequest*> requests;
    // optimization to get to know for constant time if request in RequestManager
    UnorderedSet<String> requestNames;
    // use List to preserve the order of incoming events
    List<String> requestStartedWhileInactive;
    List<String> requestUpdatedWhileInactive;
    bool isQueueChanged = false;
};

inline bool RequestManager::IsInQueue(const String& packName) const
{
    return requestNames.find(packName) != end(requestNames);
}

} // end namespace DAVA
