#include "DLCManager/Private/RequestManager.h"
#include "DLCManager/Private/DLCManagerImpl.h"
#include "Debug/DVAssert.h"
#include "Base/BaseTypes.h"
#include "Time/SystemTimer.h"
#include "Debug/ProfilerCPU.h"

namespace DAVA
{
void RequestManager::Start()
{
    if (packManager.IsRequestingEnabled())
    {
        PackRequest* request = Top();
        if (request != nullptr)
        {
            request->Start();
        }
    }
}

void RequestManager::Stop()
{
    PackRequest* request = Top();
    if (request != nullptr)
    {
        request->Stop();
    }
}

void RequestManager::FireStartLoadingWhileInactiveSignals()
{
    DAVA_PROFILER_CPU_SCOPE_CUSTOM(__FUNCTION__, &packManager.profiler);

    if (!requestStartedWhileInactive.empty())
    {
        for (const String& pack : requestStartedWhileInactive)
        {
            PackRequest* r = packManager.FindRequest(pack);
            if (r)
            {
                packManager.GetLog() << "inAct start: " << r->GetRequestedPackName() << std::endl;
                r->StartLoading();
            }
        }
        requestStartedWhileInactive.clear();
    }
}

void RequestManager::FireUpdateWhileInactiveSignals()
{
    DAVA_PROFILER_CPU_SCOPE_CUSTOM(__FUNCTION__, &packManager.profiler);

    if (!requestUpdatedWhileInactive.empty())
    {
        for (const String& pack : requestUpdatedWhileInactive)
        {
            PackRequest* r = packManager.FindRequest(pack);
            if (r)
            {
                packManager.GetLog() << "inAct update: " << r->GetRequestedPackName() << std::endl;
                packManager.requestUpdated.Emit(*r);
            }
        }
        requestUpdatedWhileInactive.clear();
    }
}

void RequestManager::FireStartLoadingSignal(PackRequest& request, bool inBackground)
{
    DAVA_PROFILER_CPU_SCOPE_CUSTOM(__FUNCTION__, &packManager.profiler);
    // if error happened and no space on device, requesting
    // may be already be disabled, so we need check it out
    if (packManager.IsRequestingEnabled())
    {
        if (inBackground)
        {
            const String& packName = request.GetRequestedPackName();
            requestStartedWhileInactive.push_back(packName);
        }
        else
        {
            request.StartLoading();
        }
    }
}

void RequestManager::FireUpdateSignal(PackRequest& request, bool inBackground)
{
    DAVA_PROFILER_CPU_SCOPE_CUSTOM(__FUNCTION__, &packManager.profiler);
    // if error happened and no space on device, requesting
    // may be already be disabled, so we need check it out
    if (packManager.IsRequestingEnabled())
    {
        if (inBackground)
        {
            const String& packName = request.GetRequestedPackName();
            auto it = find(begin(requestUpdatedWhileInactive), end(requestUpdatedWhileInactive), packName);
            // add only once for update signal
            if (it == end(requestUpdatedWhileInactive))
            {
                requestUpdatedWhileInactive.push_back(packName);
            }
        }
        else
        {
            packManager.requestUpdated.Emit(request);
        }
    }
}

void RequestManager::OneUpdateIteration(bool inBackground)
{
    DAVA_PROFILER_CPU_SCOPE_CUSTOM(__FUNCTION__, &packManager.profiler);

    isQueueChanged = false;

    PackRequest* request = Top();
    bool downloadingSizeChanged = request->Update();

    if (request->IsDownloaded())
    {
        isQueueChanged = true;
        if (downloadingSizeChanged == false && request->GetDownloadedSize() == 0)
        {
            // empty pack, so we need inform signal
            FireStartLoadingSignal(*request, inBackground);
        }
        Pop();
        request->Finalize();
        // inform pack just downloaded
        FireUpdateSignal(*request, inBackground);
        if (!Empty())
        {
            PackRequest* next = Top();
            while (next->IsDownloaded())
            {
                Pop();
                next->Finalize();
                // inform pack just downloaded
                FireUpdateSignal(*next, inBackground);
                if (!Empty() && Top()->IsDownloaded())
                {
                    next = Top();
                }
                else
                {
                    break;
                }
            }
        }
    }
    else if (downloadingSizeChanged)
    {
        // inform pack downloading size change
        FireUpdateSignal(*request, inBackground);
    }
}

void RequestManager::Update(bool inBackground)
{
    DAVA_PROFILER_CPU_SCOPE_CUSTOM(__FUNCTION__, &packManager.profiler);

    if (!inBackground)
    {
        FireStartLoadingWhileInactiveSignals();
        FireUpdateWhileInactiveSignals();
    }

    const int64 start = SystemTimer::GetUs();
    const DLCManager::Hints& hints = packManager.GetHints();

    while (!Empty())
    {
        OneUpdateIteration(inBackground);

        int64 timeIter = SystemTimer::GetUs() - start;

        if (timeIter >= hints.limitRequestUpdateIterationUs)
        {
            break;
        }

        if (!IsQueueOrderChangedDuringLastIteration())
        {
            break;
        }
    }
}

bool RequestManager::Empty() const
{
    return requests.empty();
}

size_t RequestManager::GetNumRequests() const
{
    return requests.size();
}

PackRequest* RequestManager::Top() const
{
    if (requests.empty())
    {
        return nullptr;
    }
    return requests.front();
}

PackRequest* RequestManager::Find(const String& requestedPackName) const
{
    auto it = std::find_if(begin(requests), end(requests), [&requestedPackName](const PackRequest* r) -> bool
                           {
                               return r->GetRequestedPackName() == requestedPackName;
                           });
    if (it == end(requests))
    {
        return nullptr;
    }
    return (*it);
}

void RequestManager::Push(PackRequest* request_)
{
    for (PackRequest* r : requests)
    {
        if (r == request_)
        {
            return;
        }
    }

    requests.push_back(request_);

    requestNames.insert(request_->GetRequestedPackName());

    DVASSERT(requests.size() == requestNames.size());
}

void RequestManager::SetPriorityToRequest(PackRequest* request)
{
    DVASSERT(Thread::IsMainThread());
    DVASSERT(request != nullptr);

    PackRequest* prevTop = Top();
    if (prevTop == nullptr)
    {
        return;
    }

    auto it = find(begin(requests), end(requests), request);
    if (it != end(requests))
    {
        // 1. collect all requests that are not subrequests of request
        Vector<PackRequest*> removeFromBeg;
        for (PackRequest* r : requests)
        {
            if (r == request)
            {
                break; // only check requests before
            }
            if (!request->IsSubRequest(r))
            {
                removeFromBeg.push_back(r);
            }
        }
        // 2. remove all NOT sub request from beginning queue
        for (PackRequest* r : removeFromBeg)
        {
            requests.erase(find(begin(requests), end(requests), r));
        }
        // 3. find position after "request"
        it = find(begin(requests), end(requests), request);
        ++it;
        // 4. insert all previously removed request after preserve order
        for (PackRequest* r : removeFromBeg)
        {
            it = requests.insert(it, r);
            ++it;
        }
    }

    PackRequest* newTop = Top();
    if (newTop != prevTop)
    {
        prevTop->Stop();
        newTop->Start();
    }
}

void RequestManager::Pop()
{
    if (!requests.empty())
    {
        auto it = begin(requests);

        packManager.GetLog() << "downloaded: " << (*it)->GetRequestedPackName() << std::endl;

        auto nameIt = requestNames.find((*it)->GetRequestedPackName());

        requestNames.erase(nameIt);
        requests.erase(it);

        DVASSERT(requests.size() == requestNames.size());
    }
}

void RequestManager::Remove(PackRequest* request)
{
    DVASSERT(request != nullptr);

    const auto it = find(begin(requests), end(requests), request);
    if (it != end(requests))
    {
        const auto nameIt = requestNames.find((*it)->GetRequestedPackName());

        requestNames.erase(nameIt);
        requests.erase(it);

        DVASSERT(requests.size() == requestNames.size());
    }
}

void RequestManager::SwapPointers(PackRequest* newPointer, PackRequest* oldInvalidPointer)
{
    DVASSERT(newPointer != nullptr);
    DVASSERT(oldInvalidPointer != nullptr);
    DVASSERT(newPointer != oldInvalidPointer);

    auto it = find(begin(requests), end(requests), oldInvalidPointer);
    DVASSERT(it != end(requests));
    // update old pointer value in 'requests'
    *it = newPointer;

    auto nameIt = requestNames.find(newPointer->GetRequestedPackName());
    DVASSERT(nameIt != end(requestNames));
}

const Vector<PackRequest*>& RequestManager::GetRequests() const
{
    return requests;
}

void RequestManager::Clear()
{
    requests.clear();
    requestNames.clear();
    // use List to preserve the order of incoming events
    requestStartedWhileInactive.clear();
    requestUpdatedWhileInactive.clear();
    isQueueChanged = false;
}

} // end namespace DAVA
