#pragma once

#include "Infrastructure/BaseScreen.h"

#include <Concurrency/Dispatcher.h>

namespace DAVA
{
class Engine;

class UIButton;
class Font;
struct Rect;
class BaseObject;
class Thread;
};

class TestBed;
class CoreV2Test : public BaseScreen
{
public:
    CoreV2Test(TestBed& app);
    ~CoreV2Test();

protected:
    void LoadResources() override;
    void UnloadResources() override;

private:
    void OnQuit(DAVA::BaseObject* obj, void* data, void* callerData);
    void OnTerminate(DAVA::BaseObject* obj, void* data, void* callerData);
    void OnCloseWindow(DAVA::BaseObject* obj, void* data, void* callerData);

    void OnResize(DAVA::BaseObject* obj, void* data, void* callerData);
    void OnRun(DAVA::BaseObject* obj, void* data, void* callerData);
    void OnDispatcherTest(DAVA::BaseObject* obj, void* data, void* callerData);

    void OnDisableEnableClose(DAVA::BaseObject* obj, void* data, void* callerData);
    void OnDisableEnableScreenTimeout(DAVA::BaseObject* obj, void* data, void* callerData);

    void OnWindowCreated(DAVA::Window* w);
    bool OnWindowWantsToClose(DAVA::Window* w);
    void OnWindowDestroyed(DAVA::Window* w);

    DAVA::UIButton* CreateUIButton(DAVA::Font* font, const DAVA::Rect& rect, const DAVA::String& text,
                                   void (CoreV2Test::*onClick)(DAVA::BaseObject*, void*, void*));

private:
    DAVA::Engine& engine;

    DAVA::UIButton* buttonQuit = nullptr;
    DAVA::UIButton* buttonTerminate = nullptr;
    DAVA::UIButton* buttonCloseWindow = nullptr;

    DAVA::UIButton* buttonResize640x480 = nullptr;
    DAVA::UIButton* buttonResize1024x768 = nullptr;

    DAVA::UIButton* buttonRunOnMain = nullptr;
    DAVA::UIButton* buttonRunOnUI = nullptr;

    DAVA::UIButton* buttonDispTrigger1 = nullptr;
    DAVA::UIButton* buttonDispTrigger2 = nullptr;
    DAVA::UIButton* buttonDispTrigger3 = nullptr;
    DAVA::UIButton* buttonDispTrigger1000 = nullptr;
    DAVA::UIButton* buttonDispTrigger2000 = nullptr;
    DAVA::UIButton* buttonDispTrigger3000 = nullptr;

    DAVA::UIButton* buttonDisableClose = nullptr;
    DAVA::UIButton* buttonEnableClose = nullptr;

    DAVA::UIButton* buttonDisableScreenTimeout = nullptr;
    DAVA::UIButton* buttonEnableScreenTimeout = nullptr;

    bool closeDisabled = false;

    //////////////////////////////////////////////////////////////////////////
    using TestDispatcher = DAVA::Dispatcher<int>;

    DAVA::Vector<DAVA::RefPtr<DAVA::Thread>> dispatcherThreads;
    DAVA::Vector<std::unique_ptr<TestDispatcher>> dispatchers;
    bool stopDispatchers = false;

    void DispatcherThread(TestDispatcher* dispatcher, int index);
    void DispatcherEventHandler(int type);
};
