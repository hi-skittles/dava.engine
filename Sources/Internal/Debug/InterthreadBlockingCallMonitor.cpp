#include "Debug/InterthreadBlockingCallMonitor.h"

#include "Concurrency/LockGuard.h"

namespace DAVA
{
namespace Debug
{
InterthreadBlockingCallMonitor::InterthreadBlockingCallMonitor() = default;
InterthreadBlockingCallMonitor::~InterthreadBlockingCallMonitor() = default;

bool InterthreadBlockingCallMonitor::BeginBlockingCall(uint64 callerThreadId, uint64 targetThreadId, Vector<uint64>& callChainIfDeadlock)
{
    if (callerThreadId == targetThreadId)
    {
        return false;
    }

    LockGuard<Mutex> lock(mutex);

    DVASSERT(map.find(callerThreadId) == map.end());

    map.emplace(callerThreadId, targetThreadId);

    Vector<uint64> callChain;
    bool deadlock = DetectDeadlock(callerThreadId, callChain);
    if (deadlock)
    {
        callChainIfDeadlock = std::move(callChain);
    }
    return deadlock;
}

void InterthreadBlockingCallMonitor::EndBlockingCall(uint64 callerThreadId, uint64 targetThreadId)
{
    if (callerThreadId == targetThreadId)
    {
        return;
    }

    LockGuard<Mutex> lock(mutex);

    auto it = map.find(callerThreadId);
    DVASSERT(it != map.end());
    DVASSERT(it->second == targetThreadId);
    if (it != map.end() && it->second == targetThreadId)
    {
        map.erase(it);
    }
}

String InterthreadBlockingCallMonitor::CallChainToString(const Vector<uint64>& callChain) const
{
    StringStream ss;
    if (!callChain.empty())
    {
        ss << callChain.front();
        for (size_t i = 1, n = callChain.size(); i < n; ++i)
        {
            ss << " --> " << callChain[i];
        }
    }
    return ss.str();
}

bool InterthreadBlockingCallMonitor::DetectDeadlock(uint64 callerThreadId, Vector<uint64>& callChain)
{
    auto it = map.find(callerThreadId);
    uint64 second = it->second;
    while (it != map.end())
    {
        second = it->second;
        callChain.push_back(it->first);
        if (second == callerThreadId)
        {
            callChain.push_back(callerThreadId);
            return true;
        }
        it = map.find(second);
    }
    callChain.push_back(second);
    return false;
}

} // namespace Debug
} // namespace DAVA
