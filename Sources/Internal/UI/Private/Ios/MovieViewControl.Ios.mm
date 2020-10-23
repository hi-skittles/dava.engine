#include "UI/Private/Ios/MovieViewControl.Ios.h"

#if defined(__DAVAENGINE_IPHONE__)
#if !defined(DISABLE_NATIVE_MOVIEVIEW)

#import <MediaPlayer/MediaPlayer.h>

#include "Engine/Engine.h"
#include "Engine/PlatformApiIos.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "UI/UIControlSystem.h"

@interface DavaMPMoviePlayerController : MPMoviePlayerController
- (void)subscribeNotifications;
- (void)unsubscribeNotifications;
- (void)moviePlayBackDidFinish:(NSNotification*)notification;
@end

@implementation DavaMPMoviePlayerController

- (void)subscribeNotifications
{
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(moviePlayBackDidFinish:)
                                                 name:MPMoviePlayerPlaybackDidFinishNotification
                                               object:self];
}

- (void)unsubscribeNotifications
{
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:MPMoviePlayerPlaybackDidFinishNotification
                                                  object:self];
}

- (void)moviePlayBackDidFinish:(NSNotification*)notification
{
    [self stop];
}

@end

namespace DAVA
{
struct MovieViewControl::MovieViewObjcBridge final
{
    DavaMPMoviePlayerController* moviePlayer = nullptr;
};

MovieViewControl::MovieViewControl(Window* w)
    : bridge(new MovieViewObjcBridge)
    , window(w)
{
    bridge->moviePlayer = [[DavaMPMoviePlayerController alloc] init];

    [bridge->moviePlayer subscribeNotifications];
    [bridge->moviePlayer setShouldAutoplay:FALSE];
    [[bridge->moviePlayer view] setUserInteractionEnabled:NO];

    if ([bridge->moviePlayer respondsToSelector:@selector(loadState)])
    {
        [bridge->moviePlayer setControlStyle:MPMovieControlStyleNone];
        [bridge->moviePlayer setScalingMode:MPMovieScalingModeFill];
    }

    PlatformApi::Ios::AddUIView(window, [bridge->moviePlayer view]);
}

MovieViewControl::~MovieViewControl()
{
    [bridge->moviePlayer unsubscribeNotifications];
    PlatformApi::Ios::RemoveUIView(window, [bridge->moviePlayer view]);
    [bridge->moviePlayer release];
}

void MovieViewControl::Initialize(const Rect& rect)
{
    SetRect(rect);
}

void MovieViewControl::OpenMovie(const FilePath& moviePath, const OpenMovieParams& params)
{
    NSURL* movieURL = [NSURL fileURLWithPath:[NSString stringWithCString:moviePath.GetAbsolutePathname().c_str() encoding:NSASCIIStringEncoding]];

    MPMovieScalingMode scaling = MPMovieScalingModeNone;
    switch (params.scalingMode)
    {
    case scalingModeNone:
        scaling = MPMovieScalingModeNone;
        break;
    case scalingModeFill:
        scaling = MPMovieScalingModeFill;
        break;
    case scalingModeAspectFill:
        scaling = MPMovieScalingModeAspectFill;
        break;
    case scalingModeAspectFit:
        scaling = MPMovieScalingModeAspectFit;
        break;
    default:
        scaling = MPMovieScalingModeNone;
        break;
    }
    [bridge->moviePlayer setScalingMode:scaling];
    [bridge->moviePlayer setContentURL:movieURL];
}

void MovieViewControl::SetRect(const Rect& rect)
{
    Rect r = GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToInput(rect);
    [[bridge->moviePlayer view] setFrame:CGRectMake(r.x, r.y, r.dx, r.dy)];
}

void MovieViewControl::SetVisible(bool isVisible)
{
    [[bridge->moviePlayer view] setHidden:!isVisible];
}

void MovieViewControl::Play()
{
    [bridge->moviePlayer play];
}

void MovieViewControl::Stop()
{
    [bridge->moviePlayer stop];
}

void MovieViewControl::Pause()
{
    [bridge->moviePlayer pause];
}

void MovieViewControl::Resume()
{
    [bridge->moviePlayer play];
}

eMoviePlayingState MovieViewControl::GetState() const
{
    switch ([bridge->moviePlayer playbackState])
    {
    case MPMoviePlaybackStatePlaying:
        return eMoviePlayingState::statePlaying;
    case MPMoviePlaybackStatePaused:
        return eMoviePlayingState::statePaused;
    case MPMoviePlaybackStateSeekingForward:
    case MPMoviePlaybackStateSeekingBackward:
        return eMoviePlayingState::stateLoading;
    case MPMoviePlaybackStateStopped:
    case MPMoviePlaybackStateInterrupted:
    default:
        return eMoviePlayingState::stateStopped;
    }
}

} // namespace DAVA

#endif // !DISABLE_NATIVE_MOVIEVIEW
#endif // __DAVAENGINE_IPHONE__
