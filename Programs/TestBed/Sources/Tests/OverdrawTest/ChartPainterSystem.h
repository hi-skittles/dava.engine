#pragma once

#include "OverdrawTestingScreen.h"
#include "Base/BaseTypes.h"
#include "Entity/SceneSystem.h"

namespace OverdrawPerformanceTester
{
struct FrameData;

class ChartPainterSystem : public DAVA::SceneSystem
{
public:
    ChartPainterSystem(DAVA::Scene* scene, DAVA::float32 maxFrametime_);

    void AddEntity(DAVA::Entity* entity) override;
    void PrepareForRemove() override;

    void Process(DAVA::float32 timeElapsed) override;

    void DrawGrid(DAVA::int32 w, DAVA::int32 h) const;
    void DrawCharts(DAVA::int32 w, DAVA::int32 h) const;
    void FlushDbgText();

    void ProcessPerformanceData(DAVA::Array<DAVA::Vector<FrameData>, 6>* performanceData_);

    void UpdateChartParameters();

    inline void SetMaxFrametime(DAVA::float32 frametime);
    inline DAVA::float32 GetMaxFrametime() const;

private:
    void DrawLegend(DAVA::int32 w, DAVA::int32 h) const;
    DAVA::float32 GetMaxFrametimeFromData() const;

    rhi::RenderPassConfig passConfig;
    DAVA::Array<DAVA::Vector<FrameData>, 6>* performanceData = nullptr;

    DAVA::float32 maxOverdraw = 1000.0f;
    DAVA::float32 overdrawStepCount = 100.0f;
    DAVA::float32 maxFrametime = 0.033f;
    DAVA::float32 frametimeAxisLen = 1.0f;
    DAVA::float32 frametimeStepCount = 30;

    static const DAVA::Vector2 chartOffset;
    static const DAVA::Color gridColor;
    static const DAVA::float32 chartLen;
    static const DAVA::float32 minFrametime;
    static const DAVA::float32 overdrawStep;
    static const DAVA::float32 frametimeStep;
    static const DAVA::Array<DAVA::String, 6> legend;
    static const DAVA::Array<DAVA::Color, 6> chartColors;
    const DAVA::uint32 textColor;
};

void ChartPainterSystem::SetMaxFrametime(DAVA::float32 frametime)
{
    maxFrametime = frametime;
    UpdateChartParameters();
}

DAVA::float32 ChartPainterSystem::GetMaxFrametime() const
{
    return maxFrametime;
}
}
