#include "UI/UIMovieView.h"
#include "Base/GlobalEnum.h"
#include "Debug/DVAssert.h"
#include "Engine/Engine.h"
#include "UI/UIControlSystem.h"

#if defined(DISABLE_NATIVE_MOVIEVIEW)
// Use stub movie control
#define DRAW_PLACEHOLDER_FOR_STUB_UIMOVIEVIEW
#include "UI/Private/MovieViewControlStub.h"
#include "Render/RenderHelper.h"
#elif defined(__DAVAENGINE_IPHONE__)
#include "UI/Private/Ios/MovieViewControl.Ios.h"
#elif defined(__DAVAENGINE_MACOS__)
#include "UI/Private/Mac/MovieViewControl.Macos.h"
#elif defined(__DAVAENGINE_ANDROID__)
#include "UI/Private/Android/MovieViewControl.Android.h"
#elif defined(__DAVAENGINE_WIN_UAP__)
#include "UI/Private/Win10/MovieViewControl.Win10.h"
#elif defined(__DAVAENGINE_WIN32__)
#include "UI/Private/Win32/MovieViewControl.Win32.h"
#else
// UIMovieView is not implemented for this platform yet, using stub one.
#define DRAW_PLACEHOLDER_FOR_STUB_UIMOVIEVIEW
#include "UI/Private/MovieViewControlStub.h"
#include "Render/RenderHelper.h"
#endif
#include "Reflection/ReflectionRegistrator.h"
#include "Render/2D/Systems/RenderSystem2D.h"
#include "UI/Events/UIEventsSingleComponent.h"
#include "UI/Events/UIMovieEventComponent.h"
#include "UI/Update/UIUpdateComponent.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIMovieView)
{
    ReflectionRegistrator<UIMovieView>::Begin()
    .ConstructorByPointer()
    .DestructorByPointer([](UIMovieView* o) { o->Release(); })
    .Field("state", &UIMovieView::GetState, nullptr)[M::EnumT<eMoviePlayingState>()]
    .Method<void (UIMovieView::*)(const FilePath& moviePath)>("OpenMovie", &UIMovieView::OpenMovie)
    .Method("Play", &UIMovieView::Play)
    .Method("Stop", &UIMovieView::Stop)
    .Method("Pause", &UIMovieView::Pause)
    .Method("Resume", &UIMovieView::Resume)
    .End();
}

UIMovieView::UIMovieView(const Rect& rect)
    : UIControl(rect)
    , movieViewControl(std::make_shared<MovieViewControl>(Engine::Instance()->PrimaryWindow()))
{
    movieViewControl->Initialize(rect);
    UpdateControlRect();
    GetOrCreateComponent<UIUpdateComponent>();
}

UIMovieView::~UIMovieView()
{
    movieViewControl->OwnerIsDying();
}

void UIMovieView::OpenMovie(const FilePath& moviePath)
{
    movieViewControl->OpenMovie(moviePath, OpenMovieParams());
}

void UIMovieView::OpenMovie(const FilePath& moviePath, const OpenMovieParams& params)
{
    movieViewControl->OpenMovie(moviePath, params);
}

void UIMovieView::SetPosition(const Vector2& position)
{
    UIControl::SetPosition(position);
    UpdateControlRect();
}

void UIMovieView::SetSize(const Vector2& newSize)
{
    UIControl::SetSize(newSize);
    UpdateControlRect();
}

void UIMovieView::Play()
{
    movieViewControl->Play();
}

void UIMovieView::Stop()
{
    movieViewControl->Stop();
}

void UIMovieView::Pause()
{
    movieViewControl->Pause();
}

void UIMovieView::Resume()
{
    movieViewControl->Resume();
}

bool UIMovieView::IsPlaying() const
{
    return GetState() == eMoviePlayingState::statePlaying;
}

eMoviePlayingState UIMovieView::GetState() const
{
    return movieViewControl->GetState();
}

void UIMovieView::UpdateControlRect()
{
    Rect rect = GetAbsoluteRect();
    movieViewControl->SetRect(rect);
}

void UIMovieView::Draw(const UIGeometricData& parentGeometricData)
{
    UIControl::Draw(parentGeometricData);
    movieViewControl->Draw(parentGeometricData);
#if defined(DRAW_PLACEHOLDER_FOR_STUB_UIMOVIEVIEW)
    static Color drawColor(Color(1.0f, 0.4f, 0.8f, 1.0f));

    Rect absRect = GetAbsoluteRect();
    RenderSystem2D::Instance()->DrawRect(absRect, drawColor);

    float32 minRadius = Min(GetSize().x, GetSize().y);
    RenderSystem2D::Instance()->DrawCircle(absRect.GetCenter(), minRadius / 2, drawColor);
    RenderSystem2D::Instance()->DrawCircle(absRect.GetCenter(), minRadius / 3, drawColor);
    RenderSystem2D::Instance()->DrawCircle(absRect.GetCenter(), minRadius / 4, drawColor);
#endif
}

void UIMovieView::Update(float32 timeElapsed)
{
    UIControl::Update(timeElapsed);
    movieViewControl->Update();

    eMoviePlayingState currentState = GetState();
    if (lastPlayingState != currentState)
    {
        UIMovieEventComponent* events = GetComponent<UIMovieEventComponent>();
        if (events)
        {
            FastName event;
            switch (currentState)
            {
            case eMoviePlayingState::stateStopped:
                event = events->GetStopEvent();
                break;
            case eMoviePlayingState::statePlaying:
                event = events->GetStartEvent();
                break;
            default:
                break;
            }
            if (event.IsValid())
            {
                if (GetScene())
                {
                    UIEventsSingleComponent* eventsSingle = GetScene()->GetSingleComponent<UIEventsSingleComponent>();
                    if (eventsSingle)
                    {
                        eventsSingle->SendEvent(this, event, Any());
                    }
                }
            }
        }
        lastPlayingState = currentState;
    }
}

void UIMovieView::OnVisible()
{
    UIControl::OnVisible();
    movieViewControl->SetVisible(true);
}

void UIMovieView::OnInvisible()
{
    UIControl::OnInvisible();
    movieViewControl->SetVisible(false);
}

void UIMovieView::OnActive()
{
    UIControl::OnActive();
    UpdateControlRect();
}

UIMovieView* UIMovieView::Clone()
{
    UIMovieView* uiMoviewView = new UIMovieView(GetRect());
    uiMoviewView->CopyDataFrom(this);
    return uiMoviewView;
}
} // namespace DAVA
