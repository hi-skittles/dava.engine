#pragma once

#include "Base/BaseTypes.h"
#include "Reflection/Reflection.h"
#include "UI/IMovieViewControl.h"
#include "UI/UIControl.h"

namespace DAVA
{
// The purpose of UIMovieView class is to display movies.
class UIMovieView : public UIControl
{
    DAVA_VIRTUAL_REFLECTION(UIMovieView, UIControl);

public:
    UIMovieView(const Rect& rect = Rect());

    /** Open the movie with specified path. */
    void OpenMovie(const FilePath& moviePath);
    /** Open the movie with specified path and params. */
    void OpenMovie(const FilePath& moviePath, const OpenMovieParams& params);

    void SetPosition(const Vector2& position) override;
    void SetSize(const Vector2& newSize) override;
    void Draw(const UIGeometricData& parentGeometricData) override;
    void Update(float32 timeElapsed) override;
    UIMovieView* Clone() override;

    /** Start the video playback. */
    void Play();
    /** Stop the video playback and move to first frame. */
    void Stop();

    /** Pause the playback. */
    void Pause();
    /** Resume the playback. */
    void Resume();

    // Whether the movie is being played?
    DAVA_DEPRECATED(bool IsPlaying() const);

    /** Return current playing state. */
    eMoviePlayingState GetState() const;

protected:
    virtual ~UIMovieView();

    void OnVisible() override;
    void OnInvisible() override;
    void OnActive() override;

private:
    void UpdateControlRect();

protected:
    // Platform-specific implementation of the Movie Control.
    // Make impl to be controlled by std::shared_ptr as on some platforms (e.g. uwp, android)
    // impl can live longer than its owner: native control can queue callback in UI thread
    // but impl's owner is already dead
    std::shared_ptr<IMovieViewControl> movieViewControl;

    // Player status on the previous frame
    eMoviePlayingState lastPlayingState = eMoviePlayingState::stateStopped;
};

} // namespace DAVA
