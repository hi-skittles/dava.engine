#include "Base/Any.h"
#include "Tests/AnyPerformanceTest.h"
#include "UI/Focus/UIFocusComponent.h"
#include "UI/Render/UIDebugRenderComponent.h"

using namespace DAVA;

AnyPerformanceTest::AnyPerformanceTest(TestBed& app)
    : BaseScreen(app, "AnyPerformanceTest")
{
}

void AnyPerformanceTest::LoadResources()
{
    using namespace DAVA;

    BaseScreen::LoadResources();

    ScopedPtr<FTFont> font(FTFont::Create("~res:/TestBed/Fonts/korinna.ttf"));

    float y = 10;
    float h = 35;
    float dy = h + 7;

    ScopedPtr<UIStaticText> uitext(new UIStaticText(Rect(10, y, 100, h)));
    uitext->SetText(L"Loop count:");
    uitext->SetFont(font);
    uitext->SetFontSize(14.f);
    AddControl(uitext);

    testCount = new UITextField(Rect(110, y, 100, h));
    testCount->GetOrCreateComponent<UIDebugRenderComponent>();
    testCount->SetFont(font);
    testCount->SetFontSize(14.f);
    testCount->SetInputEnabled(true);
    testCount->SetText(L"1000");
    testCount->GetOrCreateComponent<UIFocusComponent>();
    AddControl(testCount);

    y += dy;
    ScopedPtr<UIButton> testCreate(new UIButton(Rect(10, y, 200, h)));
    testCreate->GetOrCreateComponent<UIDebugRenderComponent>();
    testCreate->SetStateFont(0xFF, font);
    testCreate->SetStateFontSize(0xFF, 14.f);
    testCreate->SetStateFontColor(0xFF, Color::White);
    testCreate->SetStateText(0xFF, L"CreateTest");
    testCreate->SetStateFontColor(UIButton::STATE_PRESSED_INSIDE, Color(0, 1.0f, 1.0f, 1.0f));
    testCreate->AddEvent(UIButton::EVENT_TOUCH_DOWN, Message(this, &AnyPerformanceTest::OnCreateTest));
    resultCreate = new UIStaticText(Rect(210, y, 200, h));
    resultCreate->SetFont(font);
    AddControl(testCreate);
    AddControl(resultCreate);

    y += dy;
    ScopedPtr<UIButton> testGetSet(new UIButton(Rect(10, y, 200, h)));
    testGetSet->GetOrCreateComponent<UIDebugRenderComponent>();
    testGetSet->SetStateFont(0xFF, font);
    testGetSet->SetStateFontSize(0xFF, 14.f);
    testGetSet->SetStateFontColor(0xFF, Color::White);
    testGetSet->SetStateText(0xFF, L"GetSetTest");
    testGetSet->SetStateFontColor(UIButton::STATE_PRESSED_INSIDE, Color(0, 1.0f, 1.0f, 1.0f));
    testGetSet->AddEvent(UIButton::EVENT_TOUCH_DOWN, Message(this, &AnyPerformanceTest::OnGetSetTest));
    resultGetSet = new UIStaticText(Rect(210, y, 200, h));
    resultGetSet->SetFont(font);
    AddControl(testGetSet);
    AddControl(resultGetSet);
}

void AnyPerformanceTest::UnloadResources()
{
    SafeRelease(testCount);
    SafeRelease(resultCreate);
    SafeRelease(resultGetSet);

    BaseScreen::UnloadResources();
}

DAVA::uint64 AnyPerformanceTest::GetLoopCount()
{
    int res = 0;

    auto str = testCount->GetText();
    sscanf(DAVA::UTF8Utils::EncodeToUTF8(str).c_str(), "%u", &res);

    return res;
}

void AnyPerformanceTest::SetResult(DAVA::UIStaticText* st, DAVA::uint64 ms)
{
    st->SetText(DAVA::Format(L"%u ms", ms));
}

void AnyPerformanceTest::OnCreateTest(DAVA::BaseObject* sender, void* data, void* callerData)
{
    using namespace DAVA;

    const Type* type = nullptr;

    uint64 sz = GetLoopCount();
    uint64 startMs = SystemTimer::GetMs();
    for (uint64 i = 0; i < sz; ++i)
    {
        Any a(i);
        type = a.GetType();
    }
    uint64 endMs = SystemTimer::GetMs();

    Logger::FrameworkDebug("%p", type);
    SetResult(resultCreate, endMs - startMs);
}

void AnyPerformanceTest::OnGetSetTest(DAVA::BaseObject* sender, void* data, void* callerData)
{
    using namespace DAVA;

    Any a(float32(0));

    uint64 sz = GetLoopCount();
    uint64 startMs = SystemTimer::GetMs();
    for (uint64 i = 0; i < sz; ++i)
    {
        float32 v = a.Get<float32>();
        v += static_cast<float32>(i);
        a.Set(v);
    }
    uint64 endMs = SystemTimer::GetMs();

    Logger::FrameworkDebug("%f", a.Get<float32>());
    SetResult(resultGetSet, endMs - startMs);
}
