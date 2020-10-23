#pragma once

#include "Base/BaseTypes.h"
#include "Base/StaticSingleton.h"

#include "Concurrency/Mutex.h"

namespace DAVA
{
namespace Debug
{
// Class InterthreadBlockingCallMonitor helps in detection deadlocks when performing interthread blocking calls
class InterthreadBlockingCallMonitor final : public StaticSingleton<InterthreadBlockingCallMonitor>
{
public:
    InterthreadBlockingCallMonitor();
    ~InterthreadBlockingCallMonitor();

    bool BeginBlockingCall(uint64 callerThreadId, uint64 targetThreadId, Vector<uint64>& callChainIfDeadlock);
    void EndBlockingCall(uint64 callerThreadId, uint64 targetThreadId);

    String CallChainToString(const Vector<uint64>& callChain) const;

private:
    bool DetectDeadlock(uint64 callerThreadId, Vector<uint64>& callChain);

private:
    Mutex mutex;
    Map<uint64, uint64> map; // key is caller thread id, value is target thread id
};

} // namespace Debug
} // namespace DAVA
