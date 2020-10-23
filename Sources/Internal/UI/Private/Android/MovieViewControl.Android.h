#pragma once

#if !defined(DISABLE_NATIVE_MOVIEVIEW)

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_ANDROID__)

#include "Engine/Private/EnginePrivateFwd.h"
#include "Engine/PlatformApiAndroid.h"

#include "UI/IMovieViewControl.h"

namespace DAVA
{
class Rect;
class FilePath;
class UIMovieView;

class MovieViewControl : public IMovieViewControl,
                         public std::enable_shared_from_this<MovieViewControl>
{
    enum eAction
    {
        ACTION_PLAY = 0,
        ACTION_PAUSE,
        ACTION_RESUME,
        ACTION_STOP
    };

public:
    MovieViewControl(Window* w);
    ~MovieViewControl() override;

    void Initialize(const Rect& rect) override;
    void OwnerIsDying() override;

    void SetRect(const Rect& rect) override;
    void SetVisible(bool isVisible) override;

    void OpenMovie(const FilePath& moviePath, const OpenMovieParams& params) override;

    void Play() override;
    void Stop() override;

    void Pause() override;
    void Resume() override;

    eMoviePlayingState GetState() const override;

    void Update() override;

private:
    Window* window = nullptr;
    jobject javaMovieView = nullptr;

    std::unique_ptr<JNI::JavaClass> movieViewJavaClass;
    Function<void(jobject)> release;
    Function<void(jobject, jfloat, jfloat, jfloat, jfloat)> setRect;
    Function<void(jobject, jboolean)> setVisible;
    Function<void(jobject, jstring, jint)> openMovie;
    Function<void(jobject, jint)> doAction;
    Function<jint(jobject)> getState;
    Function<void(jobject)> update;
};

} // namespace DAVA

#endif //__DAVAENGINE_ANDROID__
#endif // !DISABLE_NATIVE_MOVIEVIEW
