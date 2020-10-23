#include "Debug/ProfilerOverlay.h"
#include "Debug/DVAssert.h"
#include "Debug/DebugColors.h"
#include "Debug/TraceEvent.h"
#include "Debug/ProfilerGPU.h"
#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerMarkerNames.h"
#include "Utils/StringFormat.h"
#include "Render/Renderer.h"
#include "Render/RHI/dbg_Draw.h"
#include "Render/RHI/Common/rhi_Utils.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "UI/UIEvent.h"
#include "UI/UIControlSystem.h"
#include "Input/InputSystem.h"
#include "Input/Keyboard.h"
#include "DeviceManager/DeviceManager.h"
#include "Engine/Engine.h"
#include "Engine/EngineContext.h"

#include <ostream>

namespace DAVA
{
//==============================================================================

namespace ProfilerOverlayDetails
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

static const int32 OVERLAY_BUTTON_SIZE = 30;

static const int32 HISTORY_CHART_TEXT_COLUMN_CHARS = 9;
static const int32 HISTORY_CHART_TEXT_COLUMN_WIDTH = DbgDraw::NormalCharW * HISTORY_CHART_TEXT_COLUMN_CHARS;
static const uint64 HISTORY_CHART_CEIL_STEP = 500; //mcs

static const uint32 MAX_CPU_FRAME_TRACES = 6;
static const uint32 MAX_TRACE_LIST_ELEMENTS_TO_DRAW = 7;

static const char* BUTTON_CLOSE_TEXT = "Close (Ctrl + F12)";
static const char* BUTTON_HISTORY_NEXT_TEXT = "Next (Ctrl + Right) ->";
static const char* BUTTON_HISTORY_PREV_TEXT = "<- Prev (Ctrl + Left)";
static const char* BUTTON_DRAW_MARKER_HISTORY_TEXT = "History (Ctrl + F9)";
static const char* BUTTON_SCALE_TEXT = "Scale (Ctrl + F10)";
static const char* BUTTON_PROFILERS_UNPAUSE_TEXT = "Unpause (Ctrl + F11)";
static const char* BUTTON_PROFILERS_PAUSE_TEXT = "Pause (Ctrl + F11)";
};

static ProfilerOverlay GLOBAL_PROFILER_OVERLAY(ProfilerCPU::globalProfiler, ProfilerCPUMarkerName::ENGINE_ON_FRAME, ProfilerGPU::globalProfiler,
                                               {
                                               FastName(ProfilerCPUMarkerName::ENGINE_ON_FRAME),
                                               FastName(ProfilerCPUMarkerName::UI_UPDATE),
                                               FastName(ProfilerCPUMarkerName::UI_DRAW),
                                               FastName(ProfilerCPUMarkerName::SCENE_UPDATE),
                                               FastName(ProfilerCPUMarkerName::SCENE_DRAW),
                                               FastName(ProfilerCPUMarkerName::RENDER_PASS_DRAW_LAYERS),
                                               FastName(ProfilerCPUMarkerName::RHI_PRESENT),
                                               FastName(ProfilerCPUMarkerName::RHI_WAIT_FRAME_CONSTRUCTION),
                                               FastName(ProfilerCPUMarkerName::RHI_WAIT_FRAME_EXECUTION),
                                               FastName(ProfilerCPUMarkerName::RHI_PROCESS_SCHEDULED_DELETE),
                                               FastName(ProfilerOverlayDetails::OVERLAY_MARKER_CPU_TIME),

                                               FastName(ProfilerGPUMarkerName::GPU_FRAME),
                                               FastName(ProfilerGPUMarkerName::RENDER_PASS_MAIN_3D),
                                               FastName(ProfilerGPUMarkerName::RENDER_PASS_WATER_REFLECTION),
                                               FastName(ProfilerGPUMarkerName::RENDER_PASS_WATER_REFRACTION),
                                               FastName(ProfilerGPUMarkerName::RENDER_PASS_2D),
                                               FastName(ProfilerGPUMarkerName::LANDSCAPE),
                                               FastName(ProfilerOverlayDetails::OVERLAY_PASS_MARKER_NAME)
                                               });

ProfilerOverlay* const ProfilerOverlay::globalProfilerOverlay = &GLOBAL_PROFILER_OVERLAY;

ProfilerOverlay::ProfilerOverlay(ProfilerCPU* _cpuProfiler, const char* _cpuCounterName, ProfilerGPU* _gpuProfiler, const Vector<FastName>& _interestMarkers)
    : interestMarkers(_interestMarkers)
    , gpuProfiler(_gpuProfiler)
    , cpuProfiler(_cpuProfiler)
    , cpuCounterName(_cpuCounterName)
{
    for (RingArray<TraceData>& t : tracesData)
        t = RingArray<TraceData>(TRACE_HISTORY_SIZE);

    Memset(buttonsText, 0, sizeof(buttonsText));

    buttonsText[BUTTON_HISTORY_NEXT] = ProfilerOverlayDetails::BUTTON_HISTORY_NEXT_TEXT;
    buttonsText[BUTTON_HISTORY_PREV] = ProfilerOverlayDetails::BUTTON_HISTORY_PREV_TEXT;
    buttonsText[BUTTON_DRAW_MARKER_HISTORY] = ProfilerOverlayDetails::BUTTON_DRAW_MARKER_HISTORY_TEXT;
    buttonsText[BUTTON_SCALE] = ProfilerOverlayDetails::BUTTON_SCALE_TEXT;
    buttonsText[BUTTON_PROFILERS_START_STOP] = ProfilerOverlayDetails::BUTTON_PROFILERS_UNPAUSE_TEXT;
    buttonsText[BUTTON_CLOSE] = ProfilerOverlayDetails::BUTTON_CLOSE_TEXT;
}

void ProfilerOverlay::SetPaused(bool paused)
{
    overlayPaused = paused;
    buttonsText[BUTTON_PROFILERS_START_STOP] = paused ? ProfilerOverlayDetails::BUTTON_PROFILERS_UNPAUSE_TEXT : ProfilerOverlayDetails::BUTTON_PROFILERS_PAUSE_TEXT;
}

bool ProfilerOverlay::IsPaused() const
{
    return overlayPaused;
}

void ProfilerOverlay::OnFrameEnd()
{
    if (!overlayEnabled)
        return;

    DAVA_PROFILER_CPU_SCOPE(ProfilerOverlayDetails::OVERLAY_MARKER_CPU_TIME);

    Update();
    Draw();
}

void ProfilerOverlay::SetCPUProfiler(ProfilerCPU* profiler, const char* rootCounterName)
{
    if (cpuProfiler == profiler && cpuCounterName == rootCounterName)
        return;

    cpuProfiler = profiler;
    cpuCounterName = rootCounterName;

    if (selectedTrace == TRACE_CPU && !cpuProfiler)
        selectedTrace = TRACE_GPU;

    Reset();
}

void ProfilerOverlay::ClearInterestMarkers()
{
    interestMarkers.clear();
}

void ProfilerOverlay::AddInterestMarker(const FastName& name)
{
    interestMarkers.push_back(name);
}

void ProfilerOverlay::AddInterestMarkers(const Vector<FastName>& markers)
{
    interestMarkers.insert(interestMarkers.end(), markers.begin(), markers.end());
}

const Vector<FastName>& ProfilerOverlay::GetInterestMarkers() const
{
    return interestMarkers;
}

Vector<FastName> ProfilerOverlay::GetAvalibleMarkers() const
{
    Vector<FastName> ret;
    for (const TraceData::ListElement& e : tracesData[TRACE_CPU].crbegin()->list)
        ret.push_back(e.name);

    for (const TraceData::ListElement& e : tracesData[TRACE_GPU].crbegin()->list)
        ret.push_back(e.name);

    return ret;
}

void ProfilerOverlay::SelectNextMarker()
{
    const TraceData& data = GetHistoricTrace(tracesData[selectedTrace]);
    int32 selectedIndex = FindListIndex(data.list, selectedMarkers[selectedTrace]);
    if (selectedIndex == -1)
    {
        selectedIndex = 0;
    }
    else if (selectedIndex < int32(data.list.size() - 1))
    {
        ++selectedIndex;
    }

    if (selectedIndex < int32(data.list.size()))
        selectedMarkers[selectedTrace] = data.list[selectedIndex].name;
}

void ProfilerOverlay::SelectPreviousMarker()
{
    const TraceData& data = GetHistoricTrace(tracesData[selectedTrace]);
    int32 selectedIndex = FindListIndex(data.list, selectedMarkers[selectedTrace]);
    if (selectedIndex == -1)
    {
        selectedIndex = 0;
    }
    else if (selectedIndex > 0)
    {
        --selectedIndex;
    }

    if (selectedIndex < int32(data.list.size()))
        selectedMarkers[selectedTrace] = data.list[selectedIndex].name;
}

void ProfilerOverlay::SelectMarker(const FastName& name)
{
    const TraceData& data = GetHistoricTrace(tracesData[selectedTrace]);
    if (FindListIndex(data.list, name) != -1)
        selectedMarkers[selectedTrace] = name;
}

bool ProfilerOverlay::OnInput(UIEvent* input)
{
    if (inputEnabled)
    {
        const Keyboard* keyboard = GetEngineContext()->deviceManager->GetKeyboard();

        if (keyboard != nullptr)
        {
            const bool lctrlPressed = keyboard->GetKeyState(eInputElements::KB_LCTRL).IsPressed();

            if (lctrlPressed && input->phase == UIEvent::Phase::KEY_DOWN && input->key == eInputElements::KB_F12)
            {
                SetEnabled(!IsEnabled());
            }
            else if (overlayEnabled)
            {
                if (lctrlPressed && input->phase == UIEvent::Phase::KEY_DOWN)
                {
                    switch (input->key)
                    {
                    case eInputElements::KB_F9:
                        OnButtonPressed(BUTTON_DRAW_MARKER_HISTORY);
                        break;

                    case eInputElements::KB_F10:
                        OnButtonPressed(BUTTON_SCALE);
                        break;

                    case eInputElements::KB_F11:
                        OnButtonPressed(BUTTON_PROFILERS_START_STOP);
                        break;

                    case eInputElements::KB_LEFT:
                        OnButtonPressed(BUTTON_HISTORY_PREV);
                        break;

                    case eInputElements::KB_RIGHT:
                        OnButtonPressed(BUTTON_HISTORY_NEXT);
                        break;

                    case eInputElements::KB_UP:
                        SelectPreviousMarker();
                        break;

                    case eInputElements::KB_DOWN:
                        SelectNextMarker();
                        break;

                    case eInputElements::KB_TAB:
                        SelectTrace((GetSelectedTrace() == ProfilerOverlay::TRACE_CPU) ? ProfilerOverlay::TRACE_GPU : ProfilerOverlay::TRACE_CPU);
                        break;

                    default:
                        break;
                    }
                }
                else
                {
                    ProcessTouch(input);
                }
            }
        }
    }

    return inputEnabled && overlayEnabled;
}

void ProfilerOverlay::ProcessTouch(UIEvent* input)
{
    if (input->phase == UIEvent::Phase::ENDED)
    {
        for (int32 i = 0; i < BUTTON_COUNT; ++i)
        {
            Vector2 physPoint = DAVA::GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToPhysical(input->point);
            Point2i point = Point2i(int32(physPoint.x / overlayScale), int32(physPoint.y / overlayScale));
            if (buttons[i].PointInside(point))
                OnButtonPressed(eButton(i));
        }
    }
}

void ProfilerOverlay::OnButtonPressed(eButton button)
{
    switch (button)
    {
    case BUTTON_CLOSE:
        SetEnabled(false);
        break;

    case BUTTON_CPU_UP:
        SelectTrace(TRACE_CPU);
        SelectPreviousMarker();
        break;

    case BUTTON_CPU_DOWN:
        SelectTrace(TRACE_CPU);
        SelectNextMarker();
        break;

    case BUTTON_GPU_UP:
        SelectTrace(TRACE_GPU);
        SelectPreviousMarker();
        break;

    case BUTTON_GPU_DOWN:
        SelectTrace(TRACE_GPU);
        SelectNextMarker();
        break;

    case BUTTON_HISTORY_NEXT:
        SetTraceHistoryOffset((GetTraceHistoryOffset() > 0) ? GetTraceHistoryOffset() - 1 : GetTraceHistoryOffset());
        break;

    case BUTTON_HISTORY_PREV:
        SetTraceHistoryOffset(GetTraceHistoryOffset() + 1);
        break;

    case BUTTON_DRAW_MARKER_HISTORY:
        drawMarkerHistory = !drawMarkerHistory;
        break;

    case BUTTON_SCALE:
        SetDrawScace(FLOAT_EQUAL(GetDrawScale(), 1.f) ? 2.f : 1.f);
        break;

    case BUTTON_PROFILERS_START_STOP:
        SetPaused(!IsPaused());
        break;

    default:
        break;
    }
}

void ProfilerOverlay::Update()
{
    if (overlayPaused)
        return;

    uint32 frameIndex = (gpuProfiler && gpuProfiler->IsStarted()) ? gpuProfiler->GetFrame().frameIndex : 0;

    bool needUpdateGPUInfo = gpuProfiler && gpuProfiler->IsStarted() && (tracesData[TRACE_GPU].crbegin()->frameIndex != frameIndex || frameIndex == 0);
    bool needUpdateCPUInfo = cpuProfiler && cpuProfiler->IsStarted() && (tracesData[TRACE_CPU].crbegin()->frameIndex != frameIndex || frameIndex == 0);

    bool needUpdateHistory = needUpdateCPUInfo || needUpdateGPUInfo;
    if (needUpdateHistory)
    {
        for (auto& i : markersHistory)
        {
            MarkerHistory::HistoryArray& history = i.second.values;
            MarkerHistory::HistoryInstance& value = history.next();
            value.accurate = 0;
            value.filtered = 0.f;
        }
    }

    if (needUpdateGPUInfo)
    {
        ProcessEventsTrace(gpuProfiler->GetFrame().GetTrace(), &tracesData[TRACE_GPU].next());
    }

    if (needUpdateCPUInfo)
    {
        ProcessEventsTrace(cpuProfiler->GetTrace(cpuCounterName, frameIndex), &tracesData[TRACE_CPU].next());
    }

    if (needUpdateHistory)
    {
        for (auto& i : markersHistory)
        {
            MarkerHistory& history = i.second;
            ++history.updatesCount;

            MarkerHistory::HistoryInstance& current = *history.values.rbegin();
            if (history.updatesCount < ProfilerOverlayDetails::MARKER_HISTORY_NON_FILTERED_COUNT)
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

void ProfilerOverlay::ProcessEventsTrace(const Vector<TraceEvent>& events, TraceData* trace)
{
    trace->frameIndex = 0;
    trace->minTimestamp = uint64(-1);
    trace->maxTimestamp = 0;
    trace->rects.clear();

    if (!events.empty())
    {
        for (const std::pair<FastName, uint32>& arg : events[0].args)
        {
            if (arg.first == ProfilerCPU::TRACE_ARG_FRAME)
                trace->frameIndex = arg.second;
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
            trace->list.push_back({ e.name, 0 });
            trace->maxMarkerNameLen = Max(trace->maxMarkerNameLen, uint32(strlen(e.name.c_str())));
        }

        if (markersColor.find(e.name) == markersColor.end())
        {
            markersColor[e.name] = rhi::NativeColorRGBA(CIEDE2000Colors[colorIndex % CIEDE2000_COLORS_COUNT]);
            ++colorIndex;
        }

        //////////////////////////////////////////////////////////////////////////

        while (!timestampsStack.empty() && (timestampsStack.back().second != 0) && (e.timestamp >= timestampsStack.back().second))
            timestampsStack.pop_back();

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

            trace->rects.push_back({ eventStart, eventDuration, eventColor, eventDepth, e.name });
        }
    }

    for (TraceData::ListElement& e : trace->list)
        e.duration = markersHistory[e.name].values.rbegin()->accurate;
}

void ProfilerOverlay::Draw()
{
    Size2i screenSize(int32(Renderer::GetFramebufferWidth() / overlayScale), int32(Renderer::GetFramebufferHeight() / overlayScale));

    DbgDraw::EnsureInited();
    DbgDraw::SetScreenSize(screenSize.dx, screenSize.dy);
    DbgDraw::SetNormalTextSize();

    TraceData& currentCPUTrace = GetHistoricTrace(tracesData[TRACE_CPU]);
    TraceData& currentGPUTrace = GetHistoricTrace(tracesData[TRACE_GPU]);

    //draw traces
    Rect2i rect = Rect2i(0, 0, screenSize.dx, 0);
    if (cpuProfiler)
    {
        rect.dy = GetEnoughRectHeight(currentCPUTrace);
        DrawTrace(currentCPUTrace, Format("CPU Frame %d", currentCPUTrace.frameIndex).c_str(), rect, selectedMarkers[TRACE_CPU], (selectedTrace == TRACE_CPU), &buttons[BUTTON_CPU_UP], &buttons[BUTTON_CPU_DOWN]);
    }
    if (gpuProfiler)
    {
        rect.y += rect.dy;
        rect.dy = GetEnoughRectHeight(currentGPUTrace);
        DrawTrace(currentGPUTrace, Format("GPU Frame %d", currentGPUTrace.frameIndex).c_str(), rect, selectedMarkers[TRACE_GPU], (selectedTrace == TRACE_GPU), &buttons[BUTTON_GPU_UP], &buttons[BUTTON_GPU_DOWN]);
    }

    //draw interest markers history
    if (drawMarkerHistory && !interestMarkers.empty())
    {
        int32 chartColumnCount = (screenSize.dy - rect.y - rect.dy - ProfilerOverlayDetails::OVERLAY_BUTTON_SIZE) / ProfilerOverlayDetails::MARKER_HISTORY_CHART_HEIGHT;
        int32 chartRowCount = int32(ceilf(float32(interestMarkers.size()) / chartColumnCount));
        int32 chartWidth = screenSize.dx / chartRowCount;
        int32 chartTableY = rect.y + rect.dy;

        rect.x = 0;
        rect.y = chartTableY;
        rect.dy = ProfilerOverlayDetails::MARKER_HISTORY_CHART_HEIGHT;
        rect.dx = chartWidth;
        for (const FastName& m : interestMarkers)
        {
            DrawHistory(m, rect);
            rect.y += rect.dy;
            if ((rect.y + rect.dy) > (screenSize.dy - ProfilerOverlayDetails::OVERLAY_BUTTON_SIZE))
            {
                rect.x += chartWidth;
                rect.y = chartTableY;
            }
        }
    }

    //draw buttons
    static const uint32 BUTTON_COLOR = rhi::NativeColorRGBA(.3f, .3f, .3f, .8f);
    static const uint32 BUTTON_TEXT_COLOR = rhi::NativeColorRGBA(1.f, 1.f, 1.f, 1.f);
    static const uint32 BUTTON_BORDER_COLOR = rhi::NativeColorRGBA(1.f, 1.f, 1.f, 1.f);

    float32 buttonWidth = screenSize.dx / float32(int32(BUTTON_COUNT) - int32(BUTTON_HISTORY_PREV));
    float32 x0 = 0.f, x1 = 0.f;
    int32 y0 = screenSize.dy - ProfilerOverlayDetails::OVERLAY_BUTTON_SIZE;
    for (int32 i = int32(BUTTON_HISTORY_PREV); i < int32(BUTTON_COUNT); ++i)
    {
        x1 += buttonWidth;
        DbgDraw::FilledRect2D(int32(x0), y0, int32(x1), screenSize.dy, BUTTON_COLOR);
        DbgDraw::Rect2D(int32(x0), y0, int32(x1), screenSize.dy, BUTTON_BORDER_COLOR);

        int32 xAlign = (int32(buttonWidth) - int32(strlen(buttonsText[i]) * int32(DbgDraw::NormalCharW))) / 2;
        int32 yAlign = (ProfilerOverlayDetails::OVERLAY_BUTTON_SIZE - DbgDraw::NormalCharH) / 2;
        DbgDraw::Text2D(int32(x0) + xAlign, y0 + yAlign, BUTTON_TEXT_COLOR, buttonsText[i]);

        buttons[i] = Rect2i(int32(x0), y0, int32(buttonWidth), ProfilerOverlayDetails::OVERLAY_BUTTON_SIZE);

        x0 += buttonWidth;
    }

    //////////////////////////////////////////////////////////////////////////

    rhi::RenderPassConfig passConfig;
    passConfig.colorBuffer[0].loadAction = rhi::LOADACTION_LOAD;
    passConfig.colorBuffer[0].storeAction = rhi::STOREACTION_STORE;
    passConfig.depthStencilBuffer.loadAction = rhi::LOADACTION_NONE;
    passConfig.depthStencilBuffer.storeAction = rhi::STOREACTION_NONE;
    passConfig.priority = PRIORITY_MAIN_2D - 10;
    passConfig.viewport.x = 0;
    passConfig.viewport.y = 0;
    passConfig.viewport.width = Renderer::GetFramebufferWidth();
    passConfig.viewport.height = Renderer::GetFramebufferHeight();
    DAVA_PROFILER_GPU_RENDER_PASS(passConfig, ProfilerOverlayDetails::OVERLAY_PASS_MARKER_NAME);

    rhi::HPacketList packetList;
    rhi::HRenderPass pass = rhi::AllocateRenderPass(passConfig, 1, &packetList);
    rhi::BeginRenderPass(pass);
    rhi::BeginPacketList(packetList);

    DbgDraw::FlushBatched(packetList);

    rhi::EndPacketList(packetList);
    rhi::EndRenderPass(pass);
}

void ProfilerOverlay::DrawTrace(const TraceData& trace, const char* traceHeader, const Rect2i& rect, const FastName& selectedMarker, bool traceSelected, Rect2i* upButton, Rect2i* downButton)
{
    static const uint32 BACKGROUND_COLOR = rhi::NativeColorRGBA(0.f, 0.f, .7f, .5f);
    static const uint32 SELECTED_BACKGROUND_COLOR = rhi::NativeColorRGBA(0.f, 0.f, .8f, .5f);
    static const uint32 TEXT_COLOR = rhi::NativeColorRGBA(1.f, 1.f, 1.f, 1.f);
    static const uint32 SELECTED_COLOR = rhi::NativeColorRGBA(1.f, 0.f, 0.f, 1.f);
    static const uint32 ARROW_COLOR = rhi::NativeColorRGBA(1.f, 0.f, 0.f, 1.f);
    static const uint32 ARROW_OUTLINE_COLOR = rhi::NativeColorRGBA(.4f, 0.f, 0.f, 1.f);
    static const uint32 LINE_COLOR = rhi::NativeColorRGBA(.5f, 0.f, 0.f, 1.f);
    static const uint32 BUTTON_COLOR = rhi::NativeColorRGBA(.3f, .3f, .3f, .6f);
    static const uint32 BUTTON_ARROW_COLOR = rhi::NativeColorRGBA(1.f, 1.f, 1.f, .6f);

    static const int32 MARGIN = ProfilerOverlayDetails::OVERLAY_RECT_MARGIN;
    static const int32 PADDING = ProfilerOverlayDetails::OVERLAY_RECT_PADDING;
    static const int32 BUTTON_ARROW_MARGIN = ProfilerOverlayDetails::OVERLAY_BUTTON_SIZE / 6;

    Rect2i drawRect(rect);
    drawRect.x += PADDING / 2;
    drawRect.y += PADDING / 2;
    drawRect.dx -= PADDING;
    drawRect.dy -= PADDING;

    DbgDraw::FilledRect2D(drawRect.x, drawRect.y, drawRect.x + drawRect.dx, drawRect.y + drawRect.dy, traceSelected ? SELECTED_BACKGROUND_COLOR : BACKGROUND_COLOR);

    int32 x0, x1, y0, y1;

    //Draw Head
    x0 = drawRect.x + MARGIN;
    y0 = drawRect.y + MARGIN;
    DbgDraw::Text2D(x0, y0, TEXT_COLOR, traceHeader);

    //Draw List (color rects + marker name) and total markers duration
    x0 = drawRect.x + MARGIN + ProfilerOverlayDetails::OVERLAY_BUTTON_SIZE + MARGIN;
    x1 = x0 + ProfilerOverlayDetails::TRACE_LIST_ICON_SIZE;
    y0 = drawRect.y + MARGIN + DbgDraw::NormalCharH + MARGIN;

    char strbuf[256];
    uint32 textColor = 0;

    int32 listWidth = ProfilerOverlayDetails::TRACE_LIST_ICON_SIZE + DbgDraw::NormalCharW + trace.maxMarkerNameLen * DbgDraw::NormalCharW + DbgDraw::NormalCharW;
    int32 selectedIndex = FindListIndex(trace.list, selectedMarker);
    int32 maxElements = int32(ProfilerOverlayDetails::MAX_TRACE_LIST_ELEMENTS_TO_DRAW);
    int32 elementsCount = int32(trace.list.size());

    int32 startIndex = Min(Max(selectedIndex - maxElements / 2, 0), Max(elementsCount - maxElements, 0));
    int32 endIndex = Min(elementsCount, startIndex + maxElements);
    for (int32 i = startIndex; i < endIndex; ++i)
    {
        const TraceData::ListElement& element = trace.list[i];
        textColor = (element.name == selectedMarker) ? SELECTED_COLOR : TEXT_COLOR;

        y1 = y0 + ProfilerOverlayDetails::TRACE_LIST_ICON_SIZE;

        DbgDraw::FilledRect2D(x0, y0, x1, y1, markersColor[element.name]);
        DbgDraw::Text2D(x1 + DbgDraw::NormalCharW, y0, textColor, element.name.c_str());

        snprintf(strbuf, countof(strbuf), "[%*d mcs]", ProfilerOverlayDetails::TRACE_LIST_DURATION_TEXT_WIDTH_CHARS - 6, uint32(element.duration));
        DbgDraw::Text2D(x0 + listWidth, y0, textColor, strbuf);

        y0 += ProfilerOverlayDetails::TRACE_LIST_ICON_SIZE + 1;
    }

    //Draw up/down list buttons
    x0 = drawRect.x + MARGIN;
    y0 = drawRect.y + MARGIN + DbgDraw::NormalCharH + MARGIN;
    y1 = y0 + ProfilerOverlayDetails::OVERLAY_BUTTON_SIZE;
    x1 = x0 + ProfilerOverlayDetails::OVERLAY_BUTTON_SIZE;

    *upButton = Rect2i(x0, y0, x1 - x0, y1 - y0);
    DbgDraw::FilledRect2D(x0, y0, x1, y1, BUTTON_COLOR);
    DbgDraw::FilledTriangle2D(
    x0 + (x1 - x0) / 2, y0 + BUTTON_ARROW_MARGIN,
    x1 - BUTTON_ARROW_MARGIN, y1 - BUTTON_ARROW_MARGIN,
    x0 + BUTTON_ARROW_MARGIN, y1 - BUTTON_ARROW_MARGIN,
    BUTTON_ARROW_COLOR);

    y1 = drawRect.y + MARGIN + DbgDraw::NormalCharH + MARGIN;
    y1 += Max(ProfilerOverlayDetails::OVERLAY_BUTTON_SIZE * 2, (endIndex - startIndex) * (ProfilerOverlayDetails::TRACE_LIST_ICON_SIZE + 1) - 1);
    y0 = y1 - ProfilerOverlayDetails::OVERLAY_BUTTON_SIZE;

    *downButton = Rect2i(x0, y0, x1 - x0, y1 - y0);
    DbgDraw::FilledRect2D(x0, y0, x1, y1, BUTTON_COLOR);
    DbgDraw::FilledTriangle2D(
    x0 + (x1 - x0) / 2, y1 - BUTTON_ARROW_MARGIN,
    x0 + BUTTON_ARROW_MARGIN, y0 + BUTTON_ARROW_MARGIN,
    x1 - BUTTON_ARROW_MARGIN, y0 + BUTTON_ARROW_MARGIN,
    BUTTON_ARROW_COLOR);

    //Draw selected marker history
    y1 += 1;
    if (selectedMarker.IsValid())
        DrawHistory(selectedMarker, Rect2i(rect.x + PADDING / 2, y1, rect.dx - PADDING, ProfilerOverlayDetails::MARKER_HISTORY_CHART_HEIGHT), false);

    //Draw separators
    x0 = drawRect.x + MARGIN;
    x1 = drawRect.x + drawRect.dx - MARGIN;
    DbgDraw::Line2D(x0, y1, x1, y1, LINE_COLOR);

    int32 durationTextWidth = ProfilerOverlayDetails::TRACE_LIST_DURATION_TEXT_WIDTH_CHARS * DbgDraw::NormalCharW;
    x0 = drawRect.x + MARGIN + ProfilerOverlayDetails::OVERLAY_BUTTON_SIZE + MARGIN + listWidth + durationTextWidth + MARGIN;
    x1 = x0;
    y0 = drawRect.y + MARGIN + DbgDraw::NormalCharH + MARGIN;
    DbgDraw::Line2D(x0, y0, x1, y1, LINE_COLOR);

    //Draw traces rects
    int32 x0trace = x0 + MARGIN;
    int32 y0trace = y0;
    int32 tracedx = drawRect.dx - x0trace - MARGIN;
    float32 dt = float32(tracedx) / (trace.maxTimestamp - trace.minTimestamp);

    Vector<const TraceData::TraceRect*> arrowedRects;
    for (const TraceData::TraceRect& r : trace.rects)
    {
        x0 = x0trace + int32(r.start * dt);
        x1 = x0 + Max(1, int32(r.duration * dt));
        y0 = y0trace + int32(r.depth * ProfilerOverlayDetails::TRACE_RECT_HEIGHT);
        y1 = y0 + ProfilerOverlayDetails::TRACE_RECT_HEIGHT;

        bool drawArrow = (r.name == selectedMarker) && ((x1 - x0) < ProfilerOverlayDetails::MIN_HIGHLIGHTED_TRACE_RECT_SIZE);
        if (drawArrow)
        {
            arrowedRects.push_back(&r);
        }

        if (r.name != selectedMarker || drawArrow)
        {
            DbgDraw::FilledRect2D(x0, y0, x1, y1, r.color);
        }
        else
        {
            DbgDraw::FilledRect2D(x0, y0, x1, y1, SELECTED_COLOR);
        }
    }

    //draw arrows
    for (const TraceData::TraceRect* r : arrowedRects)
    {
        x0 = x0trace + int32(r->start * dt);
        x1 = x0 + int32(r->duration * dt);
        y0 = y0trace + int32(r->depth * ProfilerOverlayDetails::TRACE_RECT_HEIGHT);
        y1 = y0 + ProfilerOverlayDetails::TRACE_RECT_HEIGHT;

        int32 xm = x0 + (x1 - x0) / 2;

        DbgDraw::FilledRect2D(xm - 1, y1, xm + 2, y1 + ProfilerOverlayDetails::TRACE_ARROW_HEIGHT + 1, ARROW_OUTLINE_COLOR);
        DbgDraw::FilledRect2D(xm - 2, y1 + 2, xm + 3, y1 + 4, ARROW_OUTLINE_COLOR);
        DbgDraw::FilledRect2D(xm - 3, y1 + 4, xm + 4, y1 + 7, ARROW_OUTLINE_COLOR);

        DbgDraw::FilledRect2D(xm, y1, xm + 1, y1 + ProfilerOverlayDetails::TRACE_ARROW_HEIGHT, ARROW_COLOR);
        DbgDraw::FilledRect2D(xm - 1, y1 + 2, xm + 2, y1 + 4, ARROW_COLOR);
        DbgDraw::FilledRect2D(xm - 2, y1 + 4, xm + 3, y1 + 6, ARROW_COLOR);
    }
}

void ProfilerOverlay::DrawHistory(const FastName& name, const Rect2i& rect, bool drawBackground)
{
    static const uint32 CHARTRECT_COLOR = rhi::NativeColorRGBA(0.f, 0.f, .8f, .5f);
    static const uint32 CHART_COLOR = rhi::NativeColorRGBA(.5f, .11f, .11f, 1.f);
    static const uint32 CHART_FILTERED_COLOR = rhi::NativeColorRGBA(1.f, .18f, .18f, 1.f);
    static const uint32 TEXT_COLOR = rhi::NativeColorRGBA(1.f, 1.f, 1.f, 1.f);
    static const uint32 LINE_COLOR = rhi::NativeColorRGBA(.5f, 0.f, 0.f, 1.f);

    static const int32 MARGIN = ProfilerOverlayDetails::OVERLAY_RECT_MARGIN;
    static const int32 PADDING = ProfilerOverlayDetails::OVERLAY_RECT_PADDING;

    const MarkerHistory::HistoryArray& history = markersHistory[name].values;

    Rect2i drawRect(rect);
    drawRect.x += PADDING / 2;
    drawRect.y += PADDING / 2;
    drawRect.dx -= PADDING;
    drawRect.dy -= PADDING;

    Rect2i chartRect(drawRect);
    chartRect.x += MARGIN + ProfilerOverlayDetails::HISTORY_CHART_TEXT_COLUMN_WIDTH + MARGIN;
    chartRect.y += MARGIN + DbgDraw::NormalCharH;
    chartRect.dx -= ProfilerOverlayDetails::HISTORY_CHART_TEXT_COLUMN_WIDTH + 3 * MARGIN;
    chartRect.dy -= 2 * MARGIN + DbgDraw::NormalCharH;

    uint64 maxValue = 0;
    for (const MarkerHistory::HistoryInstance& h : history)
        maxValue = Max(maxValue, Max(h.accurate, uint64(h.filtered)));

    char strbuf[128];
    float32 ceilValue = float32((maxValue / ProfilerOverlayDetails::HISTORY_CHART_CEIL_STEP + 1) * ProfilerOverlayDetails::HISTORY_CHART_CEIL_STEP);

    if (drawBackground)
        DbgDraw::FilledRect2D(drawRect.x, drawRect.y, drawRect.x + drawRect.dx, drawRect.y + drawRect.dy, CHARTRECT_COLOR);

    DbgDraw::Line2D(chartRect.x, chartRect.y, chartRect.x, chartRect.y + chartRect.dy, LINE_COLOR);
    DbgDraw::Line2D(chartRect.x, chartRect.y + chartRect.dy, chartRect.x + chartRect.dx, chartRect.y + chartRect.dy, LINE_COLOR);

    const float32 chartstep = float32(chartRect.dx) / history.size();
    const float32 valuescale = chartRect.dy / ceilValue;

    const int32 chart0x = chartRect.x;
    const int32 chart0y = chartRect.y + chartRect.dy;

    MarkerHistory::HistoryArray::const_iterator it = history.cbegin();
    int32 px = 0;
    int32 py = int32(valuescale * it->accurate);
    int32 pfy = int32(valuescale * it->filtered);
    ++it;

    int32 index = 1;
    MarkerHistory::HistoryArray::const_iterator hend = history.cend();
    for (; it != hend; ++it, ++index)
    {
        int32 x = int32(index * chartstep);
        int32 y = int32(valuescale * it->accurate);
        int32 fy = int32(valuescale * it->filtered);

        DbgDraw::Line2D(chart0x + px, chart0y - py, chart0x + x, chart0y - y, CHART_COLOR);
        DbgDraw::Line2D(chart0x + px, chart0y - pfy, chart0x + x, chart0y - fy, CHART_FILTERED_COLOR);

        px = x;
        py = y;
        pfy = fy;
    }

    DbgDraw::Text2D(drawRect.x + MARGIN, drawRect.y + MARGIN, TEXT_COLOR, "\'%s\'", name.c_str());

    snprintf(strbuf, countof(strbuf), "%lld [%.1f] mcs", history.crbegin()->accurate, history.crbegin()->filtered);
    int32 tdx = drawRect.dx - MARGIN - int32(DbgDraw::NormalCharW * strlen(strbuf));
    DbgDraw::Text2D(drawRect.x + tdx, drawRect.y + MARGIN, TEXT_COLOR, strbuf);

    snprintf(strbuf, countof(strbuf), "%d mcs", int32(ceilValue));
    DbgDraw::Text2D(drawRect.x + MARGIN, drawRect.y + MARGIN + DbgDraw::NormalCharH, TEXT_COLOR, "%*s", ProfilerOverlayDetails::HISTORY_CHART_TEXT_COLUMN_CHARS, strbuf);
    DbgDraw::Text2D(drawRect.x + MARGIN, drawRect.y + drawRect.dy - MARGIN - DbgDraw::NormalCharH, TEXT_COLOR, "%*s", ProfilerOverlayDetails::HISTORY_CHART_TEXT_COLUMN_CHARS, "0 mcs");
}

int32 ProfilerOverlay::GetEnoughRectHeight(const TraceData& trace)
{
    uint32 listElementsCount = Min(uint32(trace.list.size()), ProfilerOverlayDetails::MAX_TRACE_LIST_ELEMENTS_TO_DRAW);
    int32 listHeight = Max(int32(listElementsCount) * (DbgDraw::NormalCharH + 1), ProfilerOverlayDetails::OVERLAY_BUTTON_SIZE * 2);

    const int32 MARGIN = ProfilerOverlayDetails::OVERLAY_RECT_MARGIN;
    return MARGIN + DbgDraw::NormalCharH + MARGIN + listHeight + ProfilerOverlayDetails::MARKER_HISTORY_CHART_HEIGHT + ProfilerOverlayDetails::OVERLAY_RECT_PADDING;
}

int32 ProfilerOverlay::FindListIndex(const Vector<TraceData::ListElement>& list, const FastName& marker)
{
    auto found = std::find_if(list.begin(), list.end(), [&marker](const TraceData::ListElement& element) {
        return (element.name == marker);
    });

    if (found != list.end())
        return int32(std::distance(list.begin(), found));

    return -1;
}

ProfilerOverlay::TraceData& ProfilerOverlay::GetHistoricTrace(RingArray<TraceData>& traceData)
{
    return *(traceData.rbegin() + traceHistoryOffset);
}

void ProfilerOverlay::Reset()
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

}; //ns
