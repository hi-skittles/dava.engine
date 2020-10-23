#include "UI/Private/Win10/MovieViewControl.Win10.h"

#if defined(__DAVAENGINE_WIN_UAP__)
#if !defined(DISABLE_NATIVE_MOVIEVIEW)

#include "Debug/DVAssert.h"
#include "Engine/Engine.h"
#include "Engine/PlatformApiWin10.h"
#include "FileSystem/FileSystem.h"
#include "Logger/Logger.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "Render/RHI/rhi_Public.h"
#include "UI/UIControlSystem.h"
#include "Utils/Utils.h"

namespace DAVA
{
namespace UWPWorkaround
{
extern bool enableSurfaceSizeWorkaround;
}

void MovieViewControl::MovieViewProperties::ClearChangedFlags()
{
    anyPropertyChanged = false;
    rectChanged = false;
    visibleChanged = false;
    streamChanged = false;
    actionChanged = false;
}

MovieViewControl::MovieViewControl(Window* w)
    : window(w)
    , properties()
{
}

MovieViewControl::~MovieViewControl()
{
    nativeControl = nullptr;
}

void MovieViewControl::OwnerIsDying()
{
    if (window != nullptr)
    {
        if (nativeControl != nullptr)
        {
            auto self{ shared_from_this() };
            window->RunOnUIThreadAsync([this, self]() {
                PlatformApi::Win10::RemoveXamlControl(window, nativeControl);
            });
        }

        window->sizeChanged.Disconnect(this);
        Engine::Instance()->windowDestroyed.Disconnect(this);
    }
}

void MovieViewControl::Initialize(const Rect& rect)
{
    properties.createNew = true;

    window->sizeChanged.Connect(this, &MovieViewControl::OnWindowSizeChanged);
    Engine::Instance()->windowDestroyed.Connect(this, &MovieViewControl::OnWindowDestroyed);
}

void MovieViewControl::SetRect(const Rect& rect)
{
    if (properties.rect != rect)
    {
        properties.rect = rect;
        properties.rectInWindowSpace = VirtualToWindow(rect);
        properties.rectChanged = true;
        properties.anyPropertyChanged = true;
    }
}

void MovieViewControl::SetVisible(bool isVisible)
{
    if (properties.visible != isVisible)
    {
        properties.visible = isVisible;
        properties.visibleChanged = true;
        properties.anyPropertyChanged = true;
        if (!isVisible)
        { // Immediately hide native control if it has been already created
            if (window != nullptr)
            {
                auto self{ shared_from_this() };
                window->RunOnUIThreadAsync([this, self]() {
                    if (nativeControl != nullptr)
                    {
                        SetNativeVisible(false);
                    }
                });
            }
        }
    }
}

void MovieViewControl::OpenMovie(const FilePath& moviePath, const OpenMovieParams& params)
{
    using ::Windows::UI::Xaml::Media::Stretch;
    using ::Windows::Storage::Streams::IRandomAccessStream;

    properties.stream = nullptr;
    properties.canPlay = false;
    properties.playingState = eMoviePlayingState::stateStopped;

    IRandomAccessStream ^ stream = CreateStreamFromFilePath(moviePath);
    if (stream != nullptr)
    {
        properties.canPlay = true;
        properties.playingState = eMoviePlayingState::stateLoading;

        Stretch scaling = Stretch::None;
        switch (params.scalingMode)
        {
        case scalingModeNone:
            scaling = Stretch::None;
            break;
        case scalingModeAspectFit:
            scaling = Stretch::Uniform;
            break;
        case scalingModeAspectFill:
            scaling = Stretch::UniformToFill;
            break;
        case scalingModeFill:
            scaling = Stretch::Fill;
            break;
        default:
            scaling = Stretch::None;
            break;
        }

        properties.stream = stream;
        properties.scaling = scaling;
        properties.streamChanged = true;
        properties.anyPropertyChanged = true;
    }
}

void MovieViewControl::Play()
{
    if (properties.canPlay)
    {
        // It seems that dava.engine is client of game but not vice versa
        // Game does not take into account that video playback can take some time after Play() has been called
        // So assume movie is playing under following conditions:
        //  - movie is really playing
        //  - game has called Play() method
        properties.playingState = eMoviePlayingState::statePlaying;

        properties.action = MovieViewProperties::ACTION_PLAY;
        properties.actionChanged = true;
        properties.anyPropertyChanged = true;
    }
}

void MovieViewControl::Stop()
{
    if (properties.canPlay)
    {
        properties.playingState = eMoviePlayingState::stateStopped;

        properties.action = MovieViewProperties::ACTION_STOP;
        properties.actionChanged = true;
        properties.anyPropertyChanged = true;
    }
}

void MovieViewControl::Pause()
{
    if (properties.canPlay)
    {
        properties.playingState = eMoviePlayingState::statePaused;

        properties.action = MovieViewProperties::ACTION_PAUSE;
        properties.actionChanged = true;
        properties.anyPropertyChanged = true;
    }
}

void MovieViewControl::Resume()
{
    Play();
}

void MovieViewControl::Update()
{
    if (properties.anyPropertyChanged || properties.createNew)
    {
        auto self{ shared_from_this() };
        MovieViewProperties props(properties);
        window->RunOnUIThreadAsync([this, self, props]() {
            ProcessProperties(props);
        });

        properties.createNew = false;
        properties.stream = nullptr;
        properties.ClearChangedFlags();
    }
}

void MovieViewControl::ProcessProperties(const MovieViewProperties& props)
{
    using ::Windows::UI::Xaml::Controls::MediaElement;

    if (props.createNew)
    {
        nativeControl = ref new MediaElement();
        nativeControl->AllowDrop = false;
        nativeControl->CanDrag = false;
        nativeControl->AutoPlay = false;
        nativeControl->MinHeight = 0.0; // Force minimum control sizes to zero to
        nativeControl->MinWidth = 0.0; // allow setting any control sizes
        nativeControl->Volume = 1.0;

        PlatformApi::Win10::AddXamlControl(window, nativeControl);
        InstallEventHandlers();
    }

    if (props.anyPropertyChanged)
    {
        ApplyChangedProperties(props);
    }
}

void MovieViewControl::ApplyChangedProperties(const MovieViewProperties& props)
{
    if (props.visibleChanged)
        SetNativeVisible(props.visible);
    if (props.rectChanged)
        SetNativePositionAndSize(props.rectInWindowSpace);
    if (props.streamChanged)
    {
        nativeControl->Stretch = props.scaling;
        nativeControl->SetSource(props.stream, L"");
        movieLoaded = false;
        playAfterLoaded = false;
    }
    if (props.actionChanged)
    {
        if (movieLoaded)
        {
            switch (props.action)
            {
            case MovieViewProperties::ACTION_PLAY:
                nativeControl->Play();
                break;
            case MovieViewProperties::ACTION_PAUSE:
                nativeControl->Pause();
                break;
            case MovieViewProperties::ACTION_RESUME:
                nativeControl->Play();
                break;
            case MovieViewProperties::ACTION_STOP:
                nativeControl->Stop();
                break;
            }
        }
        playAfterLoaded = !movieLoaded && props.action == MovieViewProperties::ACTION_PLAY;
    }
}

void MovieViewControl::InstallEventHandlers()
{
    using ::Windows::UI::Xaml::RoutedEventHandler;
    using ::Windows::UI::Xaml::ExceptionRoutedEventHandler;
    using ::Windows::UI::Xaml::RoutedEventArgs;
    using ::Windows::UI::Xaml::ExceptionRoutedEventArgs;

    std::weak_ptr<MovieViewControl> self_weak(shared_from_this());
    // Install event handlers through lambdas as it seems only ref class's member functions can be event handlers directly
    auto mediaOpened = ref new RoutedEventHandler([this, self_weak](Platform::Object ^, RoutedEventArgs ^ ) {
        auto self = self_weak.lock();
        if (self != nullptr)
        {
            OnMediaOpened();
        }
    });
    auto mediaEnded = ref new RoutedEventHandler([this, self_weak](Platform::Object ^, RoutedEventArgs ^ ) {
        auto self = self_weak.lock();
        if (self != nullptr)
        {
            OnMediaEnded();
        }
    });
    auto mediaFailed = ref new ExceptionRoutedEventHandler([this, self_weak](Platform::Object ^, ExceptionRoutedEventArgs ^ args) {
        auto self = self_weak.lock();
        if (self != nullptr)
        {
            OnMediaFailed(args);
        }
    });
    nativeControl->MediaOpened += mediaOpened;
    nativeControl->MediaEnded += mediaEnded;
    nativeControl->MediaFailed += mediaFailed;
}

Windows::Storage::Streams::IRandomAccessStream ^ MovieViewControl::CreateStreamFromFilePath(const FilePath& path) const
{
    using ::Windows::Storage::StorageFile;
    using ::Windows::Storage::FileAccessMode;

    String pathName = path.GetAbsolutePathname();
    std::replace(pathName.begin(), pathName.end(), '/', '\\');
    Platform::String ^ filePath = StringToRTString(pathName);

    try
    {
        StorageFile ^ file = WaitAsync(StorageFile::GetFileFromPathAsync(filePath));
        if (file != nullptr)
        {
            return WaitAsync(file->OpenAsync(FileAccessMode::Read));
        }
        return nullptr;
    }
    catch (Platform::COMException ^ e)
    {
        Logger::Error("[MovieView] failed to load file %s: %s (0x%08x)",
                      RTStringToString(filePath).c_str(),
                      RTStringToString(e->Message).c_str(),
                      e->HResult);
        return nullptr;
    }
}

void MovieViewControl::SetNativeVisible(bool visible)
{
    using ::Windows::UI::Xaml::Visibility;

    nativeControl->Visibility = visible ? Visibility::Visible : Visibility::Collapsed;
}

void MovieViewControl::SetNativePositionAndSize(const Rect& rect)
{
    nativeControl->Width = std::max(0.0f, rect.dx);
    nativeControl->Height = std::max(0.0f, rect.dy);
    PlatformApi::Win10::PositionXamlControl(window, nativeControl, rect.x, rect.y);

    if (UWPWorkaround::enableSurfaceSizeWorkaround)
    {
        nativeControl->Height += 1.0;
    }
}

Rect MovieViewControl::VirtualToWindow(const Rect& srcRect) const
{
    VirtualCoordinatesSystem* coordSystem = GetEngineContext()->uiControlSystem->vcs;
    return coordSystem->ConvertVirtualToInput(srcRect);
}

void MovieViewControl::TellPlayingState(eMoviePlayingState state)
{
    RunOnMainThreadAsync([this, state]() {
        properties.playingState = state;
    });
}

void MovieViewControl::OnMediaOpened()
{
    movieLoaded = true;
    if (playAfterLoaded)
    {
        playAfterLoaded = false;
        nativeControl->Play();
    }
    else
    {
        // Need to setup correct playing state
        nativeControl->Stop();
    }
}

void MovieViewControl::OnMediaEnded()
{
    TellPlayingState(eMoviePlayingState::stateStopped);
}

void MovieViewControl::OnMediaFailed(::Windows::UI::Xaml::ExceptionRoutedEventArgs ^ args)
{
    TellPlayingState(eMoviePlayingState::stateStopped);
    String errMessage = UTF8Utils::EncodeToUTF8(args->ErrorMessage->Data());
    Logger::Error("[MovieView] failed to decode media file: %s", errMessage.c_str());
}

void MovieViewControl::OnWindowSizeChanged(Window* w, Size2f windowSize, Size2f surfaceSize)
{
    properties.rectInWindowSpace = VirtualToWindow(properties.rect);
    properties.rectChanged = true;
    properties.anyPropertyChanged = true;
}

void MovieViewControl::OnWindowDestroyed(Window* w)
{
    OwnerIsDying();
    window = nullptr;
}

} // namespace DAVA

#endif // !DISABLE_NATIVE_MOVIEVIEW
#endif // __DAVAENGINE_WIN_UAP__
