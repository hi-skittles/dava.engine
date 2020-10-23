#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_MACOS__)
#if !defined(DISABLE_NATIVE_MOVIEVIEW)

#include "UI/IMovieViewControl.h"

namespace DAVA
{
class Window;
// Movie View Control - MacOS implementation.
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
    void OnWindowVisibilityChanged(Window* w, bool visible);

#if defined(__DAVAENGINE_STEAM__)
    bool wasVisible = false;
    void OnSteamOverlayChanged(bool overlayActivated);
#endif

private:
    Window* window = nullptr;

    // Pointer to MacOS video player helper.
    void* moviePlayerHelper;
};
} // namespace DAVA

#endif // !DISABLE_NATIVE_MOVIEVIEW
#endif // __DAVAENGINE_MACOS__
