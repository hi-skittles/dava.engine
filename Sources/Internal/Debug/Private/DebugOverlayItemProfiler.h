#pragma once

#include "Debug/DebugOverlayItem.h"
#include "Debug/Private/RingArray.h"
#include "Debug/TraceEvent.h"

namespace DAVA
{
class ProfilerCPU;
class ProfilerGPU;

class DebugOverlayItemProfiler final : public DebugOverlayItem
{
    struct TraceData;

public:
    static const std::size_t TRACE_HISTORY_SIZE = 10;
    static const uint32 MARKER_HISTORY_LENGTH = 120;

    struct MarkerHistory
    {
        struct HistoryInstance
        {
            uint64 accurate = 0;
            float32 filtered = 0.f;
        };

        using HistoryArray = RingArray<HistoryInstance>;

        HistoryArray values = HistoryArray(MARKER_HISTORY_LENGTH);
        uint32 updatesCount = 0;
    };

    DebugOverlayItemProfiler(ProfilerGPU* gpuProfiler_, ProfilerCPU* cpuProfiler_, const char* cpuCounterName_);
    ~DebugOverlayItemProfiler() = default;

    String GetName() const override;
    void Draw() override;

private:
    void Update();
    void ProcessEventsTrace(const Vector<TraceEvent>& events, TraceData* trace);

    void Reset();

    void DrawTraceRects(TraceData& trace, FastName* selectedMarker, const char* id, float32 height);
    bool ShowMarkerHistory(const FastName& marker);

private:
    ProfilerGPU* gpuProfiler = nullptr;
    ProfilerCPU* cpuProfiler = nullptr;
    String cpuCounterName;

    enum eTrace
    {
        TRACE_CPU = 0,
        TRACE_GPU,

        TRACE_COUNT
    };

    struct TraceData
    {
        struct TraceRect
        {
            uint64 start;
            uint64 duration;
            uint32 color;
            int32 depth;
            FastName name;
        };

        struct ListElement
        {
            FastName name;
            uint64 duration;
            bool selected;
        };

        Vector<TraceRect> rects;
        Vector<ListElement> list;
        uint32 frameIndex = 0;
        uint64 minTimestamp = std::numeric_limits<uint64>::max();
        uint64 maxTimestamp = 0;
        uint32 maxMarkerNameLen = 0;
        int32 maxDepth = 0;
    };

    struct FrameTrace
    {
        Vector<TraceEvent> trace;
        uint32 frameIndex;
    };

    UnorderedMap<FastName, MarkerHistory> markersHistory;
    UnorderedMap<FastName, uint32> markersColor;
    Vector<FastName> interestMarkers;

    RingArray<TraceData> tracesData[TRACE_COUNT];

    FastName selectedMarkers[TRACE_COUNT];

    int32 traceHistoryOffset = 9;
    uint32 colorIndex = 0;

    float32 firstTraceHeight = 100.f;

    bool overlayPaused = false;
};
}