#include "Tests/FloatingPointExceptionTest.h"
#include "UI/Render/UIDebugRenderComponent.h"

#include <numeric>

FloatingPointExceptionTest::FloatingPointExceptionTest(TestBed& app)
    : BaseScreen(app, "FloatingPointExceptionTest")
{
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4723)
#pragma warning(disable : 4756)
#endif
void DoFloatingPointException(DAVA::BaseObject*, void*, void*)
{
    using namespace DAVA;
    Logger::Debug("start floating point test");

    float32 max_value = std::numeric_limits<float>::max();
    float32 min_value = std::numeric_limits<float>::min();
    float32 inf = std::numeric_limits<float>::infinity();
    try
    {
        float32 value = max_value / 0.f;
        Logger::Debug("value: max_value / 0.f == %f", value);
    }
    catch (std::exception& ex)
    {
        Logger::Debug("catch floating point exception: %s", ex.what());
    }

    try
    {
        float32 value = max_value * max_value;
        Logger::Debug("value: max_value * max_value == %f", value);
    }
    catch (std::exception& ex)
    {
        Logger::Debug("catch floating point exception: %s", ex.what());
    }

    try
    {
        float32 value = min_value / max_value;
        Logger::Debug("value: min_value / max_value == %f", value);
    }
    catch (std::exception& ex)
    {
        Logger::Debug("catch floating point exception: %s", ex.what());
    }

    try
    {
        float32 value = inf * 0.f;
        Logger::Debug("value: inf * 0.f == %f", value);
    }
    catch (std::exception& ex)
    {
        Logger::Debug("catch floating point exception: %s", ex.what());
    }

    Logger::Debug("finish floating point test");
}

void FloatingPointExceptionTest::LoadResources()
{
    using namespace DAVA;
    BaseScreen::LoadResources();

    ScopedPtr<FTFont> font(FTFont::Create("~res:/TestBed/Fonts/korinna.ttf"));

    ScopedPtr<UIButton> resetButton(new UIButton(Rect(420, 30, 200, 30)));
    resetButton->GetOrCreateComponent<UIDebugRenderComponent>();
    resetButton->SetStateFont(0xFF, font);
    resetButton->SetStateFontColor(0xFF, Color::White);
    resetButton->SetStateText(0xFF, L"Generate Floating point exception");
    resetButton->AddEvent(UIButton::EVENT_TOUCH_DOWN, Message(&DoFloatingPointException));
    AddControl(resetButton.get());
}

void FloatingPointExceptionTest::UnloadResources()
{
    RemoveAllControls();

    BaseScreen::UnloadResources();
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
