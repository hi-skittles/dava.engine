#if !defined(DISABLE_NATIVE_MOVIEVIEW)

#include "UI/Private/Android/MovieViewControl.Android.h"
#include "UI/UIControlSystem.h"

#if defined(__DAVAENGINE_ANDROID__)

#include "Math/Rect.h"
#include "Utils/Utils.h"
#include "Logger/Logger.h"

#include "Engine/Engine.h"
#include "Engine/Window.h"

extern "C"
{

JNIEXPORT void JNICALL Java_com_dava_engine_DavaMovieView_nativeReleaseWeakPtr(JNIEnv* env, jclass jclazz, jlong backendPointer)
{
    using DAVA::MovieViewControl;

    // Postpone deleting in case some other jobs are posted to main thread
    DAVA::RunOnMainThreadAsync([backendPointer]() {
        std::weak_ptr<MovieViewControl>* weak = reinterpret_cast<std::weak_ptr<MovieViewControl>*>(static_cast<uintptr_t>(backendPointer));
        delete weak;
    });
}

} // extern "C"

namespace DAVA
{
MovieViewControl::MovieViewControl(Window* w)
    : window(w)
{
}

MovieViewControl::~MovieViewControl() = default;

void MovieViewControl::Initialize(const Rect& rect)
{
    try
    {
        movieViewJavaClass.reset(new JNI::JavaClass("com/dava/engine/DavaMovieView"));
        release = movieViewJavaClass->GetMethod<void>("release");
        setRect = movieViewJavaClass->GetMethod<void, jfloat, jfloat, jfloat, jfloat>("setRect");
        setVisible = movieViewJavaClass->GetMethod<void, jboolean>("setVisible");
        openMovie = movieViewJavaClass->GetMethod<void, jstring, jint>("openMovie");
        doAction = movieViewJavaClass->GetMethod<void, jint>("doAction");
        getState = movieViewJavaClass->GetMethod<jint>("getState");
        update = movieViewJavaClass->GetMethod<void>("update");
    }
    catch (const JNI::Exception& e)
    {
        Logger::Error("[MovieViewControl] failed to init java bridge: %s", e.what());
        DVASSERT(false, e.what());
        return;
    }

    std::weak_ptr<MovieViewControl>* selfWeakPtr = new std::weak_ptr<MovieViewControl>(shared_from_this());
    jobject obj = PlatformApi::Android::CreateNativeControl(window, "com.dava.engine.DavaMovieView", selfWeakPtr);
    if (obj != nullptr)
    {
        JNIEnv* env = JNI::GetEnv();
        javaMovieView = env->NewGlobalRef(obj);
        env->DeleteLocalRef(obj);
        SetRect(rect);
    }
    else
    {
        delete selfWeakPtr;
        Logger::Error("[MovieViewControl] failed to create java movieview");
    }
}

void MovieViewControl::OwnerIsDying()
{
    if (javaMovieView != nullptr)
    {
        release(javaMovieView);
        JNI::GetEnv()->DeleteGlobalRef(javaMovieView);
        javaMovieView = nullptr;
    }
}

void MovieViewControl::SetRect(const Rect& rect)
{
    if (javaMovieView != nullptr)
    {
        Rect rc = GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToInput(rect);
        rc.dx = std::max(0.0f, rc.dx);
        rc.dy = std::max(0.0f, rc.dy);

        setRect(javaMovieView, rc.x, rc.y, rc.dx, rc.dy);
    }
}

void MovieViewControl::SetVisible(bool visible)
{
    if (javaMovieView != nullptr)
    {
        setVisible(javaMovieView, visible ? JNI_TRUE : JNI_FALSE);
    }
}

void MovieViewControl::OpenMovie(const FilePath& moviePath, const OpenMovieParams& params)
{
    if (javaMovieView != nullptr)
    {
        JNIEnv* env = JNI::GetEnv();
        jstring jpath = JNI::StringToJavaString(moviePath.GetAbsolutePathname(), env);
        openMovie(javaMovieView, jpath, static_cast<jint>(params.scalingMode));
        env->DeleteLocalRef(jpath);
    }
}

void MovieViewControl::Play()
{
    if (javaMovieView != nullptr)
    {
        doAction(javaMovieView, ACTION_PLAY);
    }
}

void MovieViewControl::Stop()
{
    if (javaMovieView != nullptr)
    {
        doAction(javaMovieView, ACTION_STOP);
    }
}

void MovieViewControl::Pause()
{
    if (javaMovieView != nullptr)
    {
        doAction(javaMovieView, ACTION_PAUSE);
    }
}

void MovieViewControl::Resume()
{
    if (javaMovieView != nullptr)
    {
        doAction(javaMovieView, ACTION_RESUME);
    }
}

eMoviePlayingState MovieViewControl::GetState() const
{
    if (javaMovieView != nullptr)
    {
        switch (getState(javaMovieView))
        {
        case 3:
            return eMoviePlayingState::statePlaying;
        case 2:
            return eMoviePlayingState::statePaused;
        case 1:
            return eMoviePlayingState::stateLoading;
        case 0:
        default:
            return eMoviePlayingState::stateStopped;
        }
    }
    return eMoviePlayingState::stateStopped;
}

void MovieViewControl::Update()
{
    if (javaMovieView != nullptr)
    {
        update(javaMovieView);
    }
}

} // namespace DAVA

#endif //__DAVAENGINE_ANDROID__
#endif // !DISABLE_NATIVE_MOVIEVIEW
