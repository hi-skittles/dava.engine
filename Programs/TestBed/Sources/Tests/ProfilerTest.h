#pragma once

#include "Infrastructure/BaseScreen.h"
#include "Debug/ProfilerCPU.h"

namespace DAVA
{
class UIButton;
class UIStaticText;
class Font;
class Color;
class Scene;
class ProfilerGPU;
}

class TestBed;
class ProfilerTest : public BaseScreen
{
public:
    ProfilerTest(TestBed& app);

protected:
    void LoadResources() override;
    void UnloadResources() override;
    void Update(DAVA::float32 timeElapsed) override;

private:
    void TestFunction0();
    void TestFunction1();
    void TestFunction2();

    void OnGlobalCPUProfilerOverlay(BaseObject* sender, void* data, void* callerData);
    void OnCustomCPUProfilerOverlay(BaseObject* sender, void* data, void* callerData);
    void OnCustomCPUProfiler(BaseObject* sender, void* data, void* callerData);
    void OnGlobalCPUProfiler(BaseObject* sender, void* data, void* callerData);
    void OnGPUProfiler(BaseObject* sender, void* data, void* callerData);

    void OnMakeSnapshot(BaseObject* sender, void* data, void* callerData);
    void OnDumpAverage(BaseObject* sender, void* data, void* callerData);
    void OnDumpAverageSnapshot(BaseObject* sender, void* data, void* callerData);

    void OnDumpJSON(BaseObject* sender, void* data, void* callerData);
    void OnDumpGlobalCPUGPU(BaseObject* sender, void* data, void* callerData);

    void DumpAverageToUI(DAVA::ProfilerCPU* profiler, DAVA::int32 snapshotID);

    DAVA::UIButton* CreateButton(const DAVA::Rect& rect, const DAVA::WideString& text, const DAVA::Message& msg);
    DAVA::UIStaticText* CreateStaticText(const DAVA::Rect& rect, const DAVA::WideString& text, DAVA::Font* font, DAVA::float32 fontSize, const DAVA::Color& color);

    DAVA::Font* textFont = nullptr;
    DAVA::Font* dumpFont = nullptr;
    DAVA::Scene* scene = nullptr;

    DAVA::ProfilerCPU* customCPUProfiler = nullptr;
    DAVA::int32 snapshotID = DAVA::ProfilerCPU::NO_SNAPSHOT_ID;

    DAVA::Vector<DAVA::FastName> defaultCPUMarkers;

    DAVA::UIStaticText* profilersText[6];
    DAVA::UIStaticText* dumpText;
    DAVA::UIScrollView* dumpScrollView;
};
