#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_IPHONE__)
#if !defined(DISABLE_NATIVE_MOVIEVIEW)

#include "UI/IMovieViewControl.h"

namespace DAVA
{
class Window;
// Movie View Control - iOS implementation.
class MovieViewControl : public IMovieViewControl
{
public:
    MovieViewControl(Window* w);
    ~MovieViewControl() override;

    // Initialize the control.
    void Initialize(const Rect& rect) override;

    // Open the Movie.
    void OpenMovie(const FilePath& moviePath, const OpenMovieParams& params) override;

    // Position/visibility.
    void SetRect(const Rect& rect) override;
    void SetVisible(bool isVisible) override;

    // Start/stop the video playback.
    void Play() override;
    void Stop() override;

    // Pause/resume the playback.
    void Pause() override;
    void Resume() override;

    eMoviePlayingState GetState() const override;

private:
    struct MovieViewObjcBridge;
    std::unique_ptr<MovieViewObjcBridge> bridge;
    Window* window = nullptr;
};
} // namespace DAVA

#endif // !DISABLE_NATIVE_MOVIEVIEW
#endif // __DAVAENGINE_IPHONE__
