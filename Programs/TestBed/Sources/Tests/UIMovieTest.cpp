#include "Tests/UIMovieTest.h"
#include "Engine/Engine.h"
#include "UI/Update/UIUpdateComponent.h"
#include "UI/Render/UIDebugRenderComponent.h"

namespace
{
FilePath path = "~res:/TestBed/TestData/MovieTest/bunny.m4v";
}

UIMovieTest::UIMovieTest(TestBed& app)
    : BaseScreen(app, "UIMovieTest")
{
    GetOrCreateComponent<UIUpdateComponent>();
}

void UIMovieTest::LoadResources()
{
    Size2f vsz = GetPrimaryWindow()->GetVirtualSize();

    movieView = new UIMovieView(Rect(10, 10, vsz.dx - 20, vsz.dy - 120));
    movieView->OpenMovie(path, OpenMovieParams());

    movieView->GetOrCreateComponent<UIDebugRenderComponent>();
    AddControl(movieView);

    // Create the "player" buttons.
    Font* font = FTFont::Create("~res:/TestBed/Fonts/korinna.ttf");
    DVASSERT(font);

    float32 y = vsz.dy - 90;

    playButton = CreateUIButton(font, Rect(10, y, 60, 40), "Play", &UIMovieTest::ButtonPressed);
    stopButton = CreateUIButton(font, Rect(80, y, 60, 40), "Stop", &UIMovieTest::ButtonPressed);
    pauseButton = CreateUIButton(font, Rect(150, y, 60, 40), "Pause", &UIMovieTest::ButtonPressed);
    resumeButton = CreateUIButton(font, Rect(220, y, 60, 40), "Resume", &UIMovieTest::ButtonPressed);
    hideButton = CreateUIButton(font, Rect(290, y, 60, 40), "Hide", &UIMovieTest::ButtonPressed);
    showButton = CreateUIButton(font, Rect(360, y, 60, 40), "Show", &UIMovieTest::ButtonPressed);

    y += 50;

    buttonScale0 = CreateUIButton(font, Rect(10, y, 100, 40), "None", &UIMovieTest::ScaleButtonPressed);
    buttonScale1 = CreateUIButton(font, Rect(120, y, 100, 40), "Aspect fit", &UIMovieTest::ScaleButtonPressed);
    buttonScale2 = CreateUIButton(font, Rect(220, y, 100, 40), "Aspect fill", &UIMovieTest::ScaleButtonPressed);
    buttonScale3 = CreateUIButton(font, Rect(320, y, 100, 40), "Fill", &UIMovieTest::ScaleButtonPressed);

    playerStateText = new UIStaticText(Rect(vsz.dx - 100, vsz.dy - 110, 100, 20));
    playerStateText->SetFont(font);
    playerStateText->SetFontSize(14.f);
    AddControl(playerStateText);

    SafeRelease(font);

    BaseScreen::LoadResources();
}

void UIMovieTest::UnloadResources()
{
    RemoveAllControls();

    SafeRelease(movieView);

    SafeRelease(playButton);
    SafeRelease(stopButton);
    SafeRelease(pauseButton);
    SafeRelease(resumeButton);
    SafeRelease(hideButton);
    SafeRelease(showButton);

    SafeRelease(buttonScale0);
    SafeRelease(buttonScale1);
    SafeRelease(buttonScale2);
    SafeRelease(buttonScale2);

    SafeRelease(playerStateText);
}

void UIMovieTest::OnActive()
{
    movieView->Play();
}

void UIMovieTest::Update(float32 timeElapsed)
{
    UpdatePlayerStateText();
    BaseScreen::Update(timeElapsed);
}

void UIMovieTest::UpdatePlayerStateText()
{
    bool isPlaying = movieView->IsPlaying();
    switch (movieView->GetState())
    {
    case eMoviePlayingState::stateStopped:
        playerStateText->SetText(L"Stopped");
        break;
    case eMoviePlayingState::stateLoading:
        playerStateText->SetText(L"Loading");
        break;
    case eMoviePlayingState::statePaused:
        playerStateText->SetText(L"Paused");
        break;
    case eMoviePlayingState::statePlaying:
        playerStateText->SetText(L"Playing");
        break;
    default:
        playerStateText->SetText(L"Unknown state");
        break;
    }
}

UIButton* UIMovieTest::CreateUIButton(Font* font, const Rect& rect, const String& text,
                                      void (UIMovieTest::*onClick)(BaseObject*, void*, void*))
{
    UIButton* button = new UIButton(rect);
    button->SetStateFont(0xFF, font);
    button->SetStateFontSize(0xFF, 14.f);
    button->SetStateText(0xFF, UTF8Utils::EncodeToWideString(text));
    button->SetStateFontColor(0xFF, Color::White);
    button->GetOrCreateComponent<UIDebugRenderComponent>();
    button->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, onClick));
    AddControl(button);
    return button;
}

void UIMovieTest::ButtonPressed(BaseObject* obj, void* data, void* callerData)
{
    if (obj == playButton)
        movieView->Play();
    else if (obj == stopButton)
        movieView->Stop();
    else if (obj == pauseButton)
        movieView->Pause();
    else if (obj == resumeButton)
        movieView->Resume();
    else if (obj == hideButton)
        movieView->SetVisibilityFlag(false);
    else if (obj == showButton)
        movieView->SetVisibilityFlag(true);
}

void UIMovieTest::ScaleButtonPressed(BaseObject* obj, void* data, void* callerData)
{
    eMovieScalingMode scaleMode = scalingModeNone;
    if (obj == buttonScale0)
        scaleMode = scalingModeNone;
    else if (obj == buttonScale1)
        scaleMode = scalingModeAspectFit;
    else if (obj == buttonScale2)
        scaleMode = scalingModeAspectFill;
    else if (obj == buttonScale3)
        scaleMode = scalingModeFill;

    OpenMovieParams params;
    params.scalingMode = scaleMode;

    movieView->OpenMovie(path, params);
    movieView->Play();
}
