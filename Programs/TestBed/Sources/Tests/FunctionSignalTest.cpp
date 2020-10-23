#include <functional>
#include <time.h>
#include "Tests/FunctionSignalTest.h"
#include "UI/Render/UIDebugRenderComponent.h"

using namespace DAVA;

template <typename F, typename... A>
String BenchTest(const char* name, F& f, A... a)
{
    int st_count = 99999999;
    int res = 0;

    clock_t time_ms = clock();
    for (int i = 0; i < st_count; ++i)
    {
        res += f(std::forward<A>(a)..., i);
    }

    return Format("%s: %lu ms, res = %d\n", name, clock() - time_ms, res);
}

FunctionSignalTest::FunctionSignalTest(TestBed& app)
    : BaseScreen(app, "FunctionSignalTest")
{
}

DAVA_NOINLINE int test(int a, int b, int i)
{
    return (i * (a + b));
}

struct TestStruct
{
    DAVA_NOINLINE int test(int a, int b, int i)
    {
        return (i * (a + b));
    }
};

void FunctionSignalTest::LoadResources()
{
    BaseScreen::LoadResources();

    Font* fontKorinna = FTFont::Create("~res:/TestBed/Fonts/korinna.ttf");
    DVASSERT(fontKorinna);
    Font* fontDroid = FTFont::Create("~res:/TestBed/Fonts/DroidSansMono.ttf");
    DVASSERT(fontDroid);

    runResult = new UIStaticText(Rect(10, 10, 450, 600));
    runResult->SetFont(fontDroid);
    runResult->SetFontSize(14.f);
    runResult->SetTextColor(Color::White);
    runResult->GetOrCreateComponent<UIDebugRenderComponent>();
    runResult->SetMultiline(true);
    runResult->SetTextAlign(ALIGN_LEFT | ALIGN_TOP);
    AddControl(runResult);

    runButton = new UIButton(Rect(10, 620, 450, 60));
    runButton->SetStateFont(0xFF, fontKorinna);
    runButton->SetStateFontSize(0xFF, 30.f);
    runButton->SetStateFontColor(0xFF, Color::White);
    runButton->SetStateText(0xFF, L"Start bench test");
    runButton->SetStateText(UIButton::STATE_DISABLED, L"Running...");
    runButton->SetDisabled(false);
    runButton->GetOrCreateComponent<UIDebugRenderComponent>();
    runButton->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &FunctionSignalTest::OnButtonPress));
    AddControl(runButton);
}

void FunctionSignalTest::UnloadResources()
{
    SafeRelease(runResult);
    SafeRelease(runButton);

    BaseScreen::UnloadResources();
}

DAVA_NOINLINE int viewStdFnCall(std::function<int(int, int, int)>& fn)
{
    return fn(10, 20, 30);
}

DAVA_NOINLINE int viewDavaFnCall(Function<int(int, int, int)>& fn)
{
    return fn(10, 20, 30);
}

void FunctionSpeedTestJob(FunctionSignalTest* fst)
{
    TestStruct ts;

    std::function<int(int, int, int)> stdStatic(&test);
    Function<int(int, int, int)> davaStatic(&test);

    // this code is using to be able to see what assembly code is
    // genered for std::function and DAVA::Function
    {
        int rrr = viewStdFnCall(stdStatic);
        rrr += viewDavaFnCall(davaStatic);

        // printf to make sure rrr variable isn't erased by c++ compiler|linker
        printf("%d\n", rrr);
    }

    GetEngineContext()->jobManager->CreateMainJob([fst] {
        fst->runResult->SetText(L"Started...");
    });

    String resStr = "Static function:\n";
    resStr += BenchTest("  native", test, 10, 20);
    resStr += BenchTest("  std   ", stdStatic, 10, 20);
    resStr += BenchTest("  dava  ", davaStatic, 10, 20);

    GetEngineContext()->jobManager->CreateMainJob([fst, resStr] {
        fst->runResult->SetText(UTF8Utils::EncodeToWideString(resStr));
    });

    int cap1 = 10, cap2 = 20;
    auto lam = [&cap1, &cap2](int index) -> int { return test(cap1, cap2, index); };
    std::function<int(int)> stdLam(lam);
    Function<int(int)> davaLam(lam);

    resStr += "\nLambda function:\n";
    resStr += BenchTest("  native", lam);
    resStr += BenchTest("  std   ", stdLam);
    resStr += BenchTest("  dava  ", davaLam);

    GetEngineContext()->jobManager->CreateMainJob([fst, resStr] {
        fst->runResult->SetText(UTF8Utils::EncodeToWideString(resStr));
    });

    auto nativeCls = [](TestStruct* ts, int a, int b, int i) -> int { return ts->test(a, b, i); };
    std::function<int(TestStruct * ts, int, int, int)> stdCls(std::mem_fn(&TestStruct::test));
    Function<int(TestStruct * ts, int, int, int)> davaCls(&TestStruct::test);

    resStr += "\nClass function:\n";
    resStr += BenchTest("  native", nativeCls, &ts, 10, 20);
    resStr += BenchTest("  std   ", stdCls, &ts, 10, 20);
    resStr += BenchTest("  dava  ", davaCls, &ts, 10, 20);

    GetEngineContext()->jobManager->CreateMainJob([fst, resStr] {
        fst->runResult->SetText(UTF8Utils::EncodeToWideString(resStr));
    });

    auto nativeObj = [&ts](int a, int b, int i) -> int { return ts.test(a, b, i); };
    std::function<int(int, int, int)> stdObj(std::bind(&TestStruct::test, &ts, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    Function<int(int, int, int)> davaObj(&ts, &TestStruct::test);

    resStr += "\nObj function:\n";
    resStr += BenchTest("  native", nativeObj, 10, 20);
    resStr += BenchTest("  std   ", stdObj, 10, 20);
    resStr += BenchTest("  dava  ", davaObj, 10, 20);

    GetEngineContext()->jobManager->CreateMainJob([fst, resStr] {
        fst->runResult->SetText(UTF8Utils::EncodeToWideString(resStr));
    });

    auto nativeBind = std::bind(&TestStruct::test, &ts, 10, 20, std::placeholders::_1);
    std::function<int(int)> stdBind = std::bind(&TestStruct::test, &ts, 10, 20, std::placeholders::_1);
    Function<int(int)> davaBind = std::bind(&TestStruct::test, &ts, 10, 20, std::placeholders::_1);

    resStr += "\nBinded function:\n";
    resStr += BenchTest("  native", nativeBind);
    resStr += BenchTest("  std   ", stdBind);
    resStr += BenchTest("  dava  ", davaBind);

    resStr += "\n\nDone!";

    GetEngineContext()->jobManager->CreateMainJob([fst, resStr] {
        fst->runResult->SetText(UTF8Utils::EncodeToWideString(resStr));
        fst->runButton->SetDisabled(false);
    });
}

void FunctionSignalTest::OnButtonPress(BaseObject* obj, void* data, void* callerData)
{
    if (!runButton->GetDisabled())
    {
        runButton->SetDisabled(true);
        GetEngineContext()->jobManager->CreateWorkerJob(std::bind(&FunctionSpeedTestJob, this));
    }
}

void FunctionSignalTest::OnExitButton(BaseObject* obj, void* data, void* callerData)
{
    if (!GetEngineContext()->jobManager->HasWorkerJobs())
    {
        BaseScreen::OnExitButton(obj, data, callerData);
    }
}
