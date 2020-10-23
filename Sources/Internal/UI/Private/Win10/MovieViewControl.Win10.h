#pragma once

#include "Base/Platform.h"

#if defined(__DAVAENGINE_WIN_UAP__)
#if !defined(DISABLE_NATIVE_MOVIEVIEW)

#include "UI/IMovieViewControl.h"

namespace DAVA
{
class Window;
class MovieViewControl : public IMovieViewControl,
                         public std::enable_shared_from_this<MovieViewControl>
{
    struct MovieViewProperties
    {
        enum
        {
            ACTION_PLAY,
            ACTION_PAUSE,
            ACTION_RESUME,
            ACTION_STOP,
        };

        void ClearChangedFlags();

        Rect rect;
        Rect rectInWindowSpace;
        bool visible = false;
        bool canPlay = false;
        eMoviePlayingState playingState = eMoviePlayingState::stateStopped;
        int32 action = ACTION_STOP;
        ::Windows::Storage::Streams::IRandomAccessStream ^ stream = nullptr;
        ::Windows::UI::Xaml::Media::Stretch scaling = ::Windows::UI::Xaml::Media::Stretch::None;

        bool createNew : 1;

        bool anyPropertyChanged : 1;
        bool rectChanged : 1;
        bool visibleChanged : 1;
        bool streamChanged : 1;
        bool actionChanged : 1;
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
    void ProcessProperties(const MovieViewProperties& props);
    void ApplyChangedProperties(const MovieViewProperties& props);

    void InstallEventHandlers();

    Windows::Storage::Streams::IRandomAccessStream ^ CreateStreamFromFilePath(const FilePath& path) const;

    void SetNativeVisible(bool visible);
    void SetNativePositionAndSize(const Rect& rect);

    Rect VirtualToWindow(const Rect& srcRect) const;
    void TellPlayingState(eMoviePlayingState state);

private: // MediaElement event handlers
    void OnMediaOpened();
    void OnMediaEnded();
    void OnMediaFailed(Windows::UI::Xaml::ExceptionRoutedEventArgs ^ args);

    // Signal handlers
    void OnWindowSizeChanged(Window* w, Size2f windowSize, Size2f surfaceSize);
    void OnWindowDestroyed(Window* w);

private:
    Window* window = nullptr;
    Windows::UI::Xaml::Controls::MediaElement ^ nativeControl = nullptr;
    bool playAfterLoaded = false; // Movie should play after loading as Play() can be invoked earlier than movie has been loaded
    bool movieLoaded = false; // Movie has been successfully loaded and decoded

    MovieViewProperties properties;
};

//////////////////////////////////////////////////////////////////////////
inline eMoviePlayingState MovieViewControl::GetState() const
{
    return properties.playingState;
}

} // namespace DAVA

#endif // !DISABLE_NATIVE_MOVIEVIEW
#endif // __DAVAENGINE_WIN_UAP__
