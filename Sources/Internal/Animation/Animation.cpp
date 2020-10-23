#include "Animation/Animation.h"
#include "Animation/AnimationManager.h"
#include "Engine/Engine.h"
#include "Logger/Logger.h"

namespace DAVA
{
Animation::Animation(AnimatedObject* _owner, float32 _animationTimeLength, Interpolation::Func _interpolationFunc, int32 _defaultState)
    : owner(_owner)
    , timeLength(_animationTimeLength)
    , interpolationFunc(_interpolationFunc)
    , state(_defaultState)
{
    GetEngineContext()->animationManager->AddAnimation(this);
}

Animation::Animation(AnimatedObject* _owner, float32 _animationTimeLength, Interpolation::FuncType _interpolationFuncType, int32 _defaultState)
    : Animation(_owner, _animationTimeLength, Interpolation::GetFunction(_interpolationFuncType), _defaultState)
{
}

Animation::~Animation()
{
    GetEngineContext()->animationManager->RemoveAnimation(this);
}

void Animation::Reset()
{
    time = 0.0f;
    normalizedTime = 0.0f;
    next = nullptr;
}

void Animation::Start(int32 _groupId)
{
    DVASSERT((state & STATE_DELETED) == 0);
    Reset();
    groupId = _groupId;

    Animation* prevAnimation = GetEngineContext()->animationManager->FindLastAnimation(owner, groupId);

    if (!prevAnimation || (prevAnimation == this))
    {
        state |= STATE_IN_PROGRESS;
        OnStart();
    }
    else
    {
        prevAnimation->next = this;
    }
}

void Animation::Stop()
{
    state &= ~STATE_IN_PROGRESS;
    state |= STATE_DELETE_ME;
    OnStop();
}

bool Animation::IsPlaying() const
{
    return (state & STATE_IN_PROGRESS);
}

void Animation::Update(float32 timeElapsed)
{
    if (state & STATE_IN_PROGRESS)
    {
        if (state & STATE_PAUSED)
            return;

        if (reverseMode)
        {
            time += timeElapsed * timeMultiplier;

            float halfTimeLength = 0.5f * timeLength;
            if (time <= halfTimeLength)
            { // normal interpolation
                normalizedTime = interpolationFunc(time / halfTimeLength);
            }
            else
            { // reverse interpolation
                normalizedTime = interpolationFunc(2.0f - (time / halfTimeLength)); /*1.0f - ((time - halfTimeLength) / halfTimeLength)*/
            }

            if (time >= timeLength)
            {
                if (repeatCount == 0)
                {
                    time = timeLength;
                    normalizedTime = 0.0f;
                    state |= STATE_FINISHED;
                }
                else
                {
                    time -= timeLength;
                    // Do not decrement repeat counter for loop
                    if (repeatCount != INFINITE_LOOP)
                    {
                        repeatCount--;
                    }
                }
            }
        }
        else //
        {
            time += timeElapsed * timeMultiplier;
            normalizedTime = interpolationFunc(time / timeLength);
            if (time >= timeLength)
            {
                if (repeatCount == 0)
                {
                    time = timeLength;
                    normalizedTime = 1.0f;
                    state |= STATE_FINISHED;
                }
                else
                {
                    time -= timeLength;
                    // Do not decrement repeat counter for loop
                    if (repeatCount != INFINITE_LOOP)
                    {
                        repeatCount--;
                    }
                }
            }
        }
    }
}

void Animation::OnStart()
{
    PerformEvent(EVENT_ANIMATION_START);
};

void Animation::OnStop()
{
    PerformEvent(EVENT_ANIMATION_END);
};

void Animation::OnCancel()
{
    PerformEvent(EVENT_ANIMATION_CANCELLED);
}

void Animation::Pause(bool _isPaused)
{
    if (_isPaused)
    {
        state |= STATE_PAUSED;
    }
    else
    {
        state &= ~STATE_PAUSED;
    }
}

bool Animation::IsPaused() const
{
    return (0 != (state & STATE_PAUSED));
}

void Animation::SetRepeatCount(int32 _repeatCount)
{
    if (INFINITE_LOOP == _repeatCount)
    {
        repeatCount = _repeatCount;
    }
    else
    {
        repeatCount = _repeatCount - 1;
    }
}
}