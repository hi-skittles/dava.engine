#include "Debug/ProfilerCPU.h"
#include "Time/SystemTimer.h"
#include "Concurrency/Thread.h"
#include "Concurrency/LockGuard.h"
#include "Base/AllocatorFactory.h"
#include "Debug/DVAssert.h"
#include "ProfilerRingArray.h"
#include <ostream>

//==============================================================================

namespace DAVA
{

#if PROFILER_CPU_ENABLED
static ProfilerCPU GLOBAL_TIME_PROFILER;
ProfilerCPU* const ProfilerCPU::globalProfiler = &GLOBAL_TIME_PROFILER;
#else
ProfilerCPU* const ProfilerCPU::globalProfiler = nullptr;
#endif

//////////////////////////////////////////////////////////////////////////
//Internal Declaration

struct ProfilerCPU::Counter
{
    uint64 startTime = 0;
    uint64 endTime = 0;
    const char* name = nullptr;
    uint64 threadID = 0;
    uint32 frame = 0;
};

namespace ProfilerCPUDetails
{
struct CounterTreeNode
{
    IMPLEMENT_POOL_ALLOCATOR(CounterTreeNode, 128)

    static CounterTreeNode* BuildTree(ProfilerCPU::CounterArray::const_iterator begin, const ProfilerCPU::CounterArray* array);
    static CounterTreeNode* CopyTree(const CounterTreeNode* node);
    static void MergeTree(CounterTreeNode* root, const CounterTreeNode* node);
    static void DumpTree(const CounterTreeNode* node, std::ostream& stream, bool average);
    static void SafeDeleteTree(CounterTreeNode*& node);

protected:
    CounterTreeNode(CounterTreeNode* _parent, const char* _name, uint64 _time, uint32 _count)
        : counterTime(_time)
        , counterName(_name)
        , parent(_parent)
        , count(_count)
    {
    }

    uint64 counterTime = 0;
    const char* counterName;

    CounterTreeNode* parent;
    Vector<CounterTreeNode*> childs;
    uint32 count; //recursive and duplicate counters
};

bool NameEquals(const char* name1, const char* name2)
{
#ifdef __DAVAENGINE_DEBUG__
    return (strcmp(name1, name2) == 0);
#else
    return name1 == name2;
#endif
}
}

const FastName ProfilerCPU::TRACE_ARG_FRAME("Frame Number");

//////////////////////////////////////////////////////////////////////////

ProfilerCPU::ScopedCounter::ScopedCounter(const char* counterName, ProfilerCPU* _profiler, uint32 frame)
{
    profiler = _profiler;
    if (profiler->isStarted)
    {
        Counter& c = profiler->counters->next();

        endTime = &c.endTime;
        c.startTime = SystemTimer::GetUs();
        c.endTime = 0;
        c.name = counterName;
        c.threadID = Thread::GetCurrentIdAsUInt64();
        c.frame = frame;
    }
}

ProfilerCPU::ScopedCounter::~ScopedCounter()
{
    // We don't write end time if profiler stopped cause
    // in this moment other thread may dump counters.
    // Potentially due to 'pseudo-thread-safe' (see ProfilerRingArray.h)
    // we can get invalid counter (only one, therefore there is 'if(started)' ).
    // We know it. But it performance reason.
    if (profiler->isStarted && endTime != nullptr)
    {
        *endTime = SystemTimer::GetUs();
    }
}

ProfilerCPU::ProfilerCPU(uint32 numCounters_)
    : numCounters(numCounters_)
{
}

ProfilerCPU::~ProfilerCPU()
{
    DeleteSnapshots();
    SafeDelete(counters);
}

void ProfilerCPU::Start()
{
    LockGuard<Mutex> lock(mutex);
    if (isStarted == false)
    {
        if (counters == nullptr)
        {
            counters = new CounterArray(numCounters);
        }
        isStarted = true;
    }
}

void ProfilerCPU::Stop()
{
    LockGuard<Mutex> lock(mutex);
    isStarted = false;
}

bool ProfilerCPU::IsStarted() const
{
    return isStarted;
}

int32 ProfilerCPU::MakeSnapshot()
{
    //CPU profiler use 'pseudo-thread-safe' ring array (see ProfilerRingArray.h)
    //So we can't read array when other thread may write
    //For performance reasons we should stop profiler before dumping or snapshotting
    DVASSERT(!isStarted && "Stop profiler before make snapshot");

    snapshots.push_back(new CounterArray(*counters));
    return int32(snapshots.size() - 1);
}

void ProfilerCPU::DeleteSnapshot(int32 snapshot)
{
    if (snapshot != NO_SNAPSHOT_ID)
    {
        DVASSERT(snapshot >= 0 && snapshot < int32(snapshots.size()));
        SafeDelete(snapshots[snapshot]);
        snapshots.erase(snapshots.begin() + snapshot);
    }
}

void ProfilerCPU::DeleteSnapshots()
{
    for (CounterArray*& c : snapshots)
    {
        SafeDelete(c);
    }
    snapshots.clear();
}

uint64 ProfilerCPU::GetLastCounterTime(const char* counterName) const
{
    uint64 timeDelta = 0;
    CounterArray::reverse_iterator it = counters->rbegin(), itEnd = counters->rend();
    for (; it != itEnd; ++it)
    {
        const Counter& c = *it;
        if (c.endTime != 0 && (strcmp(counterName, c.name) == 0))
        {
            timeDelta = c.endTime - c.startTime;
            break;
        }
    }

    return timeDelta;
}

void ProfilerCPU::DumpLast(const char* counterName, uint32 counterCount, std::ostream& stream, int32 snapshot) const
{
    DVASSERT((snapshot != NO_SNAPSHOT_ID || !isStarted) && "Stop profiler before dumping");

    stream << "================================================================\n";

    const CounterArray* array = GetCounterArray(snapshot);
    CounterArray::const_reverse_iterator it = array->rbegin(), itEnd = array->rend();
    const Counter* lastDumpedCounter = nullptr;
    for (; it != itEnd; ++it)
    {
        if (it->endTime != 0 && (strcmp(counterName, it->name) == 0))
        {
            if (lastDumpedCounter)
            {
                stream << "=== Non-tracked time [" << (lastDumpedCounter->startTime - it->endTime) << " us] ===\n";
            }
            lastDumpedCounter = &(*it);

            ProfilerCPUDetails::CounterTreeNode* treeRoot = ProfilerCPUDetails::CounterTreeNode::BuildTree(CounterArray::const_iterator(it), array);
            ProfilerCPUDetails::CounterTreeNode::DumpTree(treeRoot, stream, false);
            SafeDelete(treeRoot);

            counterCount--;
        }

        if (counterCount == 0)
            break;
    }

    stream << "================================================================\n";
    stream.flush();
}

void ProfilerCPU::DumpAverage(const char* counterName, uint32 counterCount, std::ostream& stream, int32 snapshot) const
{
    using namespace ProfilerCPUDetails;
    DVASSERT((snapshot != NO_SNAPSHOT_ID || !isStarted) && "Stop profiler before dumping");

    stream << "================================================================\n";
    stream << "=== Average time for " << counterCount << " counter(s):\n";

    const CounterArray* array = GetCounterArray(snapshot);
    CounterArray::const_reverse_iterator it = array->rbegin();
    CounterArray::const_reverse_iterator itEnd = array->rend();
    CounterTreeNode* treeRoot = nullptr;
    for (; it != itEnd; ++it)
    {
        if (it->endTime != 0 && (strcmp(counterName, it->name) == 0))
        {
            CounterTreeNode* node = CounterTreeNode::BuildTree(CounterArray::const_iterator(it), array);

            if (treeRoot)
            {
                CounterTreeNode::MergeTree(treeRoot, node);
                CounterTreeNode::SafeDeleteTree(node);
            }
            else
            {
                treeRoot = node;
            }

            counterCount--;
        }

        if (counterCount == 0)
            break;
    }

    if (treeRoot)
    {
        CounterTreeNode::DumpTree(treeRoot, stream, true);
        CounterTreeNode::SafeDeleteTree(treeRoot);
    }

    stream << "================================================================\n";
    stream.flush();
}

Vector<TraceEvent> ProfilerCPU::GetTrace(int32 snapshot) const
{
    DVASSERT((snapshot != NO_SNAPSHOT_ID || !isStarted) && "Stop profiler before tracing");

    const CounterArray* array = GetCounterArray(snapshot);
    Vector<TraceEvent> trace;
    if (array == nullptr)
    {
        return trace;
    }
    trace.reserve(array->size());

    for (const Counter& c : *array)
    {
        if (c.name == nullptr || c.startTime == 0 || c.endTime == 0)
        {
            continue;
        }

        trace.push_back({ FastName(c.name), c.startTime, (c.endTime - c.startTime), c.threadID, 0, TraceEvent::PHASE_DURATION });

        if (c.frame)
        {
            trace.back().args.push_back({ TRACE_ARG_FRAME, c.frame });
        }
    }

    return trace;
}

Vector<TraceEvent> ProfilerCPU::GetTrace(const char* counterName, uint32 desiredFrameIndex, int32 snapshot) const
{
    Vector<TraceEvent> trace;

    bool found = false;
    std::size_t countersCount = 0;
    const CounterArray* array = GetCounterArray(snapshot);
    CounterArray::const_reverse_iterator rit = array->rbegin();
    CounterArray::const_reverse_iterator rend = array->rend();
    for (; rit != rend; ++rit)
    {
        ++countersCount;
        if (rit->endTime != 0 && (strcmp(counterName, rit->name) == 0))
        {
            if ((rit->frame <= desiredFrameIndex || rit->frame == 0 || desiredFrameIndex == 0))
            {
                found = true;
                break;
            }
        }
    }

    if (found)
    {
        CounterArray::const_iterator it(rit);
        CounterArray::const_iterator end = array->end();

        trace.reserve(countersCount);

        uint64 threadID = it->threadID;
        uint64 counterEndTime = it->endTime;
        for (; it != end; ++it)
        {
            if (it->threadID == threadID)
            {
                if (it->endTime == 0 || it->startTime > counterEndTime)
                {
                    break;
                }

                trace.push_back({ FastName(it->name), it->startTime, it->endTime - it->startTime, it->threadID, 0, TraceEvent::PHASE_DURATION });

                if (it->frame)
                {
                    trace.back().args.push_back({ TRACE_ARG_FRAME, it->frame });
                }
            }
        }
    }

    return trace;
}

const ProfilerCPU::CounterArray* ProfilerCPU::GetCounterArray(int32 snapshot) const
{
    if (snapshot != NO_SNAPSHOT_ID)
    {
        DVASSERT(snapshot >= 0 && snapshot < int32(snapshots.size()));
        return snapshots[snapshot];
    }

    return counters;
}

/////////////////////////////////////////////////////////////////////////////////
//Internal Definition
namespace ProfilerCPUDetails
{
CounterTreeNode* CounterTreeNode::BuildTree(ProfilerCPU::CounterArray::const_iterator begin, const ProfilerCPU::CounterArray* array)
{
    DVASSERT(begin->endTime);

    uint64 threadID = begin->threadID;
    uint64 endTime = begin->endTime;
    Vector<uint64> nodeEndTime;

    CounterTreeNode* node = new CounterTreeNode(nullptr, begin->name, begin->endTime - begin->startTime, 1);
    nodeEndTime.push_back(begin->endTime);

    ProfilerCPU::CounterArray::const_iterator end = array->end();
    for (ProfilerCPU::CounterArray::const_iterator it = begin + 1; it != end; ++it)
    {
        const ProfilerCPU::Counter& c = *it;

        if (c.threadID != threadID)
            continue;

        if (c.startTime >= endTime || c.endTime == 0)
            break;

        if (c.threadID == threadID)
        {
            while (!nodeEndTime.empty() && (c.startTime >= nodeEndTime.back()))
            {
                DVASSERT(node->parent);

                nodeEndTime.pop_back();
                node = node->parent;
            }

            if (NameEquals(node->counterName, c.name))
            {
                node->counterTime += c.endTime - c.startTime;
                node->count++;
            }
            else
            {
                auto found = std::find_if(node->childs.begin(), node->childs.end(), [&c](CounterTreeNode* nodeArg) {
                    return (NameEquals(c.name, nodeArg->counterName));
                });

                if (found != node->childs.end())
                {
                    (*found)->counterTime += c.endTime - c.startTime;
                    (*found)->count++;
                }
                else
                {
                    node->childs.push_back(new CounterTreeNode(node, c.name, c.endTime - c.startTime, 1));
                    node = node->childs.back();

                    nodeEndTime.push_back(c.endTime);
                }
            }
        }
    }

    while (node->parent)
        node = node->parent;

    return node;
}

void CounterTreeNode::MergeTree(CounterTreeNode* root, const CounterTreeNode* node)
{
    if (NameEquals(root->counterName, node->counterName))
    {
        root->counterTime += node->counterTime;
        root->count++;

        for (CounterTreeNode* c : node->childs)
            MergeTree(root, c);
    }
    else
    {
        auto found = std::find_if(root->childs.begin(), root->childs.end(), [&node](CounterTreeNode* child) {
            return (NameEquals(child->counterName, node->counterName));
        });

        if (found != root->childs.end())
        {
            (*found)->counterTime += node->counterTime;
            (*found)->count++;

            for (CounterTreeNode* c : node->childs)
                MergeTree(*found, c);
        }
        else
        {
            root->childs.push_back(CopyTree(node));
        }
    }
}

CounterTreeNode* CounterTreeNode::CopyTree(const CounterTreeNode* node)
{
    CounterTreeNode* nodeCopy = new CounterTreeNode(nullptr, node->counterName, node->counterTime, node->count);
    for (CounterTreeNode* c : node->childs)
    {
        nodeCopy->childs.push_back(CopyTree(c));
        nodeCopy->childs.back()->parent = nodeCopy;
    }

    return nodeCopy;
}

void CounterTreeNode::DumpTree(const CounterTreeNode* node, std::ostream& stream, bool average)
{
    if (!node)
        return;

    const CounterTreeNode* c = node;
    while (c->parent)
    {
        c = c->parent;
        stream << "  ";
    }

    stream << node->counterName << " [" << (average ? node->counterTime / node->count : node->counterTime) << " us | x" << node->count << "]" << '\n';

    for (CounterTreeNode* child : node->childs)
    {
        DumpTree(child, stream, average);
    }
}

void CounterTreeNode::SafeDeleteTree(CounterTreeNode*& node)
{
    if (node)
    {
        for (CounterTreeNode* c : node->childs)
        {
            SafeDeleteTree(c);
        }
        node->childs.clear();
        SafeDelete(node);
    }
}

} //end namespace CPUProfilerDetails

} //end namespace DAVA
