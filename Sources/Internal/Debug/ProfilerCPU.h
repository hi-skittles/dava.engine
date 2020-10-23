#pragma once

#include "Base/BaseTypes.h"
#include "Debug/TraceEvent.h"
#include "Concurrency/Mutex.h"
#include <iosfwd>

#ifndef PROFILER_CPU_ENABLED
#define PROFILER_CPU_ENABLED 1
#endif

namespace DAVA
{
template <class T>
class ProfilerRingArray;

/**
    \ingroup profilers
             Profiler allows to measure execution time of code-blocks from any thread. It's cheap in performance. If profiler not started - it's almost free.
             To use this profiler, at first, you have to place counters in interesting code blocks using set of DAVA_PROFILER_CPU_SCOPE defines listed below.
             Than you just start profiler. After that you can dump counted info or build trace to view it in Chromium Trace Viewer.

             Any counter has string-name that must be passed to define and will be displayed in dump or trace. Time-measuring occurs in microseconds.

             Profiler is using ring array for counters so you are limited by count passed to ctor. If it's necessary to store counters data for later usage you can use snapshots.
             Snapshot - it just a copy of internal ring buffer. To make snapshot you have to stop profiler because it can be used by other thread.
             After snapshot was made you can dump counted info or build JSON-trace from it. Remember, that dumping or building trace is more expensive in performance than making snapshot.

             Engine has own global profiler. You can access it through static field `ProfilerCPU::globalProfiler`.
             Some predefined counters are placed all over the engine. Predefined counters names are listed in `ProfilerCPUMarkerName` namespace (ProfilerMarkerNames.h).
             You can add counters to global engine profiler or you can create own profiler and use it separately.

             The following define are used to place counters:
              - DAVA_PROFILER_CPU_SCOPE(name)                                           -- Add counter with to global engine profiler.
              - DAVA_PROFILER_CPU_SCOPE_WITH_FRAME_INDEX(name, index)                   -- Mark counter by frame index and add to global engine profiler.
              - DAVA_PROFILER_CPU_SCOPE_CUSTOM(name, profiler)                          -- Add counter with to custom profiler.
              - DAVA_PROFILER_CPU_SCOPE_CUSTOM_WITH_FRAME_INDEX(name, profiler, index)  -- Mark counter by frame index and add to custom profiler.

             Defines *_WITH_FRAME_INDEX mark counters by frame index. Frame index can be viewed later in TraceEvent args. Name of argument is `TRACE_ARG_FRAME`.
             For more information about trace events arguments see `TraceEvent`.

             Dump and trace considers hierarchy of counters. It's means that if counter A started after counter B and completed before B is completed (counters A and B placed in the same thread),
             in this case counter A became child of B. And dump of any counter include all child counters. So, dump represents as tree like call-tree. For example:
               \code
                 ================================================================
                 Counter0 [120 us | x1]
                   Counter1 [60 us | x1]
                     Counter4 [10 us | x4]
                   Counter2 [15 us | x3]
                   Counter3 [15 us | x1]
                 ================================================================
               \endcode

			 Dump everything using:
			   \code
			   std::ofstream file("tmp.json");
			   if (file)
			   {
			       profiler.Stop();
			       Vector<TraceEvent> events = profiler.GetTrace();
			       TraceEvent::DumpJSON(events, file);
			   }
			   \endcode
*/
class ProfilerCPU
{
public:
    static const FastName TRACE_ARG_FRAME; ///< Name of frame index argument of generated TraceEvent

    struct Counter;
    using CounterArray = ProfilerRingArray<Counter>;

    /**
        Scoped counter to measure executing time of code block.
        Use DAVA_PROFILER_CPU_SCOPE defines instead of manual object creation.
    */
    class ScopedCounter
    {
    public:
        ScopedCounter(const char* counterName, ProfilerCPU* profiler, uint32 frame = 0U);
        ~ScopedCounter();

    private:
        uint64* endTime = nullptr;
        ProfilerCPU* profiler;
    };

    static const int32 NO_SNAPSHOT_ID = -1; ///< Value used to dump or build trace from current counters array
    static ProfilerCPU* const globalProfiler; ///< Global Engine Profiler

    ProfilerCPU(uint32 numCounters = 2048);
    ~ProfilerCPU();

    /**
        Start time measuring
    */
    void Start();

    /**
        Stop time measuring
    */
    void Stop();

    /**
        Returns is time measuring started
    */
    bool IsStarted() const;

    /**
        Looking by name last complete counter with `counterName` and return it duration in microseconds
    */
    uint64 GetLastCounterTime(const char* counterName) const;

    /**
        Make snapshot and return their ID. Snapshot is just a copy of counters array.
        You should stop profiler before making snapshot
    */
    int32 MakeSnapshot();

    /**
        Delete snapshot by `snapshotID`
    */
    void DeleteSnapshot(int32 snapshotID);

    /**
        Delete all created snapshots
    */
    void DeleteSnapshots();

    /**
        Looking for a certain `counterCount` of last completed counters by `counterName` and dump it to `stream`.
        You can dump output from snapshot or from current counters array counters.
        In first case you have to pass `snapshotID`. In the second case you should stop profiler
    */
    void DumpLast(const char* counterName, uint32 counterCount, std::ostream& stream, int32 snapshotID = NO_SNAPSHOT_ID) const;

    /**
        Looking for a certain `counterCount` of last completed counters by `counterName` and dump it average durations to `stream` considering hierarchy.
        You can dump output from snapshot or from current counters array counters.
        In first case you have to pass `snapshotID`. In the second case you should stop profiler
    */
    void DumpAverage(const char* counterName, uint32 counterCount, std::ostream& stream, int32 snapshot = NO_SNAPSHOT_ID) const;

    /**
        Build and return trace of all available counters from snapshot with `snapshotID` or internal counters array.
        Trace can be dumped to JSON Chromium Trace Viewer format
    */
    Vector<TraceEvent> GetTrace(int32 snapshotID = NO_SNAPSHOT_ID) const;

    /**
        Looking last completed counter by `counterName` and `desiredFrameIndex`. Frame index is desired so it will used counter marked with <= index for build trace.
        If pass frame index equals 0, search-criteria by index will be ignored.
        To get trace from snapshot pass `snapshotID` and `NO_SNAPSHOT_ID` otherwise.

        Gotten trace can be dumped to JSON Chromium Trace Viewer format
    */
    Vector<TraceEvent> GetTrace(const char* counterName, uint32 desiredFrameIndex = 0, int32 snapshotID = NO_SNAPSHOT_ID) const;

private:
    const CounterArray* GetCounterArray(int32 snapshot) const;

    CounterArray* counters = nullptr;
    Vector<CounterArray*> snapshots;
    Mutex mutex;
    uint32 numCounters = 2048;
    bool isStarted = false;

    friend class ScopedCounter;
};

} //ns DAVA

#if PROFILER_CPU_ENABLED

#define DAVA_PROFILER_CPU_SCOPE(counter_name) DAVA::ProfilerCPU::ScopedCounter time_profiler_scope_counter(counter_name, DAVA::ProfilerCPU::globalProfiler);
#define DAVA_PROFILER_CPU_SCOPE_WITH_FRAME_INDEX(counter_name, index) DAVA::ProfilerCPU::ScopedCounter time_profiler_scope_counter(counter_name, DAVA::ProfilerCPU::globalProfiler, index);

#define DAVA_PROFILER_CPU_SCOPE_CUSTOM(counter_name, profiler) DAVA::ProfilerCPU::ScopedCounter time_profiler_scope_counter_custom(counter_name, profiler);
#define DAVA_PROFILER_CPU_SCOPE_CUSTOM_WITH_FRAME_INDEX(counter_name, profiler, index) DAVA::ProfilerCPU::ScopedCounter time_profiler_scope_counter_custom(counter_name, profiler, index);

#else

#define DAVA_PROFILER_CPU_SCOPE(counter_name)
#define DAVA_PROFILER_CPU_SCOPE_WITH_FRAME_INDEX(counter_name, index)

#define DAVA_PROFILER_CPU_SCOPE_CUSTOM(counter_name, profiler)
#define DAVA_PROFILER_CPU_SCOPE_CUSTOM_WITH_FRAME_INDEX(counter_name, profiler, index)

#endif