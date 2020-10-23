#pragma once

#if defined(__DAVAENGINE_WIN32__)
#if !defined(DISABLE_NATIVE_MOVIEVIEW)

#include "UI/IMovieViewControl.h"
#include "Base/ScopedPtr.h"

namespace DAVA
{
class Window;
class FfmpegPlayer;
class Texture;
class UIControlBackground;
class MovieViewControl : public IMovieViewControl
{
public:
    MovieViewControl(Window* w);
    ~MovieViewControl() override;

    // Initialize the control.
    void Initialize(const Rect& rect) override;

    // Position/visibility.
    void SetRect(const Rect& rect) override;
    void SetVisible(bool isVisible) override;

    // Open the Movie.
    void OpenMovie(const FilePath& moviePath, const OpenMovieParams& params) override;

    // Start/stop the video playback.
    void Play() override;
    void Stop() override;

    // Pause/resume the playback.
    void Pause() override;
    void Resume() override;

    eMoviePlayingState GetState() const override;

    void Update() override;

    void Draw(const class UIGeometricData& parentGeometricData) override;

private:
    std::unique_ptr<FfmpegPlayer> ffmpegPlayer;
    Rect controlRect;
    Texture* videoTexture = nullptr;
    ScopedPtr<UIControlBackground> videoBackground;
    Vector<uint8> videoTextureBuffer;
    uint32 textureWidth = 0;
    uint32 textureHeight = 0;
    uint32 textureDataLen = 0;
    eMovieScalingMode scaling = scalingModeNone;
};
}

#endif // !DISABLE_NATIVE_MOVIEVIEW
#endif // __DAVAENGINE_WIN32__
