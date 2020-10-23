#include "Debug/ProfilerGPU.h"
#include "Debug/ProfilerMarkerNames.h"
#include "Debug/DVAssert.h"
#include "Render/Renderer.h"
#include "Render/RHI/dbg_Draw.h"
#include "Engine/Engine.h"
#include <ostream>

//==============================================================================

namespace DAVA
{

#if PROFILER_GPU_ENABLED
struct ProfilerGPUDetails
{
    static ProfilerGPU GLOBAL_GPU_PROFILER;
};
ProfilerGPU ProfilerGPUDetails::GLOBAL_GPU_PROFILER;
ProfilerGPU* const ProfilerGPU::globalProfiler = &ProfilerGPUDetails::GLOBAL_GPU_PROFILER;
#else
ProfilerGPU* const ProfilerGPU::globalProfiler = nullptr;
#endif

const FastName ProfilerGPU::TRACE_ARG_FRAME("Frame Number");

bool ProfilerGPU::PerfQueryPair::IsReady()
{
    return (!query[0].IsValid() || rhi::PerfQueryIsReady(query[0])) && (!query[1].IsValid() || rhi::PerfQueryIsReady(query[1]));
}

void ProfilerGPU::PerfQueryPair::GetTimestamps(uint64& t0, uint64& t1)
{
    t0 = query[0].IsValid() ? rhi::PerfQueryTimeStamp(query[0]) : 0;
    t1 = query[1].IsValid() ? rhi::PerfQueryTimeStamp(query[1]) : 0;
}

Vector<TraceEvent> ProfilerGPU::FrameInfo::GetTrace() const
{
    Vector<const ProfilerGPU::MarkerInfo*> sortedMarkers;
    sortedMarkers.reserve(markers.size());
    for (const ProfilerGPU::MarkerInfo& m : markers)
        sortedMarkers.push_back(&m);

    std::stable_sort(sortedMarkers.begin(), sortedMarkers.end(), [](const ProfilerGPU::MarkerInfo* l, const ProfilerGPU::MarkerInfo* r) {
        return l->startTime < r->startTime;
    });

    Vector<TraceEvent> trace;
    trace.reserve(sortedMarkers.size() + 1);

    if (startTime && endTime)
        trace.push_back({ FastName(ProfilerGPUMarkerName::GPU_FRAME), startTime, endTime - startTime, 0, 0, TraceEvent::PHASE_DURATION, { { TRACE_ARG_FRAME, frameIndex } } });

    for (const MarkerInfo* m : sortedMarkers)
        trace.push_back({ FastName(m->name), m->startTime, m->endTime - m->startTime, 0, 0, TraceEvent::PHASE_DURATION });

    return trace;
}

void ProfilerGPU::Frame::Reset()
{
    perfQuery = PerfQueryPair();

    readyMarkers.clear();
    pendingMarkers.clear();

    globalFrameIndex = 0;
}

ProfilerGPU::ProfilerGPU(uint32 framesInfoCount)
    : framesInfo(RingArray<FrameInfo>(framesInfoCount))
{
    DVASSERT(framesInfoCount && "Should be > 0");
}

const ProfilerGPU::FrameInfo& ProfilerGPU::GetFrame(uint32 index) const
{
    DVASSERT(index < uint32(framesInfo.size()));
    return *(framesInfo.crbegin() - index);
}

uint32 ProfilerGPU::GetFramesCount() const
{
    return uint32(framesInfo.size());
}

void ProfilerGPU::CheckPendingFrames()
{
    List<Frame>::iterator fit = pendingFrames.begin();
    while (fit != pendingFrames.end())
    {
        Frame& frame = *fit;
        List<Marker>::iterator mit = frame.pendingMarkers.begin();
        while (mit != frame.pendingMarkers.end())
        {
            Marker& marker = *mit;
            if (marker.perfQuery.IsReady())
            {
                frame.readyMarkers.push_back(marker);
                mit = frame.pendingMarkers.erase(mit);
            }
            else
            {
                ++mit;
            }
        }

        if (frame.pendingMarkers.empty() && frame.perfQuery.IsReady())
        {
            FrameInfo frameInfo;
            frameInfo.frameIndex = frame.globalFrameIndex;
            frame.perfQuery.GetTimestamps(frameInfo.startTime, frameInfo.endTime);

            bool isFrameReliable = (frameInfo.startTime != rhi::NonreliableQueryValue) && (frameInfo.endTime != rhi::NonreliableQueryValue);
            for (Marker& marker : frame.readyMarkers)
            {
                if (isFrameReliable)
                {
                    uint64 t0, t1;
                    marker.perfQuery.GetTimestamps(t0, t1);

                    isFrameReliable = ((t0 != rhi::NonreliableQueryValue) && (t1 != rhi::NonreliableQueryValue));
                    if (isFrameReliable)
                    {
                        frameInfo.markers.push_back({ marker.name, t0, t1 });
                    }
                }

                ResetPerfQueryPair(marker.perfQuery);
            }
            frame.readyMarkers.clear();

            if (isFrameReliable && profilerStarted)
            {
                framesInfo.next() = frameInfo;
            }

            ResetPerfQueryPair(frame.perfQuery);

            fit = pendingFrames.erase(fit);
        }
        else
        {
            ++fit;
        }
    }
}
void ProfilerGPU::OnFrameEnd()
{
    CheckPendingFrames();

    if (profilerStarted)
    {
        currentFrame.globalFrameIndex = Engine::Instance()->GetGlobalFrameIndex();
        currentFrame.perfQuery = GetPerfQueryPair();
        rhi::SetFramePerfQueries(currentFrame.perfQuery.query[0], currentFrame.perfQuery.query[1]);

        pendingFrames.push_back(currentFrame);
        currentFrame.Reset();
    }
}

Vector<TraceEvent> ProfilerGPU::GetTrace()
{
    Vector<TraceEvent> trace, frameTrace;

    for (const FrameInfo& frameInfo : framesInfo)
    {
        if (frameInfo.frameIndex == 0)
            continue;

        frameTrace = frameInfo.GetTrace();
        trace.insert(trace.end(), frameTrace.begin(), frameTrace.end());
    }

    return trace;
}

void ProfilerGPU::AddMarker(rhi::HPerfQuery* query0, rhi::HPerfQuery* query1, const char* markerName)
{
    if (profilerStarted)
    {
        PerfQueryPair p = GetPerfQueryPair();
        currentFrame.pendingMarkers.push_back({ markerName, p });

        *query0 = p.query[0];
        *query1 = p.query[1];
    }
    else
    {
        *query0 = rhi::HPerfQuery();
        *query1 = rhi::HPerfQuery();
    }
}

void ProfilerGPU::Start()
{
    profilerStarted = true;
}

void ProfilerGPU::Stop()
{
    profilerStarted = false;
}

bool ProfilerGPU::IsStarted()
{
    return profilerStarted;
}

ProfilerGPU::PerfQueryPair ProfilerGPU::GetPerfQueryPair()
{
    PerfQueryPair p;

    if (rhi::DeviceCaps().isPerfQuerySupported)
    {
        for (rhi::HPerfQuery& q : p.query)
        {
            if (!queryPool.empty())
            {
                q = queryPool.back();
                queryPool.pop_back();
            }
            else
            {
                q = rhi::CreatePerfQuery();
            }
        }
    }

    return p;
}

void ProfilerGPU::ResetPerfQueryPair(const ProfilerGPU::PerfQueryPair& perfQuery)
{
    for (rhi::HPerfQuery q : perfQuery.query)
    {
        if (q.IsValid())
        {
            rhi::ResetPerfQuery(q);
            queryPool.push_back(q);
        }
    }
}
}
