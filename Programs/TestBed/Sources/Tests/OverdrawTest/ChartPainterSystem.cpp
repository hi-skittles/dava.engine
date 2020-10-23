#include "ChartPainterSystem.h"

#include "OverdrawTestConfig.h"
#include "OverdrawTesterComponent.h"

#include "Render/2D/Systems/RenderSystem2D.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "Render/RHI/dbg_Draw.h"
#include "Render/Renderer.h"
#include "Scene3D/Entity.h"
#include "UI/UIControlSystem.h"
#include "Engine/Engine.h"

namespace OverdrawPerformanceTester
{
using DAVA::int32;
using DAVA::float32;
using DAVA::Vector2;
using DAVA::Color;
using DAVA::Array;
using DAVA::String;
using DAVA::Scene;
using DAVA::SceneSystem;
using DAVA::Polygon2;
using DAVA::UIControlSystem;
using DAVA::DbgDraw;
using DAVA::RenderSystem2D;
using DAVA::VirtualCoordinatesSystem;
using DAVA::Vector;

const Vector2 ChartPainterSystem::chartOffset(0.1f, 0.1f);
const Color ChartPainterSystem::gridColor(0.4f, 0.4f, 0.4f, 0.4f);
const float32 ChartPainterSystem::chartLen = 0.8f;
const float32 ChartPainterSystem::minFrametime = 1.0f / 60.0f;
const float32 ChartPainterSystem::overdrawStep = 100.0f;
const float32 ChartPainterSystem::frametimeStep = 0.008f;

const DAVA::uint32 modsCount = 6;

const Array<String, modsCount> ChartPainterSystem::legend =
{ {
"0 tex",
"1 tex",
"2 tex",
"3 tex",
"4 tex",
"dep r"
} };

const Array<Color, modsCount> ChartPainterSystem::chartColors =
{ {
{ 0.0f, 1.0f, 0.0f, 1.0f },
{ 1.0f, 1.0f, 0.0f, 1.0f },
{ 0.0f, 1.0f, 1.0f, 1.0f },
{ 0.0f, 0.0f, 1.0f, 1.0f },
{ 1.0f, 0.0f, 1.0f, 1.0f },
{ 1.0f, 0.0f, 0.0f, 1.0f }
} };

ChartPainterSystem::ChartPainterSystem(Scene* scene, float32 maxFrametime_)
    : SceneSystem(scene)
    , maxFrametime(maxFrametime_)
    , textColor(rhi::NativeColorRGBA(1.0f, 1.0f, 1.0f, 1.0f))
{
    passConfig.colorBuffer[0].loadAction = rhi::LOADACTION_LOAD;
    passConfig.colorBuffer[0].storeAction = rhi::STOREACTION_STORE;
    passConfig.depthStencilBuffer.loadAction = rhi::LOADACTION_NONE;
    passConfig.depthStencilBuffer.storeAction = rhi::STOREACTION_NONE;
    passConfig.priority = DAVA::PRIORITY_MAIN_2D - 10;
    passConfig.viewport.x = 0;
    passConfig.viewport.y = 0;

    DAVA::DbgDraw::EnsureInited();
}

void ChartPainterSystem::AddEntity(DAVA::Entity* entity)
{
    OverdrawTesterComponent* comp = entity->GetComponent<OverdrawTesterComponent>();
    if (comp != nullptr)
    {
        maxOverdraw = comp->GetStepOverdraw() * comp->GetStepsCount();
        overdrawStepCount = maxOverdraw / overdrawStep;
    }
}

void ChartPainterSystem::PrepareForRemove()
{
}

void ChartPainterSystem::Process(float32 timeElapsed)
{
    if (performanceData == nullptr)
        return;

    int32 w = DAVA::GetEngineContext()->uiControlSystem->vcs->GetVirtualScreenSize().dx;
    int32 h = DAVA::GetEngineContext()->uiControlSystem->vcs->GetVirtualScreenSize().dy;

    DrawLegend(w, h);

    DrawGrid(w, h);
    DrawCharts(w, h);

    FlushDbgText();
}

void ChartPainterSystem::DrawGrid(int32 w, int32 h) const
{
    Vector2 origin(chartOffset.x * w, (chartOffset.y + chartLen) * h);
    Vector2 xAxis((chartOffset.x + chartLen) * w, (chartOffset.y + chartLen) * h);
    Vector2 yAxis(chartOffset.x * w, chartOffset.y * h);

    Polygon2 gridPoly;
    gridPoly.AddPoint(origin);
    gridPoly.AddPoint(xAxis);
    RenderSystem2D::Instance()->DrawPolygon(gridPoly, false, Color::White);

    gridPoly.Clear();
    gridPoly.AddPoint(origin);
    gridPoly.AddPoint(yAxis);
    RenderSystem2D::Instance()->DrawPolygon(gridPoly, false, Color::White);

    VirtualCoordinatesSystem* vcs = DAVA::GetEngineContext()->uiControlSystem->vcs;
    int32 stepCount = static_cast<int>(frametimeStepCount + 1);
    Vector2 vcsOffset = vcs->GetPhysicalDrawOffset();
    int32 vcsOffsetX = static_cast<int32>(vcsOffset.x);
    int32 vcsOffsetY = static_cast<int32>(vcsOffset.y);
    for (int32 i = 1; i < stepCount + 1; i++)
    {
        gridPoly.Clear();
        float32 normalizedFps = (i * frametimeStep) / frametimeAxisLen;
        normalizedFps *= chartLen;
        float32 pointY = 1 - (normalizedFps + chartOffset.y);
        int32 pointYInt = static_cast<int32>(pointY * DAVA::Renderer::GetFramebufferHeight());
        pointY *= h;

        gridPoly.AddPoint({ chartOffset.x * w, pointY });
        gridPoly.AddPoint({ (chartOffset.x + chartLen) * w, pointY });

        int32 textX = static_cast<int32>(vcs->ConvertVirtualToPhysicalX(0.05f * w));
        int32 textY = static_cast<int32>(vcs->ConvertVirtualToPhysicalY(pointY));
        DbgDraw::Text2D(textX + vcsOffsetX, textY + vcsOffsetY, textColor, "%.4f", i * frametimeStep + minFrametime);
        RenderSystem2D::Instance()->DrawPolygon(gridPoly, false, gridColor);
    }

    for (int32 i = 1; i < overdrawStepCount + 1; i++)
    {
        gridPoly.Clear();
        float32 normalizedOverdraw = (i * overdrawStep) / maxOverdraw;
        normalizedOverdraw *= chartLen;
        float32 pointX = normalizedOverdraw + chartOffset.x;
        int32 pointXInt = static_cast<int32>(pointX * DAVA::Renderer::GetFramebufferWidth());
        pointX *= w;

        gridPoly.AddPoint({ pointX, chartOffset.y * h });
        gridPoly.AddPoint({ pointX, (chartOffset.y + chartLen) * h });

        int32 textX = static_cast<int32>(vcs->ConvertVirtualToPhysicalX(pointX));
        int32 textY = static_cast<int32>(vcs->ConvertVirtualToPhysicalY((chartOffset.y + chartLen) * h));
        DbgDraw::Text2D(textX + vcsOffsetX, textY + vcsOffsetY, textColor, "%.2f", i * overdrawStep);
        RenderSystem2D::Instance()->DrawPolygon(gridPoly, false, gridColor);
    }
}

void ChartPainterSystem::DrawCharts(int32 w, int32 h) const
{
    Polygon2 chartsPoly;
    for (int i = 0; i < modsCount; i++)
    {
        chartsPoly.Clear();
        for (size_t j = 0; j < (*performanceData)[i].size(); j++)
        {
            if (j == 0) // Skip noise in the first frame.
                continue;

            float32 overdraw = (*performanceData)[i][j].Overdraw;
            float32 fps = static_cast<float32>((*performanceData)[i][j].FrameTime) - minFrametime;

            float32 normalizedFps = fps / frametimeAxisLen;
            float32 normalizedOverdraw = overdraw / maxOverdraw;

            normalizedFps *= chartLen;
            normalizedOverdraw *= chartLen;

            float32 pointX = (normalizedOverdraw + chartOffset.x);
            float32 pointY = 1 - (normalizedFps + chartOffset.y);
            pointX *= w;
            pointY *= h;

            chartsPoly.AddPoint({ pointX, pointY });
        }
        RenderSystem2D::Instance()->DrawPolygon(chartsPoly, false, chartColors[i]);
    }
}

void ChartPainterSystem::FlushDbgText()
{
    passConfig.viewport.width = DAVA::Renderer::GetFramebufferWidth();
    passConfig.viewport.height = DAVA::Renderer::GetFramebufferHeight();

    DbgDraw::SetScreenSize(DAVA::Renderer::GetFramebufferWidth(), DAVA::Renderer::GetFramebufferHeight());

    rhi::HPacketList packetList;
    rhi::HRenderPass pass = rhi::AllocateRenderPass(passConfig, 1, &packetList);
    rhi::BeginRenderPass(pass);
    rhi::BeginPacketList(packetList);

    DbgDraw::FlushBatched(packetList);

    rhi::EndPacketList(packetList);
    rhi::EndRenderPass(pass);
}

void ChartPainterSystem::ProcessPerformanceData(Array<Vector<FrameData>, 6>* performanceData_)
{
    performanceData = performanceData_;
    // use maxFrametime = GetMaxFrametime(); to get adaptive y axis
    UpdateChartParameters();
}

void ChartPainterSystem::UpdateChartParameters()
{
    frametimeAxisLen = maxFrametime - minFrametime;
    frametimeStepCount = frametimeAxisLen / frametimeStep;
}

void ChartPainterSystem::DrawLegend(int32 w, int32 h) const
{
    static const float offsetDivider = 14.0f;
    static const float stepDivider = 7.0f;
    static const float lineOffsetDivider = 9.0f;
    static const float yOffsetMultiplier = 0.05f;

    VirtualCoordinatesSystem* vcs = DAVA::GetEngineContext()->uiControlSystem->vcs;

    Vector2 vcsOffset = vcs->GetPhysicalDrawOffset();
    int32 vcsOffsetX = static_cast<int32>(vcsOffset.x);
    int32 vcsOffsetY = static_cast<int32>(vcsOffset.y);

    float32 initialOffset = static_cast<float32>(w) / offsetDivider;
    float32 textInitialOffset = static_cast<float32>(w) / offsetDivider;
    float32 step = static_cast<float32>(w) / stepDivider;
    float32 textStep = static_cast<float32>(w) / stepDivider;
    int32 lineOffset = static_cast<int32>(static_cast<float32>(w) / lineOffsetDivider);
    float32 yPosText = yOffsetMultiplier * h;
    float32 yPosFloat = yOffsetMultiplier * h;

    Polygon2 p;
    for (int i = 0; i < modsCount; i++)
    {
        p.Clear();

        float32 startX = textStep * i + textInitialOffset;
        float32 startXFloat = step * i + initialOffset;
        p.AddPoint({ startXFloat, yPosFloat });
        p.AddPoint({ startXFloat + lineOffset, yPosFloat });

        int32 textX = static_cast<int32>(vcs->ConvertVirtualToPhysicalX(startX));
        int32 textY = static_cast<int32>(vcs->ConvertVirtualToPhysicalY(yPosText));
        DbgDraw::Text2D(textX + vcsOffsetX, textY + vcsOffsetY, textColor, "%s", legend[i].c_str());
        RenderSystem2D::Instance()->DrawPolygon(p, false, chartColors[i]);
    }
}

float32 ChartPainterSystem::GetMaxFrametimeFromData() const
{
    // Looking for the max frametime element in whole data
    Array<float32, modsCount> frametimes;
    for (int i = 0; i < modsCount; i++)
    {
        Vector<FrameData>& currVector = (*performanceData)[i];
        auto begin = currVector.begin();
        begin++; // Skip noise in first frame.

        frametimes[i] = (*std::max_element(begin, currVector.end(),
                                           [](const FrameData& f1, const FrameData& f2)
                                           {
                                               return f1.FrameTime < f2.FrameTime;
                                           }
                                           ))
                        .FrameTime;
    }
    return *std::max_element(frametimes.begin(), frametimes.end());
}
#undef MODS_COUNT
}
