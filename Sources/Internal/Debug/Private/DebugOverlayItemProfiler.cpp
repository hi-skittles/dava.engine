#include "Debug/Private/DebugOverlayItemProfiler.h"
#include "Debug/DebugOverlay.h"
#include "Debug/Private/ImGui.h"
#include "Debug/DebugColors.h"
#include "Debug/ProfilerGPU.h"
#include "Debug/ProfilerCPU.h"
#include "Engine/Engine.h"
#include "Render/RHI/dbg_Draw.h"

namespace DAVA
{
namespace DebugOverlayItemProfilerDetails
{
static const char* OVERLAY_MARKER_CPU_TIME = "OverlayCPUTime";
static const char* OVERLAY_PASS_MARKER_NAME = "OverlayRenderPass";

static const int32 MARKER_HISTORY_CHART_HEIGHT = 60;
static const uint32 MARKER_HISTORY_NON_FILTERED_COUNT = 10;

static const int32 OVERLAY_RECT_MARGIN = 3;
static const int32 OVERLAY_RECT_PADDING = 4;
static const int32 TRACE_LIST_ICON_SIZE = DbgDraw::NormalCharH;
static const int32 TRACE_RECT_HEIGHT = DbgDraw::NormalCharH;
static const int32 TRACE_ARROW_HEIGHT = 18;
static const int32 TRACE_LIST_DURATION_TEXT_WIDTH_CHARS = 12;
static const int32 MIN_HIGHLIGHTED_TRACE_RECT_SIZE = 10;

static const int32 HISTORY_CHART_TEXT_COLUMN_CHARS = 9;
static const int32 HISTORY_CHART_TEXT_COLUMN_WIDTH = DbgDraw::NormalCharW * HISTORY_CHART_TEXT_COLUMN_CHARS;
static const uint64 HISTORY_CHART_CEIL_STEP = 500; //mcs

static const uint32 MAX_CPU_FRAME_TRACES = 6;
static const uint32 MAX_TRACE_LIST_ELEMENTS_TO_DRAW = 7;

float32 HistoryGetter(void* data, int index)
{
    DebugOverlayItemProfiler::MarkerHistory* history = reinterpret_cast<DebugOverlayItemProfiler::MarkerHistory*>(data);
    return static_cast<float32>((history->values.cbegin() + index)->accurate);
}
};

DebugOverlayItemProfiler::DebugOverlayItemProfiler(ProfilerGPU* gpuProfiler_, ProfilerCPU* cpuProfiler_, const char* cpuCounterName_)
    : gpuProfiler(gpuProfiler_)
    , cpuProfiler(cpuProfiler_)
    , cpuCounterName(cpuCounterName_)
{
    for (RingArray<TraceData>& t : tracesData)
    {
        t = RingArray<TraceData>(TRACE_HISTORY_SIZE);
    }
}

String DebugOverlayItemProfiler::GetName() const
{
    return "Profiler";
}

void DebugOverlayItemProfiler::Draw()
{
    bool shown = true;
    ImGui::SetNextWindowSizeConstraints(ImVec2(420.0f, 400.0f), ImVec2(FLOAT_MAX, FLOAT_MAX));

    if (ImGui::Begin("ProfilerWindow", &shown, ImGuiWindowFlags_NoFocusOnAppearing))
    {
        Update();

        TraceData& currentCPUTrace = *(tracesData[TRACE_CPU].begin() + traceHistoryOffset);
        TraceData& currentGPUTrace = *(tracesData[TRACE_GPU].begin() + traceHistoryOffset);

        bool isCpuProfilerStarted = cpuProfiler != nullptr && cpuProfiler->IsStarted();
        bool isGpuProfilerStarted = gpuProfiler != nullptr && gpuProfiler->IsStarted();

        ImGui::Checkbox("Paused", &overlayPaused);
        ImGui::SameLine();
        ImGui::Checkbox("CPU Profiler", &isCpuProfilerStarted);
        ImGui::SameLine();
        ImGui::Checkbox("GPU Profiler", &isGpuProfilerStarted);
        ImGui::SameLine();
        if (ImGui::Button("Reset"))
        {
            Reset();
        }

        if (cpuProfiler != nullptr && isCpuProfilerStarted != cpuProfiler->IsStarted())
        {
            isCpuProfilerStarted ? cpuProfiler->Start() : cpuProfiler->Stop();
        }

        if (gpuProfiler != nullptr && isGpuProfilerStarted != gpuProfiler->IsStarted())
        {
            isGpuProfilerStarted ? gpuProfiler->Start() : gpuProfiler->Stop();
        }

        ImGui::SliderInt("Frame History", &traceHistoryOffset, 0, TRACE_HISTORY_SIZE - 1);

        ImGui::Text("GPU Frame #%d", currentGPUTrace.frameIndex);
        DrawTraceRects(currentGPUTrace, &selectedMarkers[TRACE_GPU], "trace_gpu", firstTraceHeight);

        ImGui::InvisibleButton("hsplitter", ImVec2(-1, 8.0f));
        if (ImGui::IsItemActive())
        {
            firstTraceHeight += ImGui::GetIO().MouseDelta.y;
        }

        ImGui::Text("CPU Frame #%d", currentCPUTrace.frameIndex);
        DrawTraceRects(currentCPUTrace, &selectedMarkers[TRACE_CPU], "trace_cpu", 0.f);

        auto it = interestMarkers.begin();
        while (it != interestMarkers.end())
        {
            if (ShowMarkerHistory(*it))
            {
                it = interestMarkers.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    ImGui::End();

    if (!shown)
    {
        GetEngineContext()->debugOverlay->HideItem(this);
    }
}

void DebugOverlayItemProfiler::Update()
{
    if (overlayPaused)
    {
        return;
    }

    bool isGpuProfilerStarted = gpuProfiler != nullptr && gpuProfiler->IsStarted();
    bool isCpuProfilerStarted = cpuProfiler != nullptr && cpuProfiler->IsStarted();

    uint32 frameIndex = (isGpuProfilerStarted ? gpuProfiler->GetFrame().frameIndex : 0);

    bool needUpdateGPUInfo = isGpuProfilerStarted && (tracesData[TRACE_GPU].crbegin()->frameIndex != frameIndex || frameIndex == 0);
    bool needUpdateCPUInfo = isCpuProfilerStarted && (tracesData[TRACE_CPU].crbegin()->frameIndex != frameIndex || frameIndex == 0);

    bool needUpdateHistory = needUpdateCPUInfo || needUpdateGPUInfo;

    if (needUpdateHistory)
    {
        for (auto& p : markersHistory)
        {
            MarkerHistory::HistoryInstance& next = p.second.values.next();
            next.accurate = 0;
            next.filtered = 0.f;
        }
    }

    if (needUpdateGPUInfo)
    {
        ProcessEventsTrace(gpuProfiler->GetFrame().GetTrace(), &tracesData[TRACE_GPU].next());
    }

    if (needUpdateCPUInfo)
    {
        ProcessEventsTrace(cpuProfiler->GetTrace(cpuCounterName.c_str(), frameIndex), &tracesData[TRACE_CPU].next());
    }

    if (needUpdateHistory)
    {
        for (auto& i : markersHistory)
        {
            MarkerHistory& history = i.second;
            ++history.updatesCount;

            MarkerHistory::HistoryInstance& current = *history.values.rbegin();
            if (history.updatesCount < DebugOverlayItemProfilerDetails::MARKER_HISTORY_NON_FILTERED_COUNT)
            {
                current.filtered = float32(current.accurate);
            }
            else
            {
                const MarkerHistory::HistoryInstance& prev = *(history.values.crbegin() + 1);
                current.filtered = prev.filtered * 0.99f + current.accurate * 0.01f;
            }
        }
    }
}

void DebugOverlayItemProfiler::ProcessEventsTrace(const Vector<TraceEvent>& events, TraceData* trace)
{
    trace->frameIndex = 0;
    trace->minTimestamp = std::numeric_limits<uint64>::max();
    trace->maxTimestamp = 0;
    trace->rects.clear();

    if (!events.empty())
    {
        for (const std::pair<FastName, uint32>& arg : events[0].args)
        {
            if (arg.first == ProfilerCPU::TRACE_ARG_FRAME)
            {
                trace->frameIndex = arg.second;
            }
        }
    }

    Vector<std::pair<uint64, uint64>> timestampsStack; //<start ts, end ts>
    timestampsStack.reserve(32);
    for (const TraceEvent& e : events)
    {
        MarkerHistory& history = markersHistory[e.name];
        uint64& historyMarkerDuration = history.values.rbegin()->accurate;

        switch (e.phase)
        {
        case TraceEvent::PHASE_DURATION:
            historyMarkerDuration += e.duration;
            trace->maxTimestamp = Max(trace->maxTimestamp, e.timestamp + e.duration);
            break;

        case TraceEvent::PHASE_BEGIN:
            historyMarkerDuration -= e.timestamp;
            break;

        case TraceEvent::PHASE_END:
            historyMarkerDuration += e.timestamp;
            trace->maxTimestamp = Max(trace->maxTimestamp, e.timestamp);
            break;

        default:
            break;
        }

        trace->minTimestamp = Min(trace->minTimestamp, e.timestamp);

        auto found = std::find_if(trace->list.begin(), trace->list.end(), [&e](const TraceData::ListElement& element) {
            return (element.name == e.name);
        });

        if (found == trace->list.end())
        {
            trace->list.push_back({ e.name, 0, false });
            trace->maxMarkerNameLen = Max(trace->maxMarkerNameLen, uint32(strlen(e.name.c_str())));
        }

        if (markersColor.count(e.name) == 0)
        {
            markersColor[e.name] = rhi::NativeColorRGBA(CIEDE2000Colors[colorIndex % CIEDE2000_COLORS_COUNT]);
            ++colorIndex;
        }

        //////////////////////////////////////////////////////////////////////////

        while (!timestampsStack.empty() && (timestampsStack.back().second != 0) && (e.timestamp >= timestampsStack.back().second))
        {
            timestampsStack.pop_back();
        }

        switch (e.phase)
        {
        case TraceEvent::PHASE_DURATION:
            timestampsStack.emplace_back(e.timestamp, e.timestamp + e.duration);
            break;

        case TraceEvent::PHASE_BEGIN:
            timestampsStack.emplace_back(e.timestamp, 0);
            break;

        case TraceEvent::PHASE_END:
            timestampsStack.back().second = e.timestamp;
            break;

        default:
            break;
        }

        if (e.phase == TraceEvent::PHASE_END || e.phase == TraceEvent::PHASE_DURATION)
        {
            DVASSERT(!timestampsStack.empty());

            uint64 eventStart = timestampsStack.back().first - events.front().timestamp;
            uint64 eventDuration = timestampsStack.back().second - timestampsStack.back().first;
            uint32 eventColor = markersColor[e.name];
            int32 eventDepth = int32(timestampsStack.size()) - 1;

            trace->maxDepth = Max(trace->maxDepth, eventDepth);
            trace->rects.push_back({ eventStart, eventDuration, eventColor, eventDepth, e.name });
        }
    }

    for (TraceData::ListElement& e : trace->list)
    {
        e.duration = markersHistory[e.name].values.rbegin()->accurate;
    }
}

void DebugOverlayItemProfiler::Reset()
{
    markersHistory.clear();
    markersColor.clear();

    for (int32 i = 0; i < int32(TRACE_COUNT); ++i)
    {
        tracesData[i] = RingArray<TraceData>(TRACE_HISTORY_SIZE);
        selectedMarkers[i] = FastName();
    }

    traceHistoryOffset = 0;
    colorIndex = 0;
}

void DebugOverlayItemProfiler::DrawTraceRects(TraceData& trace, FastName* selectedMarker, const char* id, float32 height)
{
    if (ImGui::BeginChild(id, ImVec2(0, height), true, ImGuiWindowFlags_NoScrollbar))
    {
        ImGui::Columns(2);

        ImVec2 trace_canvas_pos = ImGui::GetCursorScreenPos();
        ImVec2 trace_canvas_size = ImGui::GetContentRegionAvail();
        ImGui::InvisibleButton("trace_canvas", trace_canvas_size);

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        float32 dt = float32(trace_canvas_size.x) / (trace.maxTimestamp - trace.minTimestamp);
        float32 dh = Min(float32(trace_canvas_size.y) / (trace.maxDepth + 1), 15.f);

        static const uint32 SELECTED_COLOR = rhi::NativeColorRGBA(1.f, 0.f, 0.f, 1.f);

        ImVec2 mouse = ImGui::GetIO().MousePos;

        ImVec2 pt0, pt1;
        bool rectSelected = false;
        for (const TraceData::TraceRect& r : trace.rects)
        {
            pt0.x = trace_canvas_pos.x + float32(r.start * dt);
            pt0.y = trace_canvas_pos.y + float32(r.depth * dh);
            pt1.x = pt0.x + r.duration * dt;
            pt1.y = pt0.y + dh;

            if (ImGui::IsItemHovered())
            {
                if (mouse.x > pt0.x && mouse.y > pt0.y && mouse.x < pt1.x && mouse.y < pt1.y)
                {
                    ImGui::SetTooltip("\"%s\": %llu mcs", r.name.c_str(), static_cast<unsigned long long>(r.duration));
                    if (ImGui::IsMouseClicked(0))
                    {
                        *selectedMarker = r.name;
                        rectSelected = true;
                    }
                }
            }

            draw_list->AddRectFilled(pt0, pt1, (r.name == *selectedMarker) ? SELECTED_COLOR : r.color);
        }

        ImGui::NextColumn();

        if (ImGui::BeginChild("trace_list"))
        {
            for (uint32 i = 0; i < uint32(trace.list.size()); ++i)
            {
                TraceData::ListElement& e = trace.list[i];

                String valuestr = Format("%llu mcs", e.duration);
                if (ImGui::Selectable(e.name.c_str(), e.name == *selectedMarker, ImGuiSelectableFlags_AllowDoubleClick))
                {
                    *selectedMarker = e.name;

                    if (ImGui::IsMouseDoubleClicked(0))
                    {
                        interestMarkers.push_back(e.name);
                    }
                }

                if (e.name == *selectedMarker && rectSelected)
                {
                    ImGui::SetScrollHere(float32(i) / trace.list.size());
                }

                ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - ImGui::CalcTextSize(valuestr.c_str()).x);
                ImGui::TextUnformatted(valuestr.c_str());
            }
        }

        ImGui::EndChild();
        ImGui::Columns(1);
    }

    ImGui::EndChild();
}

bool DebugOverlayItemProfiler::ShowMarkerHistory(const FastName& marker)
{
    bool open = true;

    if (ImGui::Begin(marker.c_str(), &open, ImVec2(400.f, 80.f)))
    {
        MarkerHistory& markerHistory = markersHistory[marker];
        void* history = reinterpret_cast<void*>(&markerHistory);
        int32 historySize = static_cast<int32>(markerHistory.values.size());

        using DebugOverlayItemProfilerDetails::HistoryGetter;

        ImGui::PlotHistogram("", &HistoryGetter, history, historySize, 0, nullptr, FLT_MAX, FLT_MAX, ImGui::GetContentRegionAvail());
    }

    ImGui::End();

    return !open;
}
}
