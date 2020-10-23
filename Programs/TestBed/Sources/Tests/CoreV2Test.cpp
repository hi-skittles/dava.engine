#include "Tests/CoreV2Test.h"
#include "Infrastructure/TestBed.h"

#include <Engine/Engine.h>
#include <Logger/Logger.h>
#include <UI/Render/UIDebugRenderComponent.h>

using namespace DAVA;

CoreV2Test::CoreV2Test(TestBed& app)
    : BaseScreen(app, "CoreV2Test")
    , engine(app.GetEngine())
{
    dispatchers.reserve(4);
    for (int i = 0; i < 4; ++i)
    {
        dispatchers.push_back(std::unique_ptr<TestDispatcher>(new TestDispatcher(MakeFunction(this, &CoreV2Test::DispatcherEventHandler))));
        dispatcherThreads.emplace_back(Thread::Create([this, i]() {
            DispatcherThread(dispatchers[i].get(), i);
        }));
        dispatcherThreads.back()->Start();
    }
}

CoreV2Test::~CoreV2Test()
{
    stopDispatchers = true;
}

void CoreV2Test::LoadResources()
{
    BaseScreen::LoadResources();

    Font* font = FTFont::Create("~res:/TestBed/Fonts/korinna.ttf");

    float32 h = 60.0f;
    float32 gap = 10.0f;
    float32 y = 10.0f;
    buttonQuit = CreateUIButton(font, Rect(10, y, 200, h), "Quit", &CoreV2Test::OnQuit);
    buttonTerminate = CreateUIButton(font, Rect(10, y += h + gap, 200, h), "Terminate", &CoreV2Test::OnTerminate);
    buttonCloseWindow = CreateUIButton(font, Rect(10, y += h + gap, 200, h), "Close window", &CoreV2Test::OnCloseWindow);

    buttonResize640x480 = CreateUIButton(font, Rect(10, y += h + gap, 200, h), "Resize 640x480", &CoreV2Test::OnResize);
    buttonResize1024x768 = CreateUIButton(font, Rect(10, y += h + gap, 200, h), "Resize 1024x768", &CoreV2Test::OnResize);

    buttonRunOnMain = CreateUIButton(font, Rect(10, y += h + gap, 200, h), "Run on main", &CoreV2Test::OnRun);
    buttonRunOnUI = CreateUIButton(font, Rect(10, y += h + gap, 200, h), "Run on UI", &CoreV2Test::OnRun);

    y = 10.0f;
    buttonDispTrigger1 = CreateUIButton(font, Rect(250, y, 200, h), "Trigger 1", &CoreV2Test::OnDispatcherTest);
    buttonDispTrigger2 = CreateUIButton(font, Rect(250, y += h + gap, 200, h), "Trigger 2", &CoreV2Test::OnDispatcherTest);
    buttonDispTrigger3 = CreateUIButton(font, Rect(250, y += h + gap, 200, h), "Trigger 3", &CoreV2Test::OnDispatcherTest);
    buttonDispTrigger1000 = CreateUIButton(font, Rect(250, y += h + gap, 200, h), "Trigger 1000", &CoreV2Test::OnDispatcherTest);
    buttonDispTrigger2000 = CreateUIButton(font, Rect(250, y += h + gap, 200, h), "Trigger 2000", &CoreV2Test::OnDispatcherTest);
    buttonDispTrigger3000 = CreateUIButton(font, Rect(250, y += h + gap, 200, h), "Raise deadlock", &CoreV2Test::OnDispatcherTest);

    y = 10.0f;
    buttonDisableClose = CreateUIButton(font, Rect(500, y, 200, h), "Disable close", &CoreV2Test::OnDisableEnableClose);
    buttonEnableClose = CreateUIButton(font, Rect(500, y += h + gap, 200, h), "Enable close", &CoreV2Test::OnDisableEnableClose);

    buttonDisableScreenTimeout = CreateUIButton(font, Rect(500, y += h + gap, 200, h), "Disable screen timeout", &CoreV2Test::OnDisableEnableScreenTimeout);
    buttonEnableScreenTimeout = CreateUIButton(font, Rect(500, y += h + gap, 200, h), "Enable screen timeout", &CoreV2Test::OnDisableEnableScreenTimeout);

    engine.windowCreated.Connect(this, &CoreV2Test::OnWindowCreated);
    engine.windowDestroyed.Connect(this, &CoreV2Test::OnWindowDestroyed);
    engine.SetCloseRequestHandler(MakeFunction(this, &CoreV2Test::OnWindowWantsToClose));

    SafeRelease(font);
}

void CoreV2Test::UnloadResources()
{
    engine.windowCreated.Disconnect(this);
    engine.windowDestroyed.Disconnect(this);

    SafeRelease(buttonQuit);
    SafeRelease(buttonTerminate);
    SafeRelease(buttonCloseWindow);
    SafeRelease(buttonResize640x480);
    SafeRelease(buttonResize1024x768);
    SafeRelease(buttonRunOnMain);
    SafeRelease(buttonRunOnUI);
    SafeRelease(buttonDisableClose);
    SafeRelease(buttonEnableClose);
    SafeRelease(buttonDisableScreenTimeout);
    SafeRelease(buttonEnableScreenTimeout);

    SafeRelease(buttonDispTrigger1);
    SafeRelease(buttonDispTrigger2);
    SafeRelease(buttonDispTrigger3);
    SafeRelease(buttonDispTrigger1000);
    SafeRelease(buttonDispTrigger2000);
    SafeRelease(buttonDispTrigger3000);

    engine.SetCloseRequestHandler(nullptr);

    BaseScreen::UnloadResources();
}

void CoreV2Test::OnQuit(DAVA::BaseObject* obj, void* data, void* callerData)
{
    Logger::Info("CoreV2Test: sending quit...");
    engine.QuitAsync(4);
}

void CoreV2Test::OnTerminate(DAVA::BaseObject* obj, void* data, void* callerData)
{
    Logger::Info("CoreV2Test: terminating...");
    engine.Terminate();
}

void CoreV2Test::OnCloseWindow(DAVA::BaseObject* obj, void* data, void* callerData)
{
    Logger::Info("CoreV2Test: closing primary window...");
    engine.PrimaryWindow()->CloseAsync();
}

void CoreV2Test::OnResize(DAVA::BaseObject* obj, void* data, void* callerData)
{
    float32 w = 0.0f;
    float32 h = 0.0f;
    if (obj == buttonResize640x480)
    {
        w = 640.0f;
        h = 480.0f;
    }
    else if (obj == buttonResize1024x768)
    {
        w = 1024.0f;
        h = 768.0f;
    }
    engine.PrimaryWindow()->SetSizeAsync({ w, h });
}

void CoreV2Test::OnDisableEnableClose(DAVA::BaseObject* obj, void* data, void* callerData)
{
    if (obj == buttonDisableClose)
    {
        Logger::Debug("Closing application or window by user is disabled");
        closeDisabled = true;
    }
    else if (obj == buttonEnableClose)
    {
        Logger::Debug("Closing application or window by user is enabled");
        closeDisabled = false;
    }
}

void CoreV2Test::OnDisableEnableScreenTimeout(DAVA::BaseObject* obj, void* data, void* callerData)
{
    if (obj == buttonDisableScreenTimeout)
    {
        Logger::Debug("Screen timeout is disabled");
        engine.SetScreenTimeoutEnabled(false);
    }
    else if (obj == buttonEnableScreenTimeout)
    {
        Logger::Debug("Screen timeout is enabled");
        engine.SetScreenTimeoutEnabled(true);
    }
}

void CoreV2Test::OnRun(DAVA::BaseObject* obj, void* data, void* callerData)
{
    if (obj == buttonRunOnMain)
    {
        RunOnMainThreadAsync([]() {
            Logger::Error("******** KABOOM on main thread********");
        });
    }
    else if (obj == buttonRunOnUI)
    {
        RunOnUIThreadAsync([]() {
            Logger::Error("******** KABOOM on UI thread********");
        });
    }
}

void CoreV2Test::OnDispatcherTest(DAVA::BaseObject* obj, void* data, void* callerData)
{
    TestDispatcher* disp = dispatchers[0].get();
    if (obj == buttonDispTrigger1)
    {
        disp->PostEvent(1);
    }
    else if (obj == buttonDispTrigger2)
    {
        disp->PostEvent(2);
    }
    else if (obj == buttonDispTrigger3)
    {
        disp->PostEvent(3);
    }
    else if (obj == buttonDispTrigger1000)
    {
        disp->PostEvent(1000);
    }
    else if (obj == buttonDispTrigger2000)
    {
        disp->PostEvent(2000);
    }
    else if (obj == buttonDispTrigger3000)
    {
        disp->PostEvent(3000);
    }
}

void CoreV2Test::DispatcherThread(TestDispatcher* dispatcher, int index)
{
    dispatcher->LinkToCurrentThread();
    Logger::Debug("###### CoreV2Test::DispatcherThread enter: thread=%llu, index=%d", Thread::GetCurrentIdAsUInt64(), index);
    while (!stopDispatchers)
    {
        dispatcher->ProcessEvents();
        Thread::Sleep(50);
    }
    Logger::Debug("###### CoreV2Test::DispatcherThread leave: thread=%llu, index=%d", Thread::GetCurrentIdAsUInt64(), index);
}

void CoreV2Test::DispatcherEventHandler(int type)
{
    Logger::Debug("###### CoreV2Test::EventHandler: thread=%llu, type=%d", Thread::GetCurrentIdAsUInt64(), type);
    if (type == 1 || type == 2 || type == 3)
    {
        // 1: post to even dispatchers
        // 2: post to odd dispatchers
        // 3: broadcast
        for (size_t i = 0, n = dispatchers.size(); i < n; ++i)
        {
            if ((type == 1 && (i & 1) == 0) || (type == 2 && (i & 1) == 1) || type == 3)
            {
                dispatchers[i]->PostEvent(4);
            }
        }
    }
    else if (type >= 3000)
    {
        // chained send event with deadlock
        size_t index = type - 3000;
        if (index < dispatchers.size())
        {
            dispatchers[index]->SendEvent(type + 1);
        }
        else
        {
            dispatchers[0]->SendEvent(10000);
        }
    }
    else if (type >= 2000)
    {
        // chained send event
        size_t index = type - 2000;
        if (index < dispatchers.size())
        {
            dispatchers[index]->SendEvent(type + 1);
        }
    }
    else if (type >= 1000)
    {
        // chained post event
        size_t index = type - 1000;
        if (index < dispatchers.size())
        {
            dispatchers[index]->PostEvent(type + 1);
        }
    }
}

void CoreV2Test::OnWindowCreated(DAVA::Window* w)
{
    Logger::Debug("****** CoreV2Test::OnWindowCreated");
}

bool CoreV2Test::OnWindowWantsToClose(DAVA::Window* w)
{
    if (closeDisabled)
    {
        if (w == nullptr)
        {
            Logger::Debug("User is trying to close application. Deny");
        }
        else
        {
            Logger::Debug("User is trying to close window. Deny");
        }
    }
    return !closeDisabled;
}

void CoreV2Test::OnWindowDestroyed(DAVA::Window* w)
{
    Logger::Debug("****** CoreV2Test::OnWindowDestroyed");
}

DAVA::UIButton* CoreV2Test::CreateUIButton(DAVA::Font* font, const DAVA::Rect& rect, const DAVA::String& text,
                                           void (CoreV2Test::*onClick)(DAVA::BaseObject*, void*, void*))
{
    using namespace DAVA;

    UIButton* button = new UIButton(rect);
    button->SetStateFont(0xFF, font);
    button->SetStateFontSize(0xFF, 12.f);
    button->SetStateText(0xFF, UTF8Utils::EncodeToWideString(text));
    button->SetStateFontColor(0xFF, Color::White);
    button->GetOrCreateComponent<UIDebugRenderComponent>();
    button->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, onClick));
    AddControl(button);
    return button;
}
