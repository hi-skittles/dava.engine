#include "Infrastructure/BaseScreen.h"
#include "Infrastructure/TestBed.h"

#include <Engine/Engine.h>
#include <UI/Layouts/UIAnchorComponent.h>
#include <UI/Render/UIDebugRenderComponent.h>

DAVA::int32 BaseScreen::globalScreenId = 1;

BaseScreen::BaseScreen(TestBed& app, const DAVA::String& screenName)
    : UIScreen()
    , app(app)
    , currentScreenId(globalScreenId++)
{
    SetName(screenName);
    app.RegisterScreen(this);
}

void BaseScreen::OnBackNavigation(DAVA::Window* window)
{
    OnExitButton(nullptr, nullptr, nullptr);
}

void BaseScreen::LoadResources()
{
    using namespace DAVA;
    ScopedPtr<FTFont> font(FTFont::Create("~res:/TestBed/Fonts/korinna.ttf"));

    Size2i screenSize = GetEngineContext()->uiControlSystem->vcs->GetVirtualScreenSize();
    exitButton = new UIButton(Rect(static_cast<DAVA::float32>(screenSize.dx - 300), static_cast<DAVA::float32>(screenSize.dy - 30), 300.0, 30.0));
    exitButton->SetStateFont(0xFF, font);
    exitButton->SetStateFontSize(0xFF, 30.f);
    exitButton->SetStateFontColor(0xFF, Color::White);
    exitButton->SetStateText(0xFF, L"Exit From Screen");

    exitButton->GetOrCreateComponent<UIDebugRenderComponent>();
    exitButton->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &BaseScreen::OnExitButton));

    {
        // Stick button to bottom right corner
        UIAnchorComponent* anchor = exitButton->GetOrCreateComponent<UIAnchorComponent>();
        anchor->SetBottomAnchorEnabled(true);
        anchor->SetRightAnchorEnabled(true);
    }

    AddControl(exitButton);

    DAVA::GetPrimaryWindow()->backNavigation.Connect(this, &BaseScreen::OnBackNavigation);
}

void BaseScreen::UnloadResources()
{
    RemoveAllControls();

    SafeRelease(exitButton);

    DAVA::GetPrimaryWindow()->backNavigation.Disconnect(this);

    UIScreen::UnloadResources();
}

void BaseScreen::OnExitButton(BaseObject* obj, void* data, void* callerData)
{
    app.ShowStartScreen();
}
