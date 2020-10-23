#include "UI/IMovieViewControl.h"
#include "Base/GlobalEnum.h"

ENUM_DECLARE(DAVA::eMoviePlayingState)
{
    ENUM_ADD_DESCR(DAVA::eMoviePlayingState::stateStopped, "Stopped");
    ENUM_ADD_DESCR(DAVA::eMoviePlayingState::stateLoading, "Loading");
    ENUM_ADD_DESCR(DAVA::eMoviePlayingState::statePaused, "Paused");
    ENUM_ADD_DESCR(DAVA::eMoviePlayingState::statePlaying, "Playing");
}

ENUM_DECLARE(DAVA::eMovieScalingMode)
{
    ENUM_ADD_DESCR(DAVA::eMovieScalingMode::scalingModeNone, "None");
    ENUM_ADD_DESCR(DAVA::eMovieScalingMode::scalingModeAspectFit, "AspectFit");
    ENUM_ADD_DESCR(DAVA::eMovieScalingMode::scalingModeAspectFill, "AspectFill");
    ENUM_ADD_DESCR(DAVA::eMovieScalingMode::scalingModeFill, "Fill");
};