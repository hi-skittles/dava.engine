#pragma once

#include "FileSystem/FilePath.h"
#include "Math/Rect.h"

namespace DAVA
{
enum eMovieScalingMode
{
    scalingModeNone = 0, // No scaling
    scalingModeAspectFit, // Uniform scale until one dimension fits
    scalingModeAspectFill, // Uniform scale until the movie fills the visible bounds. One dimension may have clipped contents
    scalingModeFill // Non-uniform scale. Both render dimensions will exactly match the visible bounds
};

enum eMoviePlayingState
{
    stateStopped = 0, // Movie is stopped
    stateLoading, // Movie loading
    statePaused, // Movie is paused
    statePlaying // Movie playing
};

struct OpenMovieParams
{
    OpenMovieParams(eMovieScalingMode mode = scalingModeNone)
        : scalingMode(mode)
    {
    }

    eMovieScalingMode scalingMode;
};

// Common interface for Movie View Controls for different platforms.
class IMovieViewControl
{
public:
    virtual ~IMovieViewControl() = default;

    // Initialize the control.
    virtual void Initialize(const Rect& rect) = 0;
    virtual void OwnerIsDying()
    {
    }

    // Position/visibility.
    virtual void SetRect(const Rect& rect) = 0;
    virtual void SetVisible(bool isVisible) = 0;

    // Open the Movie.
    virtual void OpenMovie(const FilePath& moviePath, const OpenMovieParams& params) = 0;

    // Start/stop the video playback.
    virtual void Play() = 0;
    virtual void Stop() = 0;

    // Pause/resume the playback.
    virtual void Pause() = 0;
    virtual void Resume() = 0;

    virtual eMoviePlayingState GetState() const = 0;

    virtual void Update()
    {
    }

    virtual void Draw(const class UIGeometricData&)
    {
    }
};

} // namespace DAVA
