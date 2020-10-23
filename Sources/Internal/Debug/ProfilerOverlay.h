#pragma once

#include "Base/BaseTypes.h"
#include "Debug/Private/RingArray.h"
#include "Debug/TraceEvent.h"
#include "Math/Math2D.h"

namespace DAVA
{
class UIEvent;
class ProfilerCPU;
class ProfilerGPU;
/**
    \ingroup profilers
             Overlay display debug information retrieved from `ProfilerCPU` and `ProfilerGPU`. To display overlay just call `SetEnabled`. If overlay disabled - it's almost free.
             Overlay store history from 10 frames. You can select frames after pause overlay-updating.
             Also Overlay store history for 120 values of interest markers. You can modify list of these markers using `ClearInterestMarkers` and `AddInterestMarker(s)`.
             To control overlay you can use keyboard shortcuts or touches.
             To enable touches and keyboard shortcuts you should call `SetInputEnabled`. If input enabled, overlay intercept on input from engine.
             Keyboard shortcuts:
               - Ctrl + F12   -- Enable/Disable overlay. Equals `SetEnabled`
               - Ctrl + F11   -- Pause/Unpause information update.
               - Ctrl + F10   -- Switch scale of overlay (x1/x2). Useful for High-DPI displays. Equals `SetScale`
               - Ctrl + F9    -- Show/Hide charts of interested markers history.
               - Ctrl + Left  -- Select previous frame from history.
               - Ctrl + Right -- Select next frame from history.
               - Ctrl + Up    -- Select previous marker from active (focused) trace.
               - Ctrl + Down  -- Select previous marker from active (focused) trace.
               - Ctrl + Tab   -- Switch focus between CPU and GPU traces.

            You can set to overlay your own `ProfilerCPU` to display info. Call `SetCPUProfiler` and pass to it root counter name. This name will be used to retrieve one counter for one frame.
            Default cpu-profiler is `ProfilerCPU::globalProfiler` with root counter `ENGINE_ON_FRAME`.
*/
class ProfilerOverlay
{
public:
    static ProfilerOverlay* const globalProfilerOverlay; ///< Global Engine Profiler Overlay

    ProfilerOverlay(ProfilerCPU* cpuProfiler, const char* cpuCounterName, ProfilerGPU* gpuProfiler, const Vector<FastName>& interestMarkers = Vector<FastName>());

    /**
        Enable/Disable overlay.
    */
    void SetEnabled(bool enabled);

    /**
        Returns is overlay enabled
    */
    bool IsEnabled();

    /**
        Enable/Disable overlay input
    */
    void SetInputEnabled(bool enabled);

    /**
        Returns is overlay input enabled
    */
    bool IsInputEnabled();

    /**
        Set draw `scale`. Useful for High-DPI displays
    */
    void SetDrawScace(float32 scale);

    /**
        Return draw scale
    */
    float32 GetDrawScale() const;

    /**
        Frame separator. You should call this method once per-frame and before `rhi::Present()`
    */
    void OnFrameEnd();

    /**
        Input events processing. Return `true` if input processed and `false` otherwise
    */
    bool OnInput(UIEvent* input);

    /**
        Change `cpuProfiler` to displaying their counters information. `rootCounterName` is used as top of trace
    */
    void SetCPUProfiler(ProfilerCPU* cpuProfiler, const char* rootCounterName);

    /**
        Clear list of interest markers
    */
    void ClearInterestMarkers();

    /**
        Add `name` to list of interested markers
    */
    void AddInterestMarker(const FastName& name);

    /**
        Add `names` to list of interested markers
    */
    void AddInterestMarkers(const Vector<FastName>& names);

    /**
        Return list of interest markers
    */
    const Vector<FastName>& GetInterestMarkers() const;

    /**
        Return list of available markers
    */
    Vector<FastName> GetAvalibleMarkers() const;

protected:
    static const std::size_t TRACE_HISTORY_SIZE = 10;
    static const uint32 MARKER_HISTORY_LENGTH = 120;

    enum eTrace
    {
        TRACE_CPU = 0,
        TRACE_GPU,

        TRACE_COUNT
    };

    struct MarkerHistory
    {
        struct HistoryInstance
        {
            uint64 accurate = 0;
            float32 filtered = 0.f;
        };
        using HistoryArray = RingArray<HistoryInstance>;

        HistoryArray values = HistoryArray(MARKER_HISTORY_LENGTH);
        uint32 updatesCount = 0U;
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
        };

        Vector<TraceRect> rects;
        Vector<ListElement> list;
        uint32 frameIndex = 0;
        uint64 minTimestamp = uint64(-1);
        uint64 maxTimestamp = 0;
        uint32 maxMarkerNameLen = 0;
    };

    struct FrameTrace
    {
        Vector<TraceEvent> trace;
        uint32 frameIndex;
    };

    enum eButton
    {
        BUTTON_CPU_UP = 0,
        BUTTON_CPU_DOWN,
        BUTTON_GPU_UP,
        BUTTON_GPU_DOWN,
        BUTTON_HISTORY_PREV,
        BUTTON_HISTORY_NEXT,
        BUTTON_DRAW_MARKER_HISTORY,
        BUTTON_SCALE,
        BUTTON_PROFILERS_START_STOP,
        BUTTON_CLOSE,

        BUTTON_COUNT
    };

    void Update();
    void ProcessEventsTrace(const Vector<TraceEvent>& events, TraceData* trace);

    void Draw();
    void DrawTrace(const TraceData& trace, const char* traceHeader, const Rect2i& rect, const FastName& selectedMarker, bool traceSelected, Rect2i* upButton, Rect2i* downButton);
    void DrawHistory(const FastName& name, const Rect2i& rect, bool drawBackground = true);

    int32 GetEnoughRectHeight(const TraceData& trace);
    int32 FindListIndex(const Vector<TraceData::ListElement>& legend, const FastName& marker);
    TraceData& GetHistoricTrace(RingArray<TraceData>& traceData);

    void Reset();

    void ProcessTouch(UIEvent* input);
    void OnButtonPressed(eButton button);

    void SetPaused(bool paused);
    bool IsPaused() const;

    //selection control
    void SelectNextMarker();
    void SelectPreviousMarker();
    void SelectMarker(const FastName& name);

    void SelectTrace(eTrace trace);
    eTrace GetSelectedTrace();

    //trace history control
    void SetTraceHistoryOffset(uint32 offset);
    uint32 GetTraceHistoryOffset() const;

    UnorderedMap<FastName, MarkerHistory> markersHistory = UnorderedMap<FastName, MarkerHistory>(128);
    UnorderedMap<FastName, uint32> markersColor;
    Vector<FastName> interestMarkers;

    RingArray<TraceData> tracesData[TRACE_COUNT];
    eTrace selectedTrace = TRACE_CPU;

    FastName selectedMarkers[TRACE_COUNT];

    ProfilerGPU* gpuProfiler = nullptr;
    ProfilerCPU* cpuProfiler = nullptr;
    const char* cpuCounterName = nullptr;

    uint32 traceHistoryOffset = 0;
    uint32 colorIndex = 0;
    float32 overlayScale = 1.f;

    Rect2i buttons[BUTTON_COUNT];
    const char* buttonsText[BUTTON_COUNT];

    bool overlayEnabled = false;
    bool overlayPaused = true;
    bool inputEnabled = false;
    bool drawMarkerHistory = false;
};

inline void ProfilerOverlay::SetEnabled(bool enabled)
{
    overlayEnabled = enabled;
}

inline bool ProfilerOverlay::IsEnabled()
{
    return overlayEnabled;
}

inline void ProfilerOverlay::SetInputEnabled(bool enabled)
{
    inputEnabled = enabled;
}

inline bool ProfilerOverlay::IsInputEnabled()
{
    return inputEnabled;
}

inline void ProfilerOverlay::SetDrawScace(float32 scale)
{
    overlayScale = scale;
}

inline float32 ProfilerOverlay::GetDrawScale() const
{
    return overlayScale;
}

inline void ProfilerOverlay::SelectTrace(eTrace trace)
{
    selectedTrace = trace;
}

inline ProfilerOverlay::eTrace ProfilerOverlay::GetSelectedTrace()
{
    return selectedTrace;
}

inline void ProfilerOverlay::SetTraceHistoryOffset(uint32 offset)
{
    traceHistoryOffset = Clamp(offset, 0U, uint32(TRACE_HISTORY_SIZE) - 1);
}

inline uint32 ProfilerOverlay::GetTraceHistoryOffset() const
{
    return traceHistoryOffset;
}
}
