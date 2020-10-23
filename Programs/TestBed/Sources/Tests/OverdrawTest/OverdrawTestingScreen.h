#pragma once

#include "Base/BaseTypes.h"
#include "Math/Rect.h"
#include "UI/UIScreen.h"

class TestBed;

namespace DAVA
{
class Scene;
class UIButton;
class FTFont;
class Message;
}

namespace OverdrawPerformanceTester
{
class OverdrawTesterSystem;
class ChartPainterSystem;

class OverdrawTestingScreen : public DAVA::UIScreen
{
public:
    OverdrawTestingScreen(TestBed& app_);
    void LoadResources() override;
    void UnloadResources() override;

private:
    void AddButtons();
    void OnChangeChartHeightButtonClick(BaseObject* sender, void* data, void* callerData);
    void OnExitButton(DAVA::BaseObject* obj, void* data, void* callerData);
    DAVA::UIButton* CreateButton(const DAVA::Rect&& rect, const DAVA::Message&& msg, const DAVA::WideString&& caption, const DAVA::int32 tag);

    DAVA::UIButton* exitButton = nullptr;
    DAVA::UIButton* frameTimePlusButton = nullptr;
    DAVA::UIButton* frameTimeMinusButton = nullptr;
    DAVA::Size2i initialVcsSize = {};
    DAVA::Scene* scene = nullptr;
    DAVA::FTFont* font = nullptr;
    TestBed& app;

    OverdrawPerformanceTester::OverdrawTesterSystem* testerSystem;
    OverdrawPerformanceTester::ChartPainterSystem* chartPainterSystem;

    static const DAVA::float32 buttonWidth;
    static const DAVA::float32 buttonHeight;
    static const DAVA::float32 minFrametimeThreshold;
    static const DAVA::float32 frametimeIncreaseStep;
};
}