#include "Tests/AssertTest.h"

#include <Debug/MessageBox.h>
#include <Engine/Engine.h>
#include <Job/JobManager.h>
#include <Logger/Logger.h>
#include <UI/Events/UIEventBindingComponent.h>
#include <UI/Update/UIUpdateComponent.h>

const static DAVA::float32 DEFAULT_TIMEOUT = 3.f;

AssertTest::AssertTest(TestBed& app)
    : BaseScreen(app, "AssertTest")
{
    GetOrCreateComponent<DAVA::UIUpdateComponent>();
}

void AssertTest::LoadResources()
{
    BaseScreen::LoadResources();

    DAVA::DefaultUIPackageBuilder pkgBuilder;
    DAVA::UIPackageLoader().LoadPackage("~res:/TestBed/UI/AssertTestScreen.yaml", &pkgBuilder);
    DAVA::UIControl* dialog = pkgBuilder.GetPackage()->GetControl("MainFrame");
    dialog->SetSize(DAVA::Vector2(500, 500));
    AddControl(dialog);

    auto actions = dialog->GetOrCreateComponent<DAVA::UIEventBindingComponent>();
    if (actions)
    {
        actions->BindAction(DAVA::FastName("ASSERT_ALWAYS"), [&](const DAVA::Any&) {
            DVASSERT_ALWAYS(false, "Demo assert");
        });
        actions->BindAction(DAVA::FastName("ASSERT"), [&](const DAVA::Any&) {
            DVASSERT(false, "Demo assert");
        });
        actions->BindAction(DAVA::FastName("DELAYED_ASSERT"), [&](const DAVA::Any&) {
            timeOut = DEFAULT_TIMEOUT;
        });

        actions->BindAction(DAVA::FastName("MSGBOX_MAINTHREAD"), DAVA::MakeFunction(this, &AssertTest::ShowMessageBoxFromMainThread));
        actions->BindAction(DAVA::FastName("MSGBOX_UITHREAD"), DAVA::MakeFunction(this, &AssertTest::ShowMessageBoxFromUIThread));
        actions->BindAction(DAVA::FastName("MSGBOX_OTHERTHREADS"), DAVA::MakeFunction(this, &AssertTest::ShowMessageBoxFromOtherThreads));
    }

    countdownText = static_cast<DAVA::UIStaticText*>(dialog->FindByName("Countdown"));
}

void AssertTest::UnloadResources()
{
    BaseScreen::UnloadResources();

    countdownText.Set(nullptr);
}

void AssertTest::Update(DAVA::float32 timeElapsed)
{
    BaseScreen::Update(timeElapsed);
    if (timeOut > 0.f)
    {
        timeOut -= timeElapsed;
        if (timeOut <= 0.f)
        {
            timeOut = 0.f;
            DVASSERT(false, "Demo assert");
        }
    }
    if (countdownText.Get())
    {
        countdownText->SetText(DAVA::Format(L"%1.1f", timeOut));
    }
}

void AssertTest::ShowMessageBoxFromMainThread(const DAVA::Any&)
{
    using namespace DAVA;
    int r = Debug::MessageBox("Message box", "Message box from main thread", { "Ping", "Pong", "Kaboom" }, 1);
    Logger::Debug("You choose button %d", r);
}

void AssertTest::ShowMessageBoxFromUIThread(const DAVA::Any&)
{
    using namespace DAVA;
    Window* primaryWindow = GetPrimaryWindow();
    primaryWindow->RunOnUIThreadAsync([]() {
        int r = Debug::MessageBox("Message box", "Message box from UI thread", { "Uno", "Duo", "Trio" }, 2);
        Logger::Debug("You choose button %d", r);
    });
}

void AssertTest::ShowMessageBoxFromOtherThreads(const DAVA::Any&)
{
    using namespace DAVA;
    JobManager* jobman = GetEngineContext()->jobManager;
    for (int i = 0; i < 3; ++i)
    {
        jobman->CreateWorkerJob([i]() {
            uint64 threadId = Thread::GetCurrentIdAsUInt64();
            String msg = Format("Message box #%d from thread 0x%llX", i, threadId);
            Vector<String> buttons;
            switch (i)
            {
            case 0:
                buttons = { "Click me" };
                break;
            case 1:
                buttons = { "To be", "Not to be" };
                break;
            case 2:
                buttons = { "Error", "Warning", "Info" };
                break;
            }
            int r = Debug::MessageBox("Message box", msg, buttons, i);
            Logger::Debug("You choose button %d [#%d thread=0x%llx]", r, i, threadId);
        });
    }
}
